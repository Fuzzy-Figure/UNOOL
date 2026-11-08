#pragma once
// json.hpp 必须早于 utils.h：后者在返回类型里直接用了 nlohmann::json。
#include <json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <print>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "application/Services.h"
#include "utils.h"
#include "UserDB.h"

namespace unool::adapters {

	// 遗留基础设施适配器：一个类实现全部五个出站端口。
	// 每个 override 都是重构前 GameLogic / Player / Character / ServerMain / utils
	// 里对应表达式的逐字平移，因此未装配的客户端走默认实例时行为完全一致。
	// 采用 header-only 内联实现，避免给 Client1/Client2 引入新的 .cpp 与链接依赖；
	// services() / installServices() 的定义放在所有 exe 都已编译的 source/utils.cpp。
	class LegacyServicesAdapter final
		: public application::IConfigProvider
		, public application::IUserRepository
		, public application::IRandom
		, public application::IClock
		, public application::IAssetCatalog {
	public:
		// === IConfigProvider ===

		std::string mode() const override {
			return unool::getServerConfig().value("mode", "normal");
		}
		std::size_t candidateCount(const std::string& rulesKey) const override {
			return unool::getServerConfig()["rules"][rulesKey].value("candidateCount", 5);
		}
		std::size_t initHandCount(const std::string& rulesKey) const override {
			return unool::getServerConfig()["rules"][rulesKey]["initHandCount"].get<std::size_t>();
		}
		std::size_t banCount() const override {
			return unool::getServerConfig()["rules"].value("banCount", 0);
		}
		std::vector<std::string> shieldedCharacters() const override {
			return unool::getServerConfig()["characters"]["shielded"]["characters"].get<std::vector<std::string>>();
		}
		std::vector<std::string> shieldedGroups() const override {
			return unool::getServerConfig()["characters"]["shielded"]["groups"].get<std::vector<std::string>>();
		}
		std::optional<std::vector<std::string>> assignedCharacters() const override {
			auto& config = unool::getServerConfig();
			if (config.contains("characters") && config["characters"].contains("assign")) {
				return config["characters"]["assign"].get<std::vector<std::string>>();
			}
			return std::nullopt;
		}
		int reconnectTimeoutSec() const override {
			return unool::getServerConfig()["network"].value("reconnectTimeoutSec", 600);
		}
		void reload() override {
			unool::reloadServerConfig();
		}

		// === IUserRepository ===
		// 端口用 string_view / 底层整数，适配器在这里还原为 UserDB 需要的具体类型。

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

		// === IRandom ===
		// 唯一真实引擎。原 utils.cpp 的全局 std::mt19937 变成 utils.h 里的
		// RandomEngine 门面后，所有 shuffle / sample / distribution 仍共用这一条状态。

		result_type next() override {
			return engine_();
		}
		int intIn(int begin, int end) override {
			std::uniform_int_distribution<int> dist(begin, end);
			return dist(engine_);
		}
		std::size_t sizeIn(std::size_t begin, std::size_t end) override {
			std::uniform_int_distribution<std::size_t> dist(begin, end);
			return dist(engine_);
		}
		bool probability(double p) override {
			if (p < 0 || p > 1) throw std::invalid_argument("参数p必须介于0和1之间");
			std::bernoulli_distribution dist(p);
			return dist(engine_);
		}
		void seed(std::uint32_t value) override {
			engine_.seed(value);
		}

		// === IClock ===

		time_point now() const override {
			return std::chrono::steady_clock::now();
		}

		// === IAssetCatalog ===
		// 原 Character::getSkins / getImagePath 的文件系统访问与路径拼装。
		// 领域侧仍负责 infos 查表与异常校验，这里只做 I/O 与字符串拼接。

		std::vector<std::string> skinsOf(const std::string& group,
										const std::string& name) const override {
			namespace fs = std::filesystem;
			const fs::path dir = fs::path("../images/characters")
				/ unool::string::to_utf16(group) / unool::string::to_utf16(name);
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
		std::string imagePath(const std::string& group, const std::string& name,
							  const std::string& skin) const override {
			return std::format("images/characters/{}/{}/{}.jpg", group, name, skin);
		}

	private:
		std::mt19937 engine_{ std::random_device{}() };
	};
}
