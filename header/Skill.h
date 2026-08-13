#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "utils.h"

class Player;
#include "Card.h"
class GameLogic;

class Skill {
protected:
	std::string name = "未知技能";
	std::string info = "无";
	std::optional<std::size_t> limit; //每局使用限制次数，std::nullopt代表无次数限制
	std::size_t count = 0; //使用次数
public:
	inline static const auto unlimited = std::nullopt;
	std::string getName() const { return name; }
	std::wstring getNameW() const { return unool::string::to_utf16(name); }
	std::string getInfo() const { return info; }
	std::wstring getInfoW() const { return unool::string::to_utf16(info); }
	std::size_t getCount() const { return count; }
	Skill(const std::string& _name, const std::string& _info, const std::optional<std::size_t>& _limit);
	virtual ~Skill() = default;
	virtual void reset();
};

class PSkill :public Skill {
public:
	enum class TriggerPlayer {
		nobody, self, others, anybody
	};
	enum class TriggerTime {
		/*
		carrier：技能携带者

		时机          player(技能触发者)   card       source
		局
		回合          回合玩家
		出牌阶段1     出牌阶段1玩家
		摸牌阶段      摸牌阶段玩家       摸到的牌
		出牌阶段2     出牌阶段2玩家
		使用牌        使用牌的玩家       打出的牌
		失去牌        失去牌的玩家       失去的牌
		成为牌目标    成为牌目标的玩家      牌         牌来源
		失去体力      失去体力的玩家
		摸牌	          摸牌的玩家         摸到的牌
		弃牌	          弃牌的玩家         弃置的牌
		重铸          重铸牌的玩家        重铸的牌
		封禁          被封禁的玩家      封禁玩家的牌    来源
		决议          决议牌的玩家        决议的牌
		*/
		never,
		game_begin, game_end,
		phase_begin, phase_end,
		phase_use1_begin, phase_use1_end,
		phase_draw_begin, phase_draw_end,
		phase_use2_begin, phase_use2_end,
		use_card_begin, use_card_end,
		lose_card_begin, lose_card_end,
		card_target_begin, card_target_end,
		lose_hp_begin, lose_hp_end,
		draw_begin, draw_end,
		discard_begin, discard_end,
		recast_begin, recast_end,
		ban_begin, ban_end,
		resolution_begin, resolution_end
	};
	struct Trigger {
	private:
		ref<GameLogic> game;
		ref<Player> carrier;
		opt_ref<Player> player = std::nullopt;
		opt_ref<Card> card = std::nullopt;
		opt_ref<Player> source = std::nullopt;
		opt_ref<std::size_t> number = std::nullopt;
		std::size_t count = 0;

	public:
		Trigger(GameLogic& _game, Player& _carrier,
				opt_ref<Player> _player,
				opt_ref<Card> _card,
				opt_ref<Player> _source,
				opt_ref<std::size_t> _number);

		bool has_player() const { return player.has_value(); }
		bool has_card() const { return card.has_value(); }
		bool has_source() const { return source.has_value(); }
		bool has_number() const { return number.has_value(); }

		GameLogic& get_game() const { return game.get(); }
		Player& get_carrier() const { return carrier.get(); }
		Player& get_player() const { return player.value().get(); }
		Card& get_card() const { return card.value().get(); }
		Player& get_source() const { return source.value().get(); }
		std::size_t& get_number() const { return number.value().get(); }
		std::size_t get_count() const { return count; }

		void set_count(const std::size_t _count) { count = _count; }
	};
	using Factory = std::function<std::unique_ptr<PSkill>()>;
	static const std::unordered_map<std::string, Factory> registry;

private:
	using Filter = std::function<bool(const Trigger& trigger)>;
	using Content = std::function<void(Trigger& trigger)>;

	static const std::unordered_map<std::string, std::string> descriptions;

	TriggerPlayer triggerPlayer;
	TriggerTime triggerTime;
	Filter filter;
	Content content;
	bool forced = false;
public:
	std::shared_ptr<bool> disabledFlag; //用于"失去技能至本局结束"机制
	std::shared_ptr<std::unordered_set<Card::Color>> usedColorsFlag;
	std::shared_ptr<std::unordered_set<std::size_t>> usedPlayerIdsFlag;
	PSkill(const std::string& _name,
		   const std::optional<std::size_t> _limit, const bool _forced,
		   const TriggerPlayer& _triggerPlayer, const TriggerTime& _triggerTime,
		   const Filter& _filter, const Content& _content);
	bool matchTrigger(const TriggerTime& currentTriggerTime,
					  const Trigger& trigger) const;
	void launch(Trigger& trigger);
	void reset() override;

	static std::unique_ptr<PSkill> 粪怒();
	static std::unique_ptr<PSkill> 隐身();
	static std::unique_ptr<PSkill> 顶置();
	static std::unique_ptr<PSkill> 带派();
	static std::unique_ptr<PSkill> 寒魄();
	static std::unique_ptr<PSkill> 割腕();
	static std::unique_ptr<PSkill> 丑皇();
	static std::unique_ptr<PSkill> 军国();
	static std::unique_ptr<PSkill> 家暴();
	static std::unique_ptr<PSkill> 健身();
	static std::unique_ptr<PSkill> 做题();
	static std::unique_ptr<PSkill> 棍击();
	static std::unique_ptr<PSkill> 神木();
	static std::unique_ptr<PSkill> 雷剑();
	static std::unique_ptr<PSkill> 买棋();
	static std::unique_ptr<PSkill> 卖棋();
	static std::unique_ptr<PSkill> 耐克();
	static std::unique_ptr<PSkill> 轰炸();
	static std::unique_ptr<PSkill> 爆破();
	static std::unique_ptr<PSkill> 电音();
	static std::unique_ptr<PSkill> 蒙面();
	static std::unique_ptr<PSkill> 锐刻();
	static std::unique_ptr<PSkill> 巨富();
	static std::unique_ptr<PSkill> 破产();
	static std::unique_ptr<PSkill> 假酒();
	static std::unique_ptr<PSkill> 窃观();
	static std::unique_ptr<PSkill> 生存();
	static std::unique_ptr<PSkill> 创造();
	static std::unique_ptr<PSkill> 炼兵();
	static std::unique_ptr<PSkill> 好火();

	static std::wstring getInfoW(const std::string& skillName);
};

class ASkill :public Skill {
public:
	ASkill(const std::string& _name, const std::string& _info, const std::optional<std::size_t>& _limit);

	using Factory = std::function<std::unique_ptr<ASkill>()>;
	static const std::unordered_map<std::string, Factory> registry;
	static std::wstring getInfoW(const std::string& skillName);
};