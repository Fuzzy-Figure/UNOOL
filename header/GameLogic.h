#pragma once
#include <SFML/Network.hpp>
#include <thread>
#include <vector>
#include "Card.h"
#include "Player.h"
#include "GameState.h"
#include "Socket.h"
#include "utils.h"

class GameLogic {
private:
	enum class Direction {
		increase, decrease
	};

#pragma region 核心数据成员
	std::unique_ptr<Pile> pile;
	std::unique_ptr<Pile> discardPile;
	std::vector<std::unique_ptr<Player>> players;
	std::size_t currentPlayerIndex = 0;
	std::size_t firstPlayerIndex = 0;
	std::vector<std::size_t> seatOrder;
	Card::Color currentColor = Card::Color::no;
	Card::Name currentName = Card::Name::no;
	Direction direction = Direction::increase;
	ServerNetwork& network;
	std::size_t matchCount = 0;
	std::optional<std::size_t> operatingPlayerId; //当前正在选牌/操作的玩家
#pragma endregion

#pragma region 私有辅助方法
	void altPlayer();
	bool currentPlayerTurn();

	struct SelectionState {
		std::vector<Character::Entry> cands[2];
		std::vector<std::size_t> bannedIdx[2];
	};

	static std::string formatCharacterLabel(const Character::Entry& entry);
	std::size_t getSeatPlayerId(std::size_t seat) const;
	std::optional<std::string> banPhase(std::size_t bannerId, std::size_t targetId, std::size_t banIndex, std::size_t banCount, SelectionState& state);
	void selectCharacter(std::size_t playerId, const SelectionState& state);
	//双将模式选将：5选1再4选1，两轮 chooseSkin 后 makeCombined，期间每选完即 markCharInfoDirty+broadcast
	//opponentNames：对方随机候选角色名集合，用于"换一批"时排除
	void selectCharacterDouble(std::size_t playerId, std::vector<Character::Entry>& cands,
							   const std::unordered_set<std::string>& opponentNames);
	//normal 模式：ban + selectCharacter
	void initPlayersNormal(std::size_t firstSeatId, std::size_t secondSeatId);
	//double 模式：无ban，按座次每家5选2，makeCombined
	void initPlayersDouble(std::size_t firstSeatId, std::size_t secondSeatId);
#pragma endregion

public:
	std::size_t getFirstPlayerId() const { return getSeatPlayerId(0); }
#pragma region 构造与析构
	GameLogic(ServerNetwork& _network);
	~GameLogic();
#pragma endregion

#pragma region 初始化
	void initPlayers();
	void initPlayers(const std::vector<std::string>& chars);
	void determineSeatOrder();
#pragma endregion

#pragma region 回合执行
	bool runTurn();
	void broadcastState();
	void flushCharInfo();
	std::size_t getMatchCount() const { return matchCount; }
	void clearMatchCount() { matchCount = 0; }
	//设置/清除当前正在操作的玩家，并广播状态
	void setOperatingPlayer(std::size_t playerId);
	void clearOperatingPlayer();
	ServerNetwork& getNetwork();
	Player& currentPlayer() const;
	GameState packStateForPlayer(std::size_t playerId) const;
	std::size_t getCurrentPlayerId() const;
#pragma endregion

#pragma region 玩家视图
	auto playersView() {
		return players | std::views::transform([](auto& p) -> Player& { return *p; });
	}
	auto playersView() const {
		return players | std::views::transform([](const auto& p) -> const Player& { return *p; });
	}
#pragma endregion

#pragma region 基于谓词的查询
	template<PlayersPredicate Cond>
	bool playersSatisfy(Cond&& cond) {
		return std::invoke(cond, players);
	}
	template<PlayerPredicate Cond>
	bool playersInclude(Cond&& cond) const {
		return std::ranges::any_of(playersView(), [&](const Player& p) {
			return std::invoke(cond, p);
		});
	}
	const std::vector<ref<Player>> getPlayers() const;
	template<PlayerPredicate Cond>
	const std::vector<ref<Player>> getPlayersIf(Cond&& cond) const {
		std::vector<ref<Player>> refs;
		//直接遍历 players：*pl 为非 const Player&（unique_ptr 解引用保留元素可变性），可构造 ref<Player>
		for (const auto& pl : players) {
			if (std::invoke(cond, *pl)) refs.emplace_back(*pl);
		}
		return refs;
	}
	const std::vector<ref<Player>> getPlayersExcludeId(const std::size_t id) const;
#pragma endregion

#pragma region 遍历迭代
	template<PlayerOperation Oper>
	void forEachPlayer(Oper&& oper) {
		for (Player& p : playersView()) std::invoke(oper, p);
	}
	template<PlayerOperation Oper>
	void forEachPlayer(Oper&& oper) const {
		for (const Player& p : playersView()) std::invoke(oper, p);
	}
	template<PlayerOperation Oper>
	void forEachOtherPlayer(const Player& self, Oper&& oper) {
		for (Player& p : playersView()) {
			if (p != self) std::invoke(oper, p);
		}
	}
	template<PlayerPredicate Cond, PlayerOperation Oper>
	void forEachPlayerIf(Cond&& cond, Oper&& oper) {
		for (Player& p : playersView()) {
			if (std::invoke(cond, p)) std::invoke(oper, p);
		}
	}
	template<PlayerPredicate Cond, PlayerOperation Oper>
	void forEachOtherPlayerIf(const Player& self, Cond&& cond, Oper&& oper) {
		for (Player& p : playersView()) {
			if (p != self && std::invoke(cond, p)) std::invoke(oper, p);
		}
	}
#pragma endregion

#pragma region 成员查询与修改
	Pile& getPile();
	Pile& getDiscardPile();
	const Pile& getDiscardPile() const;
	Player& getPlayerById(const std::size_t id);
	Card::Color getCurrentColor() const;
	void setCurrentColor(const Card::Color newColor);
	Card::Name getCurrentName() const;
	void setCurrentName(const Card::Name newName);
#pragma endregion

#pragma region 回合顺序与方向
	std::size_t nextPlayerIndex(const std::size_t curIndex) const;
	std::size_t prevPlayerIndex(const std::size_t curIndex) const;
	void reverse();
#pragma endregion

#pragma region 技能系统
	void launchPassiveSkills(const PassiveSkill::TriggerTime& triggerTime, const PassiveSkill::Trigger& trigger);
#pragma endregion

#pragma region 弃牌堆管理
	Card& putCardToDiscardPile(std::unique_ptr<Card> card, Card::DiscardReason reason, Player& player);
	std::optional<Card> lastCard() const;
#pragma endregion

#pragma region 轮次与游戏状态
	void checkRoundEnd();
	void resetGame();
	bool isGameOver() const;
	std::optional<std::size_t> getWinnerId() const;
#pragma endregion
};

