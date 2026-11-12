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

class 徒步 final : public InstantSkillImpl<徒步> {
public:
	徒步() : InstantSkillImpl<徒步>(
		"徒步",
		"出牌阶段，你可以重铸一张牌并回复1点体力，若你重铸了数字牌，失去X点体力（X为此技能本局发动次数）。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};


class 舞爪 final : public InstantSkillImpl<舞爪> {
public:
	舞爪() : InstantSkillImpl<舞爪>(
		"舞爪",
		"每回合限一次，出牌阶段，你可以弃置一张万能牌，重铸手中所有数字牌。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 四霸 final : public InstantSkillImpl<四霸> {
public:
	四霸() : InstantSkillImpl<四霸>(
		"四霸",
		"出牌阶段，你可以弃置三张【4】。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//芜湖：限定技，出牌阶段，声明颜色+牌名组合，从牌堆获得一张匹配牌；无匹配则视为未发动且可弃一张
class 芜湖 final : public InstantSkillImpl<芜湖> {
public:
	芜湖() : InstantSkillImpl<芜湖>(
		"芜湖",
		"限定技，出牌阶段，你可以声明一个牌名和颜色的组合（万能牌为黑色），\n"
		"若牌堆中已无此牌名和颜色组合的牌，此技能视为未发动过且你可弃置一张牌；\n"
		"若有，你获得一张。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//坠机：限定技，出牌阶段，展示自己与一名其他角色指定牌名的牌并依次结算
class 坠机 final : public InstantSkillImpl<坠机> {
public:
	坠机() : InstantSkillImpl<坠机>(
		"坠机",
		"限定技，出牌阶段，你可以展示自己与一名其他角色指定牌名的牌，然后依次结算：\n"
		"若其未因此展示牌，其随机获得一张数字牌且此技能视为未发动过；\n"
		"若你未展示牌，你弃置所有手牌并摸八张牌；\n"
		"若均展示了牌，你可令一名角色获得这两张牌。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

