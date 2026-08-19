#include "../header/UserDB.h"
#include "../header/utils.h"
#include <fstream>
#include <iostream>

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
	std::cout << "[UserDB] 注册成功: " << username << std::endl;
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
	std::cout << "[UserDB] 登录成功: " << username << std::endl;
	return it->second;
}

void UserDB::addMatchResult(const std::string& winnerUser, const std::string& loserUser,
							Character::Level winnerLevel, Character::Level loserLevel,
							bool winnerFullHp) {
	int wi = static_cast<int>(winnerLevel);
	int li = static_cast<int>(loserLevel);
	int delta = unool::scoreboard[wi][li];
	if (winnerFullHp) delta *= 2;

	auto wit = users_.find(winnerUser);
	auto lit = users_.find(loserUser);
	if (wit != users_.end()) {
		wit->second.points += delta;
		wit->second.wins += 1;
		std::cout << "[UserDB] 玩家" << winnerUser << "胜利，获得 "
			<< delta << " 积分（当前 " << wit->second.points << "）"
			<< (winnerFullHp ? " [满血翻倍]" : "") << std::endl;
	}
	if (lit != users_.end()) {
		lit->second.losses += 1;
	}
	save();
}

void UserDB::load() {
	std::ifstream file(DATA_FILE);
	if (!file.is_open()) {
		std::cout << "[UserDB] " << DATA_FILE << " 不存在，初始化空数据库" << std::endl;
		users_.clear();
		save();
		return;
	}
	try {
		nlohmann::json j;
		j = nlohmann::json::parse(file, nullptr, true, true);
		users_ = j.get<std::unordered_map<std::string, UserInfo>>();
	} catch (const std::exception& e) {
		std::cerr << "[UserDB] 解析 " << DATA_FILE << " 失败: " << e.what() << std::endl;
		throw;
	}
}

void UserDB::save() const {
	nlohmann::json j = users_;
	std::ofstream file(DATA_FILE);
	if (!file.is_open()) {
		std::cerr << "[UserDB] 无法写入 " << DATA_FILE << std::endl;
		return;
	}
	file << j.dump(2);
}
