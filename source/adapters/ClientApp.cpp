#include "adapters/ClientApp.h"

#include "PacketDispatcher.h"
#include "GameRenderer.h"
#include "LoginScene.h"
#include "ShopScene.h"
#include "Socket.h"
#include "adapters/IShopOpener.h"
#include "utils.h"
#include "adapters/JsonConfig.h"
#include "AccountProtocol.h"
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

// 解析 Choice 包并更新渲染器
static void handleChoicePacket(sf::Packet& packet, GameRenderer& renderer) {
	std::string title;
	packet >> title;

	std::size_t optionCount;
	packet >> optionCount;
	std::vector<std::string> options;
	for (std::size_t i = 0; i < optionCount; ++i) {
		std::string optionStr;
		packet >> optionStr;
		options.push_back(optionStr);
	}

	bool forced;
	packet >> forced;

	std::string errorMsg;
	packet >> errorMsg;

	bool hasTimeout;
	packet >> hasTimeout;
	std::optional<std::size_t> timeoutMs;
	if (hasTimeout) {
		std::size_t timeoutValue;
		packet >> timeoutValue;
		timeoutMs = timeoutValue;
	}

	std::size_t currentPage;
	packet >> currentPage;
	std::size_t totalPages;
	packet >> totalPages;

	if (title.empty() && options.empty()) {
		renderer.clearChoicePrompt();
	}
	else {
		GameRenderer::Choice prompt;
		prompt.title = title;
		prompt.options = options;
		prompt.forced = forced;
		prompt.errorMsg = errorMsg;
		prompt.timeoutMs = timeoutMs;
		prompt.currentPage = currentPage;
		prompt.totalPages = totalPages;
		renderer.setChoicePrompt(prompt);
	}
}

// 游戏主循环
static void gamePhase(ClientNetwork& net, GameRenderer& renderer, const std::string& titleBrackets) {
	renderer.setLocalPlayerId(net.getPlayerId());
	sf::Clock clock;
	while (renderer.windowIsOpen()) {
		sf::Time elapsed = clock.restart();

		for (auto event = renderer.pollEvent(); event; event = renderer.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				renderer.closeWindow();
				return;
			}
			if (event->is<sf::Event::KeyPressed>()) {
				auto keyEvent = event->getIf<sf::Event::KeyPressed>();
				if (keyEvent) {
					auto sc = keyEvent->scancode;
					if (renderer.canSelectLocal() && (sc == sf::Keyboard::Scancode::Left || sc == sf::Keyboard::Scancode::A)) {
						renderer.movePointerLeft(net.getPlayerId());
					}
					else if (renderer.canSelectLocal() && (sc == sf::Keyboard::Scancode::Right || sc == sf::Keyboard::Scancode::D)) {
						renderer.movePointerRight(net.getPlayerId());
					}
					else {
						net.sendClientInput(sc, renderer.getSelectedIndex(net.getPlayerId()));
					}
				}
			}
			if (event->is<sf::Event::MouseButtonPressed>()) {
				auto mouseEvent = event->getIf<sf::Event::MouseButtonPressed>();
				if (mouseEvent && mouseEvent->button == sf::Mouse::Button::Left) {
					sf::Vector2f mousePos = static_cast<sf::Vector2f>(mouseEvent->position);
					renderer.handleMouseClick(mousePos);
				}
			}
		}

		net.update();

		if (!net.isConnected()) {
			std::println("{} 与服务器断开连接，退出游戏", titleBrackets);
			break;
		}

		//每帧消化所有积压包，防止服务器多包推送时延迟累积
		// 阶段 4 · S2：DrainAll 模式，心跳回复由 PacketDispatcher 统一处理。
		while (auto next = unool::adapters::PacketDispatcher::next(net)) {
			auto [mt, packet] = std::move(*next);

			switch (mt) {
				case MessageType::ConnectionInfo: {
					std::size_t playerId;
					if (packet >> playerId) {
						net.setPlayerId(playerId);
						std::println("{} 分配到玩家：{}", titleBrackets, net.getUsername());
					}
					break;
				}
				case MessageType::GameStart:
					std::println("{} 游戏开始！", titleBrackets);
					break;
				case MessageType::GameState: {
					GameState state;
					packet >> state;
					renderer.updateState(state);
					break;
				}
				case MessageType::PointerUpdate: {
					std::size_t playerId, selectedIndex;
					packet >> playerId >> selectedIndex;
					renderer.updatePointer(playerId, selectedIndex);
					break;
				}
				case MessageType::CharInfo: {
					CharInfo info;
					packet >> info;
					renderer.updateCharInfo(info.playerIndex, info.fullText);
					break;
				}
				case MessageType::GameEnd: {
					bool hasWinner;
					if (packet >> hasWinner) {
						if (hasWinner) {
							std::size_t winnerId;
							std::string winnerName;
							packet >> winnerId >> winnerName;
							if (const std::string charName = renderer.getCharacterName(winnerId); !charName.empty()) {
								std::println("{} 游戏结束，{}（{}）获胜！", titleBrackets, winnerName, charName);
							}
							else {
								std::println("{} 游戏结束，{}获胜！", titleBrackets, winnerName);
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
				case MessageType::ShopPurchase:
					//客户端不接收此类型（客户端→服务器的购买请求），忽略
					break;
				default:
					break;
			}
		}

		renderer.display();
	}
}

namespace unool::adapters {

	namespace {

		// 阶段 4 · S3a（用户 49.10 口径③A）：组合根侧的商城意图实现。
		// 原 LoginScene::handleMouseClick 在这里同步构造并运行 ShopScene；
		// 上移后只换调用者，构造参数（renderer / net / username）、栈上生命周期、
		// 同步 run() 与返回后回到大厅的时序全部不变。
		class ShopOpener final : public IShopOpener {
		public:
			ShopOpener(GameRenderer& renderer, ClientNetwork& net)
				: renderer_{ renderer }, net_{ net } {}

			void openShop(const std::string& username) override {
				ShopScene shop(renderer_, net_, username);
				shop.run();
			}

		private:
			GameRenderer& renderer_;
			ClientNetwork& net_;
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

		// S3a：商城由组合根经 IShopOpener 打开；只在登录场景之前多一个局部对象，
		// 「连接 → 渲染器 → 登录场景 → 游戏阶段」的线性顺序不变。
		ShopOpener shopOpener(renderer, net_);

		// 登录场景：登录成功后进入大厅，从大厅选择开始游戏或进入商城
		LoginScene login(renderer, net_, shopOpener, titleBrackets_);
		auto session = login.run();
		if (!session.ok) {
			std::println(stderr, "{} 登录未完成，退出", titleBrackets_);
			system("pause");
			return 1;
		}

		std::println("{} 已登录，等待对手登录并开始游戏...", titleBrackets_);
		gamePhase(net_, renderer, titleBrackets_);

		return 0;
	}

}
