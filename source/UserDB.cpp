#include "UserDB.h"
#include "utils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>


namespace {
	constexpr const char* DATA_FILE = "../userDatas.json";
}

UserDB& UserDB::instance() {
	static UserDB instance;
	return instance;
}

bool UserDB::registerUser(const std::string& username, const std::string& password, std::string& errorMessage) {
	if (username.empty()) {
		errorMessage = "用户名不能为空";
		return false;
	}
	if (password.empty()) {
		errorMessage = "密码不能为空";
		return false;
	}
	if (users_.count(username)) {
		errorMessage = "用户名已存在";
		return false;
	}
	UserInfo info;
	info.password = password;
	info.points = 0;
	info.wins = 0;
	info.losses = 0;
	users_[username] = info;
	save();
	std::println("[UserDB] 注册成功: {}", username);
	return true;
}

std::optional<UserInfo> UserDB::login(const std::string& username, const std::string& password, std::string& errorMessage) const {
	auto it = users_.find(username);
	if (it == users_.end()) {
		errorMessage = "用户名不存在";
		return std::nullopt;
	}
	if (it->second.password != password) {
		errorMessage = "密码错误";
		return std::nullopt;
	}
	std::println("[UserDB] 登录成功: {}", username);
	return it->second;
}

void UserDB::addMatchResult(const std::string& winnerUser, const std::string& loserUser,
							const std::vector<Character::Level>& winnerLevels,
							const std::vector<Character::Level>& loserLevels,
							bool winnerFullHp,
							Character::SkinQuality winnerSkinQuality,
							Character::SkinQuality loserSkinQuality,
							bool winnerBonusCard) {
	// 1. 基础 delta 计算
	int delta = 0;
	std::size_t n = std::min(winnerLevels.size(), loserLevels.size());
	for (std::size_t i = 0; i < n; ++i) {
		delta += Character::getScore(winnerLevels[i], loserLevels[i]);
	}
	// 2. 满血翻倍
	if (winnerFullHp) delta *= 2;
	// 3. 增分卡翻倍（可与满血叠加为 ×4）
	if (winnerBonusCard) delta *= 2;

	// 记录加成说明
	std::string bonusDesc;
	if (winnerFullHp) bonusDesc += " [满血翻倍]";
	if (winnerBonusCard) bonusDesc += " [增分卡翻倍]";

	// 4. 胜者皮肤品质加成（在翻倍后触发）
	// 饮料限定皮肤在品质加成中等同普通（其效果为打出万能牌+1，已实时结算）
	auto winnerQualityForBonus = winnerSkinQuality;
	if (winnerQualityForBonus == Character::SkinQuality::drink)
		winnerQualityForBonus = Character::SkinQuality::normal;
	switch (winnerQualityForBonus) {
		case Character::SkinQuality::normal:  break;
		case Character::SkinQuality::fine:    delta += 1; bonusDesc += " [精品+1]"; break;
		case Character::SkinQuality::epic:    delta += 2; bonusDesc += " [史诗+2]"; break;
		case Character::SkinQuality::legend: {
			int bonus = static_cast<int>(std::ceil(delta * 0.2));
			delta += bonus;
			bonusDesc += std::format(" [传说+{}]", bonus);
			break;
		}
		case Character::SkinQuality::supreme:  delta += 5; bonusDesc += " [至尊+5]"; break;
		default: break;
	}

	auto wit = users_.find(winnerUser);
	auto lit = users_.find(loserUser);
	if (wit != users_.end()) {
		wit->second.points += delta;
		wit->second.wins += 1;
		std::println("[UserDB] 玩家{}胜利，获得 {} 积分（当前 {}）{}", winnerUser, delta, wit->second.points, bonusDesc);
	}
	if (lit != users_.end()) {
		lit->second.losses += 1;
		// 至尊皮肤败者也 +5
		auto loserQualityForBonus = loserSkinQuality;
		if (loserQualityForBonus == Character::SkinQuality::drink)
			loserQualityForBonus = Character::SkinQuality::normal;
		if (loserQualityForBonus == Character::SkinQuality::supreme) {
			lit->second.points += 5;
			std::println("[UserDB] 玩家{}败北但穿戴至尊皮肤，获得 5 积分（当前 {}）", loserUser, lit->second.points);
		}
	}
	save();
}

