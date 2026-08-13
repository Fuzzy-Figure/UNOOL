#include "../header/GameLogic.h"
#include "../header/Socket.h"
#include "../header/Character.h"
#include "../header/utils.h"
#include <Windows.h>
#include <vector>
#include <chrono>

constexpr std::array<std::array<int, 6>, 6> scoreboard = { {
		//    败者  S   A   B   C   D   F
		//胜者
		/*S*/     {10,  8,  5,  3,  2,  1},
		/*A*/     {12, 10,  8,  5,  3,  2},
		/*B*/     {15, 12, 10,  8,  5,  3},
		/*C*/     {20, 15, 12, 10,  8,  5},
		/*D*/     {30, 20, 15, 12, 10,  8},
		/*F*/     {50, 30, 20, 15, 12, 10}
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
		Sleep(3000);
		return 1;
	}

	GameLogic gameLogic(serverNetwork);

	bool gameStarted = false;
	bool gameEnded = false;

	std::cout << "[Server] 等待客户端连接..." << std::endl;

	while (!serverNetwork.isReady()) {
		serverNetwork.update();
		Sleep(16);
	}

	try {
		gameStarted = true;
		std::cout << "[Server] 游戏开始！" << std::endl;
		//gameLogic.initPlayers();
		gameLogic.initPlayers({ "Notch", "白板" });

		gameLogic.broadcastState();

		while (gameStarted && !gameLogic.isGameOver()) {
			if (gameLogic.runTurn()) {
				// 一局结束，处理体力扣除
				gameLogic.checkRoundEnd();
				gameLogic.broadcastState();

				if (gameLogic.isGameOver()) {
					// 游戏结束
					std::size_t winnerId = gameLogic.getWinnerId();
					serverNetwork.sendGameEnd(winnerId);
					std::cout << "[Server] 游戏结束，玩家" << winnerId << "获胜！" << std::endl;
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
		Sleep(1000);
	}

	return 0;
}
