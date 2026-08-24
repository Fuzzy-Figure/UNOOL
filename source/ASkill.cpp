#include "../header/ASkill.h"
#include "../header/GameLogic.h"

// ==================== 技能：摸牌 ====================
bool 摸牌::content(GameLogic& game, Player& player) {
	player.draw(1);
	std::cout << "<技能> " << player.characterName() << "发动摸牌，摸了一张牌" << std::endl;
	return true;
}


// ==================== 技能：八爪 ====================
void 八爪::transform(std::vector<ref<Card>> cards) const {
	if (cards.empty()) return;
	Card& c = cards.front().get();
	c.setColor(Card::Color::blue);
	c.setName(Card::Name::number_8);
}
