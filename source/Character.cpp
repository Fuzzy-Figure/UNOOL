#include "Character.h"
#include "PassiveSkill.h"
#include "InstantSkill.h"
#include "TransformSkill.h"
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <format>


// ==================== 静态数据 ====================

const std::unordered_map<std::string, Character::Info> Character::infos = {
	{"白板",     {"其他", Level::F, 1}},
	{"特朗普",   {"元首", Level::D, {被动<粪怒>}, 145}},
	{"棍母",     {"网络", Level::F, {被动<隐身>}, 100}},
	{"夏搏",     {"实验", Level::F, {被动<顶置>}, 114}},
	{"雨姐",     {"网络", Level::D, {被动<带派>}, 275}},
	{"神里绫华", {"原神", Level::D, {被动<寒魄>}, 175}},
	{"瑜伽一",   {"实验", Level::F, {被动<割腕, 丑皇>}, 230}},
	{"李阳",     {"网络", Level::C, {被动<军国, 家暴>}, 185}},
	{"薛维旭",   {"实验", Level::D, {被动<健身, 做题>}, 190}},
	{"Tung Tung Tung Tung Tung Tung Tung Tung Tung Sahur", {"山海经", Level::S, {被动<棍击, 神木>}, 100}},
	{"雷电将军", {"原神", Level::D, {被动<雷剑>}, 175}},
	{"王天一",   {"网络", Level::C, {被动<买棋, 卖棋>}, 140}},
	{"Tralalero Tralala",    {"山海经", Level::C, {被动<耐克>}, 175}},
	{"Bombardiro Crocodilo", {"山海经", Level::A, {被动<轰炸>, 即时<装弹>}, 185}},
	{"Bumbumbini Guzzini",   {"山海经", Level::A, {被动<爆破>, 即时<装弹>}, 185}},
	{"Alan Walker",          {"网络",   Level::D, {被动<电音, 蒙面>}, 175}},
	{"丁真",     {"网络",  Level::C, {被动<锐刻>}, 150}},
	{"代增玉",   {"实验",  Level::F, {被动<巨富, 破产>}, 275}},
	{"潘子",     {"网络",  Level::F, {被动<假酒>}, 120}},
	{"土语",     {"实验",  Level::D, {被动<窃观>}, 205}},
	{"Notch",    {"网络",  Level::C, {被动<生存, 创造>}, 140}},
	{"新诸葛亮", {"新三国", Level::B, {被动<炼兵, 好火>}, 77}},
	{"Brr Brr Patapim", {"山海经", Level::B, {被动<森罗, 大脚>}, 200}},
	{"新关羽",   {"新三国", Level::B, {被动<过江, 大盏>}, 200}},
	{"卞相壹",   {"网络",   Level::B, {被动<举报, 猥琐>}, 160}},
	{"柯洁",     {"网络",   Level::B, {被动<棋王, 金铲>}, 160}},
	{"老友",     {"实验",   Level::A, {被动<淘汰>}, 160}},
	{"屎軖",     {"实验",   Level::A, {即时<招待>}, 160}},
	{"植物人",   {"实验",   Level::F, {被动<光合>}, 200}},
	{"梅西",     {"网络",   Level::B, {被动<射门>}, 220}},
	{"二次元",   {"原神",   Level::F, {被动<追番, 崩三>}, 100}},
	{"金正日",   {"元首",   Level::B, {被动<望日>}, 188}},
	{"金日成",   {"元首",   Level::C, {被动<朔日>}, 199}},
	{"刘建龙",   {"网络",   Level::D, {即时<徒步>}, 250}},
	{"拜登",     {"元首",   Level::A, {被动<健忘>}, 125}},
	{"王耘浩",   {"实验",   Level::D, {被动<豪赌>}, 250}},
	{"Bulbito Bandito Traktorito", {"山海经", Level::B, {被动<黑帮, 拖拉>}, 225}},
	{"烟刻瑯",   {"实验",   Level::B, {被动<迷烟>}, 175}},
	{"幺幺",     {"网络",   Level::F, {被动<水鬼>}, 88}},
	{"蒋介石",   {"元首",   Level::F, {被动<叛党>}, 180}},
	{"斯大林",   {"元首",   Level::C, {被动<清洗>}, 185}},
	{"龚俊清",   {"网络",   Level::B, {被动<落水, 骚扰>}, 198}},
	{"Blueberrini Octopussini", {"山海经", Level::D, {转换<八爪>}, 100}},
	{"闫传学",   {"实验",   Level::A, {被动<犬子>}, 222}},
	{"霍金",     {"网络",   Level::C, {被动<黑洞>}, 88}},
	{"峰哥",     {"网络",   Level::A, {被动<好事, 压抑>}, 250}},
	{"包贝尔",   {"网络",   Level::B, {被动<捉奸>, 转换<我妈>}, 160}},
	{"科比",     {"网络",   Level::C, {转换<曼巴>}, 248}},
	{"翟钊",     {"实验",   Level::F, {即时<摘罩>}, 150}},
	{"田淑丽",   {"实验",   Level::C, {即时<还击>}, 145}},
	{"虎哥",     {"网络",   Level::F, {被动<有活>}, 275}},
	{"大章鱼",   {"实验",   Level::C, {被动<爬竿, 渊涡>}, 275}},
	{"杨坤",     {"网络",   Level::C, {被动<没座, 空空>}, 205}},
	{"大脚忍者", {"网络",   Level::A, {被动<暗忍>, 即时<舞爪>}, 300}},
	{"大中医",   {"实验",         Level::A, {被动<治病>}, 100}},
	{"新陆逊",   {"新三国",       Level::B, {被动<连营, 困界>}, 150}},
	{"电棍",     {"网络",         Level::F, {被动<四麻>, 即时<四霸>}, 44}},
	{"8比特",    {"荒野乱斗",     Level::S, {被动<爆射>, 即时<装币>}, 288}},
	{"格斯",     {"荒野乱斗",     Level::C, {被动<灵爆>, 即时<幽愈>}, 100}},
	{"斯图",     {"荒野乱斗",     Level::S, {被动<加速>, 即时<炫技>}, 120}},
	//{"大司马",   {"网络", Level::C, {被动<走位>}, 150}},
	{"新静姝",   {"新三国",       Level::B, {即时<调羹>},      166}},
	{"唐伯虎",   {"网络",         Level::C, {被动<九一, 白虎>}, 91}},
	{"新吕蒙",   {"新三国",       Level::B, {被动<易主, 渡荆>}, 200}},
	{"格里夫",   {"荒野乱斗",     Level::C, {被动<返现>, 即时<挥金>}, 200}},
	{"斯派克",   {"荒野乱斗",     Level::A, {被动<尖刺>, 即时<再生>}, 50}},
	{"柯尔特",   {"荒野乱斗",     Level::F, {被动<弹暴>, 即时<手枪>}, 150}},
	{"科斯莫",   {"荒野乱斗",     Level::S, {被动<星轨>, 即时<引力>}, 100}},
	{"切斯特",   {"荒野乱斗",     Level::A, {被动<铃铛>},            140}},
	{"艾尔·普里莫", {"荒野乱斗",  Level::D, {被动<健体>,       即时<肘击>}, 315}},
	{"黑鸦",        {"荒野乱斗",  Level::B, {被动<飞刃, 淬毒>, 即时<突袭>}, 100}},
	{"弗兰肯",      {"荒野乱斗",  Level::B, {被动<重锤>,       即时<猛击>}, 500}},
	{"瑟奇",        {"荒野乱斗",  Level::A, {被动<劲凉>,       即时<绝技>}, 135}},
	{"天意爷",      {"新三国",    Level::S, {被动<侵蚀, 修正, 剧变>},       333}},
	{"新司马懿",    {"新三国",    Level::A, {被动<兵多, 通天>},             150}},
	{"迪克",        {"荒野乱斗",  Level::C, {被动<地雷>},                  100}},
	{"博尔特",      {"荒野乱斗",  Level::A, {被动<冲撞>, 即时<过载>},       200}},
};

