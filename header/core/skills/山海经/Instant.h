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

class 装弹 final : public InstantSkillImpl<装弹> {
public:
	装弹() : InstantSkillImpl<装弹>(
		"装弹",
		"限定技，出牌阶段，若你手中没有封禁类牌且手牌数≤9，\n"
		"你可以将一半手牌变为随机的非数字牌（向下取整）。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};
