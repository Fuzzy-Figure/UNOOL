#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
#include <numeric>
#include <optional>

// ==================== 技能：炼兵 ====================
std::map<Card::Name, std::size_t> 炼兵::buildPairs(const Player& carrier) const {
	std::map<Card::Name, std::size_t> cnt;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		const Card& c = carrier.getHand().getCardByIndex(i);
		if (c.is(Card::Color::black)) continue;
		if (c.isWild()) continue;
		cnt[c.getName()]++;
	}
	return cnt;
}
bool 炼兵::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (carrier.handCount() < 2) return false;
	//手中【+2】<=四张
	std::size_t draw2Count = 0;
	for (const auto& c : carrier.getHand()) {
		if (c->is(Card::Name::action_draw2)) ++draw2Count;
	}
	if (draw2Count > 4) return false;

	auto cnt = buildPairs(carrier);
	for (const auto& kv : cnt) {
		if (kv.second >= 2 && !usedNames.contains(kv.first)) {
			return true;
		}
	}
	return false;
}
bool 炼兵::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto cnt = buildPairs(carrier);
	std::vector<Card::Name> validPairs;
	std::vector<std::string> opts;
	for (const auto& kv : cnt) {
		if (kv.second >= 2 && !usedNames.contains(kv.first)) {
			validPairs.push_back(kv.first);
			opts.push_back(std::format("{}({}张)", kv.first, kv.second));
		}
	}
	if (validPairs.empty()) return false;

	std::size_t idx = carrier.ask("【炼兵】选择要弃的同名牌：", opts, false);
	if (idx == 0) return false;
	if (idx - 1 >= validPairs.size()) return false;

	Card::Name target = validPairs[idx - 1];
	std::size_t i1 = carrier.handCount(), i2 = carrier.handCount();
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		const Card& c = carrier.getHand().getCardByIndex(i);
		if (c.getName() == target) {
			if (i1 >= carrier.handCount()) { i1 = i; }
			else { i2 = i; break; }
		}
	}
	if (i1 >= carrier.handCount() || i2 >= carrier.handCount()) return false;
	if (i1 > i2) std::swap(i1, i2);
	carrier.discardByIndex(i2);
	carrier.discardByIndex(i1);
	Card::Color color = unool::random::randomGet(Card::fourColors);
	carrier.gainCard(Card::make(color, Card::Name::action_draw2));
	usedNames.insert(target);
	std::println("<技能> {}发动炼兵，弃两张{}，获得一张{}+2",
				 carrier.displayName(), target, color);
	game.broadcastState();
	return true;
}


// ==================== 技能：好火 ====================
bool 好火::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!c.is(Card::Color::red)) return false;
	Player& player = trigger.getPlayer();
	if (player == carrier) return false;
	if (player.getHp() <= carrier.getHp()) return false;
	if (usedPlayerIds.contains(player.getId())) return false;
	if (carrier.handCount() == 0) return false;
	return true;
}
bool 好火::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();

	auto result = carrier.chooseToGive(std::format("选择一张手牌交给{}", target.characterName()),
									   target, false, unool::alwaysTrue);
	if (!result.has_value()) return false;

	usedPlayerIds.insert(target.getId());
	std::println("<技能> {}发动好火，交给{}一张{}", carrier.displayName(), target.displayName(), result.value().get());
	return true;
}


//================过江==============
bool 过江::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2);
}
bool 过江::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getSource().draw(2, DrawReason::skill);
	return true;
}


//================大盏==============
void 大盏::randomEnlarge(Card& c) {
	//非数字牌，不能变大
	if (c.isNotNumber()) return;
	//9不能变大
	if (c.is(Card::Name::number_9)) return;
	//变大
	std::size_t currentNumber = c.value();
	c.setName(Card::numberCardsFrom0[unool::random::randomSize_t(currentNumber + 1, 9)]);
}
bool 大盏::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude([](const Card& c) {
		return c.isNumber();
	});
}
bool 大盏::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	// 找到最小数字
	std::size_t min = 100;
	carrier.getHand().forEach([&min](const Card& c) {
		if (c.isNotNumber()) return;
		if (c.value() < min) {
			min = c.value();
		}
	});


	if (min != 9) { //min不是9，存在数字牌点数不是9
		// 让点数最小的数字牌随机变大
		carrier.getHand().forEach([&min](Card& c) {
			if (c.isNumber() && c.value() == min) {
				randomEnlarge(c);
			}
		});
	}
	else { //min == 9，说明数字牌全是9，将一张9变为红色
		carrier.chooseToOperate(
			"选择一张[9]变成红色", false,
			[](const Card& c) {
			return c.value() == 9;
		}, [](Card& c) {
			c.setColor(Card::Color::red);
		});
		carrier.recover(1);
	}
	game.broadcastState();
	return true;
}


