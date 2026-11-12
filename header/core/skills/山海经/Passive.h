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

class 棍击 final : public PassiveSkillImpl<棍击> {
public:
	棍击() : PassiveSkillImpl<棍击>(
		"棍击",
		"每局限9次，当你打出万能牌后，你可以对一名其他角色造成2^X点伤害（X为本局此技能发动次数）。",
		9, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 神木 final : public PassiveSkillImpl<神木> {
public:
	神木() : PassiveSkillImpl<神木>(
		"神木",
		"锁定技，游戏开始时，从游戏外将九张【变色】和九张【+4】加入牌堆。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 耐克 final : public PassiveSkillImpl<耐克> {
public:
	耐克() : PassiveSkillImpl<耐克>(
		"耐克",
		"锁定技，若上一张牌为蓝色或万能牌，【封禁】、【+2】和【+4】对你无效",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 轰炸 final : public PassiveSkillImpl<轰炸> {
public:
	轰炸() : PassiveSkillImpl<轰炸>(
		"轰炸",
		"锁定技，当你打出【+4】/【+2】/【封禁】时，目标失去4%/2%/1%最大体力（向上取整）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 爆破 final : public PassiveSkillImpl<爆破> {
public:
	爆破() : PassiveSkillImpl<爆破>(
		"爆破",
		"你对其他角色造成封禁效果后，可随机获得其一张牌并对其造成此牌分值点伤害。",
		unlimited, false,
		TriggerPlayer::others,
		TriggerTime::ban_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 森罗 final : public PassiveSkillImpl<森罗> {
public:
	森罗() : PassiveSkillImpl<森罗>(
		"森罗",
		"锁定技，游戏开始时，将手中所有非黑色牌变为绿色。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 大脚 final : public PassiveSkillImpl<大脚> {
public:
	大脚() : PassiveSkillImpl<大脚>(
		"大脚",
		"回合开始时，可弃置一张万能牌并发动一次【森罗】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 黑帮 final : public PassiveSkillImpl<黑帮> {
public:
	黑帮() : PassiveSkillImpl<黑帮>(
		"黑帮",
		"锁定技，游戏开始时，你随机获得X张万能牌（X为当前局数）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};


class 拖拉 final : public PassiveSkillImpl<拖拉> {
public:
	拖拉() : PassiveSkillImpl<拖拉>(
		"拖拉",
		"锁定技，其他角色打出万能牌后，你弃置手中所有此牌名的牌并回复弃牌数点体力。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

