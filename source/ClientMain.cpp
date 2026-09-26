#include "../header/GameRenderer.h"
#include "../header/LoginScene.h"
#include "../header/Socket.h"
#include "../header/utils.h"
#include "../header/AccountProtocol.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>


// 解析 Choice 包并更新渲染器
static void handleChoicePacket(sf::Packet& packet, GameRenderer& renderer) {
	sf::String titleSfStr;
	packet >> titleSfStr;
	std::wstring title = titleSfStr.toWideString();

	std::size_t optionCount;
	packet >> optionCount;
	std::vector<std::wstring> options;
	for (std::size_t i = 0; i < optionCount; ++i) {
		sf::String sfStr;
		packet >> sfStr;
		options.push_back(sfStr.toWideString());
	}

	bool forced;
	packet >> forced;

	sf::String errorSfStr;
	packet >> errorSfStr;
	std::wstring errorMsg = errorSfStr.toWideString();

	bool hasTimeout;
	packet >> hasTimeout;
	std::optional<std::size_t> timeoutMs;
	if (hasTimeout) {
		std::uint64_t timeoutValue;
		packet >> timeoutValue;
		timeoutMs = static_cast<std::size_t>(timeoutValue);
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

		auto packetOpt = net.receivePacket();
		if (packetOpt.has_value()) {
			sf::Packet packet = packetOpt.value();
			int msgType;
			if (!(packet >> msgType)) continue;

			switch (static_cast<MessageType>(msgType)) {
			case MessageType::ConnectionInfo: {
				std::size_t playerId;
				if (packet >> playerId) {
					net.setPlayerId(playerId);
					std::cout << titleBrackets << " 分配到玩家ID：" << playerId << std::endl;
				}
				break;
			}
			case MessageType::GameStart:
				std::cout << titleBrackets << " 游戏开始！" << std::endl;
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
						std::cout << titleBrackets << " 游戏结束，玩家" << winnerId << "获胜！" << std::endl;
					}
					else {
						std::cout << titleBrackets << " 游戏结束，无人获胜！" << std::endl;
					}
				}
				break;
			}
			case MessageType::ConnectionRefused:
				std::cout << titleBrackets << " 连接被拒绝（服务器已满）" << std::endl;
				renderer.closeWindow();
				break;
			case MessageType::Choice:
				handleChoicePacket(packet, renderer);
				break;
			default:
				break;
			}
		}

		renderer.display();

		if (elapsed.asSeconds() < 1.0f / 60.0f) {
			sf::sleep(sf::seconds(1.0f / 60.0f - elapsed.asSeconds()));
		}
	}
}

int main() {
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	const std::string windowTitle = "Client";
	const std::string windowTitleWithBrackets = "[Client]";
	std::cout << windowTitleWithBrackets << " 启动客户端..." << std::endl;

	ClientNetwork clientNetwork;
	const auto& config = unool::getClientConfig();
	std::string ipAddress = config["server"]["ip"];
	unsigned short port = config["server"]["port"];

	if (!clientNetwork.connect(ipAddress, port)) {
		std::cerr << windowTitleWithBrackets << " 连接服务器失败" << std::endl;
		system("pause");
		return 1;
	}

	GameRenderer::Config rendererConfig("UNOOL - " + windowTitle);
	GameRenderer renderer(rendererConfig);

	LoginScene login(renderer, clientNetwork, windowTitleWithBrackets);
	auto session = login.run();
	if (!session.ok) {
		std::cerr << windowTitleWithBrackets << " 登录未完成，退出" << std::endl;
		system("pause");
		return 1;
	}

	std::cout << windowTitleWithBrackets << " 已登录，等待对手登录并开始游戏..." << std::endl;
	gamePhase(clientNetwork, renderer, windowTitleWithBrackets);

	std::this_thread::sleep_for(3s);
	system("pause");
	return 0;
}
