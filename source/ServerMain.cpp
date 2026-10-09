#include "GameLogic.h"
#include "Socket.h"
#include "utils.h"
#include "UserDB.h"
#include <Windows.h>
#include <chrono>
#include <thread>

// 根据 server_config.json 初始化角色（指定或随机）
static void initCharacters(GameLogic& gameLogic) {
	if (unool::getServerConfig().contains("characters") && unool::getServerConfig()["characters"].contains("assign")) {
		auto chars = unool::getServerConfig()["characters"]["assign"].get<std::vector<std::string>>();
		//角色数量由 GameLogic::initPlayers(vector) 按 mode 校验
		gameLogic.initPlayers(chars);
	}
	else {
		gameLogic.initPlayers();
	}
}

// 获取玩家穿戴皮肤中最高品质（饮料限定视为普通，因为其效果在万能牌时实时结算）
static Character::SkinQuality bestSkinQuality(const Player& p) {
	const auto& names = p.getNames();
	const auto& skins = p.getSkins();
	Character::SkinQuality best = Character::SkinQuality::normal;
	for (std::size_t i = 0; i < names.size() && i < skins.size(); ++i) {
		auto q = Character::getSkinQuality(names[i], skins[i]);
		if (q == Character::SkinQuality::drink) continue; //饮料限定不参与品质加成
		if (q > best) best = q;
	}
	return best;
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
		const auto& slots = serverNetwork.getClientSlots();
		//查询双方皮肤品质，供结算加成使用
		const Character::SkinQuality wQuality = bestSkinQuality(players[wId].get());
		const Character::SkinQuality lQuality = bestSkinQuality(players[lId].get());
		const bool wBonusCard = gameLogic.hasUsedBonusCard(wId);
		UserDB::instance().addMatchResult(
			slots[wId].username, slots[lId].username,
			players[wId].get().getLevels(),
			players[lId].get().getLevels(),
			players[wId].get().getHp() == players[wId].get().getMaxHp(),
			wQuality, lQuality, wBonusCard);

		std::println("[Server] 游戏结束，玩家{}获胜！", wId);
	}
	else {
		std::println("[Server] 游戏结束，无人获胜！");
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
		gameLogic.launchPassiveSkills(PassiveSkill::TriggerTime::game_end, PassiveSkill::Trigger{});
		gameLogic.checkRoundEnd();
		gameLogic.broadcastState();

		if (gameLogic.isGameOver()) {
			handleGameOver(serverNetwork, gameLogic);
			break;
		}
		// 开始新一局
		gameLogic.resetGame();
	}
}

int main() {
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	std::println("[Server] 启动服务器...");

	ServerNetwork serverNetwork;
	unsigned short port = 8888;
	if (!serverNetwork.start(port)) {
		std::println(stderr, "[Server] 启动失败");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return 1;
	}

	GameLogic gameLogic(serverNetwork);

	std::println("[Server] 等待客户端连接...");
	while (!serverNetwork.isReady()) {
		serverNetwork.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	try {
		while (true) {
			std::println("[Server] 游戏开始！");
			unool::reloadServerConfig();
			initCharacters(gameLogic);
			gameLogic.broadcastState();
			gameLoop(serverNetwork, gameLogic);

			// 询问双方是否继续
			for (std::size_t i = 0; i < 2; ++i) {
				auto& player = gameLogic.getPlayerById(i);
				std::size_t choice = player.ask(
					"是否继续下一场对战？", { "继续", "退出" }, true);
				if (choice == 2) {
					std::println("[Server] 玩家{}选择退出，游戏结束", i);
					goto gameSessionEnd;
				}
			}
		}
	gameSessionEnd:
	} catch (std::exception& e) {
		std::println(stderr, "{}", e.what());
	}

	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