// ============ Player 模板方法定义 ============
// 放在 GameLogic.h 末尾：此处 Player 与 GameLogic 均已完整，规避 Player.h↔GameLogic.h 循环依赖

template<CardPredicate Cond>
std::optional<std::size_t> Player::handleConfirm(Cond&& condition,
												 const opt_ref<TransformSkill>& activeMode) {
	if (handEmpty()) return std::nullopt;
	ServerNetwork& network = game.getNetwork();

	if (activeMode.has_value()) {
		//转换技：尝试转化选中的牌并打出（暂仅支持单牌转换）
		TransformSkill& mode = activeMode.value();
		Card& selected = (*hand)[hand->getSelectedIndex()];
		if (mode.canSelect(selected) && mode.getCardCount() == 1) {
			Card original = selected;  //备份原牌
			std::vector<ref<Card>> cards;
			cards.emplace_back(selected);
			if (mode.transform(game, *this, std::move(cards))) {
				if (canUse(selected)) {
					//转化成功打出：执行附加效果，累加使用次数
					mode.addition(game, *this);
					mode.incrementCount();
					network.clearPlayerChoice(id);
					game.clearOperatingPlayer();
					return hand->getSelectedIndex();
				}
				else {
					selected = original;  //还原
					std::println("<{}> 转化后的牌不符合出牌规则", mode.getName());
				}
			}
			else {
				//玩家在transform交互中取消
				selected = original;
				std::println("<{}> 玩家取消转化", mode.getName());
			}
		}
		else {
			std::println("<{}> 选中的牌不能转化", mode.getName());
		}
	}
	else if (std::invoke(condition, hand->getSelectedCard())) {
		network.clearPlayerChoice(id);
		game.clearOperatingPlayer();
		return hand->getSelectedIndex();
	}
	return std::nullopt;
}

