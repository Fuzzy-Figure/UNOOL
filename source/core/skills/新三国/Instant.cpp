#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"


// ==================== 技能：调羹 ====================
bool 调羹::filter(const GameLogic& game, const Player& carrier) const {
	return carrier.handInclude(&Card::isNumber);
}

bool 调羹::content(GameLogic& game, Player& carrier) {
	carrier.draw(1, DrawReason::skill);
	game.broadcastState();

	auto idxOpt = carrier.chooseCard("选择一张数字牌", &Card::isNumber, false);
	if (!idxOpt) return false;

	auto card = carrier.takeCardByIndex(*idxOpt);
	carrier.showCard(*card);
	std::println("<技能> {}展示{}", carrier.displayName(), *card);

	auto targetOpt = carrier.choosePlayer("【调羹】令一名角色获得此牌", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	std::size_t value = card->value();
	carrier.give(target, std::move(card));
	std::println("<技能> {}将牌交给{}", carrier.displayName(), target.displayName());

	target.recover(value);
	std::println("<技能> {}回复{}点体力", target.displayName(), value);

	game.broadcastState();
	return true;
}


// ==================== 技能：修正 ====================
bool 修正::content(GameLogic& game, Player& carrier) {
	//选一张手牌
	auto idxOpt = carrier.chooseCard("选择一张手牌进行修正", unool::alwaysTrue, false);
	if (!idxOpt) return false;
	Card& card = carrier.getCardByIndex(*idxOpt);

	//构建可选牌名：所有非万能牌名，排除已用过的
	std::vector<Card::Name> availableNames;
	for (int i = static_cast<int>(Card::Name::number_0); i <= static_cast<int>(Card::Name::action_draw2); ++i) {
		Card::Name name = static_cast<Card::Name>(i);
		if (!usedNames.contains(name))
			availableNames.push_back(name);
	}
	if (availableNames.empty()) {
		carrier.hint("所有牌名已用完，无法发动修正");
		return false;
	}

	//声明牌名
	auto nameOpt = carrier.chooseCardName("声明要改为的牌名", false, availableNames);
	if (!nameOpt) return false;
	Card::Name targetName = nameOpt.value();

	//改牌
	card.set(card.getColor(), targetName);
	usedNames.insert(targetName);

	//若改为数字牌，失去3%当前体力（向上取整）
	if (card.isNumber()) {
		std::size_t loss = unool::math::ceil(carrier.getHp() * 0.03);
		carrier.damage(loss, std::nullopt);
		std::println("<技能> {}修正将一张手牌改为数字牌，失去{}点体力", carrier.displayName(), loss);
	}
	else {
		std::println("<技能> {}修正将一张手牌改为功能牌", carrier.displayName());
	}

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

// ==================== 技能：无礼 ====================
bool 无礼_计数::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto opt = carrier.findSkill<无礼>();
	if (!opt.has_value()) return false;
	无礼& skill = opt.value().get();

	const Card::Type type = trigger.getCard().getType();
	if (!skill.usedTypes.contains(type)) {
		skill.usedTypes.insert(type);
		skill.increaseLimit(1);
		std::println("<技能> {}打出了新类型的牌，无礼可用次数+1", carrier.displayName());
		carrier.hint(std::format("【无礼】可用次数+1（当前剩余{}次）",
								 skill.getLimit().value() - skill.getCount()));
		carrier.markCharInfoDirty();
	}
	return true;
}

bool 无礼::filter(const GameLogic& game, const Player& carrier) const {
	//至少有一名角色手牌≥2才能弃置半数
	return carrier.handCount() >= 2 || carrier.next().handCount() >= 2;
}

bool 无礼::content(GameLogic& game, Player& carrier) {
	auto tgtOpt = carrier.choosePlayer("【无礼】选择一名角色，弃置其半数手牌", false);
	if (!tgtOpt.has_value()) return false;
	Player& target = tgtOpt.value().get();

	const std::size_t half = target.handCount() / 2;
	if (half == 0) {
		carrier.hint("该角色手牌不足两张");
		return false;
	}

	//随机弃置半数手牌（向下取整）
	for (std::size_t i = 0; i < half; ++i) {
		const std::size_t idx = unool::random::randomSize_t(0, target.handCount() - 1);
		target.putCardToDiscardPileByIndex(idx, Card::DiscardReason::discard);
	}

	//观看牌堆顶等量张牌并交给其
	auto drawn = target.draw(half, DrawReason::skill);
	for (auto& cardRef : drawn) target.showCard(cardRef.get());

	std::println("<技能> {}发动无礼，弃置{}半数（{}张）手牌并交给其{}张牌",
				 carrier.displayName(), target.displayName(), half, drawn.size());

	game.broadcastState();
	return true;
}

void 无礼::reset() {
	ActiveSkill::reset();
	usedTypes.clear();
}

