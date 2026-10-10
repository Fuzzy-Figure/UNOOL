#include "UserDB.h"
#include "utils.h"
#include <algorithm>
#include <cmath>
#include <fstream>


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

void UserDB::save() const {
	nlohmann::json j = users_;
	std::ofstream file(DATA_FILE);
	if (!file.is_open()) {
		std::println(stderr, "[UserDB] 无法写入 {}", DATA_FILE);
		return;
	}
	file << j.dump(2);
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
