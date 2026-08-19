#include "../header/Character.h"
#include "../header/Skill.h"
#include <filesystem>
#include <algorithm>
#include <stdexcept>


// ==================== 静态数据 ====================
const std::unordered_map<std::string, Character::Info> Character::infos = {
	{"白板",     {Level::F, {}, {}, 1}},
	{"特朗普",   {Level::D, {粪怒::make}, {}, 145}},
	{"棍母",     {Level::F, {隐身::make}, {}, 100}},
	{"夏搏",     {Level::F, {顶置::make}, {}, 114}},
	{"雨姐",     {Level::D, {带派::make}, {}, 275}},
	{"神里绫华", {Level::D, {寒魄::make}, {}, 140}},
	{"瑜伽一",   {Level::F, {割腕::make, 丑皇::make}, {}, 230}},
	{"李阳",     {Level::C, {军国::make, 家暴::make}, {}, 185}},
	{"薛维旭",   {Level::D, {健身::make, 做题::make}, {}, 190}},
	//{"Tung Tung Tung Tung Tung Tung Tung Tung Tung Sahur", {Level::S, {棍击::make, 神木::make}, {}, 90}},
	{"雷电将军", {Level::D, {雷剑::make}, {}, 140}},
	{"王天一",   {Level::C, {买棋::make, 卖棋::make}, {}, 140}},
	{"Tralalero Tralala",    {Level::C, {耐克::make}, {}, 160}},
	{"Bombardiro Crocodilo", {Level::A, {轰炸::make}, {}, 185}},
	{"Bumbumbini Guzzini",   {Level::A, {爆破::make}, {}, 185}},
	{"Alan Walker", {Level::D, {电音::make, 蒙面::make}, {}, 200}},
	{"丁真",        {Level::C, {锐刻::make}, {}, 140}},
	{"代增玉",      {Level::F, {巨富::make, 破产::make}, {}, 275}},
	{"潘子",        {Level::F, {假酒::make}, {}, 120}},
	{"土语",        {Level::D, {窃观::make}, {}, 195}},
	{"Notch",       {Level::C, {生存::make, 创造::make}, {}, 140}},
	{"新诸葛亮",    {Level::B, {炼兵::make, 好火::make}, {}, 77}},
	{"Brr Brr Patapim", {Level::B, {森罗::make, 大脚::make}, {}, 185}},
	{"新关羽", {Level::B, {过江::make, 大盏::make}, {}, 210}},
	{"卞相壹", {Level::B, {举报::make, 猥琐::make}, {}, 160}},
	{"柯洁",   {Level::B, {棋王::make, 金铲::make}, {}, 160}},
	{"老友",   {Level::A, {淘汰::make}, {}, 160}},
	{"屎軖",   {Level::A, {招待::make}, {}, 160}},
	{"植物人", {Level::F, {光合::make}, {}, 200}},
	{"梅西",   {Level::B, {射门::make}, {}, 220}},
	{"二次元", {Level::F, {追番::make, 崩三::make}, {}, 100}},
	{"金正日", {Level::B, {望日::make, 慈父::make}, {}, 188}},
};


// ==================== 构造 / 工厂 ====================
Character::Character(const std::string& _name,
					 const std::string& _skin)
	:name(_name), skin(_skin) {}

std::unique_ptr<Character> Character::make(const std::string& name, const std::string& skin) {
	auto it = infos.find(name);
	if (it == infos.end()) {
		throw std::invalid_argument("角色 <" + name + "> 未在 Character::infos 中定义");
	}
	const Info& info = it->second;

	auto newChara = std::make_unique<Character>(name, skin);
	//被动技能
	for (const auto& factory : info.pSkills) {
		newChara->addSkill(factory());
	}
	//主动技能
	for (const auto& factory : info.aSkills) {
		newChara->addSkill(factory());
	}
	//初始化体力
	newChara->hp = info.hp;
	newChara->maxHp = info.maxHp == 0 ? info.hp : info.maxHp;
	return newChara;
}


// ==================== 基本信息 ====================
std::string Character::getName() const {
	return name;
}
std::wstring Character::getNameW() const {
	return unool::string::to_utf16(name);
}
Character::Level Character::getLevel() const {
	if (auto it = infos.find(name); it != infos.end()) return it->second.level;
	else throw std::invalid_argument("此角色未定义等级");
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
std::string Character::getImagePath() const {
	return getImagePath(name, skin);
}
bool Character::operator<(const Character& other) const {
	return name < other.name;
}
bool Character::operator==(const Character& other) const {
	return name == other.name;
}


// ==================== 静态工具 ====================
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
std::string Character::getImagePath(const std::string& name, const std::string& skin) {
	return "characters/" + name + "/" + skin + ".jpg";
}
std::vector<std::string> Character::getSkins(const std::string& name) {
	namespace fs = std::filesystem;
	const fs::path dir = fs::path(L"../characters") / unool::string::to_utf16(name);
	if (!fs::exists(dir) || !fs::is_directory(dir)) {
		throw std::invalid_argument("角色 <" + name + "> 的皮肤目录不存在");
	}
	std::vector<std::string> skins;
	for (const auto& entry : fs::directory_iterator(dir)) {
		if (entry.is_regular_file() && entry.path().extension() == L".jpg") {
			skins.push_back(unool::string::to_utf8(entry.path().stem().wstring()));
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


// ==================== 技能管理 ====================
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
void Character::launchPSkills(const PSkill::TriggerTime& currentTriggerTime,
							  PSkill::Trigger& trigger) const {
	//遍历被动技能
	for (auto& pSkill : pSkills) {
		//如果时机和角色都符合，则发动
		if (pSkill->matchTrigger(currentTriggerTime, trigger))
			pSkill->launch(trigger);
	}
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


// ==================== 体力管理 ====================
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
