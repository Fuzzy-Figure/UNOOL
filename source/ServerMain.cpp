#include "GameLogic.h"
#include "Socket.h"
#include "utils.h"
#include "application/Services.h"
#include "application/AccountService.h"
#include "application/ShopService.h"
#include "adapters/LegacyBlockingDecisionAdapter.h"
#include "adapters/LegacyServicesAdapter.h"
#include "adapters/NetworkServerView.h"
#include <Windows.h>
#include <chrono>
#include <thread>
#include <vector>

// 根据 server_config.json 初始化角色（指定或随机）
static void initCharacters(GameLogic& gameLogic,
						   unool::application::IConfigProvider& config) {
	if (auto chars = config.assignedCharacters(); chars.has_value()) {
		//角色数量由 GameLogic::initPlayers(vector) 按 mode 校验
		gameLogic.initPlayers(*chars);
	}
	else {
		gameLogic.initPlayers();
	}
}

// 端口只用标准库类型：角色档位以底层整数透传，由适配器还原为 Character::Level
static std::vector<int> toLevelInts(const std::vector<Character::Level>& levels) {
	std::vector<int> result;
	result.reserve(levels.size());
	for (const auto lv : levels) result.push_back(static_cast<int>(lv));
	return result;
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
static void handleGameOver(ServerNetwork& serverNetwork, GameLogic& gameLogic,
						   unool::application::IUserRepository& users) {
	std::optional<std::size_t> winnerId = gameLogic.getWinnerId();
	serverNetwork.sendGameEnd(winnerId);
	gameLogic.clearGameCount();

	if (winnerId.has_value()) {
		std::size_t wId = winnerId.value();
		std::size_t lId = 1 - wId;

		auto& players = gameLogic.getPlayers();
		const auto& slots = serverNetwork.getClientSlots();
		//查询双方皮肤品质，供结算加成使用
		const Character::SkinQuality wQuality = bestSkinQuality(players[wId].get());
		const Character::SkinQuality lQuality = bestSkinQuality(players[lId].get());
		const bool wBonusCard = gameLogic.hasUsedBonusCard(wId);
		users.addMatchResult(
			slots[wId].username, slots[lId].username,
			toLevelInts(players[wId].get().getLevels()),
			toLevelInts(players[lId].get().getLevels()),
			players[wId].get().getHp() == players[wId].get().getMaxHp(),
			static_cast<int>(wQuality), static_cast<int>(lQuality), wBonusCard);

		std::println("[Server] 游戏结束，{}获胜！", players[wId].get().displayName());
	}
	else {
		std::println("[Server] 游戏结束，无人获胜！");
	}
}

// 游戏主循环
static void gameLoop(ServerNetwork& serverNetwork, GameLogic& gameLogic,
					 unool::application::IUserRepository& users) {
	while (!gameLogic.isGameOver()) {
		bool roundEnded = gameLogic.runTurn();

		// 回合结束后立即检查游戏是否结束（技能杀人等情况）
		if (gameLogic.isGameOver()) {
			handleGameOver(serverNetwork, gameLogic, users);
			break;
		}

		if (!roundEnded) continue;

		// 一局结束，处理体力扣除
		gameLogic.launchPassiveSkills(PassiveSkill::TriggerTime::game_end, PassiveSkill::Trigger{});
		gameLogic.checkRoundEnd();
		gameLogic.broadcastState();

		if (gameLogic.isGameOver()) {
			handleGameOver(serverNetwork, gameLogic, users);
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

	// Composition Root：唯一出现「谁实现谁」的地方。
	// 五个出站服务端口各由独立适配器实现（由 LegacyServicesAdapter.h 统一引入）；
	// IServerView 由 NetworkServerView 实现、IDecisionPort 由
	// LegacyBlockingDecisionAdapter 实现，两者分别注入棋局。
	// installServices 让无法构造注入的静态工具（Character）
	// 与全局随机门面也指向同一组实例；客户端不装配时自动退回逐字等价的默认组合。
	// 随机引擎硬约束：Mt19937Random 在此处只构造一份，DomainServices::random 与
	// RandomEngine 门面都经 services() 指向它，随机序列与重构前逐字等价。
	unool::adapters::JsonConfigProvider configService;
	unool::adapters::JsonUserRepository userRepository;
	// W5 第一段：账号用例服务。ServerNetwork 实现 IAccountSession 端口，
	// 注册 / 登录 / 查重 / 准备 / 积分消费经 AccountService 与 IUserRepository 完成。
	// serverNetwork 在装配前只监听、未调用 update()，不存在「未装配」窗口。
	unool::application::AccountService accountService(serverNetwork, userRepository);
	serverNetwork.installAccountService(accountService);
	// W5 第二段（分段第一段）：商城展示 / 技能描述 / 签到用例服务，
	// 与账号服务共用 IAccountSession（由 ServerNetwork 实现）与 IUserRepository。
	// 同样在首次 update() 之前装配，不存在「未装配」窗口。
	unool::application::ShopService shopService(serverNetwork, userRepository);
	serverNetwork.installShopService(shopService);
	unool::adapters::Mt19937Random randomService;
	unool::adapters::ChronoClock clockService;
	unool::adapters::FilesystemAssetCatalog assetCatalog;
	unool::application::DomainServices domainServices{
		configService, userRepository, randomService, clockService, assetCatalog };
	unool::application::installServices(domainServices);

	unool::adapters::NetworkServerView serverView(serverNetwork);
	unool::adapters::LegacyBlockingDecisionAdapter decisionAdapter(serverNetwork, configService);
	GameLogic gameLogic(serverView, decisionAdapter, domainServices);
	serverView.attach(gameLogic);

	std::println("[Server] 等待客户端连接...");
	while (!serverNetwork.isReady()) {
		serverNetwork.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	try {
		while (true) {
			std::println("[Server] 游戏开始！");
			domainServices.config.reload();
			initCharacters(gameLogic, domainServices.config);
			gameLogic.broadcastState();
			gameLoop(serverNetwork, gameLogic, domainServices.users);

			// 询问双方是否继续
			for (std::size_t i = 0; i < 2; ++i) {
				auto& player = gameLogic.getPlayerById(i);
				std::size_t choice = player.ask(
					"是否继续下一场对战？", { "继续", "退出" }, true);
				if (choice == 2) {
					std::println("[Server] {}选择退出，游戏结束", player.displayName());
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
