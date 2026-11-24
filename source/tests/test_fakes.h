#pragma once
// Arch-C P3-5：GameLogic 测试台的假端口。
//
// 领域层只认六个出站端口（IServerView / IDecisionPort / IConfigProvider /
// IUserRepository / IRandom / IClock / IAssetCatalog），把它们全部做成假实现，
// 就能在 Tests 里构造真实 GameLogic、跑真实回合，而不开窗口、不连服务器。
//
// 约定：
// - 假实现只提供「够用且可断言」的行为，不模拟业务规则；
// - 选择请求不做默认应答：输入必须显式预置，出现未预置的交互直接抛异常，
//   把「测试台以为没人会问」变成明确的失败，而不是静默等待或死循环。

#include "contract/Types.h"
#include "core/IAssetCatalog.h"
#include "core/IClock.h"
#include "core/IConfigProvider.h"
#include "core/IDecisionPort.h"
#include "core/IRandom.h"
#include "core/IServerView.h"
#include "core/IUserRepository.h"
#include "core/Services.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <format>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace unool::tests {

	// 服务端配置：一律返回测试台要的固定值，不读文件。
	class FakeConfig final : public core::IConfigProvider {
	public:
		std::string mode() const override { return "normal"; }
		std::size_t candidateCount(const std::string&) const override { return 5; }
		std::size_t initHandCount(const std::string&) const override { return 5; }
		std::size_t banCount() const override { return 0; }
		std::vector<std::string> shieldedCharacters() const override { return {}; }
		std::vector<std::string> shieldedGroups() const override { return {}; }
		std::optional<std::vector<std::string>> assignedCharacters() const override { return std::nullopt; }
		int reconnectTimeoutSec() const override { return 600; }
		// 房间功能：测试台固定给 4 个槽位（= 2 个房间）
		std::size_t maxClients() const override { return 4; }
		void reload() override {}
	};

	// 用户数据：没有存档，任何查询都返回空 / 0。
	// 指定角色初始化路径不读角色池，这些返回值只是为了让端口可构造。
	class FakeUsers final : public core::IUserRepository {
	public:
		bool registerUser(std::string_view, std::string_view, std::string&) override { return false; }
		std::optional<core::AccountInfo> login(std::string_view, std::string_view,
											   std::string&) const override { return std::nullopt; }
		bool exists(std::string_view) const override { return false; }
		bool trySpendPoints(std::string_view, int) override { return true; }

		std::vector<std::string> getCharacterPool(std::string_view) const override { return {}; }
		std::set<std::string> getOwnedCharacters(std::string_view) const override { return {}; }
		std::set<std::string> getOwnedSkins(std::string_view, std::string_view) const override { return {}; }
		int getItemCount(std::string_view, std::string_view) const override { return 0; }
		void useItem(std::string_view, std::string_view) override {}
		void addPoints(std::string_view, int) override {}
		void addMatchResult(const std::string&, const std::string&,
							const std::vector<int>&, const std::vector<int>&,
							bool, int, int, bool) override {}

		core::ShopUserInfo getShopUserInfo(std::string_view) const override { return {}; }
		bool hasAllNonSRank(std::string_view) const override { return false; }
		core::CheckinOutcome checkin(std::string_view) override { return {}; }

		int getPoints(std::string_view) const override { return 0; }
		bool hasAllCharactersOfLevel(std::string_view, int) const override { return false; }
		bool purchaseCharacter(std::string_view, std::string_view, int) override { return false; }
		std::optional<std::string> checkAndUnlockSRank(std::string_view) override { return std::nullopt; }
		bool purchaseSkin(std::string_view, std::string_view, std::string_view, int) override { return false; }
		bool purchaseItem(std::string_view, std::string_view, int) override { return false; }
	};

	// 随机源：固定种子，一次运行内可重复；洗牌 / 抽角色都走这里。
	class FakeRandom final : public core::IRandom {
	public:
		explicit FakeRandom(const std::uint32_t seedValue = 20261120u) : engine(seedValue) {}

		result_type next() override { return engine(); }
		int intIn(const int begin, const int end) override {
			return std::uniform_int_distribution<int>{ begin, end }(engine);
		}
		std::size_t sizeIn(const std::size_t begin, const std::size_t end) override {
			return std::uniform_int_distribution<std::size_t>{ begin, end }(engine);
		}
		bool probability(const double p) override { return std::bernoulli_distribution{ p }(engine); }
		void seed(const std::uint32_t value) override { engine.seed(value); }

	private:
		std::mt19937 engine;
	};

	class FakeClock final : public core::IClock {
	public:
		time_point now() const override { return std::chrono::steady_clock::now(); }
	};

	// 资产目录：不碰磁盘，立绘路径按项目布局拼字符串。
	class FakeAssets final : public core::IAssetCatalog {
	public:
		std::vector<std::string> skinsOf(const std::string&, const std::string&) const override {
			return { "默认" };
		}
		std::string imagePath(const std::string& group, const std::string& name,
							  const std::string& skin) const override {
			return std::format("images/characters/{}/{}/{}.jpg", group, name, skin);
		}
	};

	// 出站状态通道：只计数，不发包。
	class FakeServerView final : public core::IServerView {
	public:
		void onState() override { ++stateCount; }
		void onCharInfo() override { ++charInfoCount; }

		std::string usernameOf(const contract::player_id id) const override {
			return std::format("p{}", id);
		}
		bool spendPoints(contract::player_id, int) override { return true; }

		int stateCount = 0;
		int charInfoCount = 0;
	};

	// 决策通道：输入必须预置，未预置的交互直接抛异常。
	class FakeDecision final : public core::IDecisionPort {
	public:
		void poll() override {}

		std::optional<core::DecisionInput> takeInput() override {
			if (inputs.empty()) {
				throw std::runtime_error(
					std::format("测试台输入已耗尽：出现未预置的交互 <{}>", lastTitle));
			}
			auto input = inputs.front();
			inputs.pop_front();
			return input;
		}

		void showChoice(const contract::player_id id, const core::ChoicePrompt& prompt) override {
			lastTitle = prompt.title;
			lastPlayer = id;
			prompts.push_back(prompt);
		}
		void clearChoice(contract::player_id) override {}

		bool isConnected(contract::player_id) const override { return true; }
		bool isLoggedIn(contract::player_id) const override { return true; }
		int reconnectTimeoutSec() const override { return 600; }

		// 预置一次「按 1 确认」：hint / 强制单选项都用它应答。
		void expectConfirm(const contract::player_id id) {
			inputs.push_back(core::DecisionInput{ id, contract::Key::Num1, 0 });
		}

		std::deque<core::DecisionInput> inputs{};
		std::vector<core::ChoicePrompt> prompts{};
		std::string lastTitle{};
		contract::player_id lastPlayer{};
	};

	// 世界：假端口 + 由它们拼成的 DomainServices 聚合。
	struct FakeWorld {
		FakeConfig config{};
		FakeUsers users{};
		FakeRandom random{};
		FakeClock clock{};
		FakeAssets assets{};
		core::DomainServices services{ config, users, random, clock, assets };
		FakeServerView view{};
		FakeDecision decision{};
	};
}
