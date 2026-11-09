#include "adapters/ClientApp.h"
#include "Socket.h"
#include "utils.h"
#include "adapters/JsonConfig.h"
#include <Windows.h>
#include <print>
#include <string>

// 阶段 4 · S1：入口瘦身为 Composition Root。
// 原 main 的 handleChoicePacket / gamePhase 与其余装配逻辑已逐字平移到
// source/adapters/ClientApp.cpp；这里只保留进程级控制台编码、配置读取与
// 组合根构造，装配顺序（连接 → 渲染器 → 登录场景 → 游戏阶段）保持不变。
int main() {
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	const std::string windowTitle = "Client";
	const std::string windowTitleWithBrackets = "[Client]";
	std::println("{} 启动客户端...", windowTitleWithBrackets);

	ClientNetwork clientNetwork;
	const auto& config = unool::getClientConfig();
	std::string ipAddress = config["server"]["ip"];
	unsigned short port = config["server"]["port"];

	unool::adapters::ClientApp app(clientNetwork, windowTitle, windowTitleWithBrackets, ipAddress, port);
	return app.run();
}
