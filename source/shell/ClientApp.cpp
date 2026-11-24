#include "shell/ClientApp.h"

#include "shell/ConnectionState.h"
#include "GameRenderer.h"
#include "LoginScene.h"
#include "ShopScene.h"
#include "Socket.h"
#include "shell/IShopOpener.h"
#include "shell/InputAdapter.h"
#include "utils.h"
#include "shell/JsonConfig.h"
#include "AccountProtocol.h"
#include "ShopProtocol.h"
#include "MessageCodec.h"
#include "core/IDecisionPort.h"
#include <SFML/Network.hpp>
#include "ShopData.h"
#include <string>
#include <chrono>
#include <format>
#include <print>
#include <thread>
#include <utility>

// MessageType 归 protocol 层；本单元既有全局 static 函数也有 unool::shell 段，
// 这里统一做 TU 局部的名字转发。
using unool::protocol::MessageType;

// 阶段 4 · S1：以下两个函数从 source/ClientMain.cpp 逐字平移到组合根实现单元。
// 函数体、case 顺序、心跳回复、键位映射与线上 ClientInput.key 字节均未改动；
// 仅按用户 46.5 口径③B 删除 gamePhase 中两支只 println + TODO 的商城死分支
// （MessageType::ShopData / ShopResult），这两支原本不写回任何状态、不发任何包。

// 解析 Choice 包并更新渲染器（序列化统一走 MessageCodec）。
static void handleChoicePacket(sf::Packet& packet, GameRenderer& renderer) {
	auto parsed = unool::protocol::parseChoice(packet);
	if (!parsed) return;
	if (parsed->title.empty() && parsed->options.empty()) {
		renderer.clearChoicePrompt();
	}
	else {
		GameRenderer::Choice prompt;
		prompt.title = parsed->title;
		prompt.options = std::move(parsed->options);
		prompt.forced = parsed->forced;
		prompt.errorMsg = parsed->errorMsg;
		prompt.timeoutMs = parsed->timeoutMs;
		prompt.currentPage = parsed->currentPage;
		prompt.totalPages = parsed->totalPages;
		prompt.optionMeta = std::move(parsed->optionMeta);
		renderer.setChoicePrompt(prompt);
	}
}

// 游戏主循环
static void gamePhase(ClientNetwork& net, unool::shell::ConnectionState& conn,
					  GameRenderer& renderer, const std::string& titleBrackets) {
	renderer.setLocalPlayerId(conn.getPlayerId());
	sf::Clock clock;
	unool::shell::InputAdapter input(net, renderer);
	while (renderer.windowIsOpen()) {
		sf::Time elapsed = clock.restart();

		if (!input.pollAndDispatch()) return;

		// P1-3：连接状态机单一时序点，心跳回复/入站队列由 conn 统一驱动。
		conn.pump();

		if (!conn.isConnected()) {
			std::println("{} 与服务器断开连接，退出游戏", titleBrackets);
			break;
		}

		//每帧消化所有积压包，防止服务器多包推送时延迟累积
		// P1-3：DrainAll 由 conn.takeAny() 提供，心跳已由 pump 内部回复。
		while (auto next = conn.takeAny()) {
			auto [mt, packet] = std::move(*next);

			switch (mt) {
				case MessageType::ConnectionInfo: {
					if (auto data = unool::protocol::parseConnectionInfo(packet)) {
						conn.setPlayerId(data->playerId);
						std::println("{} 分配到玩家：{}", titleBrackets, conn.getUsername());
					}
					break;
				}
				case MessageType::GameStart: {
					// 房间模式：开局包携带本局座位号（0/1），座位是桌面内的玩法身份，
					// 与连接槽位号无关；渲染器据此认出「自己的手牌」。
					if (auto seat = unool::protocol::parseGameStart(packet)) {
						conn.setPlayerId(*seat);
						renderer.setLocalPlayerId(*seat);
						std::println("{} 游戏开始！本局座位：{}", titleBrackets, *seat);
					}
					else {
						std::println("{} 游戏开始！", titleBrackets);
					}
					break;
				}
				case MessageType::GameState: {
					GameState state;
					if (unool::protocol::parseGameState(packet, state)) renderer.updateState(state);
					break;
				}
				case MessageType::PointerUpdate: {
					if (auto data = unool::protocol::parsePointerUpdate(packet)) {
						renderer.updatePointer(data->playerId, data->selectedIndex);
					}
					break;
				}
				case MessageType::CharInfo: {
					CharInfo info;
					if (unool::protocol::parseCharInfo(packet, info)) {
						renderer.updateCharInfo(info.playerIndex, info.fullText);
					}
					break;
				}
				case MessageType::GameEnd: {
					if (auto result = unool::protocol::parseGameEnd(packet)) {
						if (result->hasWinner) {
							if (const std::string charName = renderer.getCharacterName(result->winnerId); !charName.empty()) {
								std::println("{} 游戏结束，{}（{}）获胜！", titleBrackets, result->winnerName, charName);
							}
							else {
								std::println("{} 游戏结束，{}获胜！", titleBrackets, result->winnerName);
							}
						}
						else {
							std::println("{} 游戏结束，无人获胜！", titleBrackets);
						}
					}
					break;
				}
				case MessageType::ConnectionRefused:
					std::println("{} 连接被拒绝（服务器已满）", titleBrackets);
					renderer.closeWindow();
					break;
				case MessageType::Choice:
					handleChoicePacket(packet, renderer);
					break;
				case MessageType::SkillInfoResponse: {
					//BP 预览的技能全文：服务端按角色名生成，写入渲染器缓存供面板显示
					if (auto strPayload = unool::protocol::parseStringPayload(packet)) {
						if (auto parsed = unool::protocol::parseSkillInfoResponse(strPayload->text)) {
							for (const auto& [name, text] : parsed->skills) {
								renderer.setSkillInfo(name, text);
							}
						}
					}
					break;
				}
				case MessageType::ShopPurchase:
					//客户端不接收此类型（客户端→服务器的购买请求），忽略
					break;
				default:
					break;
			}
		}

		//BP 预览：把本次候选项中缺失的技能描述批量取回（服务端内联处理，不进输入队列）
		if (auto names = renderer.takePendingSkillNames(); !names.empty()) {
			sf::Packet req = unool::protocol::makeSkillInfoRequestPacket(names);
			conn.send(req);
		}

		renderer.display();
	}
}

