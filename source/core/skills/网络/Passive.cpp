#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
#include <numeric>
#include <optional>

// ==================== 技能：隐身 ====================
bool 隐身::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2, Card::Name::wild_draw4);
}
bool 隐身::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getCard().cancelEffect();
	trigger.getSource().draw(1, DrawReason::skill);
	return true;
}


// ==================== 技能：带派 ====================
bool 带派::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t choice = carrier.ask(
		"发动[带派]，选择一项：", {
		"获得一张变色",
		"获得一张+4",
		"都获得并失去25体力"
		}, true);
	switch (choice) {
		case 1:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
			break;
		case 2:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
			break;
		case 3:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
			carrier.damage(25, carrier);
			break;
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：军国 ====================
bool 军国::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	if (player != carrier) //其他角色：失去 1% 最大体力，向上取整
		player.damage(unool::math::ceil(player.getMaxHp() * 0.01), carrier);
	else //自己：失去1体力
		player.damage(1, carrier);
	return true;
}


// ==================== 技能：家暴 ====================
bool 家暴::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return game.playersInclude(
		//有玩家的血量 < 携带者
		[&trigger, &carrier](const Player& p) {
		return p.getHp() < carrier.getHp();
	});
}
bool 家暴::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional targetOpt = carrier.chooseOtherPlayer("选择家暴目标：", false, [&carrier](const Player& p) {
		return p.getHp() < carrier.getHp();
	});
	if (!targetOpt.has_value()) return false;

	Player& target = targetOpt.value();
	std::size_t damage = unool::math::ceil(target.getMaxHp() * 0.1);
	target.damage(damage, carrier);
	std::println("<技能> {}对{}发动家暴，造成{}点伤害！", carrier.displayName(), target.displayName(), damage);

	game.broadcastState();
	return true;
}


// ==================== 技能：买棋 ====================
bool 买棋::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.damage(10 * (getCount() - 1), carrier);
	if (unool::random::probability(0.5)) { //万能
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	}
	else { //+4
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
	}
	return true;
}


// ==================== 技能：卖棋 ====================
bool 卖棋::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isWild);
}
bool 卖棋::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (carrier.chooseToDiscard("弃置一张万能牌", 1, false, &Card::isWild).size() == 1) {
		carrier.recover(10);
		return true;
	}
	return false;
}


// ==================== 技能：电音 ====================
bool 电音::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Hand& hand = carrier.getHand();
	hand.forEachIf(&Card::isNumber, [](Card& card) {
		card.setName(unool::random::randomGet(Card::numberCardsFrom0));
	});
	carrier.recover(1);
	game.broadcastState();
	return true;
}


// ==================== 技能：蒙面 ====================
bool 蒙面::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getNumber() = unool::math::ceil(trigger.getNumber() * 0.75);
	return true;
}


// ==================== 技能：锐刻 ====================
bool 锐刻::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (disabled) return false;
	return trigger.getCard().getName() == Card::Name::number_5;
}
bool 锐刻::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t choice = carrier.ask(
		"发动[锐刻]，选择一项：", {
		"令一名角色摸1张牌",
		"令一名角色摸5张牌并失去此技能至本局结束"
		}, false
	);

	if (choice == 0) return false;

	const auto& candidates = game.getPlayers();
	std::vector<std::string> options;
	for (const auto& p : candidates) {
		options.push_back(p.get().characterName());
	}
	std::size_t targetChoice = carrier.ask("选择目标角色：", options, true);
	Player& target = candidates[targetChoice - 1].get();

	if (choice == 1) {
		target.draw(1, DrawReason::skill);
		std::println("<技能> {}发动锐刻，令{}摸1张牌", carrier.displayName(), target.displayName());
	}
	else {
		target.draw(5, DrawReason::skill);
		disabled = true;
		std::println("<技能> {}发动锐刻，令{}摸5张牌，失去此技能至本局结束", carrier.displayName(), target.displayName());
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：假酒 ====================
bool 假酒::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isAction();
}
bool 假酒::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto card = std::make_unique<Card>(Card::randomCard());
	if (card->isNumber()) number = true;
	else if (card->isAction()) action = true;
	else if (card->isWild()) wild = true;

	carrier.gainCard(std::move(card));

	//弃牌
	if (number && action && wild) {
		carrier.chooseToDiscard("[假酒] 弃置两张牌", 2, true);
		limit = 0;
	}
	game.broadcastState();
	return true;
}
void 假酒::reset() {
	PassiveSkill::reset();
	number = action = wild = false;
}