void UserDB::useItem(const std::string& username, const std::string& itemName) {
	auto it = users_.find(username);
	if (it == users_.end()) return;
	auto iit = it->second.items.find(itemName);
	if (iit == it->second.items.end()) return;
	if (iit->second > 0) {
		iit->second -= 1;
		const int remaining = iit->second;
		if (remaining == 0) it->second.items.erase(iit);
		save();
		std::println("[UserDB] 玩家{}消耗道具{}（剩余{}）", username, itemName, remaining);
	}
}

void UserDB::addPoints(const std::string& username, int amount) {
	auto it = users_.find(username);
	if (it == users_.end()) return;
	it->second.points += amount;
	save();
	std::println("[UserDB] 玩家{}获得{}积分（当前{}）", username, amount, it->second.points);
}

int UserDB::getPoints(const std::string& username) const {
	auto it = users_.find(username);
	if (it == users_.end()) return 0;
	return it->second.points;
}

bool UserDB::trySpendPoints(const std::string& username, int amount) {
	if (amount <= 0) return false;
	auto it = users_.find(username);
	if (it == users_.end()) return false;
	if (it->second.points < amount) return false;
	it->second.points -= amount;
	save();
	std::println("[UserDB] 玩家{}消耗{}积分（当前{}）", username, amount, it->second.points);
	return true;
}

void UserDB::load() {
	std::ifstream file(DATA_FILE);
	if (!file.is_open()) {
		std::println("[UserDB] {} 不存在，初始化空数据库", DATA_FILE);
		users_.clear();
		save();
		return;
	}
	try {
		nlohmann::json j;
		j = nlohmann::json::parse(file, nullptr, true, true);
		users_ = j.get<std::unordered_map<std::string, UserInfo>>();
	} catch (const std::exception& e) {
		std::println(stderr, "[UserDB] 解析 {} 失败: {}", DATA_FILE, e.what());
		throw;
	}
}

namespace {
// 自定义 JSON 序列化：ownedCharacters 的皮肤数组用紧凑单行格式
void dumpJson(std::ostream& os, const nlohmann::json& j, int indent, bool compactArrays) {
	auto pad = [&](int n) { for (int i = 0; i < n; ++i) os << "  "; };

	if (j.is_object()) {
		if (j.empty()) { os << "{}"; return; }
		os << "{\n";
		bool first = true;
		for (auto it = j.begin(); it != j.end(); ++it) {
			if (!first) os << ",\n";
			first = false;
			pad(indent + 1);
			os << "\"" << it.key() << "\": ";
			// 当 key 为 ownedCharacters 时，其值的数组子项用紧凑格式
			bool compact = (it.key() == "ownedCharacters") ? true : compactArrays;
			dumpJson(os, it.value(), indent + 1, compact);
		}
		os << "\n";
		pad(indent);
		os << "}";
	}
	else if (j.is_array()) {
		if (j.empty()) { os << "[]"; return; }
		if (compactArrays) {
			// 紧凑单行：只对基本类型数组有效
			os << "[ ";
			bool first = true;
			for (const auto& elem : j) {
				if (!first) os << ", ";
				first = false;
				if (elem.is_string()) os << "\"" << elem.get<std::string>() << "\"";
				else os << elem.dump();
			}
			os << " ]";
		}
		else {
			os << "[\n";
			bool first = true;
			for (const auto& elem : j) {
				if (!first) os << ",\n";
				first = false;
				pad(indent + 1);
				dumpJson(os, elem, indent + 1, false);
			}
			os << "\n";
			pad(indent);
			os << "]";
		}
	}
	else if (j.is_string()) {
		os << "\"" << j.get<std::string>() << "\"";
	}
	else {
		os << j.dump();
	}
}
}

void UserDB::save() const {
	nlohmann::json j = users_;
	std::ofstream file(DATA_FILE);
	if (!file.is_open()) {
		std::println(stderr, "[UserDB] 无法写入 {}", DATA_FILE);
		return;
	}
	dumpJson(file, j, 0, false);
}

