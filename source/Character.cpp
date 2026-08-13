#include "../header/Character.h"
#include "../header/Skill.h"
#include <filesystem>
#include <algorithm>
#include <stdexcept>


const std::unordered_map<std::string, Character::Info> Character::infos = {
	{"白板",     {Level::F, {}, {}, 200}},
	{"特朗普",   {Level::D, {"粪怒"}, {}, 145}},
	{"棍母",     {Level::F, {"隐身"}, {}, 120}},
	{"夏搏",     {Level::F, {"顶置"}, {}, 114}},
	{"雨姐",     {Level::D, {"带派"}, {}, 300}},
	{"神里绫华", {Level::D, {"寒魄"}, {}, 150}},
	{"瑜伽一",   {Level::F, {"割腕", "丑皇"}, {}, 255}},
	{"李阳",     {Level::C, {"军国", "家暴"}, {}, 200}},
	{"薛维旭",   {Level::D, {"健身", "做题"}, {}, 210}},
	//{"Tung Tung Tung Tung Tung Tung Tung Tung Tung Sahur", {Level::S, {"棍击", "神木"}, {}, 100}},
	{"雷电将军", {Level::D, {"雷剑"}, {}, 150}},
	{"王天一",   {Level::C, {"买棋", "卖棋"}, {}, 150}},
	{"Tralalero Tralala",    {Level::C, {"耐克"}, {}, 175}},
	{"Bombardiro Crocodilo", {Level::A, {"轰炸"}, {}, 200}},
	{"Bumbumbini Guzzini",   {Level::A, {"爆破"}, {}, 200}},
	{"Alan Walker",          {Level::C, {"电音", "蒙面"}, {}, 250}},
	{"丁真",                 {Level::C, {"锐刻"}, {}, 150}},
	{"代增玉",               {Level::F, {"巨富", "破产"}, {}, 275}},
	{"潘子",                 {Level::F, {"假酒"}, {}, 130}},
	{"土语",                 {Level::D, {"窃观"}, {}, 215}},
	{"Notch",                {Level::C, {"生存", "创造"}, {}, 150}},
	{"新诸葛亮",              {Level::B, {"炼兵", "好火"}, {}, 77}},
};

std::string Character::getName() const {
	return name;
}

std::wstring Character::getNameW() const {
	return unool::to_utf16(name);
}

std::string Character::skillsName() const {
	std::string result;
	for (const auto& ps : pSkills) {
		result += ps->getName() + ", ";
	}
	for (const auto& as : aSkills) {
		result += as->getName() + ", ";
	}
	return result;
}


void Character::launchPSkills(const PSkill::TriggerTime& currentTriggerTime,
							  PSkill::Trigger& trigger) const {
	//遍历被动技能
	for (auto& pSkill : pSkills) {
		//如果时机和角色都符合，则发动
		if (pSkill->matchTrigger(currentTriggerTime, trigger))
			pSkill->launch(trigger);
	}
}

Character::Character() :Character("白板") {}

Character::Character(const std::string& _name,
					 const std::string& _skin)
	:name(_name), skin(_skin) {}

bool Character::operator<(const Character& other) const {
	return name < other.name;
}

bool Character::operator==(const Character& other) const {
	return name == other.name;
}

std::string Character::getImagePath(const std::string& name, const std::string& skin) {
	return "characters/" + name + "/" + skin + ".jpg";
}

std::string Character::getImagePath() const {
	return getImagePath(name, skin);
}