// ==================== 技能：生存 ====================
bool 生存::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handCount() > 0;
}
bool 生存::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t x = getCount();
	if (x > 9) x = 9;
	Card::Name targetName = static_cast<Card::Name>(static_cast<int>(Card::Name::number_0) + x);

	Card::Color targetColor = unool::random::randomGet(Card::fourColors);

	std::unique_ptr<Card> target = Card::make(targetColor, targetName);
	auto opt = carrier.chooseToOperate(
		std::format("请选择一张牌变为【{}】", x),
		true, unool::alwaysTrue,
		[&target](Card& c) {
		c.set(*target);
	});
	if (opt.has_value()) {
		std::println("<技能> {}发动生存，将一张手牌改为【{}】", carrier.displayName(), *target);
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：创造 ====================
bool 创造::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (c.getName() != Card::Name::number_9) return false;
	if (c.is(Card::Color::black)) return false;
	return usedColors.find(c.getColor()) == usedColors.end();
}
bool 创造::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const Card& nine = trigger.getCard();
	Card::Color targetColor = nine.getColor();

	std::vector<std::string> opts = {
		"1", "2", "3", "4", "5",
		"6", "7", "8", "9", "0",
		"反转", "封禁", "+2"
	};
	std::size_t idx = carrier.ask(std::format(
		"【创造】选择获得的牌名（{}色）：", targetColor
	), opts, true);
	Card::Name name;
	if (1 <= idx && idx <= 10) { //数字牌
		name = Card::numberCardsFrom1[idx];
	}
	else if (11 <= idx && idx <= 13) { //功能牌
		name = Card::actionCards[idx - 11];
	}
	else throw std::runtime_error("意外的 idx 的值");

	std::unique_ptr<Card> newCard = Card::make(targetColor, name);
	std::println("<技能> {}发动创造，获得一张【{}】", carrier.displayName(), *newCard);
	carrier.gainCard(std::move(newCard));
	usedColors.insert(targetColor);
	game.broadcastState();
	return true;
}


//==============举报=============
bool 举报::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 举报::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	setForced(true);
	Player& player = trigger.getPlayer();
	player.damage(unool::math::ceil(0.1 * player.getHp()), carrier);
	return true;
}
void 举报::reset() {
	PassiveSkill::reset();
	setForced(false);
}


//=============猥琐====================
bool 猥琐::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//找到最大体力
	Character::hp_t maxHp = 0;
	game.forEachPlayer([&maxHp](const Player& p) {
		maxHp = std::max(maxHp, p.getHp());
	});
	return carrier.getHp() != maxHp;
}
bool 猥琐::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.recover(1);
	return true;
}



bool 棋王::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return game.getDiscardPile().count() >= 2
		&& trigger.getCard() == game.getDiscardPile()[1];
}
bool 棋王::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard("弃置两张手牌", 2, true);
	return true;
}


bool 金铲::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getNumber() == 1;
}
bool 金铲::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getNumber() = 0;
	trigger.getPlayer().damage(1, carrier);
	return true;
}


bool 射门::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isNumber();
}
bool 射门::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//选角色
	const auto& targetOpt = carrier.choosePlayer("选择射门目标：", true);
	if (!targetOpt.has_value()) return false;

	//判定
	Player& target = targetOpt.value();
	if (target.judge().is(Card::Color::blue, Card::Color::black)) {
		//执行效果
		if (target == carrier) {
			target.chooseToDiscard("[射门] 弃置一张牌", 1, true);
		}
		else {
			target.draw(1, DrawReason::skill);
		}
	}
	return true;
}


// ==================== 技能：有活 ====================
bool 有活::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t X = game.getGameCount();

	carrier.draw(2, DrawReason::skill);
	std::println("<技能> {}发动有活，摸了2张牌", carrier.displayName());

	carrier.chooseToDiscard(std::format("[有活] 弃置{}张牌", X), X, true);
	std::println("<技能> {}发动有活，弃置了{}张牌", carrier.displayName(), X);

	game.broadcastState();
	return true;
}