bool 连营::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (!trigger.hasPlayer() || !trigger.hasCards()) return false;
	if (carrier.getId() != trigger.getPlayer().getId()) return false;
	const Card& c = trigger.getCard();
	Card::Type type = c.getType();
	if (triggered.count(type)) return false;  //该类别已触发过
	//失去后手牌中该类别牌数为0（最后一张）
	return !carrier.handInclude(
		[type](const Card& hc) { return hc.is(type); });
}

bool 连营::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const Card& lost = trigger.getCard();
	Card::Type lostType = lost.getType();

	//弃置另一类别的一张牌
	auto discarded = carrier.chooseToDiscard(
		"【连营】弃置另一类别的一张牌", 1, false,
		[lostType](const Card& c) { return c.getType() != lostType; });
	if (discarded.empty()) return false;  //玩家取消

	//从游戏外获得该类别一张牌
	Card::ColorName cn = Card::randomCard([lostType](const Card& c) {
		return c.is(lostType);
	});
	carrier.gainCard(Card::make(cn));

	triggered.insert(lostType);
	std::println("<技能> {}发动连营，弃置一张牌并获得一张{}牌",
				 carrier.displayName(), lostType);
	game.broadcastState();
	return true;
}

void 连营::reset() {
	PassiveSkill::reset();
	triggered.clear();
}

std::set<Card::Type>& 困界::getTriggered(const Player& carrier) const {
	if (!triggeredCache.has_value()) {
		auto skillOpt = carrier.findSkill<连营>();
		if (!skillOpt.has_value()) throw std::runtime_error("没有找到\"连营\"技能");
		triggeredCache = skillOpt.value().get().triggered;
	}
	return triggeredCache.value().get();
}
bool 困界::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return getTriggered(carrier).size() >= 3;
}

bool 困界::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//1. 选目标角色
	auto targetOpt = carrier.choosePlayer("【困界】选择一名角色重铸其手中一种类别的所有牌", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	//重铸某类别所有牌的局部函数（每张单独走 recastByIndex，保证 reason 与触发时机正确）
	auto recastAll = [](Player& p, Card::Type type_) {
		for (std::size_t i = p.handCount(); i-- > 0; ) {
			if (p.getCardByIndex(i).is(type_)) p.recastByIndex(i);
		}
	};

	//2. 选目标重铸的类别
	std::size_t typeAIdx = carrier.ask(
		std::format("【困界】选择{}重铸的类别", target.characterName()),
		{ "数字牌", "功能牌", "万能牌" }, true);
	Card::Type typeA = static_cast<Card::Type>(typeAIdx);
	recastAll(target, typeA);

	//3. 选自己重铸的类别（必须不同于 typeA）
	std::vector<std::string> otherNames;
	std::vector<Card::Type> otherTypes;
	for (int i = 1; i <= 3; ++i) {
		Card::Type c = static_cast<Card::Type>(i);
		if (c != typeA) {
			otherNames.push_back(std::format("{}", c));
			otherTypes.push_back(c);
		}
	}
	std::size_t typeBIdx = carrier.ask("【困界】选择自己重铸的类别", otherNames, true);
	Card::Type typeB = otherTypes[typeBIdx - 1];
	recastAll(carrier, typeB);

	std::println("<技能> {}发动困界", carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：易主 ====================
bool 易主::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (phaseCount >= 1) return false;
	if (!trigger.hasNumber()) return false;
	return trigger.getNumber() == 2;
}

bool 易主::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//弃置刚获得的两张牌
	std::vector<ref<Card>> gainedCards = trigger.getCards();
	for (auto& cardRef : gainedCards) {
		//找到手中对应的牌并弃置
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			if (&carrier.getCardByIndex(i) == &cardRef.get()) {
				carrier.discardByIndex(i);
				break;
			}
		}
	}
	std::println("<技能> {}发动易主，弃置获得的两张牌", carrier.displayName());

	//渡荆可发动次数+1
	if (auto sp = carrier.findSkill<渡荆>(); sp.has_value()) {
		auto& dj = sp.value().get();
		dj.increaseLimit(1);
		carrier.markCharInfoDirty();
		std::println("<技能> {}的【渡荆】可发动次数+1，当前={}", carrier.displayName(), dj.getLimit().value());
	}

	++phaseCount;
	game.broadcastState();
	return true;
}