template<CardPredicate Cond>
std::optional<std::size_t> Player::chooseCard(const std::string& title, Cond&& condition,
											  bool forced, ActiveSkill::TriggerTime phase) {
	ServerNetwork& network = game.getNetwork();
	game.setOperatingPlayer(id);
	network.sendPlayerChoice(id, title, {}, forced);
	opt_ref<TransformSkill> activeMode;  //当前激活的转换型主动技

	std::vector<ref<InstantSkill>>   instantRefs;
	std::vector<ref<TransformSkill>> transformRefs;
	collectAvailableSkills(phase, instantRefs, transformRefs);

	while (true) {
		network.update();
		if (!network.isClientConnected(id)) {
			// 玩家掉线，等待重连
			const auto timeoutSec = unool::getServerConfig()["network"].value("reconnectTimeoutSec", 600);
			sf::Clock clock;
			bool reconnected = false;
			std::println("[Player] 玩家{} 掉线，等待重连（最多 {} 秒）", id, timeoutSec);
			while (clock.getElapsedTime().asSeconds() < timeoutSec) {
				network.update();
				if (network.isClientLoggedIn(id)) {
					reconnected = true;
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			if (!reconnected) {
				std::println("[Player] 玩家{} 掉线超时，结束选择", id);
				return std::nullopt;
			}
			std::println("[Player] 玩家{} 重连成功，恢复选择", id);
			game.broadcastState();
			network.sendPlayerChoice(id, title, {}, forced);
			continue;
		}

		auto inputOpt = network.receiveClientInput();
		if (!inputOpt.has_value()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			continue;
		}

		ClientInput clientInput = inputOpt.value();
		if (clientInput.playerId != id) continue;

		sf::Keyboard::Scancode input = clientInput.key;
		setInput(input);
		hand->setSelectedIndex(clientInput.selectedIndex);  //统一同步选中索引

		//数字1-9：即时技发动 / 转换技切换（仅出牌阶段）
		if (phase != ActiveSkill::TriggerTime::never
			&& handleDigitKey(input, instantRefs, transformRefs, activeMode))
			continue;

		switch (input) {
			case sf::Keyboard::Scancode::Space:
				sortHand();
				game.broadcastState();
				break;
			case sf::Keyboard::Scancode::Up:
			case sf::Keyboard::Scancode::W:
				if (auto result = handleConfirm(condition, activeMode); result.has_value()) {
					network.clearPlayerChoice(id);
					return result.value();
				}
				break;
			case sf::Keyboard::Scancode::Down:
			case sf::Keyboard::Scancode::S:
				if (!forced) {
					network.clearPlayerChoice(id);
					return std::nullopt;
				}
				break;
			default:
				break;
		}
	}
}

template<CardPredicate Cond>
std::vector<ref<Card>> Player::chooseCardsToDiscardPile(const std::string& title,
														std::size_t num, const bool forced,
														Cond&& cond,
														Card::DiscardReason reason) {
	std::vector<ref<Card>> discardedCards;
	if (const std::size_t _handCount = handCount(); num > _handCount)
		num = _handCount;

	ServerNetwork& network = game.getNetwork();
	std::println("玩家{}请选择{}{}张牌", id, reason, num);

	std::size_t discardedCount = 0;
	while (discardedCount < num) {
		std::string fullTitle = std::format(
			"{}（{}/{}）\n{}",
			title, discardedCount + 1, num,
			forced ? "（↑确认，不可取消）" : "（↑确认，↓取消）"
		);
		auto index = chooseCard(fullTitle, cond, forced);
		if (!index.has_value()) {
			std::println("玩家{}取消了{}", id, reason);
			return discardedCards;
		}
		discardedCards.push_back(hand->getCardByIndex(index.value()));
		putCardToDiscardPileByIndex(index.value(), reason);
		discardedCount++;
		std::println("玩家{}{}了一张牌（{}/{}）", id, reason, discardedCount, num);
		game.broadcastState();
	}
	return discardedCards;
}

template<CardPredicate Cond>
std::vector<ref<Card>> Player::chooseToDiscard(const std::string& title,
											   std::size_t num, const bool forced,
											   Cond&& cond) {
	return chooseCardsToDiscardPile(title, num, forced, std::forward<Cond>(cond), Card::DiscardReason::discard);
}

template<CardPredicate Cond>
Player::RecastResult Player::chooseToRecast(const std::string& title,
											const std::size_t num, const bool forced,
											Cond&& cond) {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::recast_begin, trigger);
	}
	std::vector discarded = chooseCardsToDiscardPile(title, num, forced, std::forward<Cond>(cond), Card::DiscardReason::recast);
	std::vector drawn = draw(discarded.size());
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::recast_end, trigger);
	}
	return RecastResult{ std::move(discarded), std::move(drawn) };
}

