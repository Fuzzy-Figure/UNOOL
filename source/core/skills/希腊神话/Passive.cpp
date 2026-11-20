#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
#include <functional>
#include <numeric>
#include <optional>
#include <ranges>
#include <string>
#include <unordered_set>
#include <vector>

// ==================== 技能：同心 ====================
bool 同心::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//根据打出牌的类型增加对应计数
	const Card::Type type = trigger.getCard().getType();
	switch (type) {
		case Card::Type::number: ++numberCount, actionCount = wildCount = 0; break;
		case Card::Type::action: ++actionCount, numberCount = wildCount = 0; break;
		case Card::Type::wild:   ++wildCount, numberCount = actionCount = 0;  break;
		default:                 std::unreachable();
	}

	//达到2张同类型时随机弃置一张牌并重置计数
	if (numberCount >= 2 || actionCount >= 2 || wildCount >= 2) {
		numberCount = actionCount = wildCount = 0;

		const std::size_t handCount = carrier.getHand().count();
		if (handCount == 0) return true;
		const std::size_t idx = unool::random::randomSize_t(0, handCount - 1);
		std::println("<技能> {}打出两张同类型牌，随机弃置：{}", carrier.displayName(),
					 carrier.getHand().getCardByIndex(idx));
		carrier.discardByIndex(idx);
		game.broadcastState();
	}
	carrier.markCharInfoDirty();
	return true;
}

std::optional<std::string> 同心::extraPlaceholders(const std::string& key) const {
	if (key == "numberCount") return std::to_string(numberCount);
	else if (key == "actionCount") return std::to_string(actionCount);
	else if (key == "wildCount") return std::to_string(wildCount);
	return std::nullopt;
}

// ==================== 技能：蝎刺 ====================
bool 蝎刺::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isTargeted();
}

bool 蝎刺::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = carrier.next();
	target.damage(damageValue, carrier);
	std::println("<技能> {}发动蝎刺，对{}造成{}点伤害", carrier.displayName(),
				 target.displayName(), damageValue);
	++damageValue;
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

std::optional<std::string> 蝎刺::extraPlaceholders(const std::string& key) const {
	if (key == "damage") return std::to_string(damageValue);
	return std::nullopt;
}

// ==================== 技能：镇魂 ====================
bool 镇魂::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return phaseCount < 1;
}

bool 镇魂::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto targetOpt = carrier.choosePlayer("【镇魂】选择一名角色：", true);
	if (!targetOpt) return false;
	Player& target = targetOpt->get();

	//随机造成1-5点伤害
	const std::size_t dmg = unool::random::randomSize_t(1, 5);
	std::println("<技能> {}发动镇魂，对{}随机造成{}点伤害", carrier.displayName(),
				 target.displayName(), dmg);
	target.damage(dmg, carrier);

	//若你手牌数不小于其，其弃置一张牌
	if (carrier.handCount() >= target.handCount() && target.handCount() > 0) {
		std::println("<技能> {}手牌数不小于{}，{}弃置一张牌", carrier.displayName(),
					 target.displayName(), target.displayName());
		target.chooseToDiscard("【镇魂】弃置一张牌：", 1, true);
	}

	++phaseCount;
	game.broadcastState();
	return true;
}

bool 镇魂_回合结束::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.getSkill<镇魂>().phaseCount < 1;
}

bool 镇魂_回合结束::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	return carrier.getSkill<镇魂>().content(game, carrier, trigger);
}

// ==================== 技能：坚甲 ====================
bool 坚甲::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//每打出一张牌后获得1点护甲，上限50点
	const std::size_t cur = carrier.getShield();
	if (cur < 50) {
		carrier.setShield(cur + 1);
		carrier.markCharInfoDirty();
	}
	return true;
}


// ==================== 新增星座角色技能 ====================
namespace {
	using CardPred = std::function<bool(const Card&)>;

	bool isBlue(const Card& c) { return c.is(Card::Color::blue); }
	bool isPurple(const Card& c) { return c.is(Card::Color::purple); }

	//收集牌堆中满足条件的牌的下标（不回收弃牌堆）
	std::vector<std::size_t> indicesIn(const Pile& pile, const CardPred& pred) {
		std::vector<std::size_t> result{};
		for (std::size_t i = 0; i < pile.count(); ++i) {
			if (pred(pile[i])) result.push_back(i);
		}
		return result;
	}

