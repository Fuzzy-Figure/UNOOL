#include "../header/Skill.h"
#include "../header/Player.h"
#include "../header/GameLogic.h"
#include "../header/utils.h"
#include <iostream>
#include <cmath>
#include <set>

Skill::Skill(const std::string& _name, const std::string& _info, const limit_t& _limit)
	:name(_name), info(_info), limit(_limit) {}

std::string Skill::formatInfo() const {
	std::string result;
	result.reserve(info.size());
	for (std::size_t i = 0; i < info.size(); ) {
		const char ch = info[i];
		if (ch == '{') {
			//转义：{{ → {
			if (i + 1 < info.size() && info[i + 1] == '{') {
				result += '{';
				i += 2;
				continue;
			}
			//占位符：找到对应的 }
			const std::size_t end = info.find('}', i + 1);
			if (end == std::string::npos) {
				//没有闭合，原样输出剩余
				result += info.substr(i);
				break;
			}
			const std::string key = info.substr(i + 1, end - i - 1);
			if (key == "limit") {
				result += limit.has_value() ? std::to_string(limit.value()) : "无限";
			}
			else if (key == "remaining") {
				result += limit.has_value()
					? std::to_string(limit.value() - std::min(count, limit.value()))
					: "无限";
			}
			else if (key == "count") {
				result += std::to_string(count);
			}
			else {
				//未知占位符，原样输出
				result += info.substr(i, end - i + 1);
			}
			i = end + 1;
		}
		else {
			result += ch;
			++i;
		}
	}
	return result;
}

void Skill::reset() {
	count = 0;
	for (auto& sub : subSkills) sub->reset();
}

bool Skill::isPassive() const {
	return getType() == Type::passive;
}
bool Skill::isInstant() const {
	return getType() == Type::instant;
}
bool Skill::isTransform() const {
	return getType() == Type::transform;
}
PassiveSkill& Skill::toPassive() {
	if (!isPassive()) throw std::runtime_error("不能将非Passive技能转化为PassiveSkill");
	return static_cast<PassiveSkill&>(*this);
}
InstantSkill& Skill::toInstant() {
	if (!isInstant()) throw std::runtime_error("不能将非Instant技能转化为InstantSkill");
	return static_cast<InstantSkill&>(*this);
}
TransformSkill& Skill::toTransform() {
	if (!isTransform()) throw std::runtime_error("不能将非Transform技能转化为TransformSkill");
	return static_cast<TransformSkill&>(*this);
}


void Skill::resetPhaseCount() {
	for (auto& sub : subSkills) sub->resetPhaseCount();
}





// **********************
//         被动技
// **********************


//无子技能
PassiveSkill::PassiveSkill(const std::string& name, const std::string& description,
						   const limit_t& limit, bool forced,
						   const TriggerPlayer& triggerPlayer,
						   const TriggerTime& triggerTime)
	: Skill(name, description, limit),
	forced(forced),
	triggerPlayer(triggerPlayer),
	triggerTime(triggerTime) {}

bool PassiveSkill::matchTrigger(const TriggerTime& currentTriggerTime,
								const Player& carrier,
								const Trigger& trigger) const {
	return triggerTime == currentTriggerTime && (
		triggerTime == TriggerTime::game_begin ||
		triggerTime == TriggerTime::game_end ||
		triggerPlayer == TriggerPlayer::anybody ||
		(triggerPlayer == TriggerPlayer::self && carrier.getId() == trigger.getPlayer().getId()) ||
		(triggerPlayer == TriggerPlayer::others && carrier.getId() != trigger.getPlayer().getId())
		);
}

void PassiveSkill::launch(GameLogic& game, Player& carrier, Trigger& trigger) {
	//不满足条件，或达到次数限制：不发动
	if ((limit != unlimited && count >= limit) || !filter(game, carrier, trigger)) return;
	//如果不是锁定技，询问玩家是否发动
	if (!forced) {
		const std::size_t choice = carrier.ask(
			L"是否发动 [" + getNameW() + L"]？",
			{ L"发动", L"不发动" },
			true
		);
		if (choice == 2) return;
	}
	//发动技能
	count += 1;

	if (!content(game, carrier, trigger)) {
		count -= 1;
		return;
	}
	std::cout << "<技能> " << carrier.characterName() << "发动了" << name << "！" << std::endl;
}

void PassiveSkill::reset() {
	Skill::reset();
}

void PassiveSkill::setForced(const bool newForced) {
	forced = newForced;
}


// **********************
//         主动技
// **********************
ActiveSkill::ActiveSkill(const std::string& _name, const std::string& _info, const limit_t& _limit,
						 const limit_t& _phaseLimit, TriggerTime _triggerTime)
	:Skill(_name, _info, _limit), triggerTime(_triggerTime), phaseLimit(_phaseLimit) {}


