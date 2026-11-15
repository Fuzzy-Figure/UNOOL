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


// ==================== 新增荒野乱斗角色技能（第二批） ====================

// ---------- 公牛【蛮冲】 ----------
//限定技，出牌阶段发动后开始记录；接下来四个自己的回合若都出过牌且未被封禁，
//摸四张牌、对一名其他角色造成 10X 点伤害（X 为本局累计发动次数），并令此技能视为未发动过。
//注：X 用独立的 activationCount 累计，不随 resetCount() 清零。
class 蛮冲 final : public InstantSkillImpl<蛮冲> {
	friend class 蛮冲_记录;
	friend class 蛮冲_结算;
	bool recording{};
	bool bannedThisTurn{};
	std::size_t turnsLeft{};
	std::size_t activationCount{};
public:
	蛮冲() : InstantSkillImpl<蛮冲>(
		"蛮冲",
		"限定技，出牌阶段，发动此技能开始记录，若你接下来四个回合没有空过或被封禁，"
		"你摸四张牌，对一名其他角色造成10X点伤害（X为此技能本局发动次数）且此技能视为未发动过。",
		1, unlimited,
		TriggerTime::phase_use,
		蛮冲_记录::make(),
		蛮冲_结算::make()
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

// ---------- 珀尔【爆汽】 ----------
//限定技，出牌阶段，逐张选万能牌弃置（↓结束），获得等量【过载】；
//弃置牌数>2 时回复10%已损体力；一张未弃则不消耗限定技。
class 爆汽 final : public InstantSkillImpl<爆汽> {
public:
	爆汽() : InstantSkillImpl<爆汽>(
		"爆汽",
		"限定技，出牌阶段，弃置任意张万能牌并获得等量张【过载】，弃置牌数>2时回复10%已损体力值。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};


// ==================== 新增荒野乱斗角色技能（第三批） ====================

//蝙蝠_计数：隐藏子技能，统计本局获得的手牌（不含 DrawReason::init 的初始手牌），每累计5张令蝙蝠次数+1
class 蝙蝠_计数 final : public PassiveSkillImpl<蝙蝠_计数> {
public:
	std::size_t gained{};    //本局累计获得牌数
	std::size_t awarded{};   //本局已发放的次数
	蝙蝠_计数() : PassiveSkillImpl<蝙蝠_计数>(
		"蝙蝠_计数", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::gain_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); gained = 0; awarded = 0; }
};

// ---------- 莫提斯【突击】【蝙蝠】 ----------
//突击：每回合限一次，出牌阶段，随机观看一名其他角色一张牌；手中无同名则获得此牌，
//有同名则对其造成3点伤害并弃置自己手中一张同名牌
class 突击 final : public InstantSkillImpl<突击> {
public:
	突击() : InstantSkillImpl<突击>(
		"突击",
		"每回合限一次，出牌阶段，你可随机观看一名其他角色一张牌，若你手中没有此牌名的牌，你获得此牌；若有，你对其造成3点伤害并弃置手中此牌。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//蝙蝠：每局游戏限零次，出牌阶段，吸取任意名座位连续的其他角色各5点体力（非伤害），自己回复等量体力
class 蝙蝠 final : public InstantSkillImpl<蝙蝠> {
public:
	蝙蝠() : InstantSkillImpl<蝙蝠>(
		"蝙蝠",
		"每局游戏限{limit}次（剩余{remaining}次），出牌阶段，你可吸取任意名座位连续的其他角色各5点体力（非伤害）。\n"
		"你每累计获得五张牌，此技能可发动次数+1。",
		0, unlimited,
		TriggerTime::phase_use,
		蝙蝠_计数::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};


// ==================== 新增荒野乱斗角色技能（第四批） ====================

// ---------- 科莱特【收税】 ----------
//限定技，出牌阶段，目标选择交出一张万能牌（自己随后弃一张牌）或受到1点伤害；
//目标没有万能牌时只能选受伤
class 收税 final : public InstantSkillImpl<收税> {
public:
	收税() : InstantSkillImpl<收税>(
		"收税",
		"限定技，出牌阶段，选择一名其他角色，其选择：\n"
		"1.交给你一张万能牌，然后你弃置一张牌；\n"
		"2.受到你的1点伤害。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

// ---------- 罗莎【生草】 ----------
//生草_连续：隐藏子技能，回合结束时结算本回合是否发动过，未发动则清零连续回合数
class 生草_连续 final : public PassiveSkillImpl<生草_连续> {
public:
	生草_连续() : PassiveSkillImpl<生草_连续>(
		"生草_连续", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//生草：每回合限一次，重铸一张绿色牌并回复X点体力（X为连续发动此技能的回合数）
class 生草 final : public InstantSkillImpl<生草> {
	friend class 生草_连续;
	std::size_t streak{};
	bool usedThisTurn{};
public:
	生草() : InstantSkillImpl<生草>(
		"生草",
		"每回合限一次，出牌阶段，你可以重铸一张绿色牌，并回复X点体力（X为你连续发动此技能的回合数）。",
		unlimited, 1,
		TriggerTime::phase_use,
		生草_连续::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override { InstantSkill::reset(); streak = 0; usedThisTurn = false; }
};

// ==================== 新增荒野乱斗角色技能（第五批） ====================

// ---------- 芬克斯【时匡】 ----------
//时匡_回合：隐藏子技能，「神速」存在时每轮回合结束可请求一个额外回合
class 时匡_回合 final : public PassiveSkillImpl<时匡_回合> {
public:
	时匡_回合() : PassiveSkillImpl<时匡_回合>(
		"时匡_回合", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//时匡_摸牌：隐藏子技能，执行摸牌阶段后移去「神速」
class 时匡_摸牌 final : public PassiveSkillImpl<时匡_摸牌> {
public:
	时匡_摸牌() : PassiveSkillImpl<时匡_摸牌>(
		"时匡_摸牌", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_draw_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//时匡_万能牌：隐藏子技能，打出万能牌后移去「神速」
class 时匡_万能牌 final : public PassiveSkillImpl<时匡_万能牌> {
public:
	时匡_万能牌() : PassiveSkillImpl<时匡_万能牌>(
		"时匡_万能牌", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//时匡：限定技，出牌阶段获得「神速」标记
class 时匡 final : public InstantSkillImpl<时匡> {
public:
	std::size_t lastExtraRound{};   //上一次请求额外回合的轮数（每轮限一次）
	时匡() : InstantSkillImpl<时匡>(
		"时匡",
		"限定技，出牌阶段，你获得\"神速\"标记；标记存在时每轮你于回合结束时可额外执行一个回合，"
		"你执行摸牌阶段或打出万能牌后，移去\"神速\"标记。",
		1, unlimited,
		TriggerTime::phase_use,
		时匡_回合::make(),
		时匡_摸牌::make(),
		时匡_万能牌::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override { InstantSkill::reset(); lastExtraRound = 0; }
};


// ==================== 新增荒野乱斗角色技能（第六批） ====================

// ---------- 波克【音波】 ----------
//音波：每回合限一次，重铸点数连续的三张数字牌，回复 1+bonus 点体力
class 音波 final : public InstantSkillImpl<音波> {
public:
	std::size_t bonus{};   //【安可】累计的回复值加成（每局清零）
	音波() : InstantSkillImpl<音波>(
		"音波",
		"每回合限一次，出牌阶段，你可重铸点数连续的三张数字牌，回复1点体力。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override { InstantSkill::reset(); bonus = 0; }
};

// ---------- 邦尼【发射】 ----------
//发射：绝地技（每场限一次），生命上限减至75，【炮击】改为三次伤害
class 发射 final : public InstantSkillImpl<发射> {
public:
	发射() : InstantSkillImpl<发射>(
		"发射",
		"绝地技（每场限一次），出牌阶段，你可将自己发射出去并将生命上限减至75；"
		"发射后本场游戏你的【炮击】改为造成三次伤害。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};
