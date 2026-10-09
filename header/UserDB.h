#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <set>
#include <map>
#include <vector>
#include <json.hpp>
#include "Character.h"

struct UserInfo {
	std::string password;
	int points = 0;
	int wins = 0;
	int losses = 0;
	std::set<std::string> ownedCharacters;
	std::map<std::string, std::set<std::string>> ownedSkins;
	std::map<std::string, int> items;
	std::vector<std::string> characterPool;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(UserInfo, password, points, wins, losses, ownedCharacters, ownedSkins, items, characterPool)
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

	// 查询用户积分，不存在返回 0
	int getPoints(const std::string& username) const;

	// 尝试消耗 amount 积分：积分足够则扣除并落盘，返回 true；不足返回 false
	bool trySpendPoints(const std::string& username, int amount);

	// 加分：按 scoreboard 表查询并更新双方积分，立即落盘
	// winnerSkinQuality/loserSkinQuality：双方穿戴皮肤品质（用于品质加成）
	// winnerBonusCard：胜者是否使用了增分卡（胜者积分×2）
	void addMatchResult(const std::string& winnerUser, const std::string& loserUser,
						const std::vector<Character::Level>& winnerLevels,
						const std::vector<Character::Level>& loserLevels,
						bool winnerFullHp = false,
						Character::SkinQuality winnerSkinQuality = Character::SkinQuality::normal,
						Character::SkinQuality loserSkinQuality = Character::SkinQuality::normal,
						bool winnerBonusCard = false);

	// 消耗一张道具（数量-1，最少为0），立即落盘
	void useItem(const std::string& username, const std::string& itemName);

	// 直接增加积分（可为负），立即落盘
	void addPoints(const std::string& username, int amount);

	void load();
	void save() const;

	// 获取用户角色池
	const std::vector<std::string>& getCharacterPool(const std::string& username) const;
	// 获取用户拥有的角色集合
	const std::set<std::string>& getOwnedCharacters(const std::string& username) const;
	// 获取用户拥有的皮肤集合
	const std::set<std::string>& getOwnedSkins(const std::string& username, const std::string& charName) const;
	// 获取用户道具数量
	int getItemCount(const std::string& username, const std::string& itemName) const;
	// 获取完整 UserInfo 引用（用于商城数据构建）
	const UserInfo& getUserInfo(const std::string& username) const;
	// 购买角色：积分校验+解锁，返回成功/失败
	bool purchaseCharacter(const std::string& username, const std::string& charName, int price);
	// 检查是否集齐指定档位所有角色（跳过"白板"）
	bool hasAllCharactersOfLevel(const std::string& username, Character::Level level) const;
	// 检查是否集齐 ABCDF 所有角色
	bool hasAllNonSRank(const std::string& username) const;
	// 尝试解锁 S 档角色（集齐某档后自动触发）：返回解锁的角色名，未触发返回 nullopt
	std::optional<std::string> checkAndUnlockSRank(const std::string& username);
	// 购买皮肤：积分校验+解锁，返回成功/失败
	bool purchaseSkin(const std::string& username, const std::string& charName, const std::string& skinName, int price);
	// 购买道具：积分校验+增加数量，返回成功/失败
	bool purchaseItem(const std::string& username, const std::string& itemName, int price);
	// 一次性初始化用户角色池/拥有数据
	void initializeUserData();

private:
	UserDB() { load(); }
	UserDB(const UserDB&) = delete;
	UserDB& operator=(const UserDB&) = delete;

	std::unordered_map<std::string, UserInfo> users_;
};
