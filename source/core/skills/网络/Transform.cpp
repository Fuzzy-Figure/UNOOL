#include "skills/Skills.h"
#include "GameLogic.h"

// ==================== 技能：我妈 ====================
std::size_t 我妈::getCardCount() const {
	return 1;
}

bool 我妈::canSelect(const Card& c) const {
	return !c.is(Card::Color::red);
}

bool 我妈::transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const {
	if (cards.empty()) return false;
	Card& c = cards.front().get();
	c.set(Card::Color::red, Card::Name::action_skip);
	return true;
}

void 我妈::addition(GameLogic& game, Player& carrier) const {
	carrier.draw(1, DrawReason::skill);
	game.broadcastState();
}

std::string 我妈::getPrompt() const {
	return "将一张非红色牌当作红【封禁】打出";
}


// ==================== 技能：曼巴 ====================
std::size_t 曼巴::getCardCount() const {
	return 1;
}

bool 曼巴::canSelect(const Card& c) const {
	return c.is(Card::Name::number_8);
}

bool 曼巴::transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const {
	if (cards.empty()) return false;
	Card& c = cards.front().get();

	auto color = carrier.chooseCardColor("[曼巴] 选择颜色", false);
	if (!color.has_value()) return false;

	std::vector<Card::Name> names;
	names.append_range(Card::numberCardsFrom0);
	names.append_range(Card::actionCards);
	auto name = carrier.chooseCardName("[曼巴] 选择牌名", false, names);
	if (!name.has_value()) return false;

	c.set(color.value(), name.value());
	return true;
}

std::string 曼巴::getPrompt() const {
	return "将一张【8】当作任意颜色的任意牌打出";
}