// ==================== 构造 / 工厂 ====================
Character::Character(const std::string& _name,
					 const std::string& _skin)
	:names{ _name }, skins{ _skin } {}

std::unique_ptr<Character> Character::make(const std::string& name, const std::string& skin) {
	auto it = infos.find(name);
	if (it == infos.end()) {
		throw std::invalid_argument(std::format("角色 <{}> 未在 Character::infos 中定义", name));
	}
	const Info& info = it->second;

	auto newChara = std::make_unique<Character>(name, skin);
	//被动技能
	for (const auto& factory : info.passiveSkills) {
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

std::unique_ptr<Character> Character::makeCombined(const std::string& name1, const std::string& skin1,
												   const std::string& name2, const std::string& skin2) {
	auto it1 = infos.find(name1);
	auto it2 = infos.find(name2);
	if (it1 == infos.end()) throw std::invalid_argument(std::format("角色 <{}> 未在 Character::infos 中定义", name1));
	if (it2 == infos.end()) throw std::invalid_argument(std::format("角色 <{}> 未在 Character::infos 中定义", name2));
	const Info& info1 = it1->second;
	const Info& info2 = it2->second;

	//组合名 = name1+name2；直接构造带 names={name1,name2} 的对象
	auto newChara = std::make_unique<Character>(name1, skin1);
	newChara->names.push_back(name2);
	newChara->skins.push_back(skin2);

	//依次加入两角色全部技能（工厂克隆，天然含子技能）
	auto addAllSkillsFrom = [&newChara](const Info& info) {
		for (const auto& f : info.passiveSkills) newChara->addSkill(f());
		for (const auto& f : info.instantSkills) newChara->addSkill(f());
		for (const auto& f : info.transformSkills) newChara->addSkill(f());
	};
	addAllSkillsFrom(info1);
	addAllSkillsFrom(info2);

	//组合角色体力：平均向上取十
	const hp_t hp1 = info1.hp;
	const hp_t maxHp1 = info1.maxHp;
	const hp_t hp2 = info2.hp;
	const hp_t maxHp2 = info2.maxHp;
	newChara->hp = static_cast<hp_t>(unool::math::ceil((hp1 + hp2) / 20.0) * 10);
	newChara->maxHp = static_cast<hp_t>(unool::math::ceil((maxHp1 + maxHp2) / 20.0) * 10);
	return newChara;
}


// ==================== 基本信息 ====================
std::string Character::getName() const {
	if (names.size() == 1) return names[0];
	return std::format("{}&{}", names[0], names[1]);
}

Character::Level Character::getLevel() const {
	//组合角色调用属编程错误，应由调用方改用 getMaxLevel/getMinLevel
	if (isCombined()) throw std::logic_error("组合角色不支持 getLevel，请用 getMaxLevel/getMinLeve");
	if (auto it = infos.find(names[0]); it != infos.end()) return it->second.level;
	else throw std::invalid_argument("此角色未定义等级");
}
std::vector<Character::Level> Character::getLevels() const {
	std::vector<Level> result;
	result.reserve(names.size());
	for (const auto& n : names) {
		if (auto it = infos.find(n); it != infos.end()) result.push_back(it->second.level);
		else throw std::invalid_argument(std::format("角色 <{}> 未在 Character::infos 中定义", n));
	}
	return result;
}
Character::Level Character::getMaxLevel() const {
	auto levels = getLevels();
	return *std::ranges::max_element(levels);
}
Character::Level Character::getMinLevel() const {
	auto levels = getLevels();
	return *std::ranges::min_element(levels);
}
int Character::getScore(Level winner, Level loser) {
	auto idx = [](Level lv) { return 5 - static_cast<int>(lv); };
	return unool::scoreboard[idx(winner)][idx(loser)];
}
std::string Character::skillsName() const {
	std::string result;
	for (const auto& ps : passiveSkills) {
		result += std::format("{}, ", ps->getName());
	}
	for (const auto& as : instantSkills) {
		result += std::format("{}, ", as->getName());
	}
	for (const auto& as : transformSkills) {
		result += std::format("{}, ", as->getName());
	}
	return result;
}
std::string Character::getSkillsText() const {
	std::string result;
	for (const auto& ps : passiveSkills) {
		result += std::format("【{}】（被动技能）\n{}\n", ps->getName(), ps->getInfo());
	}
	for (const auto& as : instantSkills) {
		result += std::format("【{}】（主动技能）\n{}\n", as->getName(), as->getInfo());
	}
	for (const auto& as : transformSkills) {
		result += std::format("【{}】（主动技能）\n{}\n", as->getName(), as->getInfo());
	}
	return result;
}
std::string Character::getImagePath() const {
	//单角色返回唯一图路径；组合角色调用属编程错误，请用 getImagePaths
	if (isCombined()) throw std::logic_error("组合角色不支持 getImagePath，请用 getImagePaths");
	return getImagePath(names[0], skins[0]);
}
std::vector<std::string> Character::getImagePaths() const {
	std::vector<std::string> result;
	result.reserve(names.size());
	for (std::size_t i = 0; i < names.size(); ++i) {
		result.push_back(getImagePath(names[i], skins[i]));
	}
	return result;
}
bool Character::operator<(const Character& other) const {
	return getName() < other.getName();
}
bool Character::operator==(const Character& other) const {
	return getName() == other.getName();
}


// ==================== 静态工具 ====================
std::string Character::to_string(Level level) {
	switch (level) {
		case Level::F: return "F";
		case Level::D: return "D";
		case Level::C: return "C";
		case Level::B: return "B";
		case Level::A: return "A";
		case Level::S: return "S";
		default:       return "?";
	}
}

std::string Character::getImagePath(const std::string& name, const std::string& skin) {
	auto it = infos.find(name);
	if (it == infos.end()) {
		throw std::invalid_argument(std::format("角色 <{}> 未在 Character::infos 中定义", name));
	}
	const std::string& group = it->second.group;
	return std::format("images/characters/{}/{}/{}.jpg", group, name, skin);
}
std::vector<std::string> Character::getSkins(const std::string& name) {
	namespace fs = std::filesystem;
	auto it = infos.find(name);
	if (it == infos.end()) {
		throw std::invalid_argument(std::format("角色 <{}> 未在 Character::infos 中定义", name));
	}
	const std::string& group = it->second.group;
	const fs::path dir = fs::path("../images/characters") / unool::string::to_utf16(group) / unool::string::to_utf16(name);
	if (!fs::exists(dir) || !fs::is_directory(dir)) {
		std::println(stderr, "角色 <{}> 的皮肤目录不存在", name);
		return {};
	}
	std::vector<std::string> skins;
	for (const auto& entry : fs::directory_iterator(dir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".jpg") {
			skins.push_back(unool::string::to_utf8(entry.path().stem().wstring()));
		}
	}
	if (skins.empty()) {
		std::println(stderr, "角色 <{}> 的皮肤目录下无 .jpg 文件", name);
		return {};
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
		std::vector chars = unool::getServerConfig()["characters"]["shielded"]["characters"].get<std::vector<std::string>>();
		return std::unordered_set<std::string>{ chars.begin(), chars.end() };
	}();
	const std::unordered_set<std::string> shieldedGroups = [] {
		std::vector chars = unool::getServerConfig()["characters"]["shielded"]["groups"].get<std::vector<std::string>>();
		return std::unordered_set<std::string>{ chars.begin(), chars.end() };
	}();

	//构造可用角色
	auto filteredChars = Character::infos | std::views::filter(
		[&shieldedCharacters, &shieldedGroups](const Character::Entry& entry) {
		//过滤掉白板和被屏蔽的角色
		return entry.first != "白板"
			&& !shieldedCharacters.contains(entry.first)
			&& !shieldedGroups.contains(entry.second.group);
	});
	const std::size_t filteredCharsSize = std::ranges::distance(filteredChars);

	//判断可用角色数量是否足够
	if (n > filteredCharsSize) {
		throw std::invalid_argument(
			std::format("候选角色数量({})不能超过可选角色数量({})", n, filteredCharsSize)
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
void Character::launchPassiveSkills(const PassiveSkill::TriggerTime& currentTriggerTime,
									GameLogic& game, Player& carrier,
									PassiveSkill::Trigger& trigger) {
	//先收集要发动的技能引用，避免content中修改passiveSkills导致迭代器失效
	std::vector<ref<PassiveSkill>> toLaunch;
	for (const auto& ps : passiveSkills) {
		if (ps->matchTrigger(currentTriggerTime, carrier, trigger))
			toLaunch.push_back(*ps);
		for (auto& sub : ps->subSkills | std::views::filter([](const std::unique_ptr<Skill>& sub) {
			return sub->isPassive();
		}) | std::views::transform([](const std::unique_ptr<Skill>& sub) -> PassiveSkill& {
			return sub->toPassive();
		})) {
			if (sub.matchTrigger(currentTriggerTime, carrier, trigger))
				toLaunch.push_back(std::ref(sub));
		}
	}
	//遍历指针列表发动；即使某个技能在content中销毁自身，也不影响后续技能
	for (PassiveSkill& skill : toLaunch) {
		skill.launch(game, carrier, trigger);
	}
}

void Character::addSkill(std::unique_ptr<PassiveSkill> skill) {
	passiveSkills.push_back(std::move(skill));
}
void Character::addSkill(std::unique_ptr<InstantSkill> skill) {
	instantSkills.push_back(std::move(skill));
}
void Character::addSkill(std::unique_ptr<TransformSkill> skill) {
	transformSkills.push_back(std::move(skill));
}

void Character::resetSkills() {
	for (auto& s : passiveSkills) {
		s->reset();
	}
	for (auto& s : instantSkills) {
		s->reset();
	}
	for (auto& s : transformSkills) {
		s->reset();
	}
}


// ==================== 体力管理 ====================
Character::hp_t Character::getHp() const {
	return hp;
}
Character::hp_t Character::getMaxHp() const {
	return maxHp;
}
void Character::setHp(hp_t newHp) {
	hp = std::min(newHp, maxHp);
}
std::size_t Character::damage(std::size_t damage) {
	hp -= static_cast<hp_t>(damage);
	return damage;
}
void Character::recover(std::size_t num) {
	hp = std::min(hp + static_cast<hp_t>(num), maxHp);
}
bool Character::isDead() const {
	return hp <= 0;
}

std::size_t Character::removeMark(const std::string& m, std::size_t count) {
	auto it = marks.find(m);
	if (it == marks.end()) return 0;
	std::size_t actual = std::min(it->second, count);
	if (it->second > count) it->second -= count;
	else marks.erase(it);
	return actual;
}


