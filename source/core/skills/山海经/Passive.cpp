#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
#include <numeric>
#include <optional>

// ==================== 技能：棍击 ====================
bool 棍击::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 棍击::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional targetOpt = carrier.chooseOtherPlayer("选择棍击目标：", false);
	if (!targetOpt.has_value()) return false;

	Player& target = targetOpt.value();
	std::size_t damage = unool::math::pow(2, getCount());
	target.damage(damage, carrier);
	std::println("<技能> {}对{}发动棍击，造成{}点伤害！", carrier.displayName(), target.displayName(), damage);
	game.broadcastState();
	return true;
}


// ==================== 技能：神木 ====================
bool 神木::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_pal), 9);
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_draw4), 9);
	game.getPile().shuffle();
	return true;
}


// ==================== 技能：耐克 ====================
bool 耐克::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	Card& card = trigger.getCard();
	if (!card.is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4))
		return false;
	auto lastOpt = game.lastCard();
	if (!lastOpt.has_value()) return false;
	const Card& last = lastOpt.value();
	return last.is(Card::Color::blue) || last.isWild();
}
bool 耐克::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getCard().cancelEffect();
	std::println("{}触发技能，使此牌无效", carrier.displayName());
	return true;
}


// ==================== 技能：轰炸 ====================
bool 轰炸::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4);
}
bool 轰炸::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer().next();
	if (trigger.getCard().is(Card::Name::wild_draw4)) {
		target.damage(unool::math::ceil(target.getMaxHp() * 0.04), carrier);
	}
	else if (trigger.getCard().is(Card::Name::action_draw2)) {
		target.damage(unool::math::ceil(target.getMaxHp() * 0.02), carrier);
	}
	else {
		target.damage(unool::math::ceil(target.getMaxHp() * 0.01), carrier);
	}
	return true;
}


// ==================== 技能：爆破 ====================
bool 爆破::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getSource() == carrier;
}
bool 爆破::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	Hand& hand = player.getHand();
	auto card = hand.takeCardByIndex(unool::random::randomSize_t(0, hand.count() - 1));
	player.damage(card->value(), carrier);
	std::println("爆破获取了 [{}]，造成了{}点伤害！", *card, card->value());
	carrier.gainCard(std::move(card));
	return true;
}


//================== 森罗 =================
bool 森罗::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.getHand().forEachIf(
		[](const Card& c) {
		return c.getColor() != Card::Color::black;
	}, [](Card& c) {
		c.setColor(Card::Color::green);
	});
	return true;
}


bool 大脚::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isWild);
}

//================大脚=====================
bool 大脚::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard("弃置一张万能牌", 1, true, &Card::isWild);
	carrier.getHand().forEachIf(
		[](const Card& c) {
		return c.getColor() != Card::Color::black;
	}, [](Card& c) {
		c.setColor(Card::Color::green);
	});
	game.broadcastState();
	return true;
}


// ==================== 技能：黑帮 ====================
bool 黑帮::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t X = game.getGameCount();
	for (std::size_t i = 0; i < X; ++i) {
		carrier.gainCard(Card::make(Card::Color::black,
									unool::random::randomGet(Card::wildCards)));
	}
	std::println("<技能> {}发动黑帮，获得了{}张万能牌", carrier.displayName(), X);
	game.broadcastState();
	return true;
}

// ==================== 技能：拖拉 ====================
bool 拖拉::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 拖拉::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card::Name targetName = trigger.getCard().getName();

	std::vector<std::size_t> matchingIndices;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		if (carrier.getCardByIndex(i).getName() == targetName) {
			matchingIndices.push_back(i);
		}
	}

	for (auto it = matchingIndices.rbegin(); it != matchingIndices.rend(); ++it) {
		carrier.discardByIndex(*it);
	}

	std::size_t count = matchingIndices.size();
	if (count > 0) {
		carrier.recover(count);
	}
	std::println("<技能> {}发动拖拉，弃置了{}张牌，回复{}点体力", carrier.displayName(), count, count);
	game.broadcastState();
	return true;
}


