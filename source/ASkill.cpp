#include "../header/ASkill.h"
#include "../header/GameLogic.h"
#include "../header/utils.h"


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
bool 徒步::content(GameLogic& game, Player& carrier) {
	Player::RecastResult result = carrier.chooseToRecast(L"[徒步] 重铸一张牌", 1, false);
	if (result.discarded.size() == 0) return false;

	carrier.recover(1);
	if (result.discarded.front().get().isNumber()) {
		carrier.damage(getCount(), carrier);
	}
	return true;
}


// ==================== 技能：招待 ====================
bool 招待::content(GameLogic& game, Player& carrier) {
	std::optional targetOpt = carrier.chooseOtherPlayer(L"[招待] 选择一名其他角色", true);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	carrier.chooseToGive(L"选择一张手牌交给" + target.characterNameW(), target, true);
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
	return c.is(Card::Name::number_8);
}

bool 曼巴::transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const {
	if (cards.empty()) return false;
	Card& c = cards.front().get();

	auto color = carrier.chooseCardColor(L"[曼巴] 选择颜色", false);
	if (!color.has_value()) return false;

	std::vector<Card::Name> names;
	names.append_range(Card::numberCardsFrom0);
	names.append_range(Card::actionCards);
	auto name = carrier.chooseCardName(L"[曼巴] 选择牌名", false, names);
	if (!name.has_value()) return false;

	c.set(color.value(), name.value());
	return true;
}

std::wstring 曼巴::getPrompt() const {
	return L"将一张万能牌或【8】当作任意颜色的任意牌打出";
}

bool 摘罩::content(GameLogic& game, Player& carrier) {
	std::wstring recordStr;
	for (const Card::Name& name : record) {
		recordStr += Card::to_wstring(name) + L',';
	}
	//1. 展示一张未展示过点数的数字牌
	auto cardOpt = carrier.chooseToShow(L"[摘罩] 展示一张数字牌\n已展示：" + recordStr, false, [this](const Card& c) {
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


// ==================== 技能：还击 ====================
bool 还击::content(GameLogic& game, Player& carrier) {
	//1. 选一名其他角色
	auto targetOpt = carrier.chooseOtherPlayer(L"[还击] 选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	const std::size_t n = target.handCount();
	if (n == 0) {
		std::cout << "<还击> 目标无手牌" << std::endl;
		return false;
	}

	//2. 取半数手牌（向上取整，至少保留一张）
	std::size_t takeCount = unool::math::ceil(static_cast<double>(n) * 0.5);
	if (takeCount >= n) takeCount = n - 1;
	if (takeCount == 0) return false;

	//3. 随机拿牌
	for (std::size_t i = 0; i < takeCount; ++i) {
		std::size_t idx = unool::random::randomSize_t(0, target.handCount() - 1);
		carrier.gainCard(target.takeCardByIndex(idx));
	}
	game.broadcastState();

	//4. 交还等量张牌（forced=true，强制完成义务）
	for (std::size_t i = 0; i < takeCount; ++i) {
		carrier.chooseToGive(
			L"[还击] 交还一张牌给" + target.characterNameW()
			+ L"（" + std::to_wstring(i + 1) + L"/" + std::to_wstring(takeCount) + L"）",
			target, true
		);
	}
	game.broadcastState();
	return true;
}

