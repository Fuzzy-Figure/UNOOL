#include "../header/GameRenderer.h"
#include "../header/Socket.h"
#include "../header/utils.h"
#include "../header/AccountProtocol.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include <string>
#include <iostream>


struct AccountSession {
	std::string username;
	int points = 0;
	int wins = 0;
	int losses = 0;
};

struct WindowTitle {
	std::string name;
	std::string withBrackets;
};

static WindowTitle parseCommandLineArgs(int argc, char* argv[]) {
	std::string windowTitle = "Client ?";
	std::string windowTitleWithBrackets = "[Client ?]";
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--title" || arg == "-t") {
			if (i + 1 < argc) {
				windowTitle = argv[++i];
				windowTitleWithBrackets = "[" + windowTitle + "]";
			}
		}
	}
	return { windowTitle, windowTitleWithBrackets };
}

// 等待一个账号响应包；超时返回 nullopt
static std::optional<AccountProtocol::AccountResponse> waitForAccountResponse(
	ClientNetwork& net, MessageType expectedResp, const std::string& titleBrackets) {
	auto startTime = std::chrono::steady_clock::now();
	while (true) {
		if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(5)) {
			std::cout << "响应超时，请重试" << std::endl;
			return std::nullopt;
		}

		net.update();
		auto packetOpt = net.receivePacket();
		if (!packetOpt.has_value()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		sf::Packet packet = packetOpt.value();
		int msgType;
		if (!(packet >> msgType)) continue;

		if (static_cast<MessageType>(msgType) == expectedResp) {
			return AccountProtocol::parseAccountResponse(packet);
		}
		else if (static_cast<MessageType>(msgType) == MessageType::ConnectionInfo) {
			std::size_t pid;
			if (packet >> pid) {
				net.setPlayerId(pid);
				std::cout << titleBrackets << " 分配到玩家ID: " << pid << std::endl;
			}
		}
	}
}

// 等待用户名预检响应；返回 exists 或 nullopt（超时）
static std::optional<bool> waitForCheckUsernameResponse(ClientNetwork& net, const std::string& titleBrackets) {
	auto startTime = std::chrono::steady_clock::now();
	while (true) {
		if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(5)) {
			std::cout << "响应超时，请重试" << std::endl;
			return std::nullopt;
		}

		net.update();
		auto packetOpt = net.receivePacket();
		if (!packetOpt.has_value()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		sf::Packet packet = packetOpt.value();
		int msgType;
		if (!(packet >> msgType)) continue;

		if (static_cast<MessageType>(msgType) == MessageType::CheckUsernameResponse) {
			return AccountProtocol::parseCheckUsernameResponse(packet);
		}
		else if (static_cast<MessageType>(msgType) == MessageType::ConnectionInfo) {
			std::size_t pid;
			if (packet >> pid) {
				net.setPlayerId(pid);
				std::cout << titleBrackets << " 分配到玩家ID: " << pid << std::endl;
			}
		}
	}
}

