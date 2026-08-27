#include "../header/ASkill.h"
#include "../header/GameLogic.h"


bool 徒步::content(GameLogic& game, Player& carrier) {
	Player::RecastResult result = carrier.chooseToRecast(L"[徒步] 重铸一张牌", 1, false);
	if (result.discarded.size() == 0) return false;

	carrier.recover(1);
	if (result.discarded.front().get().isNumber()) {
		carrier.damage(getCount(), carrier);
	}
	return true;
}

// ==================== 技能：八爪 ====================
void 八爪::transform(std::vector<ref<Card>> cards) const {
	if (cards.empty()) return;
	Card& c = cards.front().get();
	c.setColor(Card::Color::blue);
	c.setName(Card::Name::number_8);
}

