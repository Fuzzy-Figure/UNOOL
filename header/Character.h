#pragma once
#include <string>
#include <list>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Skill.h"

class Character {
#pragma region 类型定义
public:
	enum class Level { S, A, B, C, D, F };
	struct Info {
		Level level;
		std::vector<PSkill::Factory> pSkills;
		std::vector<ASkill::Factory> aSkills;
		std::size_t hp;
		std::size_t maxHp = 0;
	};
#pragma endregion

private:
	std::string name;
	std::string skin;
	std::list<std::unique_ptr<PSkill>> pSkills;
	std::list<std::unique_ptr<ASkill>> aSkills;
	std::size_t hp = 0;
	std::size_t maxHp = 0;

public:
#pragma region 构造 / 工厂
	Character(const std::string& name, const std::string& skin);
	Character(const Character&) = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&) = default;
	Character& operator=(Character&&) = default;
	static std::unique_ptr<Character> make(const std::string& name, const std::string& skin = "默认");
#pragma endregion

#pragma region 基本信息
	std::string getName() const;
	std::wstring getNameW() const;
	const std::string& getSkin() const { return skin; }
	Level getLevel() const;
	std::string skillsName() const;
	std::string getSkillsText() const;
	std::string getImagePath() const;
	bool operator<(const Character& other) const;
	bool operator==(const Character& other) const;
#pragma endregion

#pragma region 静态工具
	static std::string to_string(Level level);
	static std::wstring to_wstring(Level level);
	static std::string getImagePath(const std::string& name, const std::string& skin = "默认");
	static std::vector<std::string> getSkins(const std::string& name);
#pragma endregion

#pragma region 技能管理
	std::vector<std::string> getPSkillsName() const;
	std::vector<std::string> getASkillsName() const;
	bool hasPSkill(const std::string& skillName) const;
	void launchPSkills(const PSkill::TriggerTime& currentTriggerTime, PSkill::Trigger& trigger) const;
	void addSkill(std::unique_ptr<ASkill> aSkill);
	void addSkill(std::unique_ptr<PSkill> pSkill);
	void removeSkill(const std::string& name);
	void resetSkills();
#pragma endregion

#pragma region 体力管理
	std::size_t getHp() const;
	std::size_t getMaxHp() const;
	void setHp(std::size_t newHp);
	void takeDamage(std::size_t damage);
	void recover(std::size_t num);
	bool isDead() const;
#pragma endregion

#pragma region 静态数据
	static const std::unordered_map<std::string, Info> infos;
#pragma endregion
};
