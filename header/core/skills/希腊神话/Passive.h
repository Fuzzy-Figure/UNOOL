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

class 同心 final : public PassiveSkillImpl<同心> {
private:
	std::size_t numberCount = 0;
	std::size_t actionCount = 0;
	std::size_t wildCount = 0;
public:
	同心() : PassiveSkillImpl<同心>(
		"同心",
		"锁定技，每连续打出两张同类型牌后随机弃置一张牌。\n"
		"当前：数字{numberCount}/2；功能{actionCount}/2；万能{wildCount}/2",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<同心>::reset();
		numberCount = 0;
		actionCount = 0;
		wildCount = 0;
	}
	std::optional<std::string> extraPlaceholders(const std::string& key) const;
};

//蝎刺：锁定技，你打出有目标的牌后，对目标造成伤害。每次发动后本场游戏此技能造成伤害+1。
class 蝎刺 final : public PassiveSkillImpl<蝎刺> {
private:
	std::size_t damageValue = 1;
public:
	蝎刺() : PassiveSkillImpl<蝎刺>(
		"蝎刺",
		"锁定技，你打出有目标的牌后，对目标造成{damage}点伤害。\n"
		"每次发动此技能后，本场游戏此技能造成伤害+1。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	std::optional<std::string> extraPlaceholders(const std::string& key) const;
};

class 镇魂_回合结束 final : public PassiveSkillImpl<镇魂_回合结束> {
public:
	镇魂_回合结束() : PassiveSkillImpl<镇魂_回合结束>(
		"镇魂",
		"",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 镇魂 final : public PassiveSkillImpl<镇魂> {
	friend class 镇魂_回合结束;
private:
	std::size_t phaseCount = 0;
public:
	镇魂() : PassiveSkillImpl<镇魂>(
		"镇魂",
		"每回合限一次，回合开始或结束时，你可以选择一名角色，对其随机造成1-5点伤害，"
		"然后若你手牌数不小于其，其弃置一张牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin,
		镇魂_回合结束::make()
	) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

//坚甲：锁定技，每打出一张牌后获得1点护甲（上限50点）
class 坚甲 final : public PassiveSkillImpl<坚甲> {
public:
	坚甲() : PassiveSkillImpl<坚甲>(
		"坚甲",
		"锁定技，你每打出一张牌后获得1点护甲（上限50点）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

// ==================== 新增星座角色技能（希腊神话分组） ====================
// 共用口径：
//  · 紫色牌 = Card::Color::purple，只由 before_deal 子技能加入牌堆，不进标准牌堆；
//  · 每局重置的状态一律重写 reset() 清空（Skill::reset() 只清 count/limit）；
//  · 隐藏子技能一律用空 info，由父技能携带完整描述文字。

// ---------- 南十字座【静渊】 ----------
class 静渊_出牌 final : public PassiveSkillImpl<静渊_出牌> {
public:
	静渊_出牌() : PassiveSkillImpl<静渊_出牌>(
		"静渊", "", unlimited, true,
		TriggerPlayer::self, TriggerTime::use_card_end) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

class 静渊 final : public PassiveSkillImpl<静渊> {
	friend class 静渊_出牌;
private:
	//本局此技能对每名其他角色造成伤害的次数（0 点伤害也计数）
	std::unordered_map<std::size_t, std::size_t> damageCounts{};
public:
	静渊() : PassiveSkillImpl<静渊>(
		"静渊",
		"锁定技，游戏开始时，从牌堆获得你手中没有的蓝色牌各一张。\n"
		"你打出蓝色牌后，其他角色随机重铸一张蓝色牌，若没有蓝色牌改为你对其造成X点伤害（X为本局此技能对其造成伤害的次数）。",
		unlimited, true,
		TriggerPlayer::anybody, TriggerTime::game_begin,
		静渊_出牌::make()) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<静渊>::reset();
		damageCounts.clear();
	}
};

// ---------- 半人马座【怀海】 ----------
class 怀海 final : public PassiveSkillImpl<怀海> {
private:
	std::unordered_map<std::size_t, std::size_t> lostCounts{};
	std::size_t recoverCount = 0;
public:
	怀海() : PassiveSkillImpl<怀海>(
		"怀海",
		"锁定技，一名角色每失去两张蓝色牌后，其随机获得牌堆中一张蓝色牌，若牌堆已无此类牌改为你回复X点体力（X为本局此技能触发回复的次数）。",
		unlimited, true,
		TriggerPlayer::anybody, TriggerTime::lose_card_end) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<怀海>::reset();
		lostCounts.clear();
		recoverCount = 0;
	}
};

// ---------- 船底座【巨潮】 ----------
class 巨潮 final : public PassiveSkillImpl<巨潮> {
private:
	bool disabled = false;  //本局失效
public:
	巨潮() : PassiveSkillImpl<巨潮>(
		"巨潮",
		"锁定技，回合开始时，展示牌堆中随机三张蓝色牌，你选择获得其中一张并将其余交给一名其他角色；若展示的三张中有相同牌名，此技能本局失效。",
		unlimited, true,
		TriggerPlayer::self, TriggerTime::phase_begin) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<巨潮>::reset();
		disabled = false;
	}
};

// ---------- 鲸鱼座【鲸落】 ----------
class 鲸落_入堆 final : public PassiveSkillImpl<鲸落_入堆> {
public:
	鲸落_入堆() : PassiveSkillImpl<鲸落_入堆>(
		"鲸落", "", unlimited, true,
		TriggerPlayer::anybody, TriggerTime::before_deal) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 鲸落_出牌 final : public PassiveSkillImpl<鲸落_出牌> {
public:
	鲸落_出牌() : PassiveSkillImpl<鲸落_出牌>(
		"鲸落", "", unlimited, true,
		TriggerPlayer::self, TriggerTime::use_card_end) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 鲸落 final : public PassiveSkillImpl<鲸落> {
public:
	鲸落() : PassiveSkillImpl<鲸落>(
		"鲸落",
		"锁定技，每局发牌前，在牌堆中加入一套紫色卡牌；每局游戏开始时，你获得牌堆中每种牌名的紫色数字牌各一张。\n"
		"你打出非紫色牌后，可对一名其他角色造成X点伤害（X为你手中紫色牌数量）。",
		unlimited, true,
		TriggerPlayer::anybody, TriggerTime::game_begin,
		鲸落_入堆::make(), 鲸落_出牌::make()) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

// ---------- 天龙座【龙霖】 ----------
class 龙霖_入堆 final : public PassiveSkillImpl<龙霖_入堆> {
public:
	龙霖_入堆() : PassiveSkillImpl<龙霖_入堆>(
		"龙霖", "", unlimited, true,
		TriggerPlayer::anybody, TriggerTime::before_deal) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 龙霖_出牌 final : public PassiveSkillImpl<龙霖_出牌> {
public:
	龙霖_出牌() : PassiveSkillImpl<龙霖_出牌>(
		"龙霖", "", unlimited, true,
		TriggerPlayer::self, TriggerTime::use_card_end) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 龙霖 final : public PassiveSkillImpl<龙霖> {
	friend class 龙霖_出牌;
private:
	std::size_t purplePlayed = 0;
public:
	龙霖() : PassiveSkillImpl<龙霖>(
		"龙霖",
		"锁定技，每局发牌前，在牌堆中加入一套紫色卡牌；每局游戏开始时，你随机获得牌堆中两张紫色功能牌。\n"
		"你每打出两张紫色牌后，可选择一名角色，其手中随机两张非紫色牌变为紫色（优先万能牌）。",
		unlimited, true,
		TriggerPlayer::anybody, TriggerTime::game_begin,
		龙霖_入堆::make(), 龙霖_出牌::make()) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<龙霖>::reset();
		purplePlayed = 0;
	}
};

// ---------- 长蛇座【蟒川】 ----------
class 蟒川_入堆 final : public PassiveSkillImpl<蟒川_入堆> {
public:
	蟒川_入堆() : PassiveSkillImpl<蟒川_入堆>(
		"蟒川", "", unlimited, true,
		TriggerPlayer::anybody, TriggerTime::before_deal) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 蟒川_出牌 final : public PassiveSkillImpl<蟒川_出牌> {
public:
	蟒川_出牌() : PassiveSkillImpl<蟒川_出牌>(
		"蟒川", "", unlimited, true,
		TriggerPlayer::others, TriggerTime::use_card_end) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 蟒川 final : public PassiveSkillImpl<蟒川> {
public:
	蟒川() : PassiveSkillImpl<蟒川>(
		"蟒川",
		"锁定技，每局发牌前，在牌堆中加入一套紫色卡牌；每局游戏开始时，你将手中所有紫色牌交给一名其他角色。\n"
		"其他角色打出紫色牌后，你获得X点护盾（X为你手中紫色牌数量且至少为1），上限50点；若已达到上限，则改为你弃置一张牌。",
		unlimited, true,
		TriggerPlayer::anybody, TriggerTime::game_begin,
		蟒川_入堆::make(), 蟒川_出牌::make()) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};

// ---------- 仙王座【圣权】 ----------
class 圣权_布置 final : public PassiveSkillImpl<圣权_布置> {
public:
	圣权_布置() : PassiveSkillImpl<圣权_布置>(
		"圣权", "", unlimited, true,
		TriggerPlayer::anybody, TriggerTime::game_begin) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 圣权 final : public PassiveSkillImpl<圣权> {
private:
	std::unordered_set<Card::Type> playedTypes{};
public:
	圣权() : PassiveSkillImpl<圣权>(
		"圣权",
		"锁定技，其他角色不能打出你未打出过的类型的牌（含万能牌）。\n"
		"你首次打出一种类型的牌后，可决议至多三张牌。",
		unlimited, true,
		TriggerPlayer::self, TriggerTime::use_card_end,
		圣权_布置::make()) {}
	//供禁打钩子查询：该类型是否已被携带者打出过
	bool allows(const Card& card) const { return playedTypes.contains(card.getType()); }
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<圣权>::reset();
		playedTypes.clear();
	}
};

// ---------- 仙后座【圣柄】 ----------
class 圣柄_布置 final : public PassiveSkillImpl<圣柄_布置> {
public:
	圣柄_布置() : PassiveSkillImpl<圣柄_布置>(
		"圣柄", "", unlimited, true,
		TriggerPlayer::anybody, TriggerTime::game_begin) {}
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
};
class 圣柄 final : public PassiveSkillImpl<圣柄> {
private:
	std::unordered_set<Card::Color> playedColors{};
public:
	圣柄() : PassiveSkillImpl<圣柄>(
		"圣柄",
		"锁定技，其他角色不能打出你未打出过的颜色的牌（含黑色万能牌）。\n"
		"你首次打出一种颜色的牌后，可重铸至多三张牌。",
		unlimited, true,
		TriggerPlayer::self, TriggerTime::use_card_end,
		圣柄_布置::make()) {}
	bool allows(const Card& card) const { return playedColors.contains(card.getColor()); }
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<圣柄>::reset();
		playedColors.clear();
	}
};

// ---------- 猎户座【狩锋】 ----------
class 狩锋 final : public PassiveSkillImpl<狩锋> {
private:
	bool disabled = false;  //本局失效
public:
	狩锋() : PassiveSkillImpl<狩锋>(
		"狩锋",
		"你打出【+4】后，可重铸一半手牌（向下取整，至多八张），然后展示牌堆顶四张牌与你重铸的牌名进行匹配，若匹配牌名数为：\n"
		"0，此技能本局失效；1，你可选择展示牌中一张获得之；2，对一名其他角色造成10点伤害；3，回复10点体力；\n"
		"4，依次触发1-3全部效果并获得一张【+4】。",
		unlimited, false,
		TriggerPlayer::self, TriggerTime::use_card_end) {}
	bool filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const override;
	bool content(GameLogic& game, Player& carrier, Trigger& trigger) override;
	void reset() override {
		PassiveSkillImpl<狩锋>::reset();
		disabled = false;
	}
};