void 易主::reset() {
	phaseCount = 0;
	PassiveSkill::reset();
}


// ==================== 技能：渡荆 ====================
bool 渡荆::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//检查手牌数全场最多
	std::size_t myCount = carrier.handCount();
	for (const auto& p : game.getPlayers()) {
		if (&p.get() == &carrier) continue;
		if (p.get().handCount() > myCount) return false;
	}
	return true;
}

bool 渡荆::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//选一名其他角色拼点
	auto targetOpt = carrier.chooseOtherPlayer("【渡荆】选择一名其他角色拼点", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//拼点（carrier是发起者）
	auto result = carrier.comparePoint(target, true);
	if (!result) return false;

	std::println("<技能> {}与{}拼点", carrier.displayName(), target.displayName());

	//没赢的（lose或draw）获得随机+2
	auto giveRandomDraw2 = [&game](Player& p) {
		Card::Color color = unool::random::randomGet(Card::fourColors);
		auto card = Card::make(color, Card::Name::action_draw2);
		std::println("<技能> {}拼点未赢，从游戏外获得{}", p.displayName(), *card);
		p.gainCard(std::move(card));
	};

	if (*result == Player::CompareResult::lose) {
		//carrier没赢，target赢
		target.recover(5);
		giveRandomDraw2(carrier);
	}
	else if (*result == Player::CompareResult::draw) {
		//平局，双方都没赢
		giveRandomDraw2(carrier);
		giveRandomDraw2(target);
	}
	else {
		//carrier赢，target没赢
		carrier.recover(5);
		giveRandomDraw2(target);
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：侵蚀 ====================
bool 侵蚀::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Player& player = trigger.getPlayer();
	return player != carrier && player.handCount() <= 3;
}
bool 侵蚀::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	std::size_t dmg = player.handCount();
	player.damage(dmg, std::nullopt);
	std::println("<技能> {}侵蚀令{}失去{}点体力", carrier.displayName(), player.displayName(), dmg);
	return true;
}

// ==================== 技能：剧变 ====================
bool 剧变::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//跳过摸牌
	trigger.getNumber() = 0;

	//记录每个玩家原有手牌数
	std::unordered_map<std::size_t, std::size_t> oldCounts;
	game.forEachPlayer([&](Player& p) {
		oldCounts[p.getId()] = p.handCount();
	});

	//阶段1：所有玩家把手牌全部取出
	std::unordered_map<std::size_t, std::vector<std::unique_ptr<Card>>> oldHands;
	game.forEachPlayer([&](Player& p) {
		std::vector<std::unique_ptr<Card>> hand;
		while (!p.handEmpty()) {
			hand.push_back(p.takeCardByIndex(0));
		}
		oldHands[p.getId()] = std::move(hand);
	});

	//阶段2：每个玩家的原手牌交给下家
	game.forEachPlayer([&](Player& p) {
		Player& next = p.next();
		auto it = oldHands.find(p.getId());
		if (it != oldHands.end()) {
			for (auto& card : it->second) {
				next.gainCard(std::move(card));
			}
		}
	});

	//手牌数减少的玩家摸两张牌
	game.forEachPlayer([&](Player& p) {
		if (p.handCount() < oldCounts[p.getId()]) {
			p.draw(2, DrawReason::skill);
		}
	});

	std::println("<技能> {}发动剧变，所有角色将手牌交给下家", carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：兵多 ====================
bool 兵多::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.damage(1, std::nullopt);
	std::size_t num = (carrier.getHp() <= 50) ? 2 : 1;
	carrier.decree("兵多决议", num, false);

	std::println("<技能> {}发动兵多，决议{}张牌", carrier.displayName(), num);
	game.broadcastState();
	return true;
}

// ==================== 技能：通天 ====================
bool 通天::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::wild_draw4);
}
bool 通天::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (auto skill = carrier.findSkill<修正>()) {
		skill->get().increaseLimit(1);
		std::println("<技能> {}发动通天，修正次数+1", carrier.displayName());
	}
	else {
		carrier.addSkill(修正::make());
		carrier.findSkill<修正>()->get().setLimit(1);
		std::println("<技能> {}发动通天，获得修正技能（1次）", carrier.displayName());
	}
	return true;
}

