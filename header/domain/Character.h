#pragma once
// 头文件自足化（第十四批）：显式包含本头直接使用的标准库类型，不依赖 PCH 的传递包含。
#include <stdexcept>
#include <string_view>
#include <utility>

#include <string>
#include <list>
#include <vector>
#include <memory>
#include <optional>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include "contract/CharacterTypes.h"
#include "Skill.h"

class Character {
#pragma region 类型定义
public:
	// B+：等级 / 品质 / 标量类型的定义已移入共享契约层
	// （header/contract/CharacterTypes.h，纯标准库、不链接规则引擎）。
	// 这里保留别名，使 Character::Level / Character::SkinQuality /
	// Character::price_t / Character::hp_t 的全部既有写法与枚举值保持源码兼容。
	using hp_t = unool::contract::hp_t;
	using price_t = unool::contract::price_t;
	using Level = unool::contract::CharacterLevel;
	using SkinQuality = unool::contract::SkinQuality;
	struct Info {
		std::string group;
		Level level;
		std::vector<PassiveSkill::Factory>   passiveSkills;
		std::vector<InstantSkill::Factory>   instantSkills;
		std::vector<TransformSkill::Factory> transformSkills;
		hp_t hp = 0;
		hp_t maxHp = 0;

		// 无技能：体力 + 可选上限
		Info(std::string g, Level l, hp_t h, hp_t m = 0)
			: group(std::move(g)), level(l), hp(h), maxHp(m == 0 ? h : m) {}

		// 有技能：技能组 + 体力 + 可选上限
		Info(std::string g, Level l, HybridSkills s, hp_t h, hp_t m = 0)
			: group(std::move(g)), level(l), hp(h), maxHp(m == 0 ? h : m),
			passiveSkills(std::move(s.passive)),
			instantSkills(std::move(s.instant)),
			transformSkills(std::move(s.transform)) {}
	};
	using Entry = std::pair<std::string, Info>;
#pragma endregion

private:
	std::vector<std::string> names;
	std::vector<std::string> skins;
	std::list<std::unique_ptr<PassiveSkill>> passiveSkills;
	std::list<std::unique_ptr<InstantSkill>> instantSkills;
	std::list<std::unique_ptr<TransformSkill>> transformSkills;
	hp_t hp = 0;
	hp_t maxHp = 0;
	double damageMultiplier = 1.0;  //伤害倍率（1.0 = 原始伤害）
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
	const std::vector<std::string>& getNames() const { return names; }
	const std::vector<std::string>& getSkins() const { return skins; }
	//是否为组合角色（双将模式）
	bool isCombined() const { return names.size() == 2; }
	Level getLevel() const;
	//组合角色返回两角色等级，单角色返回单元素
	std::vector<Level> getLevels() const;
	Level getMaxLevel() const;
	Level getMinLevel() const;
	//按角色等级查积分表
	static int getScore(Level winner, Level loser);
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
	static std::string getImagePath(const std::string& name, const std::string& skin = "默认");
	static std::vector<std::string> getSkins(const std::string& name);

	static std::vector<Entry> randomChooseCharacters(std::size_t n);
	//随机抽取 n 个角色，排除 exclude 中指定的角色名
	static std::vector<Entry> randomChooseCharacters(std::size_t n, const std::unordered_set<std::string>& exclude);

