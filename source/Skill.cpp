#include "../header/Skill.h"
#include "../header/Player.h"
#include "../header/GameLogic.h"
#include "../header/utils.h"
#include <iostream>
#include <cmath>
#include <set>

Skill::Skill(const std::string& _name, const std::string& _info, const limit_t& _limit)
	:name(_name), info(_info), limit(_limit) {}

void Skill::reset() {
	count = 0;
}




// **********************
//         被动技
// **********************


//无子技能
PSkill::PSkill(const std::string& name, const std::string& description,
			   const limit_t& limit, bool forced,
			   const TriggerPlayer& triggerPlayer,
			   const TriggerTime& triggerTime)
	: Skill(name, description, limit),
	forced(forced),
	triggerPlayer(triggerPlayer),
	triggerTime(triggerTime) {}

bool PSkill::matchTrigger(const TriggerTime& currentTriggerTime,
						  const Trigger& trigger) const {
	return triggerTime == currentTriggerTime && (
		triggerTime == TriggerTime::game_begin ||
		triggerTime == TriggerTime::game_end ||
		triggerPlayer == TriggerPlayer::anybody ||
		(triggerPlayer == TriggerPlayer::self && trigger.getCarrier() == trigger.getPlayer()) ||
		(triggerPlayer == TriggerPlayer::others && trigger.getCarrier() != trigger.getPlayer())
		);
}

void PSkill::launch(Trigger& trigger) {
	//不满足条件，或达到次数限制：不发动
	if ((limit != unlimited && count >= limit) || !filter(trigger)) return;
	//如果不是锁定技，询问玩家是否发动
	if (!forced) {
		const std::size_t choice = trigger.getCarrier().ask(
			L"是否发动 [" + getNameW() + L"]？",
			{ L"发动", L"不发动" },
			true
		);
		if (choice == 2) return;
	}
	//发动技能
	count += 1;
	trigger.setCount(count);

	if (!content(trigger)) {
		count -= 1;
		return;
	}
	std::cout << "<技能> " << trigger.getCarrier().characterName() << "发动了" << name << "！" << std::endl;
}

void PSkill::reset() {
	Skill::reset();
}

void PSkill::setForced(const bool newForced) {
	forced = newForced;
}



PSkill::Trigger::Trigger(GameLogic& _game, Player& _carrier,
						 opt_ref<Player> _player,
						 std::optional<std::vector<ref<Card>>> _cards,
						 opt_ref<Player> _source,
						 opt_ref<std::size_t> _number)
	:game(_game), carrier(_carrier), player(_player),
	cards(_cards), source(_source), number(_number) {}


// **********************
//         主动技
// **********************
ASkill::ASkill(const std::string& _name, const std::string& _info, const limit_t& _limit, TriggerTime _triggerTime)
	:Skill(_name, _info, _limit), triggerTime(_triggerTime) {}