template<CardPredicate Cond>
void Player::decree(const std::string& title,
					const std::size_t num, const bool forced,
					Cond&& cond) {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::decree_begin, trigger);
	}
	draw(num);
	chooseCardsToDiscardPile(title, num, forced, std::forward<Cond>(cond), Card::DiscardReason::decree);
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::decree_end, trigger);
	}
}

template<CardPredicate Cond, CardOperation Oper>
opt_ref<Card> Player::chooseToOperate(const std::string& title, bool forced,
									  Cond&& cond, Oper&& operation) {
	std::string fullTitle = std::format("{}\n{}", title, forced ? "（↑确认，不可取消）" : "（↑确认，↓取消）");
	std::optional<std::size_t> index = chooseCard(fullTitle, cond, forced);
	if (!index.has_value()) return std::nullopt;
	ref<Card> cardRef = getHand().getCardByIndex(index.value());
	std::invoke(std::forward<Oper>(operation), getHand().getCardByIndex(index.value()));
	return cardRef;
}

template<CardPredicate Cond>
opt_ref<Card> Player::chooseToGive(const std::string& title, Player& target,
								   bool forced, Cond&& cond) {
	if (handEmpty()) return std::nullopt;

	auto index = chooseCard(title, cond, forced);

	if (!index.has_value()) {
		std::println("玩家{}取消了给{}牌", id, target.characterName());
		return std::nullopt;
	}

	ref<Card> card = hand->getCardByIndex(index.value());

	give(target, takeCardByIndex(index.value()));
	std::println("{}给了{}一张{}", characterName(), target.characterName(), card.get());
	game.broadcastState();

	return card;
}

template<CardPredicate Cond>
opt_ref<Card> Player::chooseToShow(const std::string& title, bool forced, Cond&& cond) {
	return chooseToOperate(title, forced, std::forward<Cond>(cond), [this](Card& c) {
		showCard(c);
	});
}

template<PlayerPredicate Cond>
opt_ref<Player> Player::choosePlayer(const std::string& title, bool forced, Cond&& cond) {
	//选角色
	const auto& candidates = game.getPlayersIf(cond);

	if (candidates.empty()) {
		game.broadcastState();
		return std::nullopt;
	}

	std::vector<std::string> options;
	for (auto& p : candidates) {
		options.push_back(p.get().characterName());
	}
	std::size_t choice = ask(title, options, forced);

	if (choice == 0) return std::nullopt;
	return candidates[choice - 1];
}

template<PlayerPredicate Cond>
opt_ref<Player> Player::chooseOtherPlayer(const std::string& title, bool forced, Cond&& cond) {
	return choosePlayer(title, forced, [this, &cond](const Player& p) {
		return p != *this && std::invoke(cond, p);
	});
}