bool 水鬼::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//至少一张获得的牌是蓝色
	std::size_t blueCount = 0;
	for (const auto& c : trigger.getCards()) {
		if (c.get().is(Card::Color::blue)) ++blueCount;
	}
	if (blueCount == 0) return false;
	//弃置蓝色牌后手牌非空（能弃置一张其他牌）
	return carrier.handCount() > blueCount;
}
bool 水鬼::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//找出获得的蓝色牌在手牌中的下标
	std::vector<std::size_t> blueIndices;
	for (const auto& c : trigger.getCards()) {
		if (c.get().getColor() != Card::Color::blue) continue;
		const Card& card{c.get()};
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			if (&carrier.getCardByIndex(i) == &card) {
				blueIndices.push_back(i);
				break;
			}
		}
	}
	//从大到小弃置（避免下标偏移）
	std::ranges::sort(blueIndices, std::greater{});
	for (std::size_t idx : blueIndices) {
		carrier.discardByIndex(idx);
	}
	std::println("<技能> {}发动水鬼，弃置了{}张蓝色牌", carrier.displayName(), blueIndices.size());
	//发状态包，确保客户端渲染前有最新数据
	game.broadcastState();
	//弃置一张其他牌
	carrier.chooseToDiscard("【水鬼】弃置一张其他牌", 1, true);
	game.broadcastState();
	return true;
}

bool 爆缸::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Hand& hand = carrier.getHand();
	return std::ranges::all_of(hand, &Card::isNumber);
}
bool 爆缸::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	setForced(true);
	std::optional card = carrier.chooseToOperate(
		"[爆缸] 选择一张数字牌变为同色的随机功能牌", true,
		&Card::isNumber, [](Card& c) {
		c.setName(unool::random::randomGet(Card::actionCards));
	});
	return true;
}
void 爆缸::reset() {
	PassiveSkill::reset();
	setForced(false);
}

// ==================== 技能：落水 ====================
bool 落水::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Color::blue);
}
bool 落水::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	setForced(true);
	Player& player = trigger.getPlayer();
	Pile& pile = game.getPile();

	//其他三种颜色
	std::vector<Card::Color> otherColors;
	for (auto c : Card::fourColors) {
		if (c != Card::Color::blue) otherColors.push_back(c);
	}

	for (Card::Color targetColor : otherColors) {
		//在牌堆中搜索此颜色的牌
		std::vector<std::size_t> indices;
		for (std::size_t i = 0; i < pile.count(); ++i) {
			if (pile[i].is(targetColor)) indices.push_back(i);
		}
		if (indices.empty()) continue;
		std::size_t pick = unool::random::randomSize_t(0, indices.size() - 1);
		auto card = pile.takeCardByIndex(indices[pick]);
		player.gainCard(std::move(card));
	}

	std::println("<技能> {}发动落水，{}从牌堆获得了其他三色牌各一张", carrier.displayName(), player.displayName());
	game.broadcastState();
	return true;
}


// ==================== 技能：骚扰 ====================
bool 骚扰::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return !disabled;
}
bool 骚扰::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& result = carrier.judge();
	if (result.is(Card::Color::blue)) {
		//判定为蓝色：失去此技能至本局结束
		disabled = true;
		std::println("<技能> {}发动骚扰，判定为蓝色，失去此技能至本局结束", carrier.displayName());
		game.broadcastState();
		return true;
	}

	//判定非蓝色：回复1点体力
	carrier.recover(1);
	//重置【落水】使用次数（不改是否锁定）
	if (auto opt = carrier.findSkill<落水>()) {
		opt->get().resetCount();
	}
	std::println("<技能> {}发动骚扰，判定非蓝色，回复1点体力并重置落水次数", carrier.displayName());
	game.broadcastState();
	return true;
}


bool 黑洞::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//前4张牌有未被记录的
	const Pile& discardPile = game.getDiscardPile();
	for (const auto& card : discardPile | std::views::take(4)) {
		if (!record.contains(card->getName())) return true;
	}
	return false;
}
bool 黑洞::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Pile& discardPile = game.getDiscardPile();
	std::vector<std::string> options;
	std::vector<std::size_t> validIndices;
	for (std::size_t i = 0; const auto& card : discardPile | std::views::take(4)) {
		if (!record.contains(card->getName())) {
			options.push_back(card->toString());
			validIndices.push_back(i);
		}
		++i;
	}
	const std::size_t choice = carrier.ask(
		"[黑洞] 选择一张牌获得", options, false
	);
	if (choice == 0) return false;

	std::unique_ptr<Card> card = discardPile.takeCardByIndex(validIndices[choice - 1]);
	record.insert(card->getName());
	carrier.gainCard(std::move(card));

	game.broadcastState();
	return true;
}
void 黑洞::reset() {
	PassiveSkill::reset();
	record.clear();
}

