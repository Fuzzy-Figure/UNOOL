#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"


bool 装弹::filter(const GameLogic& game, const Player& carrier) const {
	const Hand& hand = carrier.getHand();
	return hand.count() <= 9
		&& hand.exclude([](const Card& c) {
		return c.is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4);
	});
}

bool 装弹::content(GameLogic& game, Player& carrier) {
	Hand& hand = carrier.getHand();
	std::vector<ref<Card>> cards;
	std::ranges::sample(
		hand | std::views::transform([](const auto& cardPtr) { return std::ref(*cardPtr); }),
		std::back_inserter(cards),
		unool::math::floor(hand.count() / 2.0),
		unool::random::rng
	);
	for (Card& c : cards) {
		c.set(Card::randomCard(&Card::isNotNumber));
	}
	return true;
}
