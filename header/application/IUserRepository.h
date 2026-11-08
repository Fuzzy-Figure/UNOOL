#pragma once
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace unool::application {

	// 用户数据出站端口。方法与原 UserDB::instance() 调用点一一对应。
	// 端口只用标准库类型：角色档位与皮肤品质以底层整数透传
	// （与 Character::Level / Character::SkinQuality 数值一致），由适配器还原。
	class IUserRepository {
	public:
		virtual ~IUserRepository() = default;

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
	};
}
