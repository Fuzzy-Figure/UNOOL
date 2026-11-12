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