namespace unool::shell {

	namespace {

		// 阶段 4 · S3a（用户 49.10 口径③A）：组合根侧的商城意图实现。
		// 原 LoginScene::handleMouseClick 在这里同步构造并运行 ShopScene；
		// 房间功能上线后匹配横幅下线，商城不再需要共享匹配提示状态。
		class ShopOpener final : public IShopOpener {
		public:
			ShopOpener(GameRenderer& renderer, unool::shell::ConnectionState& conn,
			           unool::shell::ShopPresentationCache& cache)
				: renderer_{ renderer }, conn_{ conn }, cache_{ cache } {}

			void openShop(const std::string& username) override {
				ShopScene shop(renderer_, conn_, username, cache_);
				shop.run();
			}

		private:
			GameRenderer& renderer_;
			unool::shell::ConnectionState& conn_;
			unool::shell::ShopPresentationCache& cache_;
		};

	} // namespace

	ClientApp::ClientApp(ClientNetwork& net, const std::string& windowTitle,
						 const std::string& titleBrackets, const std::string& ipAddress,
						 unsigned short port)
		: net_{ net }, session_{ net }, windowTitle_{ windowTitle },
		titleBrackets_{ titleBrackets }, ipAddress_{ ipAddress }, port_{ port } {}

	int ClientApp::run() {
		GameRenderer::Config rendererConfig(std::format("UNOOL - {}", windowTitle_));
		GameRenderer renderer(rendererConfig);

		// P1-3：单一连接状态机在此构造，Scene 与 gamePhase 共用同一个实例，
		// 跨阶段留包（GameStart）由它的显式队列承载。
		unool::shell::ConnectionState conn(net_);
		// P1-4：商城展示缓存由组合根持有，跨多次进入商城保留（原 static 语义）。
		unool::shell::ShopPresentationCache shopCache;

		// S3a：商城由组合根经 IShopOpener 打开。
		ShopOpener shopOpener(renderer, conn, shopCache);

		// 被「同账号在别处登录」顶下线后不退出程序：带着原因重连并回到登录界面。
		// 其它断线（服务端关闭 / 网络断）仍然直接退出，避免无意义的重连风暴。
		std::string notice;

		while (renderer.windowIsOpen()) {
			if (!session_.connect(ipAddress_, port_)) {
				// 不用 system("pause")：主线程一卡住，游戏窗口就不重绘，
				// 表现为「客户端卡死」（而且控制台被窗口挡着，等按键等于永远等）。
				std::println(stderr, "{} 连接服务器失败，2 秒后退出", titleBrackets_);
				std::this_thread::sleep_for(std::chrono::seconds(2));
				return 1;
			}
			conn.clearKickedOffline();

			// 登录场景：登录成功后进入大厅，从大厅选择开始游戏或进入商城
			LoginScene login(renderer, conn, shopOpener, titleBrackets_, notice);
			notice.clear();
			auto result = login.run();

			if (!result.ok) {
				if (conn.wasKickedOffline()) {
					notice = conn.kickedReason();
					std::println(stderr, "{} {}", titleBrackets_, notice);
					continue;   // 新连接 + 登录界面，提示文案随 notice 带入
				}
				if (!renderer.windowIsOpen()) return 0;   // 用户主动关窗
				std::println(stderr, "{} 登录未完成或连接已断开，2 秒后退出", titleBrackets_);
				std::this_thread::sleep_for(std::chrono::seconds(2));
				return 1;
			}

			// LoginScene 现在要等到服务端 GameStart（双方在房间内准备）才返回
			std::println("{} 房间已开局，进入游戏...", titleBrackets_);
			gamePhase(net_, conn, renderer, titleBrackets_);

			// 对局中也可能被顶下线：同样回到登录界面而不是直接退出
			if (conn.wasKickedOffline()) {
				notice = conn.kickedReason();
				std::println(stderr, "{} {}", titleBrackets_, notice);
				continue;
			}
			return 0;
		}

		return 0;
	}

}