std::vector<std::string> Character::getSkins(const std::string& name) {
	namespace fs = std::filesystem;
	const fs::path dir = fs::path(L"../characters") / unool::to_utf16(name);
	if (!fs::exists(dir) || !fs::is_directory(dir)) {
		throw std::invalid_argument("角色 <" + name + "> 的皮肤目录不存在");
	}
	std::vector<std::string> skins;
	for (const auto& entry : fs::directory_iterator(dir)) {
		if (entry.is_regular_file() && entry.path().extension() == L".jpg") {
			skins.push_back(unool::to_utf8(entry.path().stem().wstring()));
		}
	}
	if (skins.empty()) {
		throw std::invalid_argument("角色 <" + name + "> 的皮肤目录下无 .jpg 文件");
	}
	//排序，"默认"置首
	std::ranges::sort(skins,
					  [](const std::string& name1, const std::string& name2) {
		if (name1 == "默认") return true;
		if (name2 == "默认") return false;
		return name1 < name2;
	});
	return skins;
}

Character::Level Character::getLevel() const {
	if (auto it = infos.find(name); it != infos.end()) return it->second.level;
	else throw std::invalid_argument("此角色未定义等级");
}

std::string Character::to_string(Level level) {
	switch (level) {
	case Level::S: return "S";
	case Level::A: return "A";
	case Level::B: return "B";
	case Level::C: return "C";
	case Level::D: return "D";
	case Level::F: return "F";
	default: return "?";
	}
}
std::wstring Character::to_wstring(Level level) {
	switch (level) {
	case Level::S: return L"S";
	case Level::A: return L"A";
	case Level::B: return L"B";
	case Level::C: return L"C";
	case Level::D: return L"D";
	case Level::F: return L"F";
	default: return L"?";
	}
}

std::vector<std::string> Character::getPSkillsName() const {
	std::vector<std::string> names;
	for (const auto& skill : pSkills) {
		names.push_back(skill->getName());
	}
	return names;
}

std::vector<std::string> Character::getASkillsName() const {
	std::vector<std::string> names;
	for (const auto& skill : aSkills) {
		names.push_back(skill->getName());
	}
	return names;
}

bool Character::hasPSkill(const std::string& skillName) const {
	for (const auto& skill : pSkills) {
		if (skill->getName() == skillName) return true;
	}
	return false;
}

void Character::addSkill(std::unique_ptr<ASkill> aSkill) {
	aSkills.push_back(std::move(aSkill));
}

void Character::addSkill(std::unique_ptr<PSkill> pSkill) {
	pSkills.push_back(std::move(pSkill));
}

void Character::removeSkill(const std::string& name) {
	std::erase_if(pSkills, [&name](const std::unique_ptr<PSkill>& ps) {
		return ps->getName() == name;
	});
	std::erase_if(aSkills, [&name](const std::unique_ptr<ASkill>& as) {
		return as->getName() == name;
	});
}

void Character::resetSkills() {
	for (auto& pSkill : pSkills) {
		pSkill->reset();
	}
	for (auto& aSkill : aSkills) {
		aSkill->reset();
	}
}

std::unique_ptr<Character> Character::make(const std::string& name, const std::string& skin) {
	auto it = infos.find(name);
	if (it == infos.end()) {
		throw std::invalid_argument("角色 <" + name + "> 未在 Character::infos 中定义");
	}
	const Info& info = it->second;

	auto newChara = std::make_unique<Character>(name, skin);
	//被动技能
	for (const auto& skillName : info.pSkillNames) {
		newChara->addSkill(PSkill::registry.at(skillName)());
	}
	//主动技能
	for (const auto& skillName : info.aSkillNames) {
		newChara->addSkill(ASkill::registry.at(skillName)());
	}
	//初始化体力
	newChara->hp = info.hp;
	newChara->maxHp = info.maxHp == 0 ? info.hp : info.maxHp;
	return newChara;
}

std::size_t Character::getHp() const {
	return hp;
}

std::size_t Character::getMaxHp() const {
	return maxHp;
}

void Character::setHp(std::size_t newHp) {
	hp = newHp;
}

void Character::takeDamage(std::size_t damage) {
	if (hp <= damage) hp = 0;
	else hp -= damage;
}
void Character::recover(std::size_t num) {
	hp = std::min(hp + num, maxHp);
}
bool Character::isDead() const {
	return hp == 0;
}
