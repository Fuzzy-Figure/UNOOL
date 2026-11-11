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

class 炼兵 final : public PassiveSkillImpl<炼兵> {
	std::unordered_set<Card::Name> usedNames;
	std::map<Card::Name, std::size_t> buildPairs(const Player& carrier) const;
public:
	炼兵() : PassiveSkillImpl<炼兵>(
		"炼兵",
		"每种牌名限一次，回合开始时，若你手中【+2】数量≤4，你可以弃置2张同名牌，从游戏外获得一张随机颜色的【+2】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); usedNames.clear(); }
};
class 好火 final : public PassiveSkillImpl<好火> {
	std::unordered_set<std::size_t> usedPlayerIds;
public:
	好火() : PassiveSkillImpl<好火>(
		"好火",
		"每名角色限一次，当其他角色打出红色牌后，若其体力值大于你，你可以交给其一张手牌。",
		unlimited, false,
		TriggerPlayer::others,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); usedPlayerIds.clear(); }
};

class 过江 final : public PassiveSkillImpl<过江> {
public:
	过江() : PassiveSkillImpl<过江>(
		"过江",
		"锁定技，当你成为【+2】的目标时，来源摸2张牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 大盏 final : public PassiveSkillImpl<大盏> {
	static void randomEnlarge(Card& c);
public:
	大盏() : PassiveSkillImpl<大盏>(
		"大盏",
		"锁定技，回合开始时，若你有数字牌，其中点数最小的牌均随机变大（至多变至9）。\n"
		"若全为【9】，可将其中一张变为红色并回复1点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 连营 final : public PassiveSkillImpl<连营> {
	friend class 困界;
private:
	std::set<Card::Type> triggered;
public:
	连营() : PassiveSkillImpl<连营>(
		"连营",
		"每局游戏每种类别限一次，你失去手中一种类别的最后一张牌后，你可弃置另一种类别的一张牌并从游戏外再随机获得一张此类别的牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::lose_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};

class 困界 final : public PassiveSkillImpl<困界> {
	mutable opt_ref<std::set<Card::Type>> triggeredCache;  //缓存连营的triggered引用，避免重复查找
	std::set<Card::Type>& getTriggered(const Player& carrier) const;
public:
	困界() : PassiveSkillImpl<困界>(
		"困界",
		"觉醒技，回合结束时，若你【连营】中所有类别均已触发过，你可令一名角色重铸手中一种类别的所有牌，然后你重铸另一种类别的所有牌。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};



//易主：每回合限一次，一次性获得恰好2张牌时，可弃置这些牌并令渡荆+1
class 易主 final : public PassiveSkillImpl<易主> {
public:
	易主() : PassiveSkillImpl<易主>(
		"易主",
		"每回合限一次，你一次性获得两张牌时，可弃置这些牌，然后【渡荆】本局游戏可发动次数+1。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::gain_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
private:
	std::size_t phaseCount = 0;
};

//渡荆：每局限一次，回合结束时若手牌全场最多，与一名其他角色拼点，没赢的获得随机+2
class 渡荆 final : public PassiveSkillImpl<渡荆> {
public:
	渡荆() : PassiveSkillImpl<渡荆>(
		"渡荆",
		"每局游戏限{limit}次，回合结束时，若你手牌数全场最多，\n"
		"可与其他角色拼点：赢的角色回复5体力，没赢的角色从游戏外获得一张随机颜色的【+2】。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//侵蚀：锁定技，其他角色的回合结束时，若其手牌数≤3，其失去手牌数点体力
class 侵蚀 final : public PassiveSkillImpl<侵蚀> {
public:
	侵蚀() : PassiveSkillImpl<侵蚀>(
		"侵蚀",
		"锁定技，其他角色的回合结束时，若其手牌数≤3，其失去手牌数点体力。",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//剧变：限定技，摸牌阶段，你可以改为令所有角色将全部手牌交给下家，手牌数因此减少的角色摸两张牌
class 剧变 final : public PassiveSkillImpl<剧变> {
public:
	剧变() : PassiveSkillImpl<剧变>(
		"剧变",
		"限定技，摸牌阶段，你可以改为令所有角色将全部手牌交给下家，"
		"手牌数因此减少的角色摸两张牌。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_draw_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//兵多：回合开始时，你可以失去1点体力并决议一张牌；若你体力值≤50，则改为决议两张
class 兵多 final : public PassiveSkillImpl<兵多> {
public:
	兵多() : PassiveSkillImpl<兵多>(
		"兵多",
		"回合开始时，你可以失去1点体力并决议一张牌；\n"
		"若你体力值≤50，则改为决议两张。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//通天：锁定技，你打出【+4】后，本局你获得一次发动【修正】的次数
class 通天 final : public PassiveSkillImpl<通天> {
public:
	通天() : PassiveSkillImpl<通天>(
		"通天",
		"锁定技，你打出【+4】后，本局你获得一次发动【修正】的次数。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	//todo: void reset() override {}
};

//调羹：每局限三次，摸一张，展示数字牌给其他角色并回复其分值体力
class 调羹 final : public InstantSkillImpl<调羹> {
public:
	调羹() : InstantSkillImpl<调羹>(
		"调羹",
		"每局游戏限三次，每回合限一次，出牌阶段，你可以摸一张牌，展示一张数字牌令一名角色获得之，\n"
		"并令其回复此牌分值点体力。",
		3, 1,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//修正：每回合限一次，出牌阶段，将一张手牌改为声明的非万能牌（每局每种牌名限一次）
class 修正 final : public InstantSkillImpl<修正> {
	mutable std::unordered_set<Card::Name> usedNames;
public:
	修正() : InstantSkillImpl<修正>(
		"修正",
		"每回合限{limit}次（剩余{remaining}次），出牌阶段，你可以将一张手牌改为声明的非万能牌（每局游戏每种牌名限一次），\n"
		"若你改为数字牌则失去3%当前体力（向上取整）。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override { ActiveSkill::reset(); usedNames.clear(); }
};

//无礼_计数：隐藏子技能，每打出一种新类型的牌令无礼本局可用次数+1
class 无礼_计数 final : public PassiveSkillImpl<无礼_计数> {
public:
	无礼_计数() : PassiveSkillImpl<无礼_计数>(
		"无礼_计数", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//无礼：每局限{limit}次（每打出一种新类型的牌+1次），弃置一名角色半数手牌（向下取整），
//然后观看牌堆顶等量张牌并交给其
class 无礼 final : public InstantSkillImpl<无礼> {
	friend class 无礼_计数;
	std::unordered_set<Card::Type> usedTypes;  //已打出过的牌类型
public:
	无礼() : InstantSkillImpl<无礼>(
		"无礼",
		"每局游戏限{limit}次（剩余{remaining}次），出牌阶段，你可以弃置一名角色半数手牌（向下取整），\n"
		"然后观看牌堆顶等量张牌并交给其。\n"
		"你每打出一种新类型的牌，此技能本局可用次数+1。",
		0, unlimited,
		TriggerTime::phase_use,
		无礼_计数::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

