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

// ==================== 切斯特衍生技 ====================
//硬糖：将功能牌当作同色【封禁】打出
class 硬糖 final : public TransformSkillImpl<硬糖> {
public:
	硬糖() : TransformSkillImpl<硬糖>(
		"硬糖", "你可将功能牌当做同色【封禁】打出。",
		unlimited, unlimited,
		TriggerTime::phase_use
	) {}
	bool canSelect(const Card& c) const override;
	bool transform(GameLogic& game, Player& carrier, std::vector<ref<Card>> cards) const override;
	std::string getPrompt() const override;
};