#pragma once
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace unool::application {

	// 账号用例所需的精简用户信息；端口不暴露 UserDB::UserInfo（含 JSON 存档字段）。
	struct AccountInfo {
		int points = 0;
		int wins = 0;
		int losses = 0;
	};

	// 商城展示所需的存档投影（W5 第二段）：只保留积分与「角色 → 已解锁皮肤集合」，
	// 不暴露 UserInfo 的密码、签到日期、道具等存档字段。
	struct ShopUserInfo {
		int points = 0;
		std::map<std::string, std::set<std::string>> ownedCharacters;
	};

	// 签到结果（W5 第二段）：与 UserDB::CheckinResult 字段同构，端口不暴露适配器类型。
	struct CheckinOutcome {
		bool ok = false;
		std::string msg;
		int points = 0;
	};

	// 用户数据出站端口。方法由 JsonUserRepository 转发到组合根注入的 UserDB 实例。
	// 端口只用标准库类型：角色档位与皮肤品质以底层整数透传
	// （与 Character::Level / Character::SkinQuality 数值一致），由适配器还原。
	// 阶段 3 第九批补入 4 个账号方法（注册 / 登录 / 查重 / 扣积分），
	// 使 AccountService 不依赖具体存储实现。
	class IUserRepository {
	public:
		virtual ~IUserRepository() = default;

		// === 账号用例（W5 第一段）===
		virtual bool registerUser(std::string_view username, std::string_view password,
								  std::string& errorMessage) = 0;
		virtual std::optional<AccountInfo> login(std::string_view username, std::string_view password,
												  std::string& errorMessage) const = 0;
		virtual bool exists(std::string_view username) const = 0;
		virtual bool trySpendPoints(std::string_view username, int amount) = 0;

		virtual std::vector<std::string> getCharacterPool(std::string_view username) const = 0;
		virtual std::set<std::string> getOwnedCharacters(std::string_view username) const = 0;
		virtual std::set<std::string> getOwnedSkins(std::string_view username, std::string_view charName) const = 0;
		virtual int getItemCount(std::string_view username, std::string_view itemName) const = 0;
		virtual void useItem(std::string_view username, std::string_view itemName) = 0;
		virtual void addPoints(std::string_view username, int amount) = 0;
		virtual void addMatchResult(const std::string& winnerUser, const std::string& loserUser,
								   const std::vector<int>& winnerLevels,
								   const std::vector<int>& loserLevels,
								   bool winnerFullHp,
								   int winnerSkinQuality, int loserSkinQuality,
								   bool winnerBonusCard) = 0;

	// === 商城展示 / 签到用例（W5 第二段）===
	virtual ShopUserInfo getShopUserInfo(std::string_view username) const = 0;
	virtual bool hasAllNonSRank(std::string_view username) const = 0;
	virtual CheckinOutcome checkin(std::string_view username) = 0;

	// === 购买决策用例（W5 第二段第二段）===
	// 档位以底层整数透传（与 Character::Level 数值一致），由适配器还原，
	// 端口头闭包保持纯标准库、不认识领域枚举。
	virtual int getPoints(std::string_view username) const = 0;
	virtual bool hasAllCharactersOfLevel(std::string_view username, int level) const = 0;
	virtual bool purchaseCharacter(std::string_view username, std::string_view charName, int price) = 0;
	virtual std::optional<std::string> checkAndUnlockSRank(std::string_view username) = 0;
	virtual bool purchaseSkin(std::string_view username, std::string_view charName,
							  std::string_view skinName, int price) = 0;
	virtual bool purchaseItem(std::string_view username, std::string_view itemName, int price) = 0;
};
}
