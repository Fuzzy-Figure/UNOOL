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

class 招待 final : public InstantSkillImpl<招待> {
public:
	招待() : InstantSkillImpl<招待>(
		"招待",
		"限定技，出牌阶段，你可以将一张牌交给一名其他角色。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};



class 摘罩 final : public InstantSkillImpl<摘罩> {
	std::unordered_set<Card::Name> record;
public:
	摘罩() : InstantSkillImpl<摘罩>(
		"摘罩",
		"出牌阶段，你可以展示未展示过点数的数字牌，令一名其他角色展示相同点数的牌；\n"
		"若其未展示牌，你可任意更改此牌颜色。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};


class 还击 final : public InstantSkillImpl<还击> {
public:
	还击() : InstantSkillImpl<还击>(
		"还击",
		"限定技，出牌阶段，随机获得一名其他角色半数手牌（向上取整，至少保留一张），然后交还给其等量张牌。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//炼石：限定技，出牌阶段，重铸任意张手牌，然后扣减等同于弃置牌总分值的体力
class 炼石 final : public InstantSkillImpl<炼石> {
	friend class 成金;
	bool usedThisTurn = false;
	std::vector<ref<Card>> yellowGained{};  //本回合炼石重铸时获得的黄色牌
public:
	炼石() : InstantSkillImpl<炼石>(
		"炼石",
		"限定技，出牌阶段，你可以重铸任意张手牌（不能通过重铸所有手牌获得胜利），\n"
		"然后你扣减等同于弃置牌总分值的体力。",
		1, unlimited,
		TriggerTime::phase_use,
		成金::make()
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

