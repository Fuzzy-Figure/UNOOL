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