std::vector<std::string> UserDB::getCharacterPool(const std::string& username) const {
	std::vector<std::string> result;
	auto it = users_.find(username);
	if (it == users_.end()) return result;
	for (const auto& [name, skins] : it->second.ownedCharacters) {
		result.push_back(name);
	}
	return result;
}

std::set<std::string> UserDB::getOwnedCharacters(const std::string& username) const {
	std::set<std::string> result;
	auto it = users_.find(username);
	if (it == users_.end()) return result;
	for (const auto& [name, skins] : it->second.ownedCharacters) {
		result.insert(name);
	}
	return result;
}

std::set<std::string> UserDB::getOwnedSkins(const std::string& username, const std::string& charName) const {
	auto it = users_.find(username);
	if (it == users_.end()) return {};
	auto cit = it->second.ownedCharacters.find(charName);
	if (cit == it->second.ownedCharacters.end()) return {};
	return cit->second;
}

int UserDB::getItemCount(const std::string& username, const std::string& itemName) const {
	auto it = users_.find(username);
	if (it == users_.end()) return 0;
	auto iit = it->second.items.find(itemName);
	if (iit == it->second.items.end()) return 0;
	return iit->second;
}

const UserInfo& UserDB::getUserInfo(const std::string& username) const {
	static const UserInfo empty;
	auto it = users_.find(username);
	if (it == users_.end()) return empty;
	return it->second;
}

bool UserDB::purchaseCharacter(const std::string& username, const std::string& charName, int price) {
	auto it = users_.find(username);
	if (it == users_.end()) return false;
	if (it->second.points < price) return false;
	if (!trySpendPoints(username, price)) return false;
	it->second.ownedCharacters[charName] = {"默认"};
	save();
	std::println("[UserDB] 玩家{}解锁角色{}（含默认皮肤）", username, charName);
	return true;
}

bool UserDB::hasAllCharactersOfLevel(const std::string& username, Character::Level level) const {
	auto it = users_.find(username);
	if (it == users_.end()) return false;
	const auto& owned = it->second.ownedCharacters;
	for (const auto& [name, info] : Character::infos) {
		if (name == "白板") continue;
		if (info.level == level && !owned.contains(name)) {
			return false;
		}
	}
	return true;
}

bool UserDB::hasAllNonSRank(const std::string& username) const {
	return hasAllCharactersOfLevel(username, Character::Level::F)
		&& hasAllCharactersOfLevel(username, Character::Level::D)
		&& hasAllCharactersOfLevel(username, Character::Level::C)
		&& hasAllCharactersOfLevel(username, Character::Level::B)
		&& hasAllCharactersOfLevel(username, Character::Level::A);
}

std::optional<std::string> UserDB::checkAndUnlockSRank(const std::string& username) {
	auto it = users_.find(username);
	if (it == users_.end()) return std::nullopt;
	auto& owned = it->second.ownedCharacters;

	// 检查是否集齐 F/D/C/B/A 中任一档
	const std::array<Character::Level, 5> nonSLevels = {
		Character::Level::F, Character::Level::D,
		Character::Level::C, Character::Level::B, Character::Level::A
	};
	bool collectedAny = false;
	for (auto lv : nonSLevels) {
		if (hasAllCharactersOfLevel(username, lv)) {
			collectedAny = true;
			break;
		}
	}
	if (!collectedAny) return std::nullopt;

	// 收集 S 档未解锁角色
	std::vector<std::string> sLocked;
	for (const auto& [name, info] : Character::infos) {
		if (info.level == Character::Level::S && !owned.contains(name)) {
			sLocked.push_back(name);
		}
	}
	if (sLocked.empty()) return std::nullopt;

	// 随机选一个解锁
	const std::size_t idx = unool::random::randomSize_t(0, sLocked.size() - 1);
	std::string chosen = sLocked[idx];
	owned[chosen] = {"默认"};
	save();
	std::println("[UserDB] 玩家{}集齐档位，随机解锁S档角色{}（含默认皮肤）", username, chosen);
	return chosen;
}

