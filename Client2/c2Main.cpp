#include "../header/GameRenderer.h"
#include "../header/Socket.h"
#include "../header/utils.h"
#include <Windows.h>

int main() {
	//设置控制台输入输出编码为 UTF-8
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	std::cout << "[Client2] 启动客户端..." << std::endl;

	ClientNetwork clientNetwork;
	const auto& config = unool::getConfig();
	std::string ipAddress = config["server"]["ip"];
	unsigned short port = config["server"]["port"];

	if (!clientNetwork.connect(ipAddress, port)) {
		std::cerr << "[Client2] 连接服务器失败" << std::endl;
		system("pause");
		return 1;
	}

	GameRenderer::Config rendererConfig("UNOOL - Client2");
	GameRenderer renderer(rendererConfig);

	bool gameStarted = false;
	bool gameEnded = false;

	std::cout << "[Client2] 已连接到服务器，等待游戏开始..." << std::endl;

	sf::Clock clock;
	while (renderer.windowIsOpen()) {
		sf::Time elapsed = clock.restart();

		for (auto event = renderer.pollEvent(); event; event = renderer.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				renderer.closeWindow();
				return 0;
			}
			if (event->is<sf::Event::KeyPressed>()) {
				auto keyEvent = event->getIf<sf::Event::KeyPressed>();
				if (keyEvent) {
					clientNetwork.sendClientInput(keyEvent->scancode);
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

		clientNetwork.update();

		auto packetOpt = clientNetwork.receivePacket();
		if (packetOpt.has_value()) {
			sf::Packet packet = packetOpt.value();

			int msgType;
			if (!(packet >> msgType)) continue;

			switch (static_cast<MessageType>(msgType)) {
			case MessageType::ConnectionInfo: {
				std::size_t playerId;
				if (packet >> playerId) {
					clientNetwork.setPlayerId(playerId);
					std::cout << "[Client2] 分配到玩家ID：" << playerId << std::endl;
				}
				break;
			}
			case MessageType::GameStart: {
				gameStarted = true;
				std::cout << "[Client2] 游戏开始！" << std::endl;
				break;
			}
			case MessageType::GameState: {
				GameState state;
				packet >> state;
				renderer.updateState(state);
				break;
			}
			case MessageType::GameEnd: {
				std::size_t winnerId;
				if (packet >> winnerId) {
					gameEnded = true;
					std::cout << "[Client2] 游戏结束，玩家" << winnerId << "获胜！" << std::endl;
				}
				break;
			}
			case MessageType::ConnectionRefused: {
				std::cout << "[Client2] 连接被拒绝（服务器已满）" << std::endl;
				renderer.closeWindow();
				break;
			}
			case MessageType::Choice: {
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

				if (title.empty() && options.empty()) {
					renderer.clearChoicePrompt();
				}
				else {
					GameRenderer::Choice prompt;
					prompt.title = title;
					prompt.options = options;
					prompt.forced = forced;
					prompt.errorMsg = errorMsg;
					renderer.setChoicePrompt(prompt);
				}
				break;
			}
			default:
				break;
			}
		}

		renderer.display();

		if (elapsed.asSeconds() < 1.0f / 60.0f) {
			sf::sleep(sf::seconds(1.0f / 60.0f - elapsed.asSeconds()));
		}
	}

	Sleep(3000);
	system("pause");
	return 0;
}