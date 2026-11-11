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

class 顶置 final : public PassiveSkillImpl<顶置> {
public:
	顶置() : PassiveSkillImpl<顶置>(
		"顶置",
		"锁定技，回合开始时，你观看牌堆底一张牌，然后可将此牌置于牌堆顶。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 割腕 final : public PassiveSkillImpl<割腕> {
public:
	割腕() : PassiveSkillImpl<割腕>(
		"割腕",
		"锁定技，你打出红色牌后失去随机1~5点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 丑皇 final : public PassiveSkillImpl<丑皇> {
public:
	丑皇() : PassiveSkillImpl<丑皇>(
		"丑皇",
		"锁定技，你打出万能牌后，选择一项：\n1.回复10点体力；\n2.弃置一张非数字牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 健身 final : public PassiveSkillImpl<健身> {
public:
	健身() : PassiveSkillImpl<健身>(
		"健身",
		"锁定技，每局游戏结束时，回复5点体力。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 做题 final : public PassiveSkillImpl<做题> {
public:
	做题() : PassiveSkillImpl<做题>(
		"做题",
		"限定技，你打出数字牌/非数字牌后，可弃置一张非数字牌/数字牌。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//巨富_摸牌：摸牌阶段额外摸一张牌
class 巨富_摸牌 final : public PassiveSkillImpl<巨富_摸牌> {
public:
	巨富_摸牌() : PassiveSkillImpl<巨富_摸牌>(
		"巨富_摸牌", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::draw_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//巨富：游戏开始时初始手牌改为十二张；摸牌阶段额外摸一张牌
class 巨富 final : public PassiveSkillImpl<巨富> {
public:
	巨富() : PassiveSkillImpl<巨富>(
		"巨富",
		"锁定技，游戏开始时，你的初始手牌改为十二张；你执行摸牌阶段额外摸一张牌。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin,
		巨富_摸牌::make()
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 破产 final : public PassiveSkillImpl<破产> {
public:
	破产() : PassiveSkillImpl<破产>(
		"破产",
		"锁定技，若你打出牌后手牌数全场最多，随机弃置一张牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 窃观 final : public PassiveSkillImpl<窃观> {
public:
	窃观() : PassiveSkillImpl<窃观>(
		"窃观",
		"锁定技，其他角色从牌堆一次性获得一张牌时，你得知其颜色牌名。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::draw_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 淘汰 final : public PassiveSkillImpl<淘汰> {
public:
	淘汰() : PassiveSkillImpl<淘汰>(
		"淘汰",
		"每局游戏共限六次：\n"
		"你打出数字牌后，可弃置一张点数小于等于该牌一半（向下取整）的同色数字牌；\n"
		"你打出功能牌后，可从游戏外随机获得一张同色的功能牌。",
		6, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 光合 final : public PassiveSkillImpl<光合> {
public:
	光合() : PassiveSkillImpl<光合>(
		"光合",
		"当你成为封禁类功能牌的目标时，你可判定：\n"
		"若结果为数字牌，来源摸一张牌；\n"
		"为功能牌，解除此牌封禁状态。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 豪赌 final : public PassiveSkillImpl<豪赌> {
public:
	豪赌() : PassiveSkillImpl<豪赌>(
		"豪赌",
		"回合开始时，可进行一次判定：\n"
		"绿色或黑色，你获得之；\n"
		"黄色，你跳过此回合；\n"
		"蓝色，你重铸一张手牌；\n"
		"红色，你失去5点体力。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};


class 迷烟 final : public PassiveSkillImpl<迷烟> {
public:
	迷烟() : PassiveSkillImpl<迷烟>(
		"迷烟",
		"回合结束时，你可展示一张非黑色手牌，"
		"令一名其他角色选择弃置一张万能牌或与你展示牌颜色相同的手牌，否则其摸一张牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 创世 final : public PassiveSkillImpl<创世> {
public:
	创世() : PassiveSkillImpl<创世>(
		"创世",
		"游戏开始时，你可选择一个你手中没有的牌名，将一张手牌变为此牌名的牌（颜色自选）。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 补天 final : public PassiveSkillImpl<补天> {
	std::vector<Card::Name> record;
public:
	补天() : PassiveSkillImpl<补天>(
		"补天",
		"锁定技，每种牌名限一次，你打出手中唯一一种牌名的牌后，记录其牌名，然后选择一张手牌变为随机一张未记录的牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 犬子 final : public PassiveSkillImpl<犬子> {
	mutable std::size_t playCount = 0;
public:
	犬子() : PassiveSkillImpl<犬子>(
		"犬子",
		"你每累计打出{count+1}张牌后（X为此技能发动次数，初始为1），可弃置一张牌。\n"
		"当前已打出{playCount}张牌",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
	std::optional<std::string> extraPlaceholders(const std::string& key) const override;
};


class 爬竿_伤害 final : public PassiveSkillImpl<爬竿_伤害> {
public:
	爬竿_伤害() : PassiveSkillImpl<爬竿_伤害>(
		"爬竿_伤害",
		"锁定技，目标角色本局每个未出过牌的回合结束时，失去1%最大体力（向下取整）。",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 爬竿 final : public PassiveSkillImpl<爬竿> {
	friend class 爬竿_伤害;
	opt_ref<Player> targetOpt_;
public:
	爬竿() : PassiveSkillImpl<爬竿>(
		"爬竿",
		"锁定技，每局游戏开始时，选择一名其他角色，该角色本局每个未出过牌的回合结束时，其失去1%最大体力（向下取整）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::game_begin,
		爬竿_伤害::make()
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};


//渊涡：你每个未出过牌的回合结束时，回复1点体力
class 渊涡 final : public PassiveSkillImpl<渊涡> {
public:
	渊涡() : PassiveSkillImpl<渊涡>(
		"渊涡",
		"锁定技，你每个未出过牌的回合结束时，回复1点体力。",
		unlimited, true,
		TriggerPlayer::self, TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//治病：其他角色首次打出某种牌名的非数字牌时，大中医三选一（无效/弃牌/摸牌），剩余多选项则移除本次所选
class 治病 final : public PassiveSkillImpl<治病> {
	std::unordered_set<Card::Name> playedNames;   //本局已触发过的非数字牌名
	std::vector<std::size_t> options{ 1, 2, 3 };    //剩余可选项编号
public:
	治病() : PassiveSkillImpl<治病>(
		"治病",
		"锁定技，其他角色首次打出一种牌名的非数字牌时，你选择一项：\n"
		"1.此牌无效；2.你弃置一张牌；3.其摸两张牌。\n"
		"若此时剩余多个选项，移除你本次选择的选项。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::use_card_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};

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

//成金：觉醒技（隐藏子技能），回合结束时若本回合发动过炼石，可弃置本回合获得的所有黄色牌
class 成金 final : public PassiveSkillImpl<成金> {
public:
	成金() : PassiveSkillImpl<成金>(
		"成金", "",
		1, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
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

