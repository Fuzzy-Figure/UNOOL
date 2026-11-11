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

class 粪怒 final : public PassiveSkillImpl<粪怒> {
public:
	粪怒() : PassiveSkillImpl<粪怒>(
		"粪怒",
		"限定技，一名角色手牌数变为1时，你可令其摸你手牌数张牌（至多摸五张）。",
		1, false,
		TriggerPlayer::others,
		TriggerTime::lose_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 望日_子 final : public PassiveSkillImpl<望日_子> {
public:
	望日_子() : PassiveSkillImpl<望日_子>(
		"望日", "",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 望日 final : public PassiveSkillImpl<望日> {
public:
	望日() : PassiveSkillImpl<望日>(
		"望日",
		"锁定技，回合开始时，令一张黄色牌点数+1（至多为9）。\n"
		"你打出黄色【9】后，可以获得一张【+4】。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin,
		望日_子::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 朔日 final : public PassiveSkillImpl<朔日> {
public:
	朔日() : PassiveSkillImpl<朔日>(
		"朔日",
		"你打出黄色牌后，回复1点体力并可选择一项：\n"
		"1.将一张数字牌变为黄色且可令其点数+1/-1；\n"
		"2.将一张功能牌变为黄色的随机功能牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 健忘 final : public PassiveSkillImpl<健忘> {
public:
	健忘() : PassiveSkillImpl<健忘>(
		"健忘",
		"每局游戏限两次，回合开始时，你可以任意更改当前的公共颜色。",
		2, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 叛党 final : public PassiveSkillImpl<叛党> {
public:
	叛党() : PassiveSkillImpl<叛党>(
		"叛党",
		"每局游戏开始时，你可以弃置任意张颜色各不同的牌。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 清洗 final : public PassiveSkillImpl<清洗> {
public:
	清洗() : PassiveSkillImpl<清洗>(
		"清洗",
		"限定技，回合开始时，若你手牌仅有两种颜色，你可以弃置一种颜色的所有手牌。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//投日：回收弃牌堆后，可弃置X张牌（X为此技能本场发动次数）
class 投日 final : public PassiveSkillImpl<投日> {
public:
	投日() : PassiveSkillImpl<投日>(
		"投日",
		"回收弃牌堆后，你可以弃置X张牌（X为此技能本场发动次数）。",
		unlimited, false,
		TriggerPlayer::anybody,
		TriggerTime::recycle_discardPile_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//宣战：锁定技，回合结束时调整数字牌点数，若手中【5】≥4可转化为【+4】打出
class 宣战 final : public PassiveSkillImpl<宣战> {
public:
	宣战() : PassiveSkillImpl<宣战>(
		"宣战",
		"锁定技，回合结束时，你手中点数>5的数字牌点数-1，点数<5的数字牌点数+1（若本回合未出过牌改为-2/+2）；\n"
		"然后若你手中【5】的数量≥4，你可将这些牌当作一张【+4】打出。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//赔款：你封禁其他角色后，若其没有【+2】，其随机获得一张【+2】
class 赔款 final : public PassiveSkillImpl<赔款> {
public:
	赔款() : PassiveSkillImpl<赔款>(
		"赔款",
		"你封禁其他角色后，若其手中没有【+2】，其随机获得一张【+2】。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::ban_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//伪政_计数：隐藏子技能，统计进入弃牌堆的牌数，每十张令伪政本局可用次数+1
class 伪政_计数 final : public PassiveSkillImpl<伪政_计数> {
	std::size_t totalDiscarded = 0;
public:
	伪政_计数() : PassiveSkillImpl<伪政_计数>(
		"伪政_计数", "",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::card_discard_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); totalDiscarded = 0; }
};

//伪政：每局限一次（每十张牌进弃牌堆+1），出牌阶段，展示牌堆顶和牌堆底各{view}张牌，可获得其中一张，弃置其余的牌
//每因此获得十张牌，牌堆两侧观看牌数各增加一张（至多各三张）
class 伪政 final : public InstantSkillImpl<伪政> {
	std::size_t viewCount = 1;    //每侧观看牌数（至多3）
	std::size_t gainedTotal = 0;  //因伪政累计获得的牌数
public:
	伪政() : InstantSkillImpl<伪政>(
		"伪政",
		"每局游戏限{limit}次，出牌阶段，展示牌堆顶和牌堆底各{view}张牌，可以获得其中一张，弃置其余的牌。\n"
		"每有十张牌进入弃牌堆，此技能本局可用次数+1。\n"
		"每因此获得十张牌，牌堆两侧观看牌数各增加一张（至多各三张）。",
		1, unlimited,
		TriggerTime::phase_use,
		伪政_计数::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override { ActiveSkill::reset(); viewCount = 1; gainedTotal = 0; }
	std::optional<std::string> extraPlaceholders(const std::string& key) const override {
		if (key == "view") return std::to_string(viewCount);
		return std::nullopt;
	}
};

//改革_摸牌替代：隐藏子技能，其他角色于摸牌阶段外摸牌时，可将此次摸牌改为令其失去2X%最大体力（向上取整）
class 改革_摸牌替代 final : public PassiveSkillImpl<改革_摸牌替代> {
public:
	改革_摸牌替代() : PassiveSkillImpl<改革_摸牌替代>(
		"改革_摸牌替代", "",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::draw_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//改革：每回合限一次，出牌阶段，可以失去10点体力令一名角色摸1张牌；
//其他角色于摸牌阶段外摸牌时，可以将此次摸牌改为令其失去2X%最大体力（向上取整）
class 改革 final : public InstantSkillImpl<改革> {
public:
	改革() : InstantSkillImpl<改革>(
		"改革",
		"每回合限一次，出牌阶段，你可以失去10点体力，令一名角色摸1张牌；\n"
		"其他角色于摸牌阶段外摸牌时，你可以将此次摸牌改为令其失去2X%最大体力（向上取整，X为摸牌数）。",
		unlimited, 1,
		TriggerTime::phase_use,
		改革_摸牌替代::make()
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

