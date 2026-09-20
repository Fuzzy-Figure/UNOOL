#pragma once
#include "Skill.h"

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