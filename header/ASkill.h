#include "Skill.h"

class 装弹 : public InstantSkillImpl<装弹> {
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
class 徒步 : public InstantSkillImpl<徒步> {
public:
	徒步() : InstantSkillImpl<徒步>(
		"徒步",
		"出牌阶段，你可以重铸一张牌并回复1点体力，若你重铸了数字牌，失去X点体力（X为此技能本局发动次数）。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};


class 招待 : public InstantSkillImpl<招待> {
public:
	招待() : InstantSkillImpl<招待>(
		"招待",
		"限定技，出牌阶段，你可以将一张牌交给一名其他角色。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 八爪 : public TransformSkillImpl<八爪> {
public:
	八爪() : TransformSkillImpl<八爪>(
		"八爪",
		"你可以将一张数字牌当作蓝【8】打出。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	std::size_t getCardCount() const override;
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	std::wstring getPrompt() const override;
};


class 我妈 : public TransformSkillImpl<我妈> {
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
	std::wstring getPrompt() const override;
};


class 曼巴 : public TransformSkillImpl<曼巴> {
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
	std::wstring getPrompt() const override;
};

class 摘罩 : public InstantSkillImpl<摘罩> {
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


class 还击 : public InstantSkillImpl<还击> {
public:
	还击() : InstantSkillImpl<还击>(
		"还击",
		"限定技，出牌阶段，随机获得一名其他角色半数手牌（向上取整，至少保留一张），然后交还给其等量张牌。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 舞爪 : public InstantSkillImpl<舞爪> {
public:
	舞爪() : InstantSkillImpl<舞爪>(
		"舞爪",
		"限定技，出牌阶段，你可以将手牌摸至十张并回复两倍摸牌数点体力，然后本局【暗忍】移除失去体力的效果。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 四霸 : public InstantSkillImpl<四霸> {
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

class 装币 : public InstantSkillImpl<装币> {
public:
	装币() : InstantSkillImpl<装币>(
		"装币",
		"限定技，出牌阶段，你可以将手牌摸至十八张，并令你本局造成伤害倍率+1。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 幽愈 : public InstantSkillImpl<幽愈> {
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
class 炫技 : public InstantSkillImpl<炫技> {
public:
	炫技() : InstantSkillImpl<炫技>(
		"炫技",
		"每回合限一次，出牌阶段，你可以从牌堆顶或牌堆底摸两张牌，然后将其中一张置于牌堆底。\n"
		"若你因此获得的牌颜色相同，可弃置两张牌并令【加速】本局可发动次数+1。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//调羹：每局限三次，摸一张，展示数字牌给其他角色并回复其分值体力
class 调羹 : public InstantSkillImpl<调羹> {
public:
	调羹() : InstantSkillImpl<调羹>(
		"调羹",
		"每局游戏限三次，出牌阶段，你可以摸一张牌，展示一张数字牌令一名角色获得之，\n"
		"并令其回复此牌分值点体力。",
		3, unlimited,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};

//挥金：每种类型限一次，出牌阶段，若手牌数≥3，可将一张黄色手牌交给一名其他角色
class 挥金 : public InstantSkillImpl<挥金> {
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
class 再生 : public InstantSkillImpl<再生> {
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
class 手枪 : public InstantSkillImpl<手枪> {
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
class 芜湖 : public InstantSkillImpl<芜湖> {
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

class 引力 : public InstantSkillImpl<引力> {
private:
	opt_ref<Player> target = std::nullopt;
public:
	引力() : InstantSkillImpl<引力>(
		"引力",
		"每局限0次，出牌阶段，你可以选择一名角色，本轮数字牌进入弃牌堆后，该角色获得之。",
		0, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};

// ==================== 切斯特衍生技 ====================
//硬糖：将功能牌当作同色【封禁】打出
class 硬糖 : public TransformSkillImpl<硬糖> {
public:
	硬糖() : TransformSkillImpl<硬糖>(
		"硬糖", "你可将功能牌当做同色【封禁】打出。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	std::wstring getPrompt() const override;
};

//甘草：每回合限一次，观看一名角色所有非数字牌，可弃置其中一张
class 甘草 : public InstantSkillImpl<甘草> {
public:
	甘草() : InstantSkillImpl<甘草>(
		"甘草", "每回合限一次，出牌阶段，你可以观看一名角色所有非数字牌，并可弃置其中一张。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//跳糖：每回合限一次，与一名角色拼点，胜者获得一张切斯特指定颜色的【+2】
class 跳糖 : public InstantSkillImpl<跳糖> {
public:
	跳糖() : InstantSkillImpl<跳糖>(
		"跳糖", "每回合限一次，出牌阶段，你可以与一名角色拼点，胜者获得一张你指定颜色的【+2】。",
		unlimited, 1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

//肘击：限定技，出牌阶段，摸两张牌并获得两张【变色】
class 肘击 : public InstantSkillImpl<肘击> {
public:
	肘击() : InstantSkillImpl<肘击>(
		"肘击",
		"限定技，出牌阶段，你可以摸两张牌并获得两张【变色】。",
		1, unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

