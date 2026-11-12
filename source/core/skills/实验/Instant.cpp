#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"


// ==================== 技能：招待 ====================
bool 招待::content(GameLogic& game, Player& carrier) {
	std::optional targetOpt = carrier.chooseOtherPlayer("[招待] 选择一名其他角色", true);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	carrier.chooseToGive(std::format("选择一张手牌交给{}", target.characterName()), target, true);
	return true;
}




bool 摘罩::content(GameLogic& game, Player& carrier) {
	std::string recordStr;
	for (const Card::Name& name : record) {
		recordStr += std::format("{},", name);
	}
	//1. 展示一张未展示过点数的数字牌
	auto cardOpt = carrier.chooseToShow(std::format("[摘罩] 展示一张数字牌\n已展示：{}", recordStr), false, [this](const Card& c) {
		return c.isNumber() && !record.contains(c.getName());
	});
	if (!cardOpt.has_value()) return false;
	Card& card = cardOpt.value().get();
	Card::Name point = card.getName();

	//2. 选一名其他角色，令其展示相同点数的牌
	auto targetOpt = carrier.chooseOtherPlayer("[摘罩] 选择一名其他角色", false);
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
			std::format("[摘罩] 展示一张{}", point), false,
			[point](const Card& c) { return c.getName() == point; }
		);
		targetShowed = otherCardOpt.has_value();
	}

	//4. 若目标未展示牌，carrier 可任意更改所展示牌的颜色
	if (!targetShowed) {
		auto color = carrier.chooseCardColor("[摘罩] 更改你展示的牌的颜色", false);
		if (color.has_value()) {
			card.setColor(color.value());
			game.broadcastState();
		}
	}
	return true;
}

void 摘罩::reset() {
	ActiveSkill::reset();
	record.clear();
}


// ==================== 技能：还击 ====================
bool 还击::content(GameLogic& game, Player& carrier) {
	//1. 选一名其他角色
	auto targetOpt = carrier.chooseOtherPlayer("[还击] 选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	const std::size_t n = target.handCount();
	if (n == 0) {
		std::println("<还击> 目标无手牌");
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
			std::format("[还击] 交还一张牌给{}（{}/{}）", target.characterName(), i + 1, takeCount),
			target, true
		);
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：炼石 ====================
bool 炼石::content(GameLogic& game, Player& carrier) {
	//多次选一张，取消或选完所有手牌后结束选择
	std::vector<std::size_t> selected;
	while (selected.size() < carrier.handCount()) {
		auto idxOpt = carrier.chooseCard(
			"【炼石】选择一张要重铸的手牌（取消结束选择）",
			[&](const Card& c) {
				for (const std::size_t s : selected)
					if (&carrier.getCardByIndex(s) == &c) return false;
				return true;
			}, false);
		if (!idxOpt.has_value()) break;
		selected.push_back(*idxOpt);
	}
	if (selected.empty()) {
		carrier.hint("未选择任何牌");
		return false;
	}

	yellowGained.clear();
	usedThisTurn = true;

	//降序重铸，统计弃置牌总分值，并记录重铸获得的黄色牌
	std::size_t totalValue = 0;
	for (auto it = selected.rbegin(); it != selected.rend(); ++it) {
		Card& discarded = carrier.getCardByIndex(*it);
		totalValue += discarded.value();
		carrier.recastByIndex(*it);
		if (!carrier.handEmpty() && carrier.getHand().back().is(Card::Color::yellow)) {
			yellowGained.push_back(std::ref(carrier.getHand().back()));
		}
	}

	//重铸光所有手牌时，禁止因手牌为空获胜
	if (carrier.handEmpty()) carrier.setNoWinByEmptyHand(true);

	//扣减等同于弃置牌总分值的体力
	carrier.loseHp(totalValue);
	std::println("<技能> {}发动炼石，重铸{}张牌，扣减{}点体力",
				 carrier.displayName(), selected.size(), totalValue);

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

void 炼石::reset() {
	ActiveSkill::reset();
	usedThisTurn = false;
	yellowGained.clear();
}


