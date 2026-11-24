#include "RoomGame.h"

#include "RoomManager.h"
#include "ServerNetwork.h"
#include "GameLogic.h"
#include "Player.h"
#include "GameState.h"
#include "MessageCodec.h"

#include <SFML/Network.hpp>

#include <chrono>
#include <format>
#include <print>
#include <thread>
#include <utility>
#include <vector>

// 房间对局：一房一线程、一份 GameLogic。
// 与原 ServerMain 的单桌实现相比只有三处差别：
//   ① 出站目标是「座位 → 房间成员槽位」，不再广播给全部连接；
//   ② 输入来自房间输入队列（IO 线程投递），不再自己驱动网络；
//   ③ 对局结束后回调 RoomManager 把房间交回等待状态，而不是退出进程。

namespace unool::shell {

	namespace {

		constexpr std::size_t kSeats = 2;

		// 端口只用标准库类型：角色档位以底层整数透传，由适配器还原为 Character::Level
		std::vector<int> toLevelInts(const std::vector<Character::Level>& levels) {
			std::vector<int> result;
			result.reserve(levels.size());
			for (const auto lv : levels) result.push_back(static_cast<int>(lv));
			return result;
		}

		// 获取玩家穿戴皮肤中最高品质（饮料限定视为普通，因为其效果在万能牌时实时结算）
		Character::SkinQuality bestSkinQuality(const Player& p) {
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

	}

	// 扫描码 -> 领域键码的翻译实现在 UNOOL.Adapters（source/shell/RoomKeyMapping.cpp），
	// 这里只使用声明：RoomManager 与 ServerNetwork 都经 toDomainKey 统一入口转换。

	// === RoomServerView ===

	void RoomServerView::onState() {
		if (!logic_) return;
		for (std::size_t seat = 0; seat < kSeats; ++seat) {
			if (!game_.isSeatOnline(seat)) continue;
			GameState state = logic_->get().packStateForPlayer(seat);
			sf::Packet packet = unool::protocol::makeGameStateMessage(state);
			game_.sendToSeat(seat, packet);
		}
		onCharInfo();
	}

	void RoomServerView::onCharInfo() {
		if (!logic_) return;
		const auto players = logic_->get().getPlayers();
		for (std::size_t i = 0; i < players.size() && i < kSeats; ++i) {
			Player& player = players[i].get();
			if (!player.isCharInfoDirty()) continue;

			CharInfo info;
			info.playerIndex = i;

			//levelPart
			std::string levelPart;
			if (player.isCombined()) {
				auto levels = player.getLevels();
				levelPart = std::format("{}+{}", levels[0], levels[1]);
			}
			else {
				levelPart = std::format("{}", player.characterLevel());
			}

			//marksPart
			std::string marksPart = "标记：";
			for (const auto& [name, count] : player.getMarks()) {
				marksPart += std::format("{}*{}，", name, count);
			}

			//skillsPart
			std::string skillsPart;
			if (!player.isSealed()) skillsPart = "技能：\n";
			else skillsPart = std::format("技能（已被封印，{}回合后解除）：\n", player.getSealed());
			skillsPart += player.getSkillsText();

			//合并
			info.fullText = std::format(
				"{}（{}）\n{}\n{}",
				player.characterName(), levelPart, marksPart, skillsPart
			);

			sf::Packet packet = unool::protocol::makeCharInfoMessage(info);
			game_.sendToSeat(i, packet);
			player.clearCharInfoDirty();
		}
	}

	std::string RoomServerView::usernameOf(const unool::core::player_id id) const {
		return game_.nameOfSeat(id);
	}

	bool RoomServerView::spendPoints(const unool::core::player_id id, const int amount) {
		return game_.spendPointsOfSeat(id, amount);
	}

	// === RoomDecisionAdapter ===

	void RoomDecisionAdapter::poll() {
		// 只等本房间的输入队列（5ms 上限），网络 IO 由 ServerNetwork 的 IO 线程负责。
		game_.waitInput(std::chrono::milliseconds(5));
	}

	std::optional<unool::core::DecisionInput> RoomDecisionAdapter::takeInput() {
		return game_.takeInput();
	}