	//从牌堆取走一张指定(颜色,牌名)的牌；没有则为空智能指针
	std::unique_ptr<Card> takeOne(Pile& pile, const Card::Color color, const Card::Name name) {
		for (std::size_t i = 0; i < pile.count(); ++i) {
			if (pile[i].is(color) && pile[i].is(name)) return pile.takeCardByIndex(i);
		}
		return {};
	}

	//从牌堆随机取走一张满足条件的牌；没有则为空智能指针
	std::unique_ptr<Card> takeRandom(Pile& pile, const CardPred& pred) {
		const auto indices = indicesIn(pile, pred);
		if (indices.empty()) return {};
		return pile.takeCardByIndex(indices[unool::random::randomSize_t(0, indices.size() - 1)]);
	}

	std::size_t purpleInHand(const Player& p) {
		const Hand& hand = p.getHand();
		std::size_t result = 0;
		for (std::size_t i = 0; i < hand.count(); ++i) {
			if (hand.getCardByIndex(i).is(Card::Color::purple)) ++result;
		}
		return result;
	}

	//一套紫色牌：与红牌同规格（0×2、1~9×3、反转/封禁×3、+2×4），无紫色万能牌。
	//用「牌堆里是否已有紫牌」做幂等保护：双将或同时多个紫色角色时只加一套。
	void ensurePurpleSet(GameLogic& game) {
		Pile& pile = game.getPile();
		if (pile.include(isPurple)) return;
		for (const Card::Name name : Card::numberCardsFrom0) {
			pile.push_back(Card::make(Card::Color::purple, name),
						   name == Card::Name::number_0 ? 2 : 3);
		}
		pile.push_back(Card::make(Card::Color::purple, Card::Name::action_rev), 3);
		pile.push_back(Card::make(Card::Color::purple, Card::Name::action_skip), 3);
		pile.push_back(Card::make(Card::Color::purple, Card::Name::action_draw2), 4);
		pile.shuffle();
		std::println("<技能> 牌堆加入一套紫色卡牌（39 张）");
	}
}

// ==================== 南十字座【静渊】 ====================
bool 静渊::content(GameLogic& game, Player& carrier, Trigger&) {
	Pile& pile = game.getPile();
	const Hand& hand = carrier.getHand();
	std::vector<Card::Name> blueNames{};
	for (const Card::Name name : Card::numberCardsFrom0) blueNames.push_back(name);
	for (const Card::Name name : Card::actionCards) blueNames.push_back(name);

	std::size_t gained = 0;
	for (const Card::Name name : blueNames) {
		const bool alreadyInHand = hand.include([name](const Card& c) {
			return c.is(Card::Color::blue) && c.is(name);
		});
		if (alreadyInHand) continue;
		auto card = takeOne(pile, Card::Color::blue, name);
		if (!card) continue;
		std::println("<技能> {}发动静渊，从牌堆获得{}", carrier.displayName(), *card);
		carrier.gainCard(std::move(card));
		++gained;
	}
	if (gained > 0) game.broadcastState();
	return true;
}

bool 静渊_出牌::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && trigger.getCard().is(Card::Color::blue);
}

bool 静渊_出牌::content(GameLogic& game, Player& carrier, Trigger&) {
	静渊& main = carrier.getSkill<静渊>();
	game.forEachOtherPlayer(carrier, [&](Player& other) {
		std::vector<std::size_t> blues{};
		for (std::size_t i = 0; i < other.handCount(); ++i) {
			if (other.getCardByIndex(i).is(Card::Color::blue)) blues.push_back(i);
		}
		if (!blues.empty()) {
			const std::size_t idx = blues[unool::random::randomSize_t(0, blues.size() - 1)];
			std::println("<技能> {}发动静渊，{}随机重铸一张蓝色牌",
						 carrier.displayName(), other.displayName());
			other.recastByIndex(idx);
			return;
		}
		std::size_t& times = main.damageCounts[other.getId()];
		const std::size_t X = times;
		++times;  //即使造成 0 点伤害也计入次数
		std::println("<技能> {}发动静渊，{}没有蓝色牌，对其造成{}点伤害",
					 carrier.displayName(), other.displayName(), X);
		other.damage(X, std::ref(carrier));
	});
	game.broadcastState();
	return true;
}

// ==================== 半人马座【怀海】 ====================
bool 怀海::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && trigger.getCard().is(Card::Color::blue);
}

