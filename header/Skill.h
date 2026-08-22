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

class Player;
#include "Card.h"
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
			if (cards.value().size() != 1)
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
	using Factory = std::function<std::unique_ptr<ASkill>()>;
	ASkill(const std::string& _name, const std::string& _info, const limit_t& _limit);
};

// CRTP 基类
template<class Derived>
class PSkillImpl : public PSkill {
public:
	static std::unique_ptr<PSkill> make();
protected:
	using PSkill::PSkill;
};

// CRTP 基类
template<class Derived>
class ASkillImpl : public ASkill {
public:
	static std::unique_ptr<ASkill> make();
protected:
	using ASkill::ASkill;
};

template<class Derived>
std::unique_ptr<PSkill> PSkillImpl<Derived>::make() {
	return std::make_unique<Derived>();
}

template<class Derived>
std::unique_ptr<ASkill> ASkillImpl<Derived>::make() {
	return std::make_unique<Derived>();
}



class 粪怒 : public PSkillImpl<粪怒> {
public:
	粪怒() : PSkillImpl<粪怒>(
		"粪怒",
		"限定技，一名角色手牌数变为1时，你可令其摸你手牌数张牌（至多摸五张）。",
		1, false,
		TriggerPlayer::others,
		TriggerTime::lose_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 隐身 : public PSkillImpl<隐身> {
public:
	隐身() : PSkillImpl<隐身>(
		"隐身",
		"当你成为【+2】/【+4】的目标时，改为你的下家摸1张牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 顶置 : public PSkillImpl<顶置> {
public:
	顶置() : PSkillImpl<顶置>(
		"顶置",
		"锁定技，回合开始时，你观看牌堆底一张牌，然后可将此牌置于牌堆顶。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 带派 : public PSkillImpl<带派> {
public:
	带派() : PSkillImpl<带派>(
		"带派",
		"每局游戏开始时，选择获得一张【变色】或【+4】；也可选择两张均获得并失去25点体力。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 寒魄 : public PSkillImpl<寒魄> {
public:
	寒魄() : PSkillImpl<寒魄>(
		"寒魄",
		"限定技，你打出牌后，若你手牌数为1，你可令最后一张手牌变为你打出的牌。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 割腕 : public PSkillImpl<割腕> {
public:
	割腕() : PSkillImpl<割腕>(
		"割腕",
		"锁定技，你打出红色牌后失去随机1~5点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 丑皇 : public PSkillImpl<丑皇> {
public:
	丑皇() : PSkillImpl<丑皇>(
		"丑皇",
		"锁定技，你打出万能牌后，选择一项：\n1.回复10点体力；\n2.弃置一张非数字牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 军国 : public PSkillImpl<军国> {
public:
	军国() : PSkillImpl<军国>(
		"军国",
		"锁定技，当一名角色被封禁时，其失去1%最大体力（向上取整），自己被封禁改为失去1体力。",
		unlimited, true,
		TriggerPlayer::anybody,
		TriggerTime::ban_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 家暴 : public PSkillImpl<家暴> {
public:
	家暴() : PSkillImpl<家暴>(
		"家暴",
		"限定技，回合开始时，你可以令一名体力小于你的角色失去10%最大体力（向上取整）。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 健身 : public PSkillImpl<健身> {
public:
	健身() : PSkillImpl<健身>(
		"健身",
		"锁定技，每局游戏结束时，回复5点体力。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 做题 : public PSkillImpl<做题> {
public:
	做题() : PSkillImpl<做题>(
		"做题",
		"限定技，你打出数字牌/非数字牌后，可弃置一张非数字牌/数字牌。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 棍击 : public PSkillImpl<棍击> {
public:
	棍击() : PSkillImpl<棍击>(
		"棍击",
		"每局限9次，当你打出万能牌后，你可以对一名其他角色造成2^X点伤害（X为本局此技能发动次数）。",
		9, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 神木 : public PSkillImpl<神木> {
public:
	神木() : PSkillImpl<神木>(
		"神木",
		"锁定技，游戏开始时，从游戏外将九张【变色】和九张【+4】加入牌堆。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 雷剑 : public PSkillImpl<雷剑> {
public:
	雷剑() : PSkillImpl<雷剑>(
		"雷剑",
		"你打出【反转】后，可弃置一张同色数字牌并回复此牌分值点体力。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 买棋 : public PSkillImpl<买棋> {
public:
	买棋() : PSkillImpl<买棋>(
		"买棋",
		"回合开始时，你可以失去10X点体力（X为此技能发动次数-1），从游戏外获得随机一张万能牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 卖棋 : public PSkillImpl<卖棋> {
public:
	卖棋() : PSkillImpl<卖棋>(
		"卖棋",
		"回合结束时，你可以弃置一张万能牌，回复10点体力。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 耐克 : public PSkillImpl<耐克> {
public:
	耐克() : PSkillImpl<耐克>(
		"耐克",
		"锁定技，若上一张牌为蓝色或万能牌，【封禁】、【+2】和【+4】对你无效",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 轰炸 : public PSkillImpl<轰炸> {
public:
	轰炸() : PSkillImpl<轰炸>(
		"轰炸",
		"锁定技，当你打出【+2】/【+4】时，目标失去2%/4%最大体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 爆破 : public PSkillImpl<爆破> {
public:
	爆破() : PSkillImpl<爆破>(
		"爆破",
		"你对其他角色造成封禁效果后，可随机获得其一张牌并对其造成此牌分值点伤害。",
		unlimited, false,
		TriggerPlayer::others,
		TriggerTime::ban_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 电音 : public PSkillImpl<电音> {
public:
	电音() : PSkillImpl<电音>(
		"电音",
		"每局游戏限十次，回合开始时，你可以令手牌中所有数字牌变成随机数字并回复1点体力。",
		10, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 蒙面 : public PSkillImpl<蒙面> {
public:
	蒙面() : PSkillImpl<蒙面>(
		"蒙面",
		"锁定技，当你失去体力时，失去体力的数值减少25% （向下取整）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::damage_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 锐刻 : public PSkillImpl<锐刻> {
	bool disabled = false;
public:
	锐刻() : PSkillImpl<锐刻>(
		"锐刻",
		"当你打出【5】时，你可以令一名角色摸1张牌；你可改为令其摸5张牌并失去此技能（至本局结束）。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override { PSkill::reset(); disabled = false; }
};

class 巨富 : public PSkillImpl<巨富> {
public:
	巨富() : PSkillImpl<巨富>(
		"巨富",
		"锁定技，游戏开始时，你的初始手牌改为十二张；你执行摸牌阶段额外摸一张牌。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 破产 : public PSkillImpl<破产> {
public:
	破产() : PSkillImpl<破产>(
		"破产",
		"锁定技，若你打出牌后手牌数全场最多，随机弃置一张牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 假酒 : public PSkillImpl<假酒> {
public:
	假酒() : PSkillImpl<假酒>(
		"假酒",
		"你打出功能牌后，可随机获得一张不同颜色的非数字牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 窃观 : public PSkillImpl<窃观> {
public:
	窃观() : PSkillImpl<窃观>(
		"窃观",
		"锁定技，其他角色从牌堆一次性获得一张牌时，你得知其颜色牌名。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::draw_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 生存 : public PSkillImpl<生存> {
public:
	生存() : PSkillImpl<生存>(
		"生存",
		"锁定技，回合开始时，选择一张手牌变为随机颜色的【X】\n（X为此技能本局发动次数，至多为9）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 创造 : public PSkillImpl<创造> {
	std::unordered_set<Card::Color> usedColors;
public:
	创造() : PSkillImpl<创造>(
		"创造",
		"每局每种颜色限一次，你打出【9】后，可从游戏外获得一张同色的任意牌名的牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override { PSkill::reset(); usedColors.clear(); }
};

class 炼兵 : public PSkillImpl<炼兵> {
	std::unordered_set<Card::Name> usedNames;
	std::map<Card::Name, std::size_t> buildPairs(Player& carrier) const;
public:
	炼兵() : PSkillImpl<炼兵>(
		"炼兵",
		"每种牌名限一次，回合开始时，你可以弃置2张同名牌，从游戏外获得一张随机颜色的【+2】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override { PSkill::reset(); usedNames.clear(); }
};
class 好火 : public PSkillImpl<好火> {
	std::unordered_set<std::size_t> usedPlayerIds;
public:
	好火() : PSkillImpl<好火>(
		"好火",
		"每名角色限一次，当其他角色打出红色牌后，若其体力值大于你，你可以交给其一张手牌。",
		unlimited, false,
		TriggerPlayer::others,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override { PSkill::reset(); usedPlayerIds.clear(); }
};

class 森罗 : public PSkillImpl<森罗> {
public:
	森罗() : PSkillImpl<森罗>(
		"森罗",
		"锁定技，游戏开始时，将手中所有非黑色牌变为绿色。",
		1, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool filter(const Trigger& trigger) const override;;
	bool content(Trigger& trigger) override;
};
class 大脚 : public PSkillImpl<大脚> {
public:
	大脚() : PSkillImpl<大脚>(
		"大脚",
		"回合开始时，可弃置一张万能牌并发动一次【森罗】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 过江 : public PSkillImpl<过江> {
public:
	过江() : PSkillImpl<过江>(
		"过江",
		"锁定技，当你成为【+2】的目标时，来源摸2张牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 大盏 : public PSkillImpl<大盏> {
	static void randomEnlarge(Card& c);
public:
	大盏() : PSkillImpl<大盏>(
		"大盏",
		"锁定技，回合开始时，若你有数字牌，其中点数最小的牌均随机变大（至多变至9）。\n"
		"若全为【9】，可将其中一张变为红色并回复1点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 举报 : public PSkillImpl<举报> {
public:
	举报() : PSkillImpl<举报>(
		"举报",
		"契定技，当一名角色打出万能牌后，你可令其失去10%当前体力。",
		unlimited, false,
		TriggerPlayer::anybody,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override;
};
class 猥琐 : public PSkillImpl<猥琐> {
public:
	猥琐() : PSkillImpl<猥琐>(
		"猥琐",
		"锁定技，回合结束时，若你不是全场体力最高的角色，回复1点体力。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 棋王 : public PSkillImpl<棋王> {
public:
	棋王() : PSkillImpl<棋王>(
		"棋王",
		"当你打出弃牌堆顶同色同名牌后，你可以弃置两张手牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 金铲 : public PSkillImpl<金铲> {
public:
	金铲() : PSkillImpl<金铲>(
		"金铲",
		"锁定技，当其他角色回复1体力时，改为其失去1体力。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::recover_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 淘汰 : public PSkillImpl<淘汰> {
public:
	淘汰() : PSkillImpl<淘汰>(
		"淘汰",
		"每局游戏共限五次：\n"
		"你打出数字牌后，可弃置一张点数小于等于该牌一半（向下取整）的同色数字牌；\n"
		"你打出功能牌后，可从游戏外随机获得一张同色的功能牌。",
		5, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 光合 : public PSkillImpl<光合> {
public:
	光合() : PSkillImpl<光合>(
		"光合",
		"当你成为封禁类功能牌的目标时，你可判定：\n"
		"若结果为数字牌，来源摸一张牌；\n"
		"为功能牌，解除此牌封禁状态。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::card_target_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 射门 : public PSkillImpl<射门> {
public:
	射门() : PSkillImpl<射门>(
		"射门",
		"你打出数字牌后，可令一名角色进行判定，若结果为蓝色/黑色，其摸1张牌；"
		"若判定角色为你，改为弃置一张牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 招待 : public PSkillImpl<招待> {
public:
	招待() : PSkillImpl<招待>(
		"招待",
		"限定技，回合开始时，你可以将一张牌交给一名其他角色。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 追番 : public PSkillImpl<追番> {
public:
	追番() : PSkillImpl<追番>(
		"追番",
		"回合开始时，你可以将一张点数≤5的数字牌点数+1~3点。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 崩三 : public PSkillImpl<崩三> {
	mutable std::size_t count3 = 0; //累计打出的【3】次数
public:
	崩三() : PSkillImpl<崩三>(
		"崩三",
		"你每累计打出两张【3】后，可弃置一张【6】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override { PSkill::reset(); count3 = 0; }
};


class 望日 : public PSkillImpl<望日> {
public:
	望日() : PSkillImpl<望日>(
		"望日",
		"锁定技，回合开始时，令一张黄色牌点数+1（至多为9）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};
class 慈父 : public PSkillImpl<慈父> {
public:
	慈父() : PSkillImpl<慈父>(
		"慈父",
		"你打出黄色【9】后，可以获得一张【+4】。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 朔日 : public PSkillImpl<朔日> {
public:
	朔日() : PSkillImpl<朔日>(
		"朔日",
		"你打出黄色牌后，回复1点体力并可选择一项：\n"
		"1.将一张数字牌变为黄色且可令其点数+1/-1；\n"
		"2.将一张功能牌变为黄色的随机功能牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 徒步 : public PSkillImpl<徒步> {
public:
	徒步() : PSkillImpl<徒步>(
		"徒步",
		"契定技，你打出非数字牌后，可失去X点体力并重铸一张手牌（X为此技能本局发动次数）。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override { PSkill::reset(); setForced(false); }
};

class 健忘 : public PSkillImpl<健忘> {
public:
	健忘() : PSkillImpl<健忘>(
		"健忘",
		"每局游戏限两次，回合开始时，你可以任意更改当前的公共颜色。",
		2, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 豪赌 : public PSkillImpl<豪赌> {
public:
	豪赌() : PSkillImpl<豪赌>(
		"豪赌",
		"回合开始时，可进行一次判定：\n"
		"绿色或黑色，你获得之；\n"
		"黄色，你跳过此回合；\n"
		"蓝色，你重铸一张手牌；\n"
		"红色，你失去5点体力。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};


class 黑帮 : public PSkillImpl<黑帮> {
public:
	黑帮() : PSkillImpl<黑帮>(
		"黑帮",
		"锁定技，游戏开始时，你随机获得X张万能牌（X为当前局数）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 拖拉 : public PSkillImpl<拖拉> {
public:
	拖拉() : PSkillImpl<拖拉>(
		"拖拉",
		"锁定技，其他角色打出万能牌后，你弃置手中所有此牌名的牌并回复弃牌数点体力。",
		unlimited, true,
		TriggerPlayer::others,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};


class 互质 : public PSkillImpl<互质> {
	// 判断两个整数是否互质
	static bool areCoprime(const int a, const int b);
	// 判断 vector 中的所有整数是否两两互质
	static bool isPairwiseCoprime(const std::vector<int>& nums);
public:
	互质() : PSkillImpl<互质>(
		"互质",
		"锁定技，回合结束时，若你手中数字牌点数两两互质，你失去X点体力\n"
		"（X为你手中数字牌点数之积）。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::phase_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};


//难题子技能：回合开始时变牌
class 难题_变牌 : public PSkillImpl<难题_变牌> {
	std::shared_ptr<std::vector<Card::Name>> record;
public:
	难题_变牌(std::shared_ptr<std::vector<Card::Name>> _record)
		: PSkillImpl<难题_变牌>(
			"难题_变牌",
			"回合开始时，你可将一张非万能牌变为随机已记录点数的同色数字牌。",
			unlimited, true,
			TriggerPlayer::self,
			TriggerTime::phase_begin
		), record(std::move(_record)) {}

	static std::unique_ptr<PSkill> makeWith(std::shared_ptr<std::vector<Card::Name>> r) {
		return std::make_unique<难题_变牌>(std::move(r));
	}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

//难题主技能：摸牌时记录数字
class 难题 : public PSkillImpl<难题> {
	std::shared_ptr<std::vector<Card::Name>> record;
public:
	难题() : 难题(std::make_shared<std::vector<Card::Name>>()) {}

	难题(std::shared_ptr<std::vector<Card::Name>> _record)
		: PSkillImpl<难题>(
			"难题",
			"你于摸牌阶段获得数字牌时，若点数未记录，记录之。\n"
			"回合开始时，你可将一张非万能牌变为随机已记录点数的同色数字牌。",
			unlimited, true,
			TriggerPlayer::self,
			TriggerTime::phase_draw_end,
			难题_变牌::makeWith(_record)
		), record(_record) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 迷烟 : public PSkillImpl<迷烟> {
public:
	迷烟() : PSkillImpl<迷烟>(
		"迷烟",
		"摸牌阶段结束时，你可展示此阶段摸到的牌，"
		"令一名其他角色选择弃置一张万能牌或与你展示牌颜色相同的手牌，否则其摸一张牌。",
		unlimited, false,
		TriggerPlayer::self,
		TriggerTime::phase_draw_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 创世 : public PSkillImpl<创世> {
public:
	创世() : PSkillImpl<创世>(
		"创世",
		"游戏开始时，你可选择一个你手中没有的牌名，将一张手牌变为此牌名的牌（颜色自选）。",
		1, false,
		TriggerPlayer::self,
		TriggerTime::game_begin
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
};

class 补天 : public PSkillImpl<补天> {
	std::vector<Card::Name> record;
public:
	补天() : PSkillImpl<补天>(
		"补天",
		"锁定技，每种牌名限一次，你打出手中唯一一种牌名的牌后，记录其牌名，然后选择一张手牌变为随机一张未记录的牌。",
		unlimited, true,
		TriggerPlayer::self,
		TriggerTime::use_card_end
	) {}
	bool filter(const Trigger& trigger) const override;
	bool content(Trigger& trigger) override;
	void reset() override;
};










