#pragma once
#include <string>
#include <list>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "Skill.h"

class Character {
#pragma region 类型定义
public:
	enum class Level { S, A, B, C, D, F };
	struct Info {
		std::string group;
		Level level;
		std::vector<PSkill::Factory> pSkills;
		std::vector<std::function<std::unique_ptr<ASkillInstantBase>()>> instantSkills;
		std::vector<std::function<std::unique_ptr<ASkillTransformBase>()>> transformSkills;
		std::size_t hp;
		std::size_t maxHp = 0;
	};
	using Entry = std::pair<std::string, Info>;
#pragma endregion

private:
	std::vector<std::string> names;
	std::vector<std::string> skins;
	std::list<std::unique_ptr<PSkill>> pSkills;
	std::list<std::unique_ptr<ASkillInstantBase>> instantSkills;
	std::list<std::unique_ptr<ASkillTransformBase>> transformSkills;
	std::size_t hp = 0;
	std::size_t maxHp = 0;
	std::size_t damageMultiplier = 1;
	std::size_t wins = 0;
	std::size_t losses = 0;
	std::unordered_map<std::string, std::size_t> marks;

public:
#pragma region 构造 / 工厂
	Character(const std::string& name, const std::string& skin);
	Character(const Character&) = delete;
	Character& operator=(const Character&) = delete;
	Character(Character&&) = default;
	Character& operator=(Character&&) = default;
	static std::unique_ptr<Character> make(const std::string& name, const std::string& skin = "默认");
	//组合两个角色为一个新角色：name=name1+name2，sources={name1,name2}，技能合集，体力叠加
	static std::unique_ptr<Character> makeCombined(const std::string& name1, const std::string& skin1,
												   const std::string& name2, const std::string& skin2);
#pragma endregion

#pragma region 基本信息
	std::string getName() const;
	std::wstring getNameW() const;
	const std::vector<std::string>& getNames() const { return names; }
	const std::vector<std::string>& getSkins() const { return skins; }
	//是否为组合角色（双将模式）
	bool isCombined() const { return names.size() == 2; }
	Level getLevel() const;
	//组合角色返回两角色等级，单角色返回单元素
	std::vector<Level> getLevels() const;
	Level getMaxLevel() const;
	Level getMinLevel() const;
	std::string skillsName() const;
	std::string getSkillsText() const;
	std::string getImagePath() const;
	//组合角色返回多张图路径，单角色返回单元素
	std::vector<std::string> getImagePaths() const;
	bool operator<(const Character& other) const;
	bool operator==(const Character& other) const;
#pragma endregion

#pragma region 静态工具
	static std::string to_string(Level level);
	static std::wstring to_wstring(Level level);
	static std::string getImagePath(const std::string& name, const std::string& skin = "默认");
	static std::vector<std::string> getSkins(const std::string& name);

	static std::vector<Entry> randomChooseCharacters(std::size_t n);
#pragma endregion

#pragma region 技能管理
	std::list<std::unique_ptr<ASkillInstantBase>>& getInstantSkills() { return instantSkills; }
	std::list<std::unique_ptr<ASkillTransformBase>>& getTransformSkills() { return transformSkills; }
	std::list<std::unique_ptr<PSkill>>& getPSkills() { return pSkills; }
	bool hasSkill(const std::string& skillName) const;
	opt_ref<Skill> findSkill(const std::string& skillName);
	void launchPSkills(const PSkill::TriggerTime& currentTriggerTime, PSkill::Trigger& trigger) const;
	void addSkill(std::unique_ptr<ASkillInstantBase>   skill);
	void addSkill(std::unique_ptr<ASkillTransformBase> skill);
	void addSkill(std::unique_ptr<PSkill> pSkill);
	void removeSkill(const std::string& name);
	void resetSkills();
#pragma endregion

#pragma region 体力管理
	std::size_t getHp() const;
	std::size_t getMaxHp() const;
	void setHp(std::size_t newHp);
	std::size_t damage(std::size_t damage);
	void recover(std::size_t num);
	bool isDead() const;
#pragma endregion

#pragma region 伤害倍率
	std::size_t getDamageMultiplier() const { return damageMultiplier; }
	void setDamageMultiplier(std::size_t m) { damageMultiplier = m; }
	std::size_t getWins() const { return wins; }
	std::size_t getLosses() const { return losses; }
	void incrementWins() { ++wins; }
	void incrementLosses() { ++losses; }
#pragma endregion

#pragma region 标记
	bool hasMark(const std::string& m) const { return marks.contains(m); }
	void addMark(const std::string& m, std::size_t count = 1) { marks[m] += count; }
	void removeMark(const std::string& m, std::size_t count = 1);
	const std::unordered_map<std::string, std::size_t>& getMarks() const { return marks; }
	void clearMark(const std::string& m) { marks.erase(m); }
	void clearAllMarks() { marks.clear(); }
#pragma endregion

#pragma region 静态数据
	static const std::unordered_map<std::string, Info> infos;
#pragma endregion
};
