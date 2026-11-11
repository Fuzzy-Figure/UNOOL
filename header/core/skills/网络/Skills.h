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

class 隐身 final : public PassiveSkillImpl<隐身> {
public:
	隐身() : PassiveSkillImpl<隐身>(
		"隐身",
		"锁定技，当你成为【+2】/【+4】的目标时，改为你的下家摸1张牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 带派 final : public PassiveSkillImpl<带派> {
public:
	带派() : PassiveSkillImpl<带派>(
		"带派",
		"每局游戏开始时，选择获得一张【变色】或【+4】；也可选择两张均获得并失去25点体力。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 军国 final : public PassiveSkillImpl<军国> {
public:
	军国() : PassiveSkillImpl<军国>(
		"军国",
		"锁定技，当一名角色被封禁时，其失去1%最大体力（向上取整），自己被封禁改为失去1体力。",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::ban_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 家暴 final : public PassiveSkillImpl<家暴> {
public:
	家暴() : PassiveSkillImpl<家暴>(
		"家暴",
		"限定技，回合开始时，你可以令一名体力小于你的角色失去10%最大体力（向上取整）。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 买棋 final : public PassiveSkillImpl<买棋> {
public:
	买棋() : PassiveSkillImpl<买棋>(
		"买棋",
		"回合开始时，你可以失去10X点体力（X为此技能发动次数-1），从游戏外获得随机一张万能牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 卖棋 final : public PassiveSkillImpl<卖棋> {
public:
	卖棋() : PassiveSkillImpl<卖棋>(
		"卖棋",
		"回合结束时，你可以弃置一张万能牌，回复10点体力。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 电音 final : public PassiveSkillImpl<电音> {
public:
	电音() : PassiveSkillImpl<电音>(
		"电音",
		"每局游戏限十次，回合开始时，你可以令手牌中所有数字牌变成随机数字并回复1点体力。",
		10, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 蒙面 final : public PassiveSkillImpl<蒙面> {
public:
	蒙面() : PassiveSkillImpl<蒙面>(
		"蒙面",
		"锁定技，当你失去体力时，失去体力的数值减少25% （向下取整）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::damage_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 锐刻 final : public PassiveSkillImpl<锐刻> {
	bool disabled = false;
public:
	锐刻() : PassiveSkillImpl<锐刻>(
		"锐刻",
		"当你打出【5】时，你可以令一名角色摸1张牌；你可改为令其摸5张牌并失去此技能（至本局结束）。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); disabled = false; }
};

class 假酒 final : public PassiveSkillImpl<假酒> {
	bool number = false, action = false, wild = false;
public:
	假酒() : PassiveSkillImpl<假酒>(
		"假酒",
		"你打出功能牌后，可以随机获得一张牌。\n"
		"若此时你已累计获得三种类型的牌，弃置两张牌并失去此技能至本局结束。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};

class 生存 final : public PassiveSkillImpl<生存> {
public:
	生存() : PassiveSkillImpl<生存>(
		"生存",
		"锁定技，回合开始时，选择一张手牌变为随机颜色的【X】\n（X为此技能本局发动次数，至多为9）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 创造 final : public PassiveSkillImpl<创造> {
	std::unordered_set<Card::Color> usedColors;
public:
	创造() : PassiveSkillImpl<创造>(
		"创造",
		"每局每种颜色限一次，你打出【9】后，可从游戏外获得一张同色的任意牌名的牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); usedColors.clear(); }
};

class 举报 final : public PassiveSkillImpl<举报> {
public:
	举报() : PassiveSkillImpl<举报>(
		"举报",
		"契定技，当一名角色打出万能牌后，你可令其失去10%当前体力。",
		unlimited, false,
		TriggerPlayer::anybody,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};
class 猥琐 final : public PassiveSkillImpl<猥琐> {
public:
	猥琐() : PassiveSkillImpl<猥琐>(
		"猥琐",
		"锁定技，回合结束时，若你不是全场体力最高的角色，回复1点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 棋王 final : public PassiveSkillImpl<棋王> {
public:
	棋王() : PassiveSkillImpl<棋王>(
		"棋王",
		"当你打出弃牌堆顶同色同名牌后，你可以弃置两张手牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 金铲 final : public PassiveSkillImpl<金铲> {
public:
	金铲() : PassiveSkillImpl<金铲>(
		"金铲",
		"锁定技，当其他角色回复1体力时，改为其失去1体力。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::recover_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 射门 final : public PassiveSkillImpl<射门> {
public:
	射门() : PassiveSkillImpl<射门>(
		"射门",
		"你打出数字牌后，可令一名角色进行判定，若结果为蓝色/黑色，其摸1张牌；"
		"若判定角色为你，改为弃置一张牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 有活 final : public PassiveSkillImpl<有活> {
public:
	有活() : PassiveSkillImpl<有活>(
		"有活",
		"锁定技，每局游戏开始时，你摸两张牌并弃置X张牌（X为当前局数）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};


class 水鬼 final : public PassiveSkillImpl<水鬼> {
public:
	水鬼() : PassiveSkillImpl<水鬼>(
		"水鬼",
		"锁定技，你获得蓝色牌后，弃置这些蓝色牌，然后弃置一张其他牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::gain_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 爆缸 final : public PassiveSkillImpl<爆缸> {
public:
	爆缸() : PassiveSkillImpl<爆缸>(
		"爆缸",
		"契定技，回合开始时，若你手中均为数字牌，可将其中一张变为同色的随机功能牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};

class 落水 final : public PassiveSkillImpl<落水> {
public:
	落水() : PassiveSkillImpl<落水>(
		"落水",
		"契定技，限定技，一名角色失去蓝色牌后，其从牌堆随机获得其他三种颜色的牌各一张。",
		1, false,
		TriggerPlayer::anybody,
		TriggerTime::lose_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); setForced(false); }
};

class 骚扰 final : public PassiveSkillImpl<骚扰> {
	bool disabled = false;
public:
	骚扰() : PassiveSkillImpl<骚扰>(
		"骚扰",
		"回合结束时，你可以判定，若结果不为蓝色，你回复1点体力并重置【落水】使用次数；"
		"否则你失去此技能至本局结束。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override { PassiveSkill::reset(); disabled = false; }
};

class 黑洞 final : public PassiveSkillImpl<黑洞> {
	std::set<Card::Name> record;
public:
	黑洞() : PassiveSkillImpl<黑洞>(
		"黑洞",
		"每种牌名限一次，回合开始时，你可以从弃牌堆顶4张牌中选择一张获得之。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};


class 好事 final : public PassiveSkillImpl<好事> {
public:
	好事() : PassiveSkillImpl<好事>(
		"好事",
		"锁定技，摸牌阶段摸到万能牌后，展示之并回复5点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_draw_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 压抑 final : public PassiveSkillImpl<压抑> {
public:
	压抑() : PassiveSkillImpl<压抑>(
		"压抑",
		"一名角色打出万能牌结算后，你可选择一名角色弃置手中所有点数最大的数字牌，"
		"若因此弃置牌数≥2张其失去10%最大体力（向上取整）。",
		unlimited, false,
		TriggerPlayer::anybody,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 捉奸_弃牌 final : public PassiveSkillImpl<捉奸_弃牌> {
public:
	捉奸_弃牌() : PassiveSkillImpl<捉奸_弃牌>(
		"捉奸_弃牌",
		"其他角色打出万能牌后，你随机弃置一张牌，若为红色你失去 5% 当前体力（向上取整）。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 捉奸 final : public PassiveSkillImpl<捉奸> {
public:
	捉奸() : PassiveSkillImpl<捉奸>(
		"捉奸",
		"锁定技，回合结束时，你手中所有非红色牌变为红色，红色牌变为非红色。\n"
		"其他角色打出万能牌后，你随机弃置一张牌，若为红色你失去 5% 当前体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end,
		捉奸_弃牌::make()
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};


//没座：回合开始时，没红牌则摸一张（手牌为1时不触发），有红牌则可弃一张非红色牌
class 没座 final : public PassiveSkillImpl<没座> {
public:
	没座() : PassiveSkillImpl<没座>(
		"没座",
		"回合开始时，若你手中没有红色牌，你随机获得一张红色牌（手牌数为1时不触发）；"
		"若你手中有红色牌，你可以弃置一张其他颜色的牌。",
		unlimited, true,
		TriggerPlayer::self, TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//空空：每局限两次，回合结束时，可将一张功能牌变为红色的【封禁】
class 空空 final : public PassiveSkillImpl<空空> {
public:
	空空() : PassiveSkillImpl<空空>(
		"空空",
		"每局游戏限两次，回合结束时，你可以将一张功能牌变为红色的【封禁】。",
		2, false,
		TriggerPlayer::self, TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 暗忍 final : public PassiveSkillImpl<暗忍> {
public:
	暗忍() : PassiveSkillImpl<暗忍>(
		"暗忍",
		"锁定技，回合结束时，你失去1点体力并将随机一张功能牌变为随机颜色的【封禁】。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//四麻：锁定技，回合结束时，将手牌调整至四张
class 四麻 final : public PassiveSkillImpl<四麻> {
public:
	四麻() : PassiveSkillImpl<四麻>(
		"四麻",
		"锁定技，回合结束时，将手牌调整至四张。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//走位：回合开始时，决议至多X张牌（X为已输局数，至多3）；若未因此决议牌，失去此技能并获芜湖
class 走位 final : public PassiveSkillImpl<走位> {
public:
	走位() : PassiveSkillImpl<走位>(
		"走位",
		"回合开始时，你可以决议至多X张牌（X为你已输局数且至多为3）；\n"
		"若你未因此决议牌，则失去此技能并获得【芜湖】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};


//九一：任意角色打出9/1后，可弃一张9/1
class 九一 final : public PassiveSkillImpl<九一> {
public:
	九一() : PassiveSkillImpl<九一>(
		"九一",
		"任意角色打出【9】/【1】后，你可弃置一张【9】/【1】。",
		unlimited, false,
		TriggerPlayer::anybody,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//白虎：每局每名角色限一次，回合开始时若其仅剩一张牌，令其获得一张指定颜色牌名的9/1
class 白虎 final : public PassiveSkillImpl<白虎> {
	std::unordered_set<std::size_t> triggeredPlayers;
public:
	白虎() : PassiveSkillImpl<白虎>(
		"白虎",
		"每局每名角色限一次，一名角色回合开始时，若其仅剩一张牌，\n"
		"你可令其获得一张你指定颜色牌名的【9】/【1】。",
		unlimited, false,
		TriggerPlayer::anybody,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
};


//熬鹰_计数：隐藏子技能，回合开始时统计已执行的回合数（被封禁跳过的回合不计入）
class 熬鹰_计数 final : public PassiveSkillImpl<熬鹰_计数> {
public:
	熬鹰_计数() : PassiveSkillImpl<熬鹰_计数>(
		"熬鹰_计数", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//熬鹰_解除限制：隐藏子技能，觉醒后第二个出牌阶段解除出牌限制
class 熬鹰_解除限制 final : public PassiveSkillImpl<熬鹰_解除限制> {
public:
	熬鹰_解除限制() : PassiveSkillImpl<熬鹰_解除限制>(
		"熬鹰_解除限制", "",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_use2_begin
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//熬鹰：觉醒技，累计执行20个回合后从游戏外获得一张【+4】，并解锁第二个出牌阶段的出牌限制
class 熬鹰 final : public PassiveSkillImpl<熬鹰> {
	friend class 熬鹰_计数;
	friend class 熬鹰_解除限制;
	std::size_t turnCount = 0;  //已执行的回合数（被封禁跳过的回合不计入）
	bool awakened = false;      //是否已觉醒
public:
	熬鹰() : PassiveSkillImpl<熬鹰>(
		"熬鹰",
		"觉醒技，若你已累计执行过20个回合（被封禁跳过的回合不计入），\n"
		"你从游戏外获得一张【+4】，然后第二个出牌阶段打出牌不受限制。\n"
		"当前已执行回合数：{turns}",
		1, true,
		TriggerPlayer::self,
		TriggerTime::phase_end,
		熬鹰_计数::make(),
		熬鹰_解除限制::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	bool isAwakened() const { return awakened; }
	void reset() override;
	std::optional<std::string> extraPlaceholders(const std::string& key) const override;
};

//连杀：锁定技，每连续三回合打出牌后，从游戏外随机获得一张万能牌（被封禁或未出牌的回合中断连续）
class 连杀 final : public PassiveSkillImpl<连杀> {
private:
	std::size_t consecutive = 0;  //当前连续打出牌的回合数
public:
	连杀() : PassiveSkillImpl<连杀>(
		"连杀",
		"锁定技，你每连续三回合打出牌后，从游戏外随机获得一张万能牌。\n"
		"当前连续打出牌回合数：{turns}",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override;
	std::optional<std::string> extraPlaceholders(const std::string& key) const override;
};

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

class 我妈 final : public TransformSkillImpl<我妈> {
public:
	我妈() : TransformSkillImpl<我妈>(
		"我妈",
		"每局游戏限三次，出牌阶段，你可将非红色牌当作红【封禁】打出，然后摸一张牌。",
		3, unlimited,
		TriggerTime::phase_use
	) {}
	std::size_t getCardCount() const override;
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	void addition(GameLogic& game, Player& carrier) const override;
	std::string getPrompt() const override;
};


class 曼巴 final : public TransformSkillImpl<曼巴> {
public:
	曼巴() : TransformSkillImpl<曼巴>(
		"曼巴",
		"出牌阶段，你可将一张【8】当作任意基础颜色的任意牌打出。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	std::size_t getCardCount() const override;
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	std::string getPrompt() const override;
};

