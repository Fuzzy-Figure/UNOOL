#pragma once
#include <json.hpp>

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "application/IUserRepository.h"
#include "utils.h"
#include "Character.h"
#include "UserDB.h"

namespace unool::adapters {

	// 用户数据出站端口的 UserDB 实现。
	// 端口用 string_view / 底层整数，适配器在这里还原为 UserDB 需要的具体类型。
	// header-only：utils.cpp 现由 UNOOL.Protocol 静态库编译一次、3 个 exe 共同链接，不新增链接依赖。
	class JsonUserRepository final : public application::IUserRepository {
	public:
		std::vector<std::string> getCharacterPool(std::string_view username) const override {
			return UserDB::instance().getCharacterPool(std::string(username));
		}
		std::set<std::string> getOwnedCharacters(std::string_view username) const override {
			return UserDB::instance().getOwnedCharacters(std::string(username));
		}
		std::set<std::string> getOwnedSkins(std::string_view username,
											std::string_view charName) const override {
			return UserDB::instance().getOwnedSkins(std::string(username), std::string(charName));
		}
		int getItemCount(std::string_view username, std::string_view itemName) const override {
			return UserDB::instance().getItemCount(std::string(username), std::string(itemName));
		}
		void useItem(std::string_view username, std::string_view itemName) override {
			UserDB::instance().useItem(std::string(username), std::string(itemName));
		}
		void addPoints(std::string_view username, int amount) override {
			UserDB::instance().addPoints(std::string(username), amount);
		}
		void addMatchResult(const std::string& winnerUser, const std::string& loserUser,
							const std::vector<int>& winnerLevels,
							const std::vector<int>& loserLevels,
							bool winnerFullHp,
							int winnerSkinQuality, int loserSkinQuality,
							bool winnerBonusCard) override {
			const auto toLevels = [](const std::vector<int>& src) {
				std::vector<Character::Level> out;
				out.reserve(src.size());
				for (const int lv : src) out.push_back(static_cast<Character::Level>(lv));
				return out;
			};
			UserDB::instance().addMatchResult(
				winnerUser, loserUser,
				toLevels(winnerLevels), toLevels(loserLevels),
				winnerFullHp,
				static_cast<Character::SkinQuality>(winnerSkinQuality),
				static_cast<Character::SkinQuality>(loserSkinQuality),
				winnerBonusCard);
		}
	};
}
