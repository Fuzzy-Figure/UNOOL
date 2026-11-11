#include "skills/Skills.h"
#include "GameLogic.h"

// ==================== 技能：八爪 ====================
std::size_t 八爪::getCardCount() const {
	return 1;
}
bool 八爪::canSelect(const Card& c) const {
	return c.isNumber();
}
bool 八爪::transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const {
	if (cards.empty()) return false;
	Card& c = cards.front().get();
	c.set(Card::Color::blue, Card::Name::number_8);
	return true;
}
std::string 八爪::getPrompt() const {
	return "将一张数字牌当作蓝8打出";
}


