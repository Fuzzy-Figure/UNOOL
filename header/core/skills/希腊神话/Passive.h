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

class 同心 final : public PassiveSkillImpl<同心> {
private:
	std::size_t numberCount = 0;
	std::size_t actionCount = 0;
	std::size_t wildCount = 0;
public:
	同心() : PassiveSkillImpl<同心>(
		"同心",
		"锁定技，每连续打出两张同类型牌后随机弃置一张牌。\n"
		"当前：数字{numberCount}/2；功能{actionCount}/2；万能{wildCount}/2",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<同心>::reset();
		numberCount = 0;
		actionCount = 0;
		wildCount = 0;
	}
	std::optional<std::string> extraPlaceholders(const std::string& key) const;
};

//蝎刺：锁定技，你打出有目标的牌后，对目标造成伤害。每次发动后本场游戏此技能造成伤害+1。
class 蝎刺 final : public PassiveSkillImpl<蝎刺> {
private:
	std::size_t damageValue = 1;
public:
	蝎刺() : PassiveSkillImpl<蝎刺>(
		"蝎刺",
		"锁定技，你打出有目标的牌后，对目标造成{damage}点伤害。\n"
		"每次发动此技能后，本场游戏此技能造成伤害+1。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	std::optional<std::string> extraPlaceholders(const std::string& key) const;
};

class 镇魂_回合结束 final : public PassiveSkillImpl<镇魂_回合结束> {
public:
	镇魂_回合结束() : PassiveSkillImpl<镇魂_回合结束>(
		"镇魂",
		"",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 镇魂 final : public PassiveSkillImpl<镇魂> {
	friend class 镇魂_回合结束;
private:
	std::size_t phaseCount = 0;
public:
	镇魂() : PassiveSkillImpl<镇魂>(
		"镇魂",
		"每回合限一次，回合开始或结束时，你可以选择一名角色，对其随机造成1-5点伤害，"
		"然后若你手牌数不小于其，其弃置一张牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin,
		镇魂_回合结束::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//坚甲：锁定技，每打出一张牌后获得1点护甲（上限50点）
class 坚甲 final : public PassiveSkillImpl<坚甲> {
public:
	坚甲() : PassiveSkillImpl<坚甲>(
		"坚甲",
		"锁定技，你每打出一张牌后获得1点护甲（上限50点）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

