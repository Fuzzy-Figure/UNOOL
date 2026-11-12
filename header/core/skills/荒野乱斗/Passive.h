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

//爆射：锁定技，打出牌后，若手牌数恰为某其他角色两倍，弃置至多两张牌并对一名其他角色造成6点伤害
class 爆射 final : public PassiveSkillImpl<爆射> {
public:
	爆射() : PassiveSkillImpl<爆射>(
		"爆射",
		"锁定技，你打出牌后，若你手牌数恰为一名其他角色的两倍，你弃置至多两张牌并对一名其他角色造成6点伤害。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//灵爆_子：打出有目标的牌时，若有幽灵标记则移去并对目标造成10点伤害
class 灵爆_子 final : public PassiveSkillImpl<灵爆_子> {
public:
	灵爆_子() : PassiveSkillImpl<灵爆_子>(
		"灵爆_子", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//灵爆：累计打出三张牌后获得"幽灵"标记（限一个）；拥有标记期间不计数，标记移去后重新累计
class 灵爆 final : public PassiveSkillImpl<灵爆> {
private:
	std::shared_ptr<std::size_t> playCount = std::make_shared<std::size_t>(0);
public:
	灵爆() : 灵爆(std::make_shared<std::size_t>(0)) {}
	灵爆(std::shared_ptr<std::size_t> _playCount)
		: PassiveSkillImpl<灵爆>(
			"灵爆",
			"你累计打出三张牌后，获得\"幽灵\"标记（限一个）。\n"
			"你打出有目标的牌时，若你有\"幽灵\"标记，则移去并对目标造成10点伤害。",
			unlimited, true,
			TriggerPlayer::self,
			TriggerTime::use_card_end,
			灵爆_子::make()
		), playCount(std::move(_playCount)) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};

//加速：每局游戏限零次，成为牌的目标时可发动一次炫技并令此牌无效
class 加速 final : public PassiveSkillImpl<加速> {
public:
	加速() : PassiveSkillImpl<加速>(
		"加速",
		"每局游戏限{limit}次（剩余{remaining}次），你成为牌的目标时，可以发动一次【炫技】并令此牌无效。",
		0, false,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};

//返现：限定技，结束阶段，重铸所有非黄色手牌并回复等量体力
class 返现 final : public PassiveSkillImpl<返现> {
public:
	返现() : PassiveSkillImpl<返现>(
		"返现",
		"限定技，结束阶段，你可以重铸所有非黄色手牌，并回复重铸牌数点体力。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//尖刺：锁定技，受到伤害后，对一名其他角色造成等量伤害并回复1点体力
class 尖刺 final : public PassiveSkillImpl<尖刺> {
public:
	尖刺() : PassiveSkillImpl<尖刺>(
		"尖刺",
		"锁定技，你受到伤害后，若体力>0，对一名其他角色造成等量伤害并回复1点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::damage_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//弹暴：回合结束时，可弃一张万能牌发动X次手枪（X为本局发动次数），然后手枪伤害+1
class 弹暴 final : public PassiveSkillImpl<弹暴> {
public:
	弹暴() : PassiveSkillImpl<弹暴>(
		"弹暴",
		"回合结束时，你可以弃置一张万能牌，发动X次【手枪】，然后【手枪】本局伤害值+1\n"
		"（X为此技能本局发动次数）。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//星轨：回合开始时，三次判定类型全相同则引力+1
class 星轨 final : public PassiveSkillImpl<星轨> {
public:
	星轨() : PassiveSkillImpl<星轨>(
		"星轨",
		"回合结束时，你可以进行三次判定，若类型均相同，【引力】可用次数+1。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//引力_目标：引力重定向拦截器，数字牌进弃牌堆后给自身获得
class 引力_目标 final : public PassiveSkillImpl<引力_目标> {
public:
	引力_目标() : PassiveSkillImpl<引力_目标>(
		"引力_目标",
		"数字牌进入弃牌堆后，你获得之。",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::card_discard_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//引力_清除目标：轮开始时清除所有玩家身上的引力_目标
class 引力_清除目标 final : public PassiveSkillImpl<引力_清除目标> {
public:
	引力_清除目标() : PassiveSkillImpl<引力_清除目标>(
		"引力_清除目标",
		"每轮开始时，清除所有角色的“引力_目标”。",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::round_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 铃铛 final : public PassiveSkillImpl<铃铛> {
public:
	铃铛() : PassiveSkillImpl<铃铛>(
		"铃铛",
		"锁定技，回合开始时，若你手牌数不为1，随机重铸X张手牌（X为此技能发动次数，若为3则重置为1），\n"
		"若因此重铸了三张手牌，你失去其他技能，并随机获得一个衍生技。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//爆糖：锁定技，打出有目标的牌后，对目标造成5%最大体力伤害（向上取整）
class 爆糖 final : public PassiveSkillImpl<爆糖> {
public:
	爆糖() : PassiveSkillImpl<爆糖>(
		"爆糖",
		"锁定技，当你打出有目标的牌后，对目标造成5%最大体力点伤害（向上取整）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//薄荷：锁定技，回合结束时，随机回复1~3体力
class 薄荷 final : public PassiveSkillImpl<薄荷> {
public:
	薄荷() : PassiveSkillImpl<薄荷>(
		"薄荷",
		"锁定技，回合结束时，随机回复1~3体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//健体：锁定技，打出手中非唯一牌名的牌后，回复1点体力
class 健体 final : public PassiveSkillImpl<健体> {
public:
	健体() : PassiveSkillImpl<健体>(
		"健体",
		"锁定技，打出手中非唯一牌名的牌后，回复1点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//飞刃：当你打出万能牌后，你可令一名其他角色获得一个"毒"标记
class 飞刃 final : public PassiveSkillImpl<飞刃> {
public:
	飞刃() : PassiveSkillImpl<飞刃>(
		"飞刃",
		"当你打出万能牌后，你可令一名其他角色获得一个\"毒\"标记。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//前向声明，供子技能通过 findSkill 访问
class 淬毒;

//淬毒_感毒：子技能，监听"毒"标记增加，重置淬毒的衰减计时
class 淬毒_感毒 final : public PassiveSkillImpl<淬毒_感毒> {
public:
	淬毒_感毒() : PassiveSkillImpl<淬毒_感毒>(
		"淬毒_感毒", "",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::add_mark
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//淬毒_衰减：子技能，回合结束时对满五回合未叠加的毒标记减一
class 淬毒_衰减 final : public PassiveSkillImpl<淬毒_衰减> {
public:
	淬毒_衰减() : PassiveSkillImpl<淬毒_衰减>(
		"淬毒_衰减", "",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::phase_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//淬毒：锁定技，有"毒"标记角色的回合结束时，其受到标记数量点伤害；每过10回合，若标记未叠加则减少一个
class 淬毒 final : public PassiveSkillImpl<淬毒> {
	mutable std::size_t tick = 0;
	mutable std::unordered_map<std::size_t, std::size_t> lastChangeTick;
public:
	淬毒() : PassiveSkillImpl<淬毒>(
		"淬毒",
		"锁定技，有\"毒\"标记角色的回合结束时，其受到标记数量点伤害；\n"
		"每过10回合，若标记未叠加则减少一个。",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::phase_end,
		淬毒_感毒::make(),
		淬毒_衰减::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void resetPoisonTimer(std::size_t playerId) { lastChangeTick[playerId] = tick; }
	void tryDecay(GameLogic& game);
	void reset() override;
};

//重锤：锁定技，回合结束时，若体力值>100，失去10%当前体力（向下取整），对一名其他角色造成3%已损体力点伤害（向上取整）
class 重锤 final : public PassiveSkillImpl<重锤> {
public:
	重锤() : PassiveSkillImpl<重锤>(
		"重锤",
		"锁定技，回合结束时，若你体力值>100，你失去10%当前体力（向下取整），"
		"对一名其他角色造成3%你已损体力点伤害（向上取整）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//劲凉：锁定技，每连续打出3张同类型牌后，摸一张牌且【绝技】使用次数+1
class 劲凉 final : public PassiveSkillImpl<劲凉> {
	mutable Card::Type lastType = Card::Type::unknown;
	mutable std::size_t consecutive = 0;
public:
	劲凉() : PassiveSkillImpl<劲凉>(
		"劲凉",
		"锁定技，每连续打出3张同类型牌后，你摸一张牌且【绝技】使用次数+1。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};

//绝技_额外摸牌：被动，瑟奇打出+2时目标额外摸一张牌（绝技第2次发动后获得）
class 绝技_额外摸牌 final : public PassiveSkillImpl<绝技_额外摸牌> {
public:
	绝技_额外摸牌() : PassiveSkillImpl<绝技_额外摸牌>(
		"绝技_额外摸牌",
		"锁定技，你打出【+2】时，目标额外摸一张牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//地雷_引爆：子技能，其他角色打出地雷颜色牌时受到1点伤害
class 地雷_引爆 final : public PassiveSkillImpl<地雷_引爆> {
public:
	地雷_引爆() : PassiveSkillImpl<地雷_引爆>(
		"地雷_引爆", "",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::use_card_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//地雷_撤雷：子技能，回合开始时撤掉地雷
class 地雷_撤雷 final : public PassiveSkillImpl<地雷_撤雷> {
public:
	地雷_撤雷() : PassiveSkillImpl<地雷_撤雷>(
		"地雷_撤雷", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//地雷：每种颜色限一次，回合结束时选择一种颜色，下一个回合开始前其他角色打出此颜色牌时受到1点伤害，所有颜色都选择过后重置限制
class 地雷 final : public PassiveSkillImpl<地雷> {
	mutable std::optional<Card::Color> armedColor;
	mutable std::unordered_set<Card::Color> usedColors;
public:
	地雷() : PassiveSkillImpl<地雷>(
		"地雷",
		"每种颜色限一次，回合结束时，你可以选择一种颜色。\n"
		"你的下一个回合开始前，其他角色打出此颜色牌时受到1点伤害；\n"
		"所有颜色都选择过后重置限制。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end,
		地雷_引爆::make(),
		地雷_撤雷::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	std::optional<Card::Color> getArmedColor() const { return armedColor; }
	void disarm() { armedColor.reset(); }
	void reset() override { PassiveSkill::reset(); armedColor.reset(); usedColors.clear(); }
};

//冲撞_加速：子技能，出牌时获得1个"速度"标记（上限10）
class 冲撞_加速 final : public PassiveSkillImpl<冲撞_加速> {
public:
	冲撞_加速() : PassiveSkillImpl<冲撞_加速>(
		"冲撞_加速", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//冲撞_伤害：子技能，打出有目标的牌后，目标受到2×速度标记数量点伤害
class 冲撞_伤害 final : public PassiveSkillImpl<冲撞_伤害> {
public:
	冲撞_伤害() : PassiveSkillImpl<冲撞_伤害>(
		"冲撞_伤害", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//冲撞_清标记：子技能，未出牌的回合结束时清空"速度"标记
class 冲撞_清标记 final : public PassiveSkillImpl<冲撞_清标记> {
public:
	冲撞_清标记() : PassiveSkillImpl<冲撞_清标记>(
		"冲撞_清标记", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//冲撞：锁定技，出牌时获得"速度"标记（上限10）；打出有目标的牌后对目标造成2×速度伤害；未出牌的回合结束时清空标记
class 冲撞 final : public PassiveSkillImpl<冲撞> {
public:
	冲撞() : PassiveSkillImpl<冲撞>(
		"冲撞",
		"锁定技，你出牌时获得1个\"速度\"标记（上限10）。\n"
		"你打出有目标的牌后，目标受到2×\"速度\"标记数量点伤害。\n"
		"你未出牌的回合结束时，清空所有\"速度\"标记。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end,
		冲撞_加速::make(),
		冲撞_伤害::make(),
		冲撞_清标记::make()
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};


//分身_获取：隐藏子技能，打出万能牌时获得一个「分身」标记
class 分身_获取 final : public PassiveSkillImpl<分身_获取> {
public:
	分身_获取() : PassiveSkillImpl<分身_获取>(
		"分身_获取", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//分身_发动：隐藏子技能，打出牌后可移去一个「分身」，打出一张同名牌（无需符合跟牌规则）
class 分身_发动 final : public PassiveSkillImpl<分身_发动> {
public:
	分身_发动() : PassiveSkillImpl<分身_发动>(
		"分身_发动", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//分身：游戏开始时或打出万能牌时获得「分身」；打出牌后可用「分身」打出一张同名牌
class 分身 final : public PassiveSkillImpl<分身> {
public:
	分身() : PassiveSkillImpl<分身>(
		"分身",
		"游戏开始时或你打出万能牌时，你可以获得一个「分身」。\n"
		"你打出牌后，若你手中有同牌名的牌，你可以移去一个「分身」，选择打出其中一张同名牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::game_begin,
		分身_获取::make(),
		分身_发动::make()
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//飞手_整轮：隐藏子技能，每轮开始时记录上一轮是否发动过飞手
class 飞手_整轮 final : public PassiveSkillImpl<飞手_整轮> {
public:
	飞手_整轮() : PassiveSkillImpl<飞手_整轮>(
		"飞手_整轮", "",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::round_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//筑丛_摸牌：隐藏子技能，摸牌至摸到绿色牌，返回摸到的所有牌
class 筑丛_摸牌 final : public PassiveSkillImpl<筑丛_摸牌> {
public:
	筑丛_摸牌() : PassiveSkillImpl<筑丛_摸牌>(
		"筑丛_摸牌", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::never
	) {}
	std::vector<ref<Card>> drawUntilGreen(GameLogic& game, Player& carrier);
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override { return true; }
};

//筑丛_累计：隐藏子技能，统计筑丛发动后打出的非绿色牌数，满十张令筑丛视为未发动过
class 筑丛_累计 final : public PassiveSkillImpl<筑丛_累计> {
public:
	筑丛_累计() : PassiveSkillImpl<筑丛_累计>(
		"筑丛_累计", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

