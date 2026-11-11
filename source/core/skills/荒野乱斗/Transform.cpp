#include "skills/Skills.h"
#include "GameLogic.h"

// ==================== 切斯特衍生技 ====================
//硬糖
bool 硬糖::canSelect(const Card& c) const {
	return c.isAction();
}
bool 硬糖::transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const {
	if (cards.empty()) return false;
	Card& c = cards.front().get();
	c.set(c.getColor(), Card::Name::action_skip);
	return true;
}
std::string 硬糖::getPrompt() const {
	return "将一张功能牌当作同色【封禁】打出";
}