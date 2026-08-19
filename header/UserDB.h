#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <json.hpp>
#include "../header/Character.h"

struct UserInfo {
	std::string password;
	int points = 0;
	int wins = 0;
	int losses = 0;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(UserInfo, password, points, wins, losses)
};

class UserDB {
public:
	static UserDB& instance();

	// 注册：成功返回 true；失败返回 false 并通过 errorMessage 输出原因
	bool registerUser(const std::string& username, const std::string& password, std::string& errorMessage);

	// 登录：成功返回 UserInfo；失败返回 nullopt 并通过 errorMessage 输出原因
	std::optional<UserInfo> login(const std::string& username, const std::string& password, std::string& errorMessage) const;

	// 查询用户名是否已存在
	bool exists(const std::string& username) const { return users_.count(username) > 0; }

	// 加分：按 scoreboard 表查询并更新双方积分，立即落盘
	void addMatchResult(const std::string& winnerUser, const std::string& loserUser,
						Character::Level winnerLevel, Character::Level loserLevel,
						bool winnerFullHp = false);

	void load();
	void save() const;

private:
	UserDB() { load(); }
	UserDB(const UserDB&) = delete;
	UserDB& operator=(const UserDB&) = delete;

	std::unordered_map<std::string, UserInfo> users_;
};
