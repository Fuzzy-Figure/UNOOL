#include "Skill.h"

class 装弹 : public ASkillInstant<装弹> {
public:
	装弹() : ASkillInstant<装弹>(
		"装弹",
		"限定技，出牌阶段，若你手中没有封禁类牌且手牌数≤9，\n"
		"你可以将一半手牌变为随机的非数字牌（向下取整）。",
		1,
		TriggerTime::phase_use
	) {}
	bool filter(const GameLogic& game, const Player& carrier) const override;
	bool content(GameLogic& game, Player& carrier) override;
};
class 徒步 : public ASkillInstant<徒步> {
public:
	徒步() : ASkillInstant<徒步>(
		"徒步",
		"出牌阶段，你可以重铸一张牌并回复1点体力，若你重铸了数字牌，失去X点体力（X为此技能本局发动次数）。",
		unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};


class 招待 : public ASkillInstant<招待> {
public:
	招待() : ASkillInstant<招待>(
		"招待",
		"限定技，出牌阶段，你可以将一张牌交给一名其他角色。",
		1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

class 八爪 : public ASkillTransform<八爪> {
public:
	八爪() : ASkillTransform<八爪>(
		"八爪",
		"你可以将一张数字牌当作蓝【8】打出。",
		unlimited,
		TriggerTime::phase_use
	) {}
	std::size_t getCardCount() const override;
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	std::wstring getPrompt() const override;
};


class 我妈 : public ASkillTransform<我妈> {
public:
	我妈() : ASkillTransform<我妈>(
		"我妈",
		"每局游戏限两次，出牌阶段，你可将非红色牌当作红【封禁】打出，然后摸一张牌。",
		2,
		TriggerTime::phase_use
	) {}
	std::size_t getCardCount() const override;
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	void addition(GameLogic& game, Player& carrier) const override;
	std::wstring getPrompt() const override;
};


class 曼巴 : public ASkillTransform<曼巴> {
public:
	曼巴() : ASkillTransform<曼巴>(
		"曼巴",
		"出牌阶段，你可将一张【8】当作任意基础颜色的任意牌打出。",
		unlimited,
		TriggerTime::phase_use
	) {}
	std::size_t getCardCount() const override;
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	std::wstring getPrompt() const override;
};

class 摘罩 : public ASkillInstant<摘罩> {
	std::unordered_set<Card::Name> record;
public:
	摘罩() : ASkillInstant<摘罩>(
		"摘罩",
		"出牌阶段，你可以展示未展示过点数的数字牌，令一名其他角色展示相同点数的牌；\n"
		"若其未展示牌，你可任意更改此牌颜色。",
		unlimited,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
	void reset() override;
};


class 还击 : public ASkillInstant<还击> {
public:
	还击() : ASkillInstant<还击>(
		"还击",
		"限定技，出牌阶段，随机获得一名其他角色半数手牌（向上取整，至少保留一张），然后交还给其等量张牌。",
		1,
		TriggerTime::phase_use
	) {}
	bool content(GameLogic& game, Player& carrier) override;
};

