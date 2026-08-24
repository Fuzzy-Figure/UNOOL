#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <memory>
#include "utils.h"
#include "Card.h"

class Player;
class GameLogic;

class Skill {
protected:
	using limit_t = std::optional<std::size_t>;
	std::string name = "未知技能";
	std::string info = "无";
	limit_t limit; //每局使用限制次数，std::nullopt代表无次数限制
	std::size_t count = 0; //使用次数
public:
	inline static const auto unlimited = std::nullopt;

	std::string getName() const { return name; }
	std::wstring getNameW() const { return unool::string::to_utf16(name); }
	std::string getInfo() const { return info; }
	std::wstring getInfoW() const { return unool::string::to_utf16(info); }
	std::size_t getCount() const { return count; }

	Skill(const std::string& _name, const std::string& _info, const limit_t& _limit);
	virtual ~Skill() = default;
	virtual void reset();
};

class PSkill : public Skill {
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
		damage_begin, damage_end,
		recover_begin, recover_end,
		draw_begin, draw_end,
		gain_card_begin, gain_card_end,
		discard_begin, discard_end,
		recast_begin, recast_end,
		ban_begin, ban_end,
		judge_begin, judge_end,
		decree_begin, decree_end
	};
	struct Trigger {
	private:
		ref<GameLogic> game;
		ref<Player> carrier;
		opt_ref<Player> player = std::nullopt;
		std::optional<std::vector<ref<Card>>> cards = std::nullopt;
		opt_ref<Player> source = std::nullopt;
		opt_ref<std::size_t> number = std::nullopt;
		std::size_t count = 0;

	public:
		Trigger(GameLogic& _game, Player& _carrier,
				opt_ref<Player> _player,
				std::optional<std::vector<ref<Card>>> _cards,
				opt_ref<Player> _source,
				opt_ref<std::size_t> _number);

		bool hasPlayer() const { return player.has_value(); }
		bool hasCards() const { return cards.has_value(); }
		bool hasSource() const { return source.has_value(); }
		bool hasNumber() const { return number.has_value(); }

		GameLogic& getGame() const { return game.get(); }
		Player& getCarrier() const { return carrier.get(); }
		Player& getPlayer() const { return player.value().get(); }
		Card& getCard() const {
			if (cards.value().size() > 1)
				std::cout << "[警告] 在cards含有多于一张牌的情况下调用getCard" << std::endl;
			return cards.value().front().get();
		}
		std::vector<ref<Card>> getCards() const { return cards.value(); }
		Player& getSource() const { return source.value().get(); }
		std::size_t& getNumber() const { return number.value().get(); }
		std::size_t getCount() const { return count; }

		void setCount(const std::size_t _count) { count = _count; }
	};
	using Factory = std::function<std::unique_ptr<PSkill>()>;

	virtual bool filter(const Trigger& trigger) const = 0;
	virtual bool content(Trigger& trigger) = 0;

	//无子技能
	PSkill(const std::string& name, const std::string& description,
		   const limit_t& limit, bool forced,
		   const TriggerPlayer& triggerPlayer,
		   const TriggerTime& triggerTime);

	//有子技能
	template<typename... SubSkills>
		requires (std::same_as<std::decay_t<SubSkills>, std::unique_ptr<PSkill>> && ...)
	PSkill(const std::string& _name, const std::string& _description,
		   const limit_t& _limit, bool _forced,
		   const TriggerPlayer& _triggerPlayer,
		   const TriggerTime& _triggerTime,
		   SubSkills&&... _subSkills)
		: PSkill(_name, _description, _limit, _forced, _triggerPlayer, _triggerTime) {
		(subSkills.push_back(std::forward<SubSkills>(_subSkills)), ...);
	}
	bool matchTrigger(const TriggerTime& currentTriggerTime, const Trigger& trigger) const;
	void launch(Trigger& trigger);
	void reset() override;
	void setForced(const bool newForced);

	std::vector<std::unique_ptr<PSkill>> subSkills;
private:
	TriggerPlayer triggerPlayer;
	TriggerTime triggerTime;
	bool forced = false;
};

class ASkill : public Skill {
public:
	//主动技发动时机
	enum class TriggerTime {
		never,       //永不触发
		phase_use1,  //出牌阶段1
		phase_use2,  //出牌阶段2
		phase_use,   //出牌阶段（1和2均可）
	};

	ASkill(const std::string& _name, const std::string& _info, const limit_t& _limit, TriggerTime _triggerTime = TriggerTime::never);

	//是否还能发动（次数限制检查）
	bool canUse() const { return !limit.has_value() || count < limit.value(); }
	//判断当前阶段能否发动
	bool canTriggerAt(TriggerTime currentPhase) const {
		if (triggerTime == TriggerTime::never) return false;
		if (triggerTime == TriggerTime::phase_use) return currentPhase == TriggerTime::phase_use1 || currentPhase == TriggerTime::phase_use2;
		return triggerTime == currentPhase;
	}
private:
	TriggerTime triggerTime;
};

//即时型主动技抽象接口：按数字键直接发动
class ASkillInstantBase : public ASkill {
public:
	//执行发动；成功返回 true（基类内部负责 canUse 检查与 count 累加）
	virtual bool tryActivate(GameLogic& game, Player& player) = 0;
protected:
	using ASkill::ASkill;
};

//转换型主动技抽象接口：按数字键切换激活态，激活后影响选牌/出牌
class ASkillTransformBase : public ASkill {
public:
	//转化所需的牌数（默认1）
	virtual std::size_t getCardCount() const { return 1; }
	//某张牌能否被选入此次转化
	virtual bool canSelect(const Card& c) const = 0;
	//执行转化（cards 大小 == getCardCount()）
	virtual void transform(std::vector<ref<Card>> cards) const = 0;
	//激活时右侧显示的提示文字
	virtual std::wstring getPrompt() const = 0;
protected:
	using ASkill::ASkill;
};

// CRTP 基类
template<class Derived> class PSkillImpl : public PSkill {
public:
	static std::unique_ptr<PSkill> make();
protected:
	using PSkill::PSkill;
};
template<class Derived> std::unique_ptr<PSkill> PSkillImpl<Derived>::make() {
	return std::make_unique<Derived>();
}

//即时型 CRTP 层：提供 make() 并实现 tryActivate 模板
template<class Derived>
class ASkillInstant : public ASkillInstantBase {
public:
	static std::unique_ptr<ASkillInstantBase> make() { return std::make_unique<Derived>(); }
	bool tryActivate(GameLogic& game, Player& player) final {
		if (!canUse()) return false;
		if (!content(game, player)) return false;
		++count;
		return true;
	}
protected:
	using ASkillInstantBase::ASkillInstantBase;
	//具体技能效果，派生类实现
	virtual bool content(GameLogic& game, Player& player) = 0;
};

//转换型 CRTP 层：提供 make()
template<class Derived>
class ASkillTransform : public ASkillTransformBase {
public:
	static std::unique_ptr<ASkillTransformBase> make() { return std::make_unique<Derived>(); }
protected:
	using ASkillTransformBase::ASkillTransformBase;
};


