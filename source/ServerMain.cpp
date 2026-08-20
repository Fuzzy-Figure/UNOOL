#include "../header/GameLogic.h"
#include "../header/Socket.h"
#include "../header/utils.h"
#include "../header/UserDB.h"
#include <Windows.h>
#include <chrono>
#include <thread>

// 根据 config.json 初始化角色（指定或随机）
static void initCharacters(GameLogic& gameLogic) {
	if (unool::getConfig().contains("characters")) {
		const auto& chars = unool::getConfig()["characters"];
		if (chars.size() != 2)
			throw std::invalid_argument("指定角色时，角色数量必须为2");
		gameLogic.initPlayers({ chars[0], chars[1] });
	}
	else {
		gameLogic.initPlayers();
	}
}

// 处理游戏结束：发送 GameEnd 包、按角色等级加分、打印日志
static void handleGameOver(ServerNetwork& serverNetwork, GameLogic& gameLogic) {
	std::optional<std::size_t> winnerId = gameLogic.getWinnerId();
	serverNetwork.sendGameEnd(winnerId);
	gameLogic.clearMatchCount();

	if (winnerId.has_value()) {
		std::size_t wId = winnerId.value();
		std::size_t lId = 1 - wId;

		auto& players = gameLogic.getPlayers();
		Character::Level wLv = players[wId].get().characterLevel();
		Character::Level lLv = players[lId].get().characterLevel();
		const auto& slots = serverNetwork.getClientSlots();
		UserDB::instance().addMatchResult(
			slots[wId].username, slots[lId].username, wLv, lLv,
			players[wId].get().getHp() == players[wId].get().getMaxHp());

		std::cout << "[Server] 游戏结束，玩家" << wId << "获胜！" << std::endl;
	}
	else {
		std::cout << "[Server] 游戏结束，无人获胜！" << std::endl;
	}
}

// 游戏主循环
static void gameLoop(ServerNetwork& serverNetwork, GameLogic& gameLogic) {
	while (!gameLogic.isGameOver()) {
		bool roundEnded = gameLogic.runTurn();

		// 回合结束后立即检查游戏是否结束（技能杀人等情况）
		if (gameLogic.isGameOver()) {
			handleGameOver(serverNetwork, gameLogic);
			break;
		}

		if (!roundEnded) continue;

		// 一局结束，处理体力扣除
		gameLogic.checkRoundEnd();
		gameLogic.broadcastState();

		if (gameLogic.isGameOver()) {
			handleGameOver(serverNetwork, gameLogic);
			break;
		}
		// 开始新一局
		gameLogic.resetRound();
	}
}

int main() {
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

	std::cout << "[Server] 等待客户端连接..." << std::endl;
	while (!serverNetwork.isReady()) {
		serverNetwork.update();
		std::this_thread::sleep_for(16ms);
	}

	try {
		while (true) {
			std::cout << "[Server] 游戏开始！" << std::endl;
			initCharacters(gameLogic);
			gameLogic.broadcastState();
			gameLoop(serverNetwork, gameLogic);

			// 询问双方是否继续
			for (std::size_t i = 0; i < 2; ++i) {
				auto& player = gameLogic.getPlayerById(i);
				std::size_t choice = player.ask(
					L"是否继续下一场对战？", { L"继续", L"退出" }, true);
				if (choice == 2) {
					std::cout << "[Server] 玩家" << i << "选择退出，游戏结束" << std::endl;
					goto gameSessionEnd;
				}
			}
		}
		gameSessionEnd:;
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}

	while (true) {
		std::this_thread::sleep_for(1s);
	}
	return 0;
}
