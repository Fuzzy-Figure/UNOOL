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

class 寒魄 final : public PassiveSkillImpl<寒魄> {
public:
	寒魄() : PassiveSkillImpl<寒魄>(
		"寒魄",
		"你打出牌后，若你手牌数为1，你可令最后一张手牌变为你打出的牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 雷剑 final : public PassiveSkillImpl<雷剑> {
public:
	雷剑() : PassiveSkillImpl<雷剑>(
		"雷剑",
		"你打出【反转】后，可弃置一张数字牌并回复此牌分值点体力。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 追番 final : public PassiveSkillImpl<追番> {
public:
	追番() : PassiveSkillImpl<追番>(
		"追番",
		"回合开始时，你可以将一张点数≤5的数字牌点数+1~3点。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 崩三 final : public PassiveSkillImpl<崩三> {
	mutable std::size_t count3 = 0; //累计打出的【3】次数
public:
	崩三() : PassiveSkillImpl<崩三>(
		"崩三",
		"你每累计打出两张【3】后，可弃置一张【6】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); count3 = 0; }
};