	void RoomDecisionAdapter::showChoice(const unool::core::player_id id,
										 const unool::core::ChoicePrompt& prompt) {
		if (id >= kSeats) return;
		sf::Packet packet = unool::protocol::makeChoice(prompt);
		game_.sendToSeat(id, packet);
	}

	void RoomDecisionAdapter::clearChoice(const unool::core::player_id id) {
		// 与原 clearPlayerChoice 等价：发一个 title / options 均为空的 Choice 包
		unool::core::ChoicePrompt empty{};
		showChoice(id, empty);
	}

	bool RoomDecisionAdapter::isConnected(const unool::core::player_id id) const {
		return id < kSeats && game_.isSeatOnline(id);
	}

	bool RoomDecisionAdapter::isLoggedIn(const unool::core::player_id id) const {
		// 对局中的座位一定属于本房间成员：能发包即视为已登录，重连顶替会恢复同一槽位
		return isConnected(id);
	}

	int RoomDecisionAdapter::reconnectTimeoutSec() const {
		return game_.reconnectTimeoutSec();
	}

	// === RoomGame ===

	RoomGame::RoomGame(const std::uint64_t roomId, std::string roomName,
					   std::array<std::size_t, 2> slots, std::array<std::string, 2> names,
					   ServerNetwork& net, RoomManager& manager,
					   unool::core::DomainServices& services)
		: roomId_(roomId), roomName_(std::move(roomName)),
		  slots_(slots), names_(std::move(names)),
		  net_(net), manager_(manager), services_(services),
		  view_(*this), decision_(*this) {}

	// 见头文件说明：unique_ptr<GameLogic> 的删除要求此处能看到完整类型
	RoomGame::~RoomGame() = default;

	void RoomGame::start() {
		auto self = shared_from_this();
		std::thread([self] { self->run(); }).detach();
	}

	void RoomGame::pushInput(const std::size_t seat, unool::core::DecisionInput input) {
		// 座位号以服务端判定为准：客户端上传的 playerId 不参与对局（只用于它本地的选牌同步）
		input.player = seat;
		{
			std::lock_guard lock(inputMutex_);
			inputs_.push_back(input);
		}
		inputCv_.notify_one();
	}

	void RoomGame::waitInput(const std::chrono::milliseconds timeout) {
		std::unique_lock lock(inputMutex_);
		inputCv_.wait_for(lock, timeout, [this] { return !inputs_.empty(); });
	}

	std::optional<unool::core::DecisionInput> RoomGame::takeInput() {
		std::lock_guard lock(inputMutex_);
		if (inputs_.empty()) return std::nullopt;
		auto input = inputs_.front();
		inputs_.pop_front();
		return input;
	}

	bool RoomGame::isSeatOnline(const std::size_t seat) const {
		if (seat >= kSeats) return false;
		return net_.isSlotOnline(slots_[seat]);
	}

	std::size_t RoomGame::slotOfSeat(const std::size_t seat) const {
		return seat < kSeats ? slots_[seat] : 0;
	}

	const std::string& RoomGame::nameOfSeat(const std::size_t seat) const {
		return seat < kSeats ? names_[seat] : names_[0];
	}

	bool RoomGame::sendToSeat(const std::size_t seat, const sf::Packet& packet) {
		if (seat >= kSeats) return false;
		return net_.sendToSlot(slots_[seat], packet);
	}

	bool RoomGame::spendPointsOfSeat(const std::size_t seat, const int amount) {
		if (seat >= kSeats) return false;
		return net_.trySpendPoints(slots_[seat], amount);
	}

	int RoomGame::reconnectTimeoutSec() const {
		return services_.config.reconnectTimeoutSec();
	}

	void RoomGame::run() {
		try {
			sessionLoop();
		}
		catch (const std::exception& e) {
			std::println(stderr, "[Room {}] 对局异常结束：{}", roomId_, e.what());
		}
		// 交回房间状态：RoomManager 会清掉 game 指针并广播「回到等待」，随后本对象由线程自行释放
		manager_.onGameFinished(roomId_);
	}

