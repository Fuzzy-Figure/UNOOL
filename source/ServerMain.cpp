#include "../header/GameLogic.h"
#include "../header/Socket.h"
#include "../header/utils.h"
#include <Windows.h>
#include <chrono>
#include <thread>

constexpr std::array<std::array<int, 6>, 6> scoreboard = { {
		//     败者  S   A   B   C   D   F
		//胜者
		/*S*/      {10,  8,  5,  3,  2,  1},
		/*A*/      {12, 10,  8,  5,  3,  2},
		/*B*/      {15, 12, 10,  8,  5,  3},
		/*C*/      {20, 15, 12, 10,  8,  5},
		/*D*/      {30, 20, 15, 12, 10,  8},
		/*F*/      {50, 30, 20, 15, 12, 10}
	   } };


int main() {
	//设置控制台输入输出编码为 UTF-8
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	std::cout << "[Server] 启动服务器..." << std::endl;

	ServerNetwork serverNetwork;
	unsigned short port = 8888;
	if (!serverNetwork.start(port)) {
		std::cerr << "[Server] 启动失败" << std::endl;
		std::this_thread::sleep_for(3s);
		return 1;
	}

	GameLogic gameLogic(serverNetwork);

	bool gameStarted = false;
	bool gameEnded = false;

	std::cout << "[Server] 等待客户端连接..." << std::endl;

	while (!serverNetwork.isReady()) {
		serverNetwork.update();
		std::this_thread::sleep_for(16ms);
	}

	try {
		gameStarted = true;
		std::cout << "[Server] 游戏开始！" << std::endl;

		if (unool::getConfig().contains("characters")) {
			//指定角色
			const auto& chars = unool::getConfig()["characters"];
			if (chars.size() != 2)
				throw std::invalid_argument("指定角色时，角色数量必须为2");
			gameLogic.initPlayers({ chars[0], chars[1] });
		}
		else { //随机选角色
			gameLogic.initPlayers();
		}

		gameLogic.broadcastState();

		while (gameStarted && !gameLogic.isGameOver()) {
			if (gameLogic.runTurn()) {
				// 一局结束，处理体力扣除
				gameLogic.checkRoundEnd();
				gameLogic.broadcastState();

				if (gameLogic.isGameOver()) {
					// 游戏结束
					std::optional<std::size_t> winnerId = gameLogic.getWinnerId();
					serverNetwork.sendGameEnd(winnerId);
					if (winnerId.has_value()) {
						std::cout << "[Server] 游戏结束，玩家" << winnerId.value() << "获胜！" << std::endl;
					}
					else {
						std::cout << "[Server] 游戏结束，无人获胜！" << std::endl;
					}
					break;
				}
				else {
					// 开始新一局
					gameLogic.resetRound();
				}
			}
		}
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	while (true) {
		std::this_thread::sleep_for(1s);
	}

	return 0;
}
