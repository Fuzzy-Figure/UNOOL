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
#include <Windows.h>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <format>
#include <print>
#include <utility>

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
				case MessageType::GameStart:
					std::println("{} 游戏开始！", titleBrackets);
					break;
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
		// 上移后只换调用者，栈上生命周期、同步 run() 与返回后回到大厅的时序不变；
		// 构造参数在原 renderer / net / username 之外多一个匹配提示状态引用。
		class ShopOpener final : public IShopOpener {
		public:
			ShopOpener(GameRenderer& renderer, unool::shell::ConnectionState& conn,
			           unool::shell::ShopPresentationCache& cache)
				: renderer_{ renderer }, conn_{ conn }, cache_{ cache } {}

			void openShop(const std::string& username, MatchNoticeState& notice) override {
				ShopScene shop(renderer_, conn_, username, notice, cache_);
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
		// 与原 ClientMain.cpp::main 的装配顺序逐字一致：连接 → 渲染器 → 登录场景 → 游戏阶段。
		if (!session_.connect(ipAddress_, port_)) {
			std::println(stderr, "{} 连接服务器失败", titleBrackets_);
			system("pause");
			return 1;
		}

		GameRenderer::Config rendererConfig(std::format("UNOOL - {}", windowTitle_));
		GameRenderer renderer(rendererConfig);

		// P1-3：单一连接状态机在此构造，Scene 与 gamePhase 共用同一个实例，
		// 跨阶段留包（GameStart）由它的显式队列承载。
		unool::shell::ConnectionState conn(net_);
		// P1-4：商城展示缓存由组合根持有，跨多次进入商城保留（原 static 语义）。
		unool::shell::ShopPresentationCache shopCache;

		// S3a：商城由组合根经 IShopOpener 打开；只在登录场景之前多一个局部对象，
		// 「连接 → 渲染器 → 登录场景 → 游戏阶段」的线性顺序不变。
		ShopOpener shopOpener(renderer, conn, shopCache);

		// 登录场景：登录成功后进入大厅，从大厅选择开始游戏或进入商城
		LoginScene login(renderer, conn, shopOpener, titleBrackets_);
		auto session = login.run();
		if (!session.ok) {
			std::println(stderr, "{} 登录未完成或连接已断开，退出", titleBrackets_);
			system("pause");
			return 1;
		}

		// LoginScene 现在要等到服务端 GameStart 才返回；退出匹配只是回到大厅继续等用户操作
		std::println("{} 匹配完成，进入游戏...", titleBrackets_);
		gamePhase(net_, conn, renderer, titleBrackets_);

		return 0;
	}

}
