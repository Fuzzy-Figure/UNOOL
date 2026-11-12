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

class 装币 final : public InstantSkillImpl<装币> {
public:
	装币() : InstantSkillImpl<装币>(
		"装币",
		"限定技，出牌阶段，你可以将手牌摸至十八张，并令你本局造成伤害倍率+1。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 幽愈 final : public InstantSkillImpl<幽愈> {
public:
	幽愈() : InstantSkillImpl<幽愈>(
		"幽愈",
		"出牌阶段，若你有\"幽灵\"标记，你可以移去并回复15%已损体力（向下取整）。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//炫技：出牌阶段，从牌堆顶或底摸两张，将一张置于牌堆底；若两牌同色可弃两张令加速+1
class 炫技 final : public InstantSkillImpl<炫技> {
public:
	炫技() : InstantSkillImpl<炫技>(
		"炫技",
		"每回合限一次，出牌阶段，你可以从牌堆顶或牌堆底摸两张牌，然后将一张手牌置于牌堆底。\n"
		"若你因此获得的牌颜色相同，可弃置两张牌并令【加速】本局可发动次数+1。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//挥金：每种类型限一次，出牌阶段，若手牌数≥3，可将一张黄色手牌交给一名其他角色
class 挥金 final : public InstantSkillImpl<挥金> {
	std::set<Card::Type> usedTypes;
public:
	挥金() : InstantSkillImpl<挥金>(
		"挥金",
		"每种类型限一次，出牌阶段，若你手牌数≥3，你可以将一张黄色手牌交给一名其他角色。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

//再生：限定技，出牌阶段，弃置所有绿色牌回复至50点；若回复≤25则次数改为2
class 再生 final : public InstantSkillImpl<再生> {
public:
	再生() : InstantSkillImpl<再生>(
		"再生",
		"限定技，出牌阶段，你可以弃置手中所有绿色牌将生命值回复至50点；\n"
		"若你因此回复生命不超过25，此技能本局可发动次数改为2。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//手枪：每回合限一次，出牌阶段，对一名其他角色造成1点伤害（伤害值由【弹暴】递增）
class 手枪 final : public InstantSkillImpl<手枪> {
private:
	std::size_t damageValue = 1;
	friend class 弹暴;
public:
	手枪() : InstantSkillImpl<手枪>(
		"手枪",
		"每回合限一次，出牌阶段，你可以对一名其他角色造成1点伤害。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

class 引力 final : public InstantSkillImpl<引力> {
private:
	opt_ref<Player> target = std::nullopt;
public:
	引力() : InstantSkillImpl<引力>(
		"引力",
		"每局限{limit}次（剩余{remaining}次），每回合限1次，出牌阶段，你可以选择一名角色，本轮数字牌进入弃牌堆后，该角色获得之。",
		0, 1,
		TriggerTime::phase_use,
		引力_清除目标::make()
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};



//甘草：每回合限一次，观看一名角色所有非数字牌，可弃置其中一张
class 甘草 final : public InstantSkillImpl<甘草> {
public:
	甘草() : InstantSkillImpl<甘草>(
		"甘草",
		"每回合限一次，出牌阶段，你可以观看一名角色所有非数字牌，并可弃置其中一张。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//跳糖：每回合限一次，与一名角色拼点，胜者获得一张切斯特指定颜色的【+2】
class 跳糖 final : public InstantSkillImpl<跳糖> {
public:
	跳糖() : InstantSkillImpl<跳糖>(
		"跳糖",
		"每回合限一次，出牌阶段，你可以与一名其他角色拼点，胜者获得一张你指定颜色的【+2】。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//肘击：限定技，出牌阶段，摸两张牌并获得两张【变色】
class 肘击 final : public InstantSkillImpl<肘击> {
public:
	肘击() : InstantSkillImpl<肘击>(
		"肘击",
		"限定技，出牌阶段，你可以摸两张牌并获得两张【变色】。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};


class 突袭 final : public InstantSkillImpl<突袭> {
public:
	突袭() : InstantSkillImpl<突袭>(
		"突袭",
		"限定技，出牌阶段，你可以令一名其他角色的\"毒\"标记数量翻倍。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};


class 猛击 final : public InstantSkillImpl<猛击> {
public:
	猛击() : InstantSkillImpl<猛击>(
		"猛击",
		"限定技，出牌阶段，你可以与一名其他角色各回复20%的已损体力（向下取整），"
		"因此回复体力较少的角色被封印十轮。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};


class 绝技 final : public InstantSkillImpl<绝技> {
public:
	绝技() : InstantSkillImpl<绝技>(
		"绝技",
		"每局限{limit}次（剩余{remaining}次），出牌阶段，\n"
		"第1次：随机获得一张【封禁】；\n"
		"第2次：随机获得一张【+2】，且以后你出【+2】时目标额外摸一张牌；\n"
		"第3次：随机获得一张万能牌；\n"
		"第4次及以上：将一张手牌变为随机颜色的随机非数字牌，并回复发动次数点体力。",
		0, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 过载 final : public InstantSkillImpl<过载> {
public:
	过载() : InstantSkillImpl<过载>(
		"过载",
		"限定技，出牌阶段，你可以增加5个\"速度\"标记。",
		1, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//飞手：每回合限一次，出牌阶段，若上轮未发动过，可获得上/下家随机一张牌（其至少保留一张），然后交还一张
class 飞手 final : public InstantSkillImpl<飞手> {
	friend class 飞手_整轮;
	bool usedThisRound = false;  //本轮是否发动过
	bool usedLastRound = false;  //上一整轮是否发动过
public:
	飞手() : InstantSkillImpl<飞手>(
		"飞手",
		"每回合限一次，出牌阶段，若你上轮未发动过此技能，\n"
		"你可以获得上/下家随机一张牌（其至少保留一张牌），然后交还一张。",
		unlimited, 1,
		TriggerTime::phase_use,
		飞手_整轮::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

//筑丛：限定技，出牌阶段，摸牌至摸到绿色牌并回复X点体力；每累计打出十张非绿色牌后视为未发动过
class 筑丛 final : public InstantSkillImpl<筑丛> {
	friend class 筑丛_累计;
	bool accumulating = false;     //发动后是否正在累计非绿色牌
	std::size_t nonGreenCount = 0; //当前已打出非绿色牌数
public:
	筑丛() : InstantSkillImpl<筑丛>(
		"筑丛",
		"限定技，出牌阶段，你可以摸牌至摸到绿色牌，然后回复X点体力（X为摸牌数）。\n"
		"你每累计打出十张非绿色牌后，此技能视为未发动过。\n"
		"当前已打出非绿色牌数：{nongreen}",
		1, unlimited,
		TriggerTime::phase_use,
		筑丛_摸牌::make(),
		筑丛_累计::make()
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
	bool isAccumulating() const { return accumulating; }
	void setAccumulating(bool v) { accumulating = v; }
	std::size_t getNonGreenCount() const { return nonGreenCount; }
	void setNonGreenCount(std::size_t v) { nonGreenCount = v; }
	std::optional<std::string> extraPlaceholders(const std::string& key) const override;
};