bool 好事::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 好事::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.showCard(trigger.getCard());
	carrier.recover(5);
	return true;
}

bool 压抑::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 压抑::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional targetOpt = carrier.choosePlayer(
		"[压抑] 选择一名角色", false
	);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value();

	//找出目标手牌中点数最大的数字牌
	Hand& hand = target.getHand();
	std::optional<Card::Name> maxName;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		Card& card = hand[i];
		if (card.isNumber()) {
			if (!maxName.has_value() || card.getName() > *maxName) {
				maxName = card.getName();
			}
		}
	}
	if (!maxName.has_value()) {
		std::println("<技能> {}发动压抑，{}手中无数字牌", carrier.displayName(), target.displayName());
		game.broadcastState();
		return true;
	}

	//收集所有最大点数的牌的索引
	std::vector<std::size_t> indices;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].isNumber() && hand[i].getName() == *maxName) {
			indices.push_back(i);
		}
	}

	//从后往前弃牌，避免索引偏移
	for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
		target.discardByIndex(*it);
	}

	std::println("<技能> {}发动压抑，{}弃置了{}张点数最大的数字牌", carrier.displayName(), target.displayName(), indices.size());
	//弃置牌数≥2张：失去10%最大体力（向上取整）
	if (indices.size() >= 2) {
		const std::size_t damage = unool::math::ceil(target.getMaxHp() * 0.1);
		target.damage(damage, carrier);
		std::println("{}失去{}点体力", target.displayName(), damage);
	}

	game.broadcastState();
	return true;
}


bool 捉奸_弃牌::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 捉奸_弃牌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (carrier.discardByIndex(unool::random::randomSize_t(0, carrier.handCount() - 1)).is(Card::Color::red)) {
		carrier.damage(unool::math::ceil(carrier.getHp() * 0.05), carrier);
	}
	game.broadcastState();
	return true;
}

bool 捉奸::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Hand& hand = carrier.getHand();
	//寻找非红的非万能牌下标
	std::vector<std::size_t> notRedIndex;
	for (const auto [i, c] : hand | std::views::enumerate) {
		if (c->getColor() != Card::Color::red && c->isNotWild())
			notRedIndex.push_back(i);
	}

	//寻找红牌下标
	std::vector<std::size_t> redIndex;
	for (const auto [i, c] : hand | std::views::enumerate) {
		if (c->is(Card::Color::red))
			redIndex.push_back(i);
	}

	//变色
	for (const std::size_t index : notRedIndex) {
		hand[index].setColor(Card::Color::red);
	}

	static const std::array<Card::Color, 3> colors = {
		Card::Color::blue, Card::Color::green, Card::Color::yellow
	};
	for (const std::size_t index : redIndex) {
		hand[index].setColor(unool::random::randomGet(colors));
	}

	game.broadcastState();
	return true;
}

// ==================== 技能：没座 ====================
bool 没座::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const auto& hand = carrier.getHand();
	const bool hasRed = hand.include([](const Card& c) {
		return c.is(Card::Color::red);
	});
	//没红且手牌为1时不触发；其余情况触发
	return hasRed || carrier.handCount() != 1;
}
bool 没座::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const auto& hand = carrier.getHand();
	const bool hasRed = hand.include([](const Card& c) {
		return c.is(Card::Color::red);
	});
	if (!hasRed) {
		//没红牌：随机获得一张红色牌
		carrier.gainCard(std::make_unique<Card>(Card::randomCard([](const Card& c) {
			return c.is(Card::Color::red);
		})));
		std::println("<技能> {}发动没座，随机获得了一张红牌", carrier.displayName());
	}
	else {
		//有红牌：可弃一张非红色牌
		auto discarded = carrier.chooseToDiscard("[没座] 弃置一张非红色牌", 1, false,
												 [](const Card& c) { return !c.is(Card::Color::red); });
		if (discarded.empty()) return false; //玩家取消
		std::println("<技能> {}发动没座，弃置了一张非红色牌", carrier.displayName());
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：空空 ====================
bool 空空::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isAction);
}
bool 空空::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto cardRef = carrier.chooseToOperate(
		"【空空】选择一张功能牌变为红色的【封禁】", false,
		&Card::isAction, [](Card&) {});
	if (!cardRef.has_value()) return false;
	Card& card = cardRef->get();
	card.setColor(Card::Color::red);
	card.setName(Card::Name::action_skip);
	std::println("<技能> {}发动空空，将一张功能牌变为红色的【封禁】", carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：暗忍 ====================
bool 暗忍::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isAction);
}

