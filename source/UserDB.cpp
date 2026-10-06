#include "UserDB.h"
#include "utils.h"
#include <algorithm>
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
							bool winnerFullHp) {
	int delta = 0;
	std::size_t n = std::min(winnerLevels.size(), loserLevels.size());
	for (std::size_t i = 0; i < n; ++i) {
		delta += Character::getScore(winnerLevels[i], loserLevels[i]);
	}
	if (winnerFullHp) delta *= 2;

	auto wit = users_.find(winnerUser);
	auto lit = users_.find(loserUser);
	if (wit != users_.end()) {
		wit->second.points += delta;
		wit->second.wins += 1;
		std::println("[UserDB] 玩家{}胜利，获得 {} 积分（当前 {}）{}", winnerUser, delta, wit->second.points, (winnerFullHp ? " [满血翻倍]" : ""));
	}
	if (lit != users_.end()) {
		lit->second.losses += 1;
	}
	save();
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
