#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
#include <numeric>
#include <optional>

// ==================== 技能：寒魄 ====================
bool 寒魄::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handCount() == 1;
}
bool 寒魄::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	carrier.getCardByIndex(0).set(card);
	game.broadcastState();
	return true;
}


// ==================== 技能：雷剑 ====================
bool 雷剑::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_rev);
}
bool 雷剑::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	Card::Color color = card.getColor();

	auto discarded = carrier.chooseToDiscard("弃置一张数字牌", 1, false, &Card::isNumber);

	if (!discarded.empty()) {
		std::size_t value = discarded.front().get().value();
		carrier.recover(value);
		std::println("<技能> {}发动雷剑，弃置 [{}] 并回复{}点体力！！！！！！！！！！！！！", carrier.displayName(), discarded.front().get(), value);
		game.broadcastState();
		return true;
	}
	return false;
}


// ==================== 技能：追番 ====================
bool 追番::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//先检测手中是否有点数≤5的数字牌
	return carrier.handInclude([](const Card& c) {
		return c.isNumber() && c.value() <= 5;
	});
}
bool 追番::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	static auto lessEqual5Number = [](const Card& c) {
		return c.isNumber() && c.value() <= 5;
	};

	//选择一张点数≤5的数字牌
	return carrier.chooseToOperate(
		"选择一张点数≤5的数字牌", false, lessEqual5Number,
		[&carrier, &game](Card& card) {
		//自选+1/+2/+3
		const std::size_t addChoice = carrier.ask(
			"选择增加的点数：",
			{ "+1", "+2", "+3" },
			false
		);
		card.setName(Card::numberCardsFrom0[card.value() + addChoice]);
		std::println("<技能> {}发动追番，将一张{}的点数+{}", carrier.displayName(), card, addChoice);
		game.broadcastState();
	}
	).has_value();
}

// ==================== 技能：崩三 ====================
bool 崩三::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (trigger.getCard().is(Card::Name::number_3)) ++count3;
	else return false;
	return count3 % 2 == 0
		&& carrier.handInclude([](const Card& c) { return c.is(Card::Name::number_6); });
}
bool 崩三::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard(
		"选择一张[6]弃置", 1, false,
		[](const Card& c) { return c.is(Card::Name::number_6); });
	game.broadcastState();
	return true;
}