bool 暗忍::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.damage(1, carrier);
	//收集功能牌索引
	Hand& hand = carrier.getHand();
	std::vector<std::size_t> actionIndices;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].isAction()) actionIndices.push_back(i);
	}
	if (!actionIndices.empty()) {
		std::size_t pick = actionIndices[unool::random::randomSize_t(0, actionIndices.size() - 1)];
		Card& c = carrier.getCardByIndex(pick);
		Card::Color randomColor = unool::random::randomGet(Card::fourColors);
		c.set(randomColor, Card::Name::action_skip);
		std::println("<技能> {}发动暗忍，失去1点体力并将一张功能牌变为随机基础颜色的【封禁】", carrier.displayName());
	}
	else {
		std::println("<技能> {}发动暗忍，失去1点体力（无功能牌可变）", carrier.displayName());
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：四麻 ====================
bool 四麻::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handCount() != 4;
}

bool 四麻::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t cnt = carrier.handCount();
	if (cnt > 4) {
		carrier.chooseToDiscard("【四麻】弃置牌调整至四张", cnt - 4, true);
		std::println("<技能> {}发动四麻，弃置了{}张牌", carrier.displayName(), (cnt - 4));
	}
	else if (cnt < 4) {
		carrier.draw(4 - cnt, DrawReason::skill);
		std::println("<技能> {}发动四麻，摸了{}张牌", carrier.displayName(), (4 - cnt));
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：走位 ====================
bool 走位::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//X = 已输局数，至多3
	std::size_t X = std::min(carrier.getLosses(), std::size_t{ 3 });

	//未输过（X=0）：失去走位+获得芜湖
	if (X == 0) {
		carrier.removeSkill<走位>();
		carrier.addSkill(芜湖::make());
		std::println("<技能> {}未输过局，失去【走位】并获得【芜湖】", carrier.displayName());
		game.broadcastState();
		return true;
	}

	//询问决议几张（0..X）
	std::vector<std::string> opts;
	opts.push_back("不决议");
	for (std::size_t i = 1; i <= X; ++i) {
		opts.push_back(std::format("决议{}张", i));
	}
	std::size_t choice = carrier.ask(std::format("【走位】决议至多{}张牌", X), opts, false);

	//玩家取消或选0张：失去走位+获得芜湖
	if (choice == 0 || choice == 1) {
		carrier.removeSkill<走位>();
		carrier.addSkill(芜湖::make());
		std::println("<技能> {}未决议牌，失去【走位】并获得【芜湖】", carrier.displayName());
		game.broadcastState();
		return true;
	}

	//决议 N 张
	std::size_t N = choice - 1;  //choice=2对应N=1
	carrier.decree("【走位】决议牌", N, true);
	std::println("<技能> {}发动走位，决议了{}张牌", carrier.displayName(), N);
	game.broadcastState();
	return true;
}


// ==================== 技能：九一 ====================
bool 九一::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!(c.is(Card::Name::number_9, Card::Name::number_1))) return false;
	return carrier.handInclude(&Card::isNumber);
}

bool 九一::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard("弃置一张【9】或【1】", 1, false,
							[](const Card& c) { return c.is(Card::Name::number_9, Card::Name::number_1); });
	std::println("<技能> {}发动九一", carrier.displayName());
	game.broadcastState();
	return true;
}


// ==================== 技能：白虎 ====================
bool 白虎::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Player& target = trigger.getPlayer();
	if (triggeredPlayers.contains(target.getId())) return false;
	return target.handCount() == 1;
}