// 账号阶段：注册/登录菜单循环，返回登录成功的账号信息
static AccountSession accountPhase(ClientNetwork& net, const std::string& titleBrackets) {
	while (true) {
		// 处理已收到的非账号包（如 ConnectionInfo），避免菜单阻塞时丢包
		net.update();
		while (auto early = net.receivePacket()) {
			int t;
			if (!(*early >> t)) continue;
			if (static_cast<MessageType>(t) == MessageType::ConnectionInfo) {
				std::size_t pid;
				if (*early >> pid) {
					net.setPlayerId(pid);
					std::cout << titleBrackets << " 分配到玩家ID: " << pid << std::endl;
				}
			}
		}

		std::cout << "\n========== UNOOL 账号系统 ==========\n"
			<< "1. 注册\n"
			<< "2. 登录\n"
			<< "请选择: ";
		std::optional<int> choice = unool::input::safeReadInt(1, 2);
		if (!choice.has_value()) {
			std::cout << "无效选项，请重新输入" << std::endl;
			continue;
		}

		std::string username, password;
		std::cout << "用户名: ";
		username = unool::input::safeReadLine();
		if (username.empty()) {
			std::cout << "用户名不能为空，请重新输入" << std::endl;
			continue;
		}

		// 注册：先预检用户名是否存在
		if (choice == 1) {
			sf::Packet checkReq = AccountProtocol::makeCheckUsernameRequest(username);
			if (!net.send(checkReq)) { std::cout << "发送失败，请重试" << std::endl; continue; }
			auto exists = waitForCheckUsernameResponse(net, titleBrackets);
			if (!exists) continue;
			if (*exists) {
				std::cout << "该用户名已存在，请重新选择" << std::endl;
				continue;
			}
			// 用户名可用，继续输入密码
			std::cout << "密码: ";
			password = unool::input::safeReadNoSpace();
			if (password.empty()) {
				std::cout << "密码不能为空或含有空格，请重新输入" << std::endl;
				continue;
			}

			sf::Packet req = AccountProtocol::makeRegisterRequest(username, password);
			if (!net.send(req)) { std::cout << "发送失败，请重试" << std::endl; continue; }
			auto resp = waitForAccountResponse(net, MessageType::RegisterResponse, titleBrackets);
			if (!resp) continue;
			std::cout << (resp->ok ? "注册成功" : "注册失败") << ": " << resp->msg << std::endl;
			if (!resp->ok) continue;
			// 注册成功，自动登录
			std::cout << "自动登录中..." << std::endl;
		}
		else {
			// 登录：直接输入密码
			std::cout << "密码: ";
			password = unool::input::safeReadNoSpace();
			if (password.empty()) {
				std::cout << "密码不能为空或含有空格，请重新输入" << std::endl;
				continue;
			}
		}

		// 登录
		sf::Packet req = AccountProtocol::makeLoginRequest(username, password);
		if (!net.send(req)) { std::cout << "发送失败，请重试" << std::endl; continue; }
		auto resp = waitForAccountResponse(net, MessageType::LoginResponse, titleBrackets);
		if (!resp) continue;
		if (!resp->ok) { std::cout << "登录失败: " << resp->msg << std::endl; continue; }

		AccountSession s;
		s.username = username;
		s.points = resp->points;
		s.wins = resp->wins;
		s.losses = resp->losses;
		std::cout << "登录成功: " << resp->msg << std::endl;
		std::cout << "当前积分: " << resp->points
			<< "  胜场: " << resp->wins
			<< "  负场: " << resp->losses << std::endl;
		return s;
	}
}

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
				if (!renderer.hasChoiceOptions() && (renderer.isLocalTurn() || renderer.isChoiceActive()) && (sc == sf::Keyboard::Scancode::Left || sc == sf::Keyboard::Scancode::A)) {
					renderer.movePointerLeft(net.getPlayerId());
				}
				else if (!renderer.hasChoiceOptions() && (renderer.isLocalTurn() || renderer.isChoiceActive()) && (sc == sf::Keyboard::Scancode::Right || sc == sf::Keyboard::Scancode::D)) {
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
			renderer.updateCharInfo(info.playerIndex, info.levelStr, info.skills);
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

int main(int argc, char* argv[]) {
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	auto [windowTitle, windowTitleWithBrackets] = parseCommandLineArgs(argc, argv);
	std::cout << windowTitleWithBrackets << " 启动客户端..." << std::endl;

	ClientNetwork clientNetwork;
	const auto& config = unool::getConfig();
	std::string ipAddress = config["server"]["ip"];
	unsigned short port = config["server"]["port"];

	if (!clientNetwork.connect(ipAddress, port)) {
		std::cerr << windowTitleWithBrackets << " 连接服务器失败" << std::endl;
		system("pause");
		return 1;
	}

	accountPhase(clientNetwork, windowTitleWithBrackets);
	std::cout << windowTitleWithBrackets << " 已登录，等待对手登录并开始游戏..." << std::endl;

	GameRenderer::Config rendererConfig("UNOOL - " + windowTitle);
	GameRenderer renderer(rendererConfig);
	gamePhase(clientNetwork, renderer, windowTitleWithBrackets);

	std::this_thread::sleep_for(3s);
	system("pause");
	return 0;
}
