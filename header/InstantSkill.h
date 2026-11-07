#pragma once
#include "Skill.h"
#include <unordered_set>

class 装弹 final : public InstantSkillImpl<装弹> {
public:
	装弹() : InstantSkillImpl<装弹>(
		"装弹",
		"限定技，出牌阶段，若你手中没有封禁类牌且手牌数≤9，\n"
		"你可以将一半手牌变为随机的非数字牌（向下取整）。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
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

//炎烈：每回合限一次，出牌阶段，随机获得两张不同牌名的红色牌，然后可以弃置其中一张
class 炎烈 final : public InstantSkillImpl<炎烈> {
public:
	炎烈() : InstantSkillImpl<炎烈>(
		"炎烈",
		"每回合限一次，出牌阶段，你可以随机获得两张不同牌名的红色牌，然后可以弃置其中一张。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//勇进：每回合限一次，出牌阶段，失去1点体力并摸一张牌，若为非数字牌，弃置一张其他牌并回复2点体力
class 勇进 final : public InstantSkillImpl<勇进> {
public:
	勇进() : InstantSkillImpl<勇进>(
		"勇进",
		"每回合限一次，出牌阶段，你可以失去1点体力并摸一张牌，"
		"若为非数字牌，弃置一张其他牌并回复2点体力。",
		unlimited,1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//净身：出牌阶段，若你手中有五种颜色的牌，你可以弃置一张牌并回复1点体力
class 净身 final : public InstantSkillImpl<净身> {
public:
	净身() : InstantSkillImpl<净身>(
		"净身",
		"出牌阶段，若你手中有五种颜色的牌，你可以弃置一张牌并回复1点体力。",
		unlimited, unlimited,
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

//星矢：每回合限一次，出牌阶段，失去5点体力并重铸一种颜色的所有牌；
//若未因此获得同色牌，本局伤害倍率+0.1并获得10%吸血（至多50%）
class 星矢 final : public InstantSkillImpl<星矢> {
public:
	星矢() : InstantSkillImpl<星矢>(
		"星矢",
		"每回合限一次，出牌阶段，你可以失去5点体力并重铸一种颜色的所有牌；\n"
		"若未因此获得同色牌，你本局造成伤害倍率+0.1并获得10%吸血（至多50%）。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
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

//奔突：每回合限一次，出牌阶段，令一名其他角色摸一张牌并展示，然后随机弃置一张牌
class 奔突 final : public InstantSkillImpl<奔突> {
public:
	奔突() : InstantSkillImpl<奔突>(
		"奔突",
		"每回合限一次，出牌阶段，选择一名其他角色，令其摸一张牌并展示，然后其随机弃置一张牌。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//炼石：限定技，出牌阶段，重铸任意张手牌，然后扣减等同于弃置牌总分值的体力
class 炼石 final : public InstantSkillImpl<炼石> {
	friend class 成金;
	bool usedThisTurn = false;
	std::vector<Card*> yellowGained;  //本回合炼石重铸时获得的黄色牌
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

