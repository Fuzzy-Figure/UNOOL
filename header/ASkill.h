#include "Skill.h"


//主动技：摸牌 - 出牌阶段1，你可以摸一张牌（即时型）
class 摸牌 : public ASkillInstant<摸牌> {
public:
	摸牌() : ASkillInstant<摸牌>(
		"摸牌",
		"出牌阶段1，你可以摸一张牌。",
		unlimited,
		TriggerTime::phase_use1
	) {}
	bool content(GameLogic& game, Player& player) override;
};

//主动技：八爪 - 你可以将一张数字牌当作蓝色的8打出（转换型）
class 八爪 : public ASkillTransform<八爪> {
public:
	八爪() : ASkillTransform<八爪>(
		"八爪",
		"你可以将一张数字牌当作蓝色的8打出。",
		unlimited,
		TriggerTime::phase_use
	) {}
	std::size_t  getCardCount() const override { return 1; }
	bool         canSelect(const Card& c) const override { return c.isNumber(); }
	void         transform(std::vector<ref<Card>> cards) const override;
	std::wstring getPrompt() const override { return L"将一张数字牌当作蓝8打出"; }
};
