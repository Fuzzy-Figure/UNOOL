#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
#include <numeric>
#include <optional>

// ==================== 技能：同心 ====================
bool 同心::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//根据打出牌的类型增加对应计数
	const Card::Type type = trigger.getCard().getType();
	switch (type) {
		case Card::Type::number: ++numberCount, actionCount = wildCount = 0; break;
		case Card::Type::action: ++actionCount, numberCount = wildCount = 0; break;
		case Card::Type::wild:   ++wildCount, numberCount = actionCount = 0;  break;
		default:                 std::unreachable();
	}

	//达到2张同类型时随机弃置一张牌并重置计数
	if (numberCount >= 2 || actionCount >= 2 || wildCount >= 2) {
		numberCount = actionCount = wildCount = 0;

		const std::size_t handCount = carrier.getHand().count();
		if (handCount == 0) return true;
		const std::size_t idx = unool::random::randomSize_t(0, handCount - 1);
		std::println("<技能> {}打出两张同类型牌，随机弃置：{}", carrier.displayName(),
					 carrier.getHand().getCardByIndex(idx));
		carrier.discardByIndex(idx);
		game.broadcastState();
	}
	carrier.markCharInfoDirty();
	return true;
}

std::optional<std::string> 同心::extraPlaceholders(const std::string& key) const {
	if (key == "numberCount") return std::to_string(numberCount);
	else if (key == "actionCount") return std::to_string(actionCount);
	else if (key == "wildCount") return std::to_string(wildCount);
	return std::nullopt;
}

// ==================== 技能：蝎刺 ====================
bool 蝎刺::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isTargeted();
}

bool 蝎刺::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = carrier.next();
	target.damage(damageValue, carrier);
	std::println("<技能> {}发动蝎刺，对{}造成{}点伤害", carrier.displayName(),
				 target.displayName(), damageValue);
	++damageValue;
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

std::optional<std::string> 蝎刺::extraPlaceholders(const std::string& key) const {
	if (key == "damage") return std::to_string(damageValue);
	return std::nullopt;
}

// ==================== 技能：镇魂 ====================
bool 镇魂::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return phaseCount < 1;
}

bool 镇魂::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto targetOpt = carrier.choosePlayer("【镇魂】选择一名角色：", true);
	if (!targetOpt) return false;
	Player& target = targetOpt->get();

	//随机造成1-5点伤害
	const std::size_t dmg = unool::random::randomSize_t(1, 5);
	std::println("<技能> {}发动镇魂，对{}随机造成{}点伤害", carrier.displayName(),
				 target.displayName(), dmg);
	target.damage(dmg, carrier);

	//若你手牌数不小于其，其弃置一张牌
	if (carrier.handCount() >= target.handCount() && target.handCount() > 0) {
		std::println("<技能> {}手牌数不小于{}，{}弃置一张牌", carrier.displayName(),
					 target.displayName(), target.displayName());
		target.chooseToDiscard("【镇魂】弃置一张牌：", 1, true);
	}

	++phaseCount;
	game.broadcastState();
	return true;
}

bool 镇魂_回合结束::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.getSkill<镇魂>().phaseCount < 1;
}

bool 镇魂_回合结束::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	return carrier.getSkill<镇魂>().content(game, carrier, trigger);
}

// ==================== 技能：坚甲 ====================
bool 坚甲::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//每打出一张牌后获得1点护甲，上限50点
	const std::size_t cur = carrier.getShield();
	if (cur < 50) {
		carrier.setShield(cur + 1);
		carrier.markCharInfoDirty();
	}
	return true;
}