bool UserDB::purchaseSkin(const std::string& username, const std::string& charName, const std::string& skinName, int price) {
	auto it = users_.find(username);
	if (it == users_.end()) return false;
	// 没有英雄无法购买皮肤
	if (!it->second.ownedCharacters.contains(charName)) return false;
	if (it->second.points < price) return false;
	if (!trySpendPoints(username, price)) return false;
	it->second.ownedCharacters[charName].insert(skinName);
	save();
	std::println("[UserDB] 玩家{}解锁角色{}的皮肤{}", username, charName, skinName);
	return true;
}

bool UserDB::purchaseItem(const std::string& username, const std::string& itemName, int price) {
	auto it = users_.find(username);
	if (it == users_.end()) return false;
	if (it->second.points < price) return false;
	if (!trySpendPoints(username, price)) return false;
	it->second.items[itemName] += 1;
	save();
	std::println("[UserDB] 玩家{}购买道具{}（当前{}）", username, itemName, it->second.items[itemName]);
	return true;
}


namespace {
	//返回相对当前北京时间（UTC+8）偏移 offsetDays 天的日期字符串 yyyy-mm-dd
	std::string beijingDate(const int offsetDays = 0) {
		using namespace std::chrono;
		const auto shifted = system_clock::now() + hours(8) + hours(24LL * offsetDays);
		const std::time_t stamp = system_clock::to_time_t(shifted);
		std::tm tm{};
#ifdef _WIN32
		gmtime_s(&tm, &stamp);
#else
		gmtime_r(&stamp, &tm);
#endif
		return std::format("{:04}-{:02}-{:02}", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
	}
}

CheckinResult UserDB::checkin(const std::string& username) {
	CheckinResult result;
	auto it = users_.find(username);
	if (it == users_.end()) {
		result.msg = "用户不存在";
		return result;
	}
	UserInfo& info = it->second;

	const std::string today = beijingDate();
	const std::string yesterday = beijingDate(-1);
	//测试账号 1 和 2 不受每日一次限制，可无限签到
	const bool unlimitedCheckin = (username == "1" || username == "2");
	if (!unlimitedCheckin && info.lastCheckinDate == today) {
		result.msg = "今天已签到！";
		result.points = info.points;
		return result;
	}

	//连续签到额外概率次数：昨天签过到才累加，最多七次
	const bool continued = info.lastCheckinDate == yesterday;
	const int bonusTimes = continued ? std::min(info.checkinStreak + 1, 7) : 0;

	//当日“双抽”奖项的衰减统计按日重置
	if (info.checkinPrize3Date != today) {
		info.checkinPrize3Date = today;
		info.checkinPrize3Today = 0;
	}

	//奖池基础概率（%）与每次累计的修正值（%）
	static constexpr std::array<double, 9> baseWeight{ 33.1, 17.9, 12.0, 10.2, 9.2, 7.1, 5.5, 3.5, 1.5 };
	static constexpr std::array<double, 9> weightDelta{ -0.8, -0.5, 0.4, 0.3, 0.2, 0.1, 0.1, 0.1, 0.1 };

	auto currentWeights = [&]() {
		std::array<double, 9> w = baseWeight;
		for (std::size_t i = 0; i < w.size(); ++i) w[i] += weightDelta[i] * bonusTimes;
		//“双抽”项：当日每触发一次本项概率-1.5%
		w[2] -= 1.5 * info.checkinPrize3Today;
		return w;
	};

	//抽取一项奖品，返回奖品描述（“双抽”项会递归再抽两次，可再次抽到本项）
	std::function<std::vector<std::string>(int)> drawPrize;
	drawPrize = [&](const int depth) -> std::vector<std::string> {
		std::array<double, 9> w = currentWeights();
		//极端递归保护：最多约32件奖品，超过深度后“双抽”项不再触发
		if (depth >= 5) w[2] = 0.0;
		double total = 0.0;
		for (const double x : w) total += std::max(0.0, x);

		double roll = static_cast<double>(unool::random::randomInt(0, 9999)) / 100.0;
		if (total > 0.0) roll = roll * total / 100.0;
		double acc = 0.0;
		std::size_t idx = 0;
		for (std::size_t i = 0; i < w.size(); ++i) {
			acc += std::max(0.0, w[i]);
			idx = i;
			if (roll < acc) break;
		}

		switch (idx) {
		case 0:  //5积分
			info.points += 5;
			return { "5积分" };
		case 1: {  //一张加分卡/不死图腾，或两张炸弹卡
			const std::size_t pick = unool::random::randomSize_t(1, 3);
			if (pick == 1) {
				info.items["增分卡"] += 1;
				return { "增分卡×1" };
			}
			if (pick == 2) {
				info.items["不死图腾"] += 1;
				return { "不死图腾×1" };
			}
			info.items["炸弹卡"] += 2;
			return { "炸弹卡×2" };
		}
		case 2: {  //本次改为从奖池随机抽取两项（仅当天逐次衰减；再次命中本项则继续翻倍）
			info.checkinPrize3Today += 1;
			auto first = drawPrize(depth + 1);
			auto second = drawPrize(depth + 1);
			first.insert(first.end(), second.begin(), second.end());
			return first;
		}
		case 3:  //20积分
			info.points += 20;
			return { "20积分" };
		case 4: {  //随机未拥有的价格100以下的任意英雄
			std::vector<std::string> candidates;
			for (const auto& [name, data] : Character::infos) {
				if (name == "白板" || info.ownedCharacters.contains(name)) continue;
				auto price = Character::getCharacterPrice(name);
				if (!price.has_value() || price.value() >= 100) continue;
				candidates.push_back(name);
			}
			if (candidates.empty()) {
				info.points += 5;
				return { "5积分（已无可用英雄）" };
			}
			const std::string chosen = unool::random::randomGet(candidates);
			info.ownedCharacters[chosen] = { "默认" };
			return { std::format("英雄【{}】", chosen) };
		}
		case 5:    //随机未拥有的普通或精品品质皮肤
		case 7: {  //随机未拥有的任意非至尊品质皮肤
			std::vector<std::pair<std::string, std::string>> candidates;
			for (const auto& [name, data] : Character::infos) {
				if (name == "白板") continue;
				for (const std::string& skin : Character::getSkins(name)) {
					if (skin == "默认") continue;
					const Character::SkinQuality quality = Character::getSkinQuality(name, skin);
					if (idx == 5 && !(quality == Character::SkinQuality::normal || quality == Character::SkinQuality::fine)) continue;
					if (idx == 7 && quality == Character::SkinQuality::supreme) continue;
					const auto ownedIt = info.ownedCharacters.find(name);
					if (ownedIt != info.ownedCharacters.end() && ownedIt->second.contains(skin)) continue;
					candidates.emplace_back(name, skin);
				}
			}
			if (candidates.empty()) {
				info.points += 5;
				return { "5积分（已无可用皮肤）" };
			}
			const auto chosen = unool::random::randomGet(candidates);
			info.ownedCharacters[chosen.first].insert(chosen.second);
			return { std::format("{}的皮肤【{}】", chosen.first, chosen.second) };
		}
		case 6: {  //随机未拥有的非S任意英雄
			std::vector<std::string> candidates;
			for (const auto& [name, data] : Character::infos) {
				if (name == "白板" || info.ownedCharacters.contains(name)) continue;
				if (data.level == Character::Level::S) continue;
				candidates.push_back(name);
			}
			if (candidates.empty()) {
				info.points += 5;
				return { "5积分（已无可用英雄）" };
			}
			const std::string chosen = unool::random::randomGet(candidates);
			info.ownedCharacters[chosen] = { "默认" };
			return { std::format("英雄【{}】", chosen) };
		}
		case 8:  //500积分
			info.points += 500;
			return { "500积分" };
		default:
			info.points += 5;
			return { "5积分" };
		}
	};

	auto rewards = drawPrize(0);

	//记录签到信息
	info.lastCheckinDate = today;
	info.checkinStreak = continued ? info.checkinStreak + 1 : 1;
	if (info.points < 0) info.points = 0;
	save();

	//每件奖品单独一行：xxx用户获得了【xxx】！
	std::string msg = "签到成功！";
	for (const std::string& reward : rewards) {
		msg += std::format("\n{}用户获得了【{}】！", username, reward);
	}

	result.ok = true;
	result.msg = msg;
	result.points = info.points;
	std::println("[UserDB] {}", result.msg);
	return result;
}
