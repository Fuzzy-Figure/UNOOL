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

//主动技：八爪 - 你可以将一张数字牌当作蓝色的8打出（转换型）
class 八爪 : public ASkillTransform<八爪> {
public:
	八爪() : ASkillTransform<八爪>(
		"八爪",
		"你可以将一张数字牌当作蓝8打出。",
		unlimited,
		TriggerTime::phase_use
	) {}
	std::size_t  getCardCount() const override { return 1; }
	bool         canSelect(const Card& c) const override { return c.isNumber(); }
	void         transform(std::vector<ref<Card>> cards) const override;
	std::wstring getPrompt() const override { return L"将一张数字牌当作蓝8打出"; }
};
