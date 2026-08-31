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
std::wstring 八爪::getPrompt() const {
	return L"将一张数字牌当作蓝8打出";
}


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
	carrier.draw(1, Player::DrawReason::skill);
	game.broadcastState();
}

std::wstring 我妈::getPrompt() const {
	return L"将一张非红色牌当作红【封禁】打出";
}


// ==================== 技能：曼巴 ====================
std::size_t 曼巴::getCardCount() const {
	return 1;
}

bool 曼巴::canSelect(const Card& c) const {
	return c.isWild() || c.is(Card::Name::number_8);
}

bool 曼巴::transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const {
	if (cards.empty()) return false;
	Card& c = cards.front().get();

	auto color = carrier.chooseCardColor(L"[曼巴] 选择颜色", false);
	if (!color.has_value()) return false;

	std::vector<Card::Name> allNames(Card::allCards.begin(), Card::allCards.end());
	auto name = carrier.chooseCardName(L"[曼巴] 选择牌名", false, allNames);
	if (!name.has_value()) return false;

	c.set(color.value(), name.value());
	return true;
}

std::wstring 曼巴::getPrompt() const {
	return L"将一张万能牌或【8】当作任意颜色的任意牌打出";
}

bool 摘罩::content(GameLogic& game, Player& carrier) {
	//1. 展示一张未展示过点数的数字牌
	auto cardOpt = carrier.chooseToShow(L"[摘罩] 展示一张数字牌", false, [this](const Card& c) {
		return c.isNumber() && !record.contains(c.getName());
	});
	if (!cardOpt.has_value()) return false;
	Card& card = cardOpt.value().get();
	Card::Name point = card.getName();

	//2. 选一名其他角色，令其展示相同点数的牌
	auto targetOpt = carrier.chooseOtherPlayer(L"[摘罩] 选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();
	record.insert(point);  //技能确认发动，记录已展示点数

	//3. 检查目标是否有相同点数的牌，有则令其展示
	bool targetShowed = false;
	for (std::size_t i = 0; i < target.handCount(); ++i) {
		if (target.getCardByIndex(i).getName() == point) {
			targetShowed = true;
			break;
		}
	}
	if (targetShowed) {
		auto otherCardOpt = target.chooseToShow(
			L"[摘罩] 展示一张" + Card::to_wstring(point), false,
			[point](const Card& c) { return c.getName() == point; }
		);
		targetShowed = otherCardOpt.has_value();
	}

	//4. 若目标未展示牌，carrier 可任意更改所展示牌的颜色
	if (!targetShowed) {
		auto color = carrier.chooseCardColor(L"[摘罩] 更改你展示的牌的颜色", false);
		if (color.has_value()) {
			card.setColor(color.value());
			game.broadcastState();
		}
	}
	return true;
}

void 摘罩::reset() {
	Skill::reset();
	record.clear();
}