bool 怀海::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& loser = trigger.getPlayer();
	std::size_t& lost = lostCounts[loser.getId()];
	++lost;
	if (lost < 2) return true;
	lost = 0;

	if (auto card = takeRandom(game.getPile(), isBlue); card) {
		std::println("<技能> {}发动怀海，{}从牌堆随机获得{}",
					 carrier.displayName(), loser.displayName(), *card);
		loser.gainCard(std::move(card));
	}
	else {
		const std::size_t X = recoverCount;
		++recoverCount;  //即使回复 0 点也计入次数
		std::println("<技能> {}发动怀海，牌堆已无蓝色牌，回复{}点体力",
					 carrier.displayName(), X);
		carrier.recover(X);
	}
	game.broadcastState();
	return true;
}

// ==================== 船底座【巨潮】 ====================
bool 巨潮::filter(const GameLogic&, const Player&, const Trigger&) const {
	return !disabled;
}

bool 巨潮::content(GameLogic& game, Player& carrier, Trigger&) {
	Pile& pile = game.getPile();
	auto indices = indicesIn(pile, isBlue);
	const std::size_t count = std::min<std::size_t>(3, indices.size());
	if (count == 0) return true;

	std::ranges::shuffle(indices, unool::random::rng);
	indices.resize(count);
	//按下标降序取牌，避免前面的取出导致后面的下标失效
	std::ranges::sort(indices, std::greater<>{});
	std::vector<std::unique_ptr<Card>> shown{};
	for (const std::size_t i : indices) shown.push_back(pile.takeCardByIndex(i));
	for (const auto& c : shown) carrier.showCard(*c);

	std::vector<std::string> options{};
	for (const auto& c : shown) options.push_back(c->toString());
	const std::size_t choice = carrier.ask("【巨潮】选择获得其中一张：", options, true);
	const opt_ref<Player> other = carrier.chooseOtherPlayer(
		"【巨潮】将其余展示牌交给一名其他角色：", true);

	//口径：展示的牌中出现相同牌名，此技能本局失效
	std::unordered_set<Card::Name> names{};
	bool duplicate = false;
	for (const auto& c : shown) {
		if (!names.insert(c->getName()).second) duplicate = true;
	}
	if (duplicate) {
		disabled = true;
		std::println("<技能> {}的巨潮因展示牌出现相同牌名，本局失效", carrier.displayName());
	}

	if (choice >= 1 && choice <= shown.size()) carrier.gainCard(std::move(shown[choice - 1]));
	for (auto& c : shown) {
		if (!c) continue;
		if (other.has_value()) other->get().gainCard(std::move(c));
		else game.putCardToDiscardPile(std::move(c), Card::DiscardReason::discard, carrier);
	}
	game.broadcastState();
	return true;
}

// ==================== 鲸鱼座【鲸落】 ====================
bool 鲸落_入堆::content(GameLogic& game, Player&, Trigger&) {
	ensurePurpleSet(game);
	return true;
}

bool 鲸落::content(GameLogic& game, Player& carrier, Trigger&) {
	Pile& pile = game.getPile();
	for (const Card::Name name : Card::numberCardsFrom0) {
		auto card = takeOne(pile, Card::Color::purple, name);
		if (!card) continue;
		std::println("<技能> {}发动鲸落，获得{}", carrier.displayName(), *card);
		carrier.gainCard(std::move(card));
	}
	game.broadcastState();
	return true;
}

bool 鲸落_出牌::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && !trigger.getCard().is(Card::Color::purple);
}

bool 鲸落_出牌::content(GameLogic& game, Player& carrier, Trigger&) {
	const std::size_t X = purpleInHand(carrier);
	const opt_ref<Player> target = carrier.chooseOtherPlayer(
		std::format("【鲸落】选择一名其他角色造成{}点伤害（按0取消）：", X), false);
	if (!target.has_value()) return true;
	std::println("<技能> {}发动鲸落，对{}造成{}点伤害",
				 carrier.displayName(), target->get().displayName(), X);
	target->get().damage(X, std::ref(carrier));
	game.broadcastState();
	return true;
}

// ==================== 天龙座【龙霖】 ====================
bool 龙霖_入堆::content(GameLogic& game, Player&, Trigger&) {
	ensurePurpleSet(game);
	return true;
}

bool 龙霖::content(GameLogic& game, Player& carrier, Trigger&) {
	Pile& pile = game.getPile();
	const CardPred purpleAction = [](const Card& c) {
		return c.is(Card::Color::purple) && c.is(Card::Type::action);
	};
	for (std::size_t i = 0; i < 2; ++i) {
		auto card = takeRandom(pile, purpleAction);
		if (!card) break;
		std::println("<技能> {}发动龙霖，随机获得{}", carrier.displayName(), *card);
		carrier.gainCard(std::move(card));
	}
	game.broadcastState();
	return true;
}

