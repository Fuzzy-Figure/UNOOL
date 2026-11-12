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

//炎烈：每回合限一次，出牌阶段，随机获得两张不同牌名的红色牌，然后可以弃置其中一张
class 炎烈 final : public InstantSkillImpl<炎烈> {
public:
	炎烈() : InstantSkillImpl<炎烈>(
		"炎烈",
		"每回合限一次，出牌阶段，你可以随机获得两张不同牌名的红色牌，然后可以弃置其中一张。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//勇进：每回合限一次，出牌阶段，失去1点体力并摸一张牌，若为非数字牌，弃置一张其他牌并回复2点体力
class 勇进 final : public InstantSkillImpl<勇进> {
public:
	勇进() : InstantSkillImpl<勇进>(
		"勇进",
		"每回合限一次，出牌阶段，你可以失去1点体力并摸一张牌，"
		"若为非数字牌，弃置一张其他牌并回复2点体力。",
		unlimited,1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//净身：出牌阶段，若你手中有五种颜色的牌，你可以弃置一张牌并回复1点体力
class 净身 final : public InstantSkillImpl<净身> {
public:
	净身() : InstantSkillImpl<净身>(
		"净身",
		"出牌阶段，若你手中有五种颜色的牌，你可以弃置一张牌并回复1点体力。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//星矢：每回合限一次，出牌阶段，失去5点体力并重铸一种颜色的所有牌；
//若未因此获得同色牌，本局伤害倍率+0.1并获得10%吸血（至多50%）
class 星矢 final : public InstantSkillImpl<星矢> {
public:
	星矢() : InstantSkillImpl<星矢>(
		"星矢",
		"每回合限一次，出牌阶段，你可以失去5点体力并重铸一种颜色的所有牌；\n"
		"若未因此获得同色牌，你本局造成伤害倍率+0.1并获得10%吸血（至多50%）。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//奔突：每回合限一次，出牌阶段，令一名其他角色摸一张牌并展示，然后随机弃置一张牌
class 奔突 final : public InstantSkillImpl<奔突> {
public:
	奔突() : InstantSkillImpl<奔突>(
		"奔突",
		"每回合限一次，出牌阶段，选择一名其他角色，令其摸一张牌并展示，然后其随机弃置一张牌。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