bool 白虎::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& caster = carrier;   //唐伯虎（技能持有者）
	Player& target = trigger.getPlayer();    //回合开始的角色
	//caster 选颜色
	auto colorOpt = caster.chooseCardColor("发动【白虎】，选择一个颜色", true,
										   { Card::Color::red, Card::Color::yellow, Card::Color::green, Card::Color::blue });
	if (!colorOpt) return false;
	Card::Color color = *colorOpt;

	//caster 选牌名（9或1）
	auto nameOpt = caster.chooseCardName("选择一个牌名", true,
										 { Card::Name::number_9, Card::Name::number_1 });
	if (!nameOpt) return false;
	Card::Name name = *nameOpt;

	//新建一张牌给目标
	auto card = Card::make(color, name);
	std::println("<技能> {}发动白虎，令{}获得{}", caster.displayName(), target.displayName(), *card);
	target.gainCard(std::move(card));

	triggeredPlayers.insert(target.getId());
	game.broadcastState();
	return true;
}

void 白虎::reset() {
	triggeredPlayers.clear();
	PassiveSkill::reset();
}


//熬鹰_计数：回合开始时累计已执行的回合数
bool 熬鹰_计数::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//被封禁的回合会被跳过，不计入累计回合数
	if (carrier.isBanned()) return false;
	auto opt = carrier.findSkill<熬鹰>();
	if (!opt.has_value()) return false;
	//已觉醒后不再计数
	return !opt.value().get().isAwakened();
}

bool 熬鹰_计数::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto opt = carrier.findSkill<熬鹰>();
	if (!opt.has_value()) return false;
	++opt.value().get().turnCount;
	//刷新角色信息中的回合数显示
	carrier.markCharInfoDirty();
	return true;
}

//熬鹰_解除限制：觉醒后，第二个出牌阶段解除出牌限制
bool 熬鹰_解除限制::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	auto opt = carrier.findSkill<熬鹰>();
	if (!opt.has_value()) return false;
	return opt.value().get().isAwakened();
}

bool 熬鹰_解除限制::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.setUnrestrictedPlay(true);
	std::println("<技能> {}已觉醒【熬鹰】，第二个出牌阶段出牌不受限制", carrier.displayName());
	game.broadcastState();
	return true;
}

//熬鹰：第20个回合结束时觉醒，从游戏外获得一张【+4】
bool 熬鹰::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (awakened) return false;
	return turnCount >= 20;
}

bool 熬鹰::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	awakened = true;
	//从游戏外获得一张【+4】
	carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
	std::println("<技能> {}觉醒，发动熬鹰，从游戏外获得一张【+4】，第二个出牌阶段出牌不受限制",
				 carrier.displayName());
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

void 熬鹰::reset() {
	PassiveSkill::reset();
	turnCount = 0;
	awakened = false;
}

std::optional<std::string> 熬鹰::extraPlaceholders(const std::string& key) const {
	if (key == "turns") return std::to_string(turnCount);
	return std::nullopt;
}


// ==================== 技能：连杀 ====================
bool 连杀::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//本回合未打出牌（含被封禁而跳过的回合）：连续回合数归零
	if (!carrier.getHasUsed()) {
		if (consecutive == 0) return true;
		consecutive = 0;
		std::println("<技能> {}本回合未打出牌，连杀连续回合数归零", carrier.displayName());
		carrier.markCharInfoDirty();
		game.broadcastState();
		return true;
	}
	++consecutive;
	std::println("<技能> {}连杀，当前连续打出牌回合数：{}", carrier.displayName(), consecutive);
	carrier.markCharInfoDirty();
	//每连续三回合打出牌，从游戏外随机获得一张万能牌
	if (consecutive >= 3) {
		consecutive = 0;
		Card::ColorName cn = Card::randomCard([](const Card& c) { return c.isWild(); });
		const std::string cardName{ Card::to_string(std::get<1>(cn)) };
		carrier.gainCard(Card::make(cn));
		std::println("<技能> {}发动连杀，从游戏外随机获得一张【{}】", carrier.displayName(), cardName);
	}
	game.broadcastState();
	return true;
}

void 连杀::reset() {
	PassiveSkill::reset();
	consecutive = 0;
}

std::optional<std::string> 连杀::extraPlaceholders(const std::string& key) const {
	if (key == "turns") return std::to_string(consecutive);
	return std::nullopt;
}


