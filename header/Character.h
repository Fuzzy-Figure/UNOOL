#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Skill.h"

class Skill;

class Character {
public:
	enum class Level {
		S, A, B, C, D, F
	};
	struct Info {
		Level level;
		std::vector<std::string> pSkillNames;
		std::vector<std::string> aSkillNames;
		std::size_t hp = 1;
		std::size_t maxHp = 1;
		Info(Level _level,
			 std::vector<std::string> _pSkillNames,
			 std::vector<std::string> _aSkillNames,
			 std::size_t _hp, std::size_t _maxHp = 0)
			:level(_level), pSkillNames(std::move(_pSkillNames)), aSkillNames(std::move(_aSkillNames)),
			hp(_hp), maxHp(_maxHp == 0 ? _hp : _maxHp) {}
	};
private:
	std::string name = "白板";
	std::string skin = "默认";
	std::vector<std::unique_ptr<PSkill>> pSkills = {};
	std::vector<std::unique_ptr<ASkill>> aSkills = {};
	std::size_t hp = 1;
	std::size_t maxHp = 1;
public:
	//构造函数
	Character();
	Character(const std::string& _name, const std::string& _skin = "默认");
	static std::unique_ptr<Character> make(const std::string& name, const std::string& skin = "默认");

	//获取成员
	std::string getName() const;
	std::wstring getNameW() const;
	Level getLevel() const;
	static std::string levelToString(Level level);
	std::string getImagePath() const;
	std::string getSkin() const { return skin; }
	static std::string getImagePath(const std::string& name, const std::string& skin = "默认");
	static std::vector<std::string> getSkins(const std::string& name);
	std::string skillsName() const;
	std::size_t getHp() const;
	std::size_t getMaxHp() const;
	std::vector<std::string> getPSkillsName() const;
	std::vector<std::string> getASkillsName() const;
	bool hasPSkill(const std::string& skillName) const;
	void setHp(std::size_t newHp);
	void takeDamage(std::size_t damage);
	void recover(std::size_t num);
	bool isDead() const;

	//发动被动技能
	void launchPSkills(const PSkill::TriggerTime& currentTriggerTime,
					   PSkill::Trigger& trigger) const;

	//增加/失去 技能
	void addSkill(std::unique_ptr<ASkill> aSkill);
	void addSkill(std::unique_ptr<PSkill> pSkill);
	void removeSkill(const std::string& name);

	//重置技能使用次数
	void resetSkills();

	bool operator<(const Character& other) const;
	bool operator==(const Character& other) const;

	Character(const Character&) = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&) = default;
	Character& operator=(Character&&) = default;

	static const std::unordered_map<std::string, Info> infos;
};

