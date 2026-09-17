#include "../header/Character.h"
#include "../header/PSkill.h"
#include "../header/ASkill.h"
#include <filesystem>
#include <algorithm>
#include <stdexcept>


// ==================== 静态数据 ====================
const std::unordered_map<std::string, Character::Info> Character::infos = {
	{"白板",     {"其他",          Level::F, {},           {}, {}, 1}},
	{"特朗普",   {"元首",           Level::D, {粪怒::make}, {}, {}, 145}},
	{"棍母",     {"网络",           Level::F, {隐身::make}, {}, {}, 100}},
	{"夏搏",     {"实验",           Level::F, {顶置::make}, {}, {}, 114}},
	{"雨姐",     {"网络",           Level::D, {带派::make}, {}, {}, 275}},
	{"神里绫华", {"原神",           Level::D, {寒魄::make}, {}, {}, 175}},
	{"瑜伽一",   {"实验",           Level::F, {割腕::make, 丑皇::make}, {}, {}, 230}},
	{"李阳",     {"网络",          Level::C, {军国::make, 家暴::make}, {}, {}, 185}},
	{"薛维旭",   {"实验",           Level::D, {健身::make, 做题::make}, {}, {}, 190}},
	{"Tung Tung Tung Tung Tung Tung Tung Tung Tung Sahur", {"山海经", Level::S, {棍击::make, 神木::make}, {}, {}, 100}},
	{"雷电将军", {"原神",           Level::D, {雷剑::make}, {}, {}, 175}},
	{"王天一",   {"网络",          Level::C, {买棋::make, 卖棋::make}, {}, {}, 140}},
	{"Tralalero Tralala",    {"山海经", Level::C, {耐克::make}, {}, {}, 175}},
	{"Bombardiro Crocodilo", {"山海经", Level::A, {轰炸::make}, {装弹::make}, {}, 185}},
	{"Bumbumbini Guzzini",   {"山海经", Level::A, {爆破::make}, {装弹::make}, {}, 185}},
	{"Alan Walker", {"网络",  Level::D, {电音::make, 蒙面::make}, {}, {}, 175}},
	{"丁真",        {"网络",  Level::C, {锐刻::make}, {}, {}, 150}},
	{"代增玉",      {"实验",  Level::F, {巨富::make, 破产::make}, {}, {}, 275}},
	{"潘子",        {"网络",  Level::F, {假酒::make}, {}, {}, 120}},
	{"土语",        {"实验",  Level::D, {窃观::make}, {}, {}, 205}},
	{"Notch",       {"网络",  Level::C, {生存::make, 创造::make}, {}, {}, 140}},
	{"新诸葛亮",    {"新三国", Level::B, {炼兵::make, 好火::make}, {}, {}, 77}},
	{"Brr Brr Patapim", {"山海经", Level::B, {森罗::make, 大脚::make}, {}, {}, 200}},
	{"新关羽", {"新三国", Level::B, {过江::make, 大盏::make}, {}, {}, 200}},
	{"卞相壹", {"网络", Level::B, {举报::make, 猥琐::make}, {}, {}, 160}},
	{"柯洁",   {"网络", Level::B, {棋王::make, 金铲::make}, {}, {}, 160}},
	{"老友",   {"实验", Level::A, {淘汰::make}, {}, {}, 160}},
	{"屎軖",   {"实验", Level::A, {}, {招待::make}, {}, 160}},
	{"植物人", {"实验", Level::F, {光合::make}, {}, {}, 200}},
	{"梅西",   {"网络", Level::B, {射门::make}, {}, {}, 220}},
	{"二次元", {"原神", Level::F, {追番::make, 崩三::make}, {}, {}, 100}},
	{"金正日", {"元首", Level::B, {望日::make}, {}, {}, 188}},
	{"金日成", {"元首", Level::C, {朔日::make}, {}, {}, 199}},
	{"刘建龙", {"网络", Level::D, {}, {徒步::make}, {}, 250}},
	{"拜登",   {"元首", Level::A, {健忘::make}, {}, {}, 125}},
	{"王耘浩", {"实验", Level::D, {豪赌::make}, {}, {}, 250}},
	{"Bulbito Bandito Traktorito", {"山海经", Level::B, {黑帮::make, 拖拉::make}, {}, {}, 225}},
	{"烟刻瑯", {"实验", Level::B, {迷烟::make}, {}, {}, 175}},
	{"幺幺",   {"网络", Level::F, {水鬼::make}, {}, {}, 88}},
	{"蒋介石", {"元首", Level::F, {叛党::make}, {}, {}, 180}},
	{"斯大林", {"元首", Level::C, {清洗::make}, {}, {}, 185}},
	{"龚俊清", {"网络", Level::B, {落水::make, 骚扰::make}, {}, {}, 198}},
	{"Blueberrini Octopussini", {"山海经", Level::D, {}, {}, {八爪::make}, 100}},
	{"闫传学", {"实验", Level::A, {犬子::make}, {}, {}, 222}},
	{"霍金",   {"网络", Level::C, {黑洞::make}, {}, {}, 88}},
	{"峰哥",   {"网络", Level::A, {好事::make, 压抑::make}, {}, {}, 250}},
	{"包贝尔", {"网络", Level::B, {捉奸::make}, {}, {我妈::make}, 160}},
	{"科比",   {"网络", Level::C, {}, {}, {曼巴::make}, 248}},
	{"翟钊",   {"实验", Level::F, {}, {摘罩::make}, {}, 150}},
	{"田淑丽", {"实验", Level::C, {}, {还击::make}, {}, 145}},
	{"虎哥",   {"网络", Level::F, {有活::make}, {}, {}, 275}},
	{"大章鱼", {"实验", Level::C, {爬竿::make, 渊涡::make}, {}, {}, 275}},
	{"杨坤",   {"网络", Level::C, {没座::make, 空空::make}, {}, {}, 205}},
	//{"大脚忍者", {"网络", Level::A, {暗忍::make}, {舞爪::make}, {}, 280}},
	{"大中医",   {"实验",     Level::A, {治病::make}, {}, {}, 100}},
	{"新陆逊",   {"新三国",   Level::B, {连营::make, 困界::make}, {}, {}, 150}},
	{"电棍",     {"网络",     Level::F, {四麻::make}, {四霸::make}, {}, 44}},
	{"8比特",    {"荒野乱斗", Level::S, {爆射::make}, {装币::make}, {}, 288}},
	{"格斯",     {"荒野乱斗", Level::B, {灵爆::make}, {幽愈::make}, {}, 100}},
	{"斯图",     {"荒野乱斗", Level::A, {加速::make}, {炫技::make}, {}, 120}},
	{"大司马",   {"网络",     Level::C, {走位::make}, {}, {}, 150}},
	{"新静姝",   {"新三国",   Level::B, {}, {调羹::make}, {}, 166}},
	{"唐伯虎",   {"网络",     Level::C, {九一::make, 白虎::make}, {}, {}, 91}},
	{"新吕蒙",   {"新三国",   Level::B, {易主::make, 渡荆::make}, {}, {}, 200}},
	{"格里夫",   {"荒野乱斗", Level::C, {返现::make}, {挥金::make}, {}, 200}},
	{"斯派克",   {"荒野乱斗", Level::A, {尖刺::make}, {再生::make}, {}, 50}},
	{"柯尔特",   {"荒野乱斗", Level::F, {弹暴::make}, {手枪::make}, {}, 150}},
	{"科斯莫",   {"荒野乱斗", Level::S, {星轨::make, 引力_清除目标::make}, {引力::make}, {}, 100}},
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
	//即时型主动技
	for (const auto& factory : info.instantSkills) {
		newChara->addSkill(factory());
	}
	//转换型主动技
	for (const auto& factory : info.transformSkills) {
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
	for (const auto& as : instantSkills) {
		result += as->getName() + ", ";
	}
	for (const auto& as : transformSkills) {
		result += as->getName() + ", ";
	}
	return result;
}
std::string Character::getSkillsText() const {
	std::string result;
	for (const auto& ps : pSkills) {
		result += "【" + ps->getName() + "】（被动技能）\n" + ps->getInfo() + "\n";
	}
	for (const auto& as : instantSkills) {
		result += "【" + as->getName() + "】（主动技能）\n" + as->getInfo() + "\n";
	}
	for (const auto& as : transformSkills) {
		result += "【" + as->getName() + "】（主动技能）\n" + as->getInfo() + "\n";
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
		default:       return "?";
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
		default:       return L"?";
	}
}
std::string Character::getImagePath(const std::string& name, const std::string& skin) {
	auto it = infos.find(name);
	if (it == infos.end()) {
		throw std::invalid_argument("角色 <" + name + "> 未在 Character::infos 中定义");
	}
	const std::string& group = it->second.group;
	return "images/characters/" + group + "/" + name + "/" + skin + ".jpg";
}
std::vector<std::string> Character::getSkins(const std::string& name) {
	namespace fs = std::filesystem;
	auto it = infos.find(name);
	if (it == infos.end()) {
		throw std::invalid_argument("角色 <" + name + "> 未在 Character::infos 中定义");
	}
	const std::string& group = it->second.group;
	const fs::path dir = fs::path(L"../images/characters") / unool::string::to_utf16(group) / unool::string::to_utf16(name);
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

std::vector<Character::Entry> Character::randomChooseCharacters(std::size_t n) {
	//加载被屏蔽的角色和分组
	const std::unordered_set<std::string> shieldedCharacters = [] {
		std::vector chars = unool::getServerConfig()["shielded"]["characters"].get<std::vector<std::string>>();
		return std::unordered_set<std::string>{ chars.begin(), chars.end() };
	}();
	const std::unordered_set<std::string> shieldedGroups = [] {
		std::vector chars = unool::getServerConfig()["shielded"]["groups"].get<std::vector<std::string>>();
		return std::unordered_set<std::string>{ chars.begin(), chars.end() };
	}();

	//构造可用角色
	auto filteredChars = Character::infos | std::views::filter(
		[&shieldedCharacters, &shieldedGroups](const std::pair<std::string, Character::Info>& info) {
		//过滤掉白板和被屏蔽的角色
		return info.first != "白板"
			&& !shieldedCharacters.contains(info.first)
			&& !shieldedGroups.contains(info.second.group);
	});
	const std::size_t filteredCharsSize = std::ranges::distance(filteredChars);

	//判断可用角色数量是否足够
	if (n > filteredCharsSize) {
		throw std::invalid_argument(
			"候选角色数量(" + std::to_string(n) + ")"
			"不能超过可选角色数量(" + std::to_string(filteredCharsSize) + ")"
		);
	}

	//抽角色
	std::vector<Entry> result;
	result.reserve(n);
	std::ranges::sample(filteredChars, std::back_inserter(result), n, unool::random::rng);
	std::ranges::shuffle(result, unool::random::rng);
	return result;
}


// ==================== 技能管理 ====================
bool Character::hasSkill(const std::string& skillName) const {
	for (const auto& skill : pSkills) {
		if (skill->getName() == skillName) return true;
	}
	for (const auto& skill : instantSkills) {
		if (skill->getName() == skillName) return true;
	}
	for (const auto& skill : transformSkills) {
		if (skill->getName() == skillName) return true;
	}
	return false;
}
opt_ref<Skill> Character::findSkill(const std::string& skillName) {
	for (auto& skill : pSkills) {
		if (skill->getName() == skillName) return *skill;
	}
	for (const auto& skill : instantSkills) {
		if (skill->getName() == skillName) return *skill;
	}
	for (const auto& skill : transformSkills) {
		if (skill->getName() == skillName) return *skill;
	}
	return std::nullopt;
}
void Character::launchPSkills(const PSkill::TriggerTime& currentTriggerTime,
							  PSkill::Trigger& trigger) const {
	//先收集要发动的技能指针，避免content中修改pSkills导致迭代器失效
	std::vector<PSkill*> toLaunch;
	for (const auto& pSkill : pSkills) {
		if (pSkill->matchTrigger(currentTriggerTime, trigger))
			toLaunch.push_back(pSkill.get());
		for (const auto& sub : pSkill->subSkills) {
			if (sub->matchTrigger(currentTriggerTime, trigger))
				toLaunch.push_back(sub.get());
		}
	}
	//遍历指针列表发动；即使某个技能在content中销毁自身，也不影响后续技能
	for (PSkill* skill : toLaunch) {
		skill->launch(trigger);
	}
}
void Character::addSkill(std::unique_ptr<ASkillInstantBase> skill) {
	instantSkills.push_back(std::move(skill));
}
void Character::addSkill(std::unique_ptr<ASkillTransformBase> skill) {
	transformSkills.push_back(std::move(skill));
}
void Character::addSkill(std::unique_ptr<PSkill> pSkill) {
	pSkills.push_back(std::move(pSkill));
}
void Character::removeSkill(const std::string& name) {
	std::erase_if(pSkills, [&name](const std::unique_ptr<PSkill>& ps) {
		return ps->getName() == name;
	});
	std::erase_if(instantSkills, [&name](const std::unique_ptr<ASkillInstantBase>& s) {
		return s->getName() == name;
	});
	std::erase_if(transformSkills, [&name](const std::unique_ptr<ASkillTransformBase>& s) {
		return s->getName() == name;
	});
}
void Character::resetSkills() {
	for (auto& pSkill : pSkills) {
		pSkill->reset();
	}
	for (auto& s : instantSkills) {
		s->reset();
	}
	for (auto& s : transformSkills) {
		s->reset();
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
	hp = std::min(newHp, maxHp);
}
std::size_t Character::damage(std::size_t damage) {
	std::size_t actualDamageValue = damage;
	if (hp <= damage) {
		actualDamageValue = hp;
		hp = 0;
	}
	else {
		hp -= damage;
	}
	return actualDamageValue;
}
void Character::recover(std::size_t num) {
	hp = std::min(hp + num, maxHp);
}
bool Character::isDead() const {
	return hp == 0;
}