	void RoomGame::sessionLoop() {
		// 每个房间独占一份 GameLogic（牌堆、玩家、技能状态都在这份实例里）
		logic_ = std::make_unique<GameLogic>(view_, decision_, services_);
		view_.attach(*logic_);

		while (true) {
			// 新一局开始前清掉上一局残留的按键：否则「是否继续下一场」之后多按的那一下
			// 会被本局第一个提示（例如「你是X号位」）直接吃掉，玩家再按就落到下一个提示上。
			{
				std::lock_guard lock(inputMutex_);
				inputs_.clear();
			}

			std::println("[Room {}] 对局开始：{} vs {}", roomId_, names_[0], names_[1]);
			services_.config.reload();
			initCharacters();
			logic_->broadcastState();
			gameLoop();

			if (!continueRequested()) {
				std::println("[Room {}] 对局结束，房间回到等待状态", roomId_);
				break;
			}
			std::println("[Room {}] 双方选择继续，开始下一场", roomId_);
		}
		logic_.reset();
	}

	void RoomGame::initCharacters() {
		if (auto chars = services_.config.assignedCharacters(); chars.has_value()) {
			//角色数量由 GameLogic::initPlayers(vector) 按 mode 校验
			logic_->initPlayers(*chars);
		}
		else {
			logic_->initPlayers();
		}
	}

	void RoomGame::gameLoop() {
		while (!logic_->isGameOver()) {
			const bool roundEnded = logic_->runTurn();

			// 回合结束后立即检查游戏是否结束（技能杀人等情况）
			if (logic_->isGameOver()) {
				handleGameOver();
				break;
			}
			if (!roundEnded) continue;

			// 一局结束，处理体力扣除
			logic_->launchPassiveSkills(PassiveSkill::TriggerTime::game_end, PassiveSkill::Trigger{});
			logic_->checkRoundEnd();
			logic_->broadcastState();

			if (logic_->isGameOver()) {
				handleGameOver();
				break;
			}
			// 开始新一局
			logic_->resetGame();
		}
	}

	void RoomGame::handleGameOver() {
		const std::optional<std::size_t> winnerId = logic_->getWinnerId();
		const std::string winnerName = winnerId ? names_[*winnerId] : std::string{};

		sf::Packet end = unool::protocol::makeGameEnd(winnerId, winnerName);
		for (std::size_t seat = 0; seat < kSeats; ++seat) sendToSeat(seat, end);

		logic_->clearGameCount();

		if (!winnerId) {
			std::println("[Room {}] 对局结束，无人获胜", roomId_);
			return;
		}

		const std::size_t wId = *winnerId;
		const std::size_t lId = 1 - wId;
		auto& players = logic_->getPlayers();
		const auto wQuality = bestSkinQuality(players[wId].get());
		const auto lQuality = bestSkinQuality(players[lId].get());
		const bool wBonusCard = logic_->hasUsedBonusCard(wId);
		services_.users.addMatchResult(
			names_[wId], names_[lId],
			toLevelInts(players[wId].get().getLevels()),
			toLevelInts(players[lId].get().getLevels()),
			players[wId].get().getHp() == players[wId].get().getMaxHp(),
			static_cast<int>(wQuality), static_cast<int>(lQuality), wBonusCard);
		std::println("[Room {}] 对局结束，{}获胜", roomId_, names_[wId]);
	}

	bool RoomGame::continueRequested() {
		// 任一方已掉线就不再询问：直接交回房间，避免为掉线方空等 600 秒重连
		for (std::size_t seat = 0; seat < kSeats; ++seat) {
			if (!isSeatOnline(seat)) return false;
		}
		for (std::size_t seat = 0; seat < kSeats; ++seat) {
			Player& player = logic_->getPlayerById(seat);
			const std::size_t choice = player.ask("是否继续下一场对战？", { "继续", "退出" }, true);
			// 0 = 超时 / 掉线超时，2 = 退出；两者都不再开下一场
			if (choice == 0 || choice == 2 || !isSeatOnline(seat)) return false;
		}
		return true;
	}

}
