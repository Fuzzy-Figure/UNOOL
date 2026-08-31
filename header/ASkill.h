#include "Skill.h"

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
		"每局游戏限三次，出牌阶段，你可将非红色牌当作红【封禁】打出，然后摸一张牌。",
		3,
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
		"出牌阶段，你可将一张万能牌或【8】当作任意颜色的任意牌打出。",
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

