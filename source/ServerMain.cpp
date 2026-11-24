#include "ServerNetwork.h"
#include "RoomManager.h"
#include "utils.h"
#include "core/Services.h"
#include "core/AccountService.h"
#include "core/ShopService.h"
#include "shell/LegacyServicesAdapter.h"
#include "platform/Platform.h"

#include <algorithm>
#include <chrono>
#include <print>
#include <thread>

// 服务端组合根（协议版本 3：房间功能）。
//
// 与房间功能之前的差别：
//   - 登录后不再匹配，大厅里创建 / 加入房间，房内双方准备后开局；
//   - 服务端同时主持多个房间：每个房间一份 GameLogic + 一个独立线程（RoomGame）；
//   - 主线程只剩一件事——驱动 ServerNetwork 的 IO（收发 / 心跳 / 超时 / 出站冲刷），
//     socket 因此始终单线程使用，房间线程只与输入队列和出站队列打交道。

namespace {
	constexpr unsigned short kDefaultPort = 8888;
	// 没有任何 socket 就绪时主循环的让出粒度：selector 自身最多等 10ms，
	// 这里再补 2ms，避免满核空转。
	constexpr int kIdleSleepMs = 2;
}

int main() {
	unool::platform::setupConsoleUtf8();

	std::println("[Server] 启动服务器（房间模式）...");

	// Composition Root：唯一出现「谁实现谁」的地方。
	// 五个出站端口各有独立适配器；随机引擎硬约束仍然是「全进程只有一份」。
	unool::shell::JsonConfigProvider configService;
	UserDB userDatabase;
	unool::shell::JsonUserRepository userRepository(userDatabase);
	unool::shell::Mt19937Random randomService;
	unool::shell::ChronoClock clockService;
	unool::shell::FilesystemAssetCatalog assetCatalog;
	unool::core::DomainServices domainServices{
		configService, userRepository, randomService, clockService, assetCatalog };
	unool::core::installServices(domainServices);

	// 连接槽位上限来自 server_config.json（network.maxClients，默认 32）
	const std::size_t maxClients = std::max<std::size_t>(configService.maxClients(), 2);

	// 多连接网络层在 unool::shell：Server 端不再有「全局命名空间的 ServerNetwork」
	unool::shell::ServerNetwork serverNetwork;
	if (!serverNetwork.start(kDefaultPort, maxClients)) {
		std::println(stderr, "[Server] 启动失败");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return 1;
	}

	// 账号 / 商城 / 房间三个用例层共用同一份 UserDB 与同一张连接表。
	// 三者都在首次 update() 之前装配，不存在「未装配」窗口。
	unool::core::AccountService accountService(serverNetwork, userRepository, maxClients);
	serverNetwork.installAccountService(accountService);

	unool::core::ShopService shopService(serverNetwork, userRepository);
	serverNetwork.installShopService(shopService);

	unool::shell::RoomManager roomManager(serverNetwork, domainServices);
	serverNetwork.installRoomManager(roomManager);

	std::println("[Server] 等待客户端连接...");

	try {
		while (true) {
			serverNetwork.update();
			std::this_thread::sleep_for(std::chrono::milliseconds(kIdleSleepMs));
		}
	}
	catch (const std::exception& e) {
		std::println(stderr, "[Server] 主循环异常退出：{}", e.what());
	}

	return 0;
}
