#pragma once
// 头文件自足化（第十四批）：显式包含本头直接使用的标准库类型，不依赖 PCH 的传递包含。
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Skill.h"
#include <set>
#include <unordered_set>

//调羹：每局限三次，摸一张，展示数字牌给其他角色并回复其分值体力
class 调羹 final : public InstantSkillImpl<调羹> {
public:
	调羹() : InstantSkillImpl<调羹>(
		"调羹",
		"每局游戏限三次，每回合限一次，出牌阶段，你可以摸一张牌，展示一张数字牌令一名角色获得之，\n"
		"并令其回复此牌分值点体力。",
		3, 1,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//修正：每回合限一次，出牌阶段，将一张手牌改为声明的非万能牌（每局每种牌名限一次）
class 修正 final : public InstantSkillImpl<修正> {
	mutable std::unordered_set<Card::Name> usedNames;
public:
	修正() : InstantSkillImpl<修正>(
		"修正",
		"每回合限{limit}次（剩余{remaining}次），出牌阶段，你可以将一张手牌改为声明的非万能牌（每局游戏每种牌名限一次），\n"
		"若你改为数字牌则失去3%当前体力（向上取整）。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override { ActiveSkill::reset(); usedNames.clear(); }
};

//无礼：每局限{limit}次（每打出一种新类型的牌+1次），弃置一名角色半数手牌（向下取整），
//然后观看牌堆顶等量张牌并交给其
class 无礼 final : public InstantSkillImpl<无礼> {
	friend class 无礼_计数;
	std::unordered_set<Card::Type> usedTypes;  //已打出过的牌类型
public:
	无礼() : InstantSkillImpl<无礼>(
		"无礼",
		"每局游戏限{limit}次（剩余{remaining}次），出牌阶段，你可以弃置一名角色半数手牌（向下取整），\n"
		"然后观看牌堆顶等量张牌并交给其。\n"
		"你每打出一种新类型的牌，此技能本局可用次数+1。",
		0, unlimited,
		TriggerTime::phase_use,
		无礼_计数::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

