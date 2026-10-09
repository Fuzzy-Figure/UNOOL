#include "GameRenderer.h"
#include "LoginScene.h"
#include "ShopScene.h"
#include "Socket.h"
#include "utils.h"
#include "AccountProtocol.h"
#include "ShopData.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include <string>



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
		while (auto packetOpt = net.receivePacket()) {
			sf::Packet packet = std::move(*packetOpt);
			int msgType;
			if (!(packet >> msgType)) continue;

			switch (static_cast<MessageType>(msgType)) {
				case MessageType::ConnectionInfo: {
					std::size_t playerId;
					if (packet >> playerId) {
						net.setPlayerId(playerId);
						std::println("{} 分配到玩家ID：{}", titleBrackets, playerId);
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
							packet >> winnerId;
							std::println("{} 游戏结束，玩家{}获胜！", titleBrackets, winnerId);
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
				case MessageType::Heartbeat: {
					//收到服务器心跳，立即回一个 Heartbeat 让服务器知道自己还活着
					sf::Packet hb;
					hb << static_cast<int>(MessageType::Heartbeat);
					net.send(hb);
					break;
				}
				case MessageType::ShopData: {
					//商城数据（JSON 字符串载荷）：解析后供 ShopScene 使用
					std::string jsonStr;
					if (packet >> jsonStr) {
						try {
							nlohmann::json j = nlohmann::json::parse(jsonStr);
							ShopData shopData = j.get<ShopData>();
							std::println("{} 收到商城数据：积分{}，角色{}个，道具{}个",
										 titleBrackets, shopData.points,
										 shopData.characters.size(), shopData.items.size());
							// TODO: ShopScene 将基于 shopData 渲染商城界面（Task 7）
						}
						catch (const std::exception& e) {
							std::println(stderr, "{} 商城数据解析失败：{}", titleBrackets, e.what());
						}
					}
					break;
				}
				case MessageType::ShopResult: {
					//购买结果（JSON 字符串载荷）
					std::string jsonStr;
					if (packet >> jsonStr) {
						try {
							nlohmann::json j = nlohmann::json::parse(jsonStr);
							ShopPurchaseResult result = j.get<ShopPurchaseResult>();
							std::println("{} 购买结果：{}（{}），剩余积分 {}",
										 titleBrackets,
										 (result.ok ? "成功" : "失败"),
										 result.msg, result.points);
							// TODO: ShopScene 将基于 result 更新 UI 和积分显示（Task 7）
						}
						catch (const std::exception& e) {
							std::println(stderr, "{} 购买结果解析失败：{}", titleBrackets, e.what());
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

		renderer.display();
	}
}

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

	if (!clientNetwork.connect(ipAddress, port)) {
		std::println(stderr, "{} 连接服务器失败", windowTitleWithBrackets);
		system("pause");
		return 1;
	}

	GameRenderer::Config rendererConfig(std::format("UNOOL - {}", windowTitle));
	GameRenderer renderer(rendererConfig);

	// 登录循环：用户可从登录界面进入商城，从商城返回后回到登录界面
	while (true) {
		LoginScene login(renderer, clientNetwork, windowTitleWithBrackets);
		auto session = login.run();
		if (!session.ok) {
			std::println(stderr, "{} 登录未完成，退出", windowTitleWithBrackets);
			system("pause");
			return 1;
		}
		// 商城入口：登录成功后用户点击了"积分商城"，进入 ShopScene
		if (session.enterShop) {
			std::println("{} 进入积分商城...", windowTitleWithBrackets);
			ShopScene shop(renderer, clientNetwork, session.username);
			shop.run();
			// 商城退出后若连接已断开则退出
			if (!clientNetwork.isConnected() || !renderer.windowIsOpen()) {
				std::println(stderr, "{} 与服务器断开连接，退出", windowTitleWithBrackets);
				system("pause");
				return 1;
			}
			continue; // 回到登录界面
		}
		break;
	}

	std::println("{} 已登录，等待对手登录并开始游戏...", windowTitleWithBrackets);
	gamePhase(clientNetwork, renderer, windowTitleWithBrackets);

	return 0;
}