bool 龙霖_出牌::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && trigger.getCard().is(Card::Color::purple);
}

bool 龙霖_出牌::content(GameLogic& game, Player& carrier, Trigger&) {
	龙霖& main = carrier.getSkill<龙霖>();
	++main.purplePlayed;
	if (main.purplePlayed < 2) return true;
	main.purplePlayed = 0;

	const opt_ref<Player> target = carrier.choosePlayer(
		"【龙霖】选择一名角色，其随机两张非紫色牌变为紫色（按0取消）：", false);
	if (!target.has_value()) return true;
	Player& p = target->get();
	Hand& hand = p.getHand();
	std::vector<std::size_t> wilds{};
	std::vector<std::size_t> others{};
	for (std::size_t i = 0; i < hand.count(); ++i) {
		Card& c = hand.getCardByIndex(i);
		if (c.is(Card::Color::purple)) continue;
		if (c.isWild()) wilds.push_back(i);
		else others.push_back(i);
	}
	std::ranges::shuffle(wilds, unool::random::rng);
	std::ranges::shuffle(others, unool::random::rng);
	std::vector<std::size_t> picked{};
	for (const std::size_t i : wilds) {
		if (picked.size() >= 2) break;
		picked.push_back(i);
	}
	for (const std::size_t i : others) {
		if (picked.size() >= 2) break;
		picked.push_back(i);
	}
	for (const std::size_t i : picked) {
		Card& c = hand.getCardByIndex(i);
		std::println("<技能> {}发动龙霖，{}的{}变为紫色",
					 carrier.displayName(), p.displayName(), c);
		c.setColor(Card::Color::purple);
	}
	game.broadcastState();
	return true;
}

// ==================== 长蛇座【蟒川】 ====================
bool 蟒川_入堆::content(GameLogic& game, Player&, Trigger&) {
	ensurePurpleSet(game);
	return true;
}

bool 蟒川::content(GameLogic& game, Player& carrier, Trigger&) {
	Hand& hand = carrier.getHand();
	std::vector<std::size_t> purpleIdx{};
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand.getCardByIndex(i).is(Card::Color::purple)) purpleIdx.push_back(i);
	}
	if (purpleIdx.empty()) return true;
	const opt_ref<Player> other = carrier.chooseOtherPlayer(
		"【蟒川】将手中所有紫色牌交给一名其他角色：", true);
	if (!other.has_value()) return true;
	Player& to = other->get();
	std::ranges::sort(purpleIdx, std::greater<>{});
	for (const std::size_t i : purpleIdx) {
		std::println("<技能> {}发动蟒川，将{}交给{}",
					 carrier.displayName(), hand.getCardByIndex(i), to.displayName());
		to.gainCard(hand.takeCardByIndex(i));
	}
	game.broadcastState();
	return true;
}

bool 蟒川_出牌::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && trigger.getCard().is(Card::Color::purple);
}

bool 蟒川_出牌::content(GameLogic& game, Player& carrier, Trigger&) {
	if (carrier.getShield() >= 50) {
		std::println("<技能> {}发动蟒川，护盾已达上限，改为弃置一张牌", carrier.displayName());
		carrier.chooseToDiscard("【蟒川】护盾已达上限，弃置一张牌：", 1, true);
	}
	else {
		const std::size_t X = std::max<std::size_t>(1, purpleInHand(carrier));
		carrier.setShield(std::min<std::size_t>(50, carrier.getShield() + X));
		carrier.markCharInfoDirty();
		std::println("<技能> {}发动蟒川，获得{}点护盾（当前{}点）",
					 carrier.displayName(), X, carrier.getShield());
	}
	game.broadcastState();
	return true;
}

// ==================== 仙王座【圣权】 ====================
bool 圣权_布置::content(GameLogic& game, Player& carrier, Trigger&) {
	//观察用引用包装器：钩子只读取携带者的技能状态，不持有所有权
	const std::reference_wrapper<Player> owner = std::ref(carrier);
	game.forEachOtherPlayer(carrier, [owner](Player& other) {
		other.setPlayVeto("圣权", [owner](const Card& card) {
			Player& p = owner.get();
			if (p.isSealed()) return false;  //封印时技能失效
			const opt_ref<圣权> skill = p.findSkill<圣权>();
			return skill.has_value() && !skill->get().allows(card);
		});
	});
	return true;
}