	// 获取皮肤品质（根据角色名和皮肤名）
	static SkinQuality getSkinQuality(const std::string& charName, const std::string& skinName);
	// 获取皮肤售价（不可购买返回 std::nullopt）
	static price_t getSkinPrice(const std::string& charName, const std::string& skinName);
	// 获取皮肤品质售价（不可购买返回 std::nullopt）
	static price_t getSkinPriceByQuality(SkinQuality quality);
	// 皮肤品质转中文字符串
	static std::string to_string(SkinQuality quality);
	// 获取角色售价（按等级和角色名；不可购买/不存在返回 std::nullopt）
	static price_t getCharacterPrice(const std::string& name);
#pragma endregion

#pragma region 技能管理
	std::list<std::unique_ptr<PassiveSkill>>& getPassiveSkills() { return passiveSkills; }
	std::list<std::unique_ptr<InstantSkill>>& getInstantSkills() { return instantSkills; }
	std::list<std::unique_ptr<TransformSkill>>& getTransformSkills() { return transformSkills; }
	template<SpecificSkill T>
	bool hasSkill() const { return findSkill<T>().has_value(); }
	template<SpecificSkill T>
	opt_ref<T> findSkill() const {
		for (auto& s : passiveSkills)
			if (typeid(*s) == typeid(T)) return s->to<T>();
		for (auto& s : instantSkills)
			if (typeid(*s) == typeid(T)) return s->to<T>();
		for (auto& s : transformSkills)
			if (typeid(*s) == typeid(T)) return s->to<T>();
		return std::nullopt;
	}
	template<SpecificSkill T>
	T& getSkill() {
		if (opt_ref<T> s = findSkill<T>(); s.has_value()) return s.value();
		else throw std::runtime_error("没有找到技能");
	}
	template<SpecificSkill T>
	T& getSkill() const {
		if (const opt_ref<T> s = findSkill<T>(); s.has_value()) return s.value();
		else throw std::runtime_error("没有找到技能");
	}
	template<SpecificSkill T>
	bool removeSkill() {
		for (auto it = passiveSkills.begin(); it != passiveSkills.end(); ++it) {
			if (typeid(**it) == typeid(T)) { passiveSkills.erase(it); return true; }
		}
		for (auto it = instantSkills.begin(); it != instantSkills.end(); ++it) {
			if (typeid(**it) == typeid(T)) { instantSkills.erase(it); return true; }
		}
		for (auto it = transformSkills.begin(); it != transformSkills.end(); ++it) {
			if (typeid(**it) == typeid(T)) { transformSkills.erase(it); return true; }
		}
		return false;
	}
	void launchPassiveSkills(const PassiveSkill::TriggerTime& currentTriggerTime, GameLogic& game, Player& carrier, PassiveSkill::Trigger& trigger);
	void addSkill(std::unique_ptr<PassiveSkill> skill);
	void addSkill(std::unique_ptr<InstantSkill> skill);
	void addSkill(std::unique_ptr<TransformSkill> skill);
	void resetSkills();
#pragma endregion

#pragma region 体力管理
	hp_t getHp() const;
	hp_t getMaxHp() const;
	void setHp(hp_t newHp);
	std::size_t damage(std::size_t damage);
	void recover(std::size_t num);
	bool isDead() const;
#pragma endregion

#pragma region 伤害倍率
	double getDamageMultiplier() const { return damageMultiplier; }
	void setDamageMultiplier(double m) { damageMultiplier = m; }
	void addDamageMultiplier(double delta) { damageMultiplier += delta; }
	std::size_t getWins() const { return wins; }
	std::size_t getLosses() const { return losses; }
	void incrementWins() { ++wins; }
	void incrementLosses() { ++losses; }
#pragma endregion

#pragma region 标记
	bool hasMark(const std::string& m) const { return marks.contains(m); }
	std::size_t getMarkCount(const std::string& m) const { auto it = marks.find(m); return it != marks.end() ? it->second : 0; }
	void addMark(const std::string& m, std::size_t count = 1) { marks[m] += count; }
	void setMark(const std::string& m, std::size_t count) { marks[m] = count; }
	std::size_t removeMark(const std::string& m, std::size_t count = 1);
	const std::unordered_map<std::string, std::size_t>& getMarks() const { return marks; }
	void clearMark(const std::string& m) { marks.erase(m); }
	void clearAllMarks() { marks.clear(); }
#pragma endregion

#pragma region 静态数据
	static const std::unordered_map<std::string, Info> infos;
#pragma endregion
};

// B+：operator<=> 与 std::formatter 特化已移入 header/contract/CharacterTypes.h。
// 由于 Character::Level 现在只是契约枚举的别名，特化仍作用于同一类型，
// 所有格式化输出（"F"…"S"）与比较语义逐字不变；此处不再重复定义，
// 否则会与契约头形成同一特化的重复定义。