bool 圣权::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const Card::Type type = trigger.getCard().getType();
	if (playedTypes.contains(type)) return true;
	playedTypes.insert(type);
	std::println("<技能> {}首次打出{}，决议至多三张牌", carrier.displayName(), type);
	//forced=false：玩家可按 ↓ 中止；决议三张
	carrier.decree("【圣权】决议至多三张牌（↓中止）", 3, false);
	game.broadcastState();
	return true;
}

// ==================== 仙后座【圣柄】 ====================
bool 圣柄_布置::content(GameLogic& game, Player& carrier, Trigger&) {
	const std::reference_wrapper<Player> owner = std::ref(carrier);
	game.forEachOtherPlayer(carrier, [owner](Player& other) {
		other.setPlayVeto("圣柄", [owner](const Card& card) {
			Player& p = owner.get();
			if (p.isSealed()) return false;
			const opt_ref<圣柄> skill = p.findSkill<圣柄>();
			return skill.has_value() && !skill->get().allows(card);
		});
	});
	return true;
}

bool 圣柄::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const Card::Color color = trigger.getCard().getColor();
	if (playedColors.contains(color)) return true;
	playedColors.insert(color);
	std::println("<技能> {}首次打出{}色牌，重铸至多三张牌", carrier.displayName(), color);
	//forced=false：玩家可按 ↓ 中止；重铸三张
	carrier.chooseToRecast("【圣柄】重铸至多三张牌（↓中止）", 3, false);
	game.broadcastState();
	return true;
}

// ==================== 猎户座【狩锋】 ====================
bool 狩锋::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return !disabled && trigger.hasCards() && trigger.getCard().is(Card::Name::wild_draw4);
}

bool 狩锋::content(GameLogic& game, Player& carrier, Trigger&) {
	const std::size_t half = std::min<std::size_t>(carrier.handCount() / 2, 8);
	if (half == 0) return true;
	const auto result = carrier.chooseToRecast("【狩锋】重铸一半手牌（↓中止）", half, false);
	if (result.discarded.empty()) return true;  //↓ 中止，不进入展示结算

	std::unordered_set<Card::Name> recastNames{};
	for (const auto& c : result.discarded) recastNames.insert(c.get().getName());

	//展示牌堆顶四张：取出牌堆，未获得的最终进弃牌堆
	Pile& pile = game.getPile();
	const std::size_t available = pile.count() + game.getDiscardPile().count();
	const std::size_t revealCount = std::min<std::size_t>(4, available);
	std::vector<std::unique_ptr<Card>> shown{};
	for (std::size_t i = 0; i < revealCount; ++i) {
		auto card = pile.take_front(game.getDiscardPile());
		carrier.showCard(*card);
		shown.push_back(std::move(card));
	}
	std::size_t matches = 0;
	for (const auto& c : shown) {
		if (recastNames.contains(c->getName())) ++matches;
	}
	std::println("<技能> {}发动狩锋，匹配牌名数为{}", carrier.displayName(), matches);

	//1：可选择展示牌中一张获得之
	const auto gainOne = [&]() {
		std::vector<std::string> options{};
		for (const auto& c : shown) options.push_back(c->toString());
		const std::size_t choice = carrier.ask(
			"【狩锋】选择展示牌中一张获得（按0取消）：", options, false);
		if (choice >= 1 && choice <= shown.size()) carrier.gainCard(std::move(shown[choice - 1]));
	};
	//2：对一名其他角色造成10点伤害（两座位桌：下家即唯一其他角色）
	const auto dealTen = [&]() {
		Player& target = carrier.next();
		std::println("<技能> {}发动狩锋，对{}造成10点伤害",
					 carrier.displayName(), target.displayName());
		target.damage(10, std::ref(carrier));
	};
	//3：回复10点体力
	const auto recoverTen = [&]() {
		std::println("<技能> {}发动狩锋，回复10点体力", carrier.displayName());
		carrier.recover(10);
	};

	switch (matches) {
		case 0:
			disabled = true;
			std::println("<技能> {}的狩锋本局失效", carrier.displayName());
			break;
		case 1: gainOne(); break;
		case 2: dealTen(); break;
		case 3: recoverTen(); break;
		default:
			gainOne();
			dealTen();
			recoverTen();
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
			break;
	}

	for (auto& c : shown) {
		if (!c) continue;
		game.putCardToDiscardPile(std::move(c), Card::DiscardReason::discard, carrier);
	}
	game.broadcastState();
	return true;
}


