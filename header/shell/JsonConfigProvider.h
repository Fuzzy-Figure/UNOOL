#pragma once
// 本适配器要调用 json 的 value() / operator[] / get<>()，需要完整定义。
#include <json.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "core/IConfigProvider.h"
#include "utils.h"
#include "shell/JsonConfig.h"

namespace unool::shell {

	// 服务端配置出站端口的 JSON 实现。
	// 每个 override 都是重构前 GameLogic / Character / ServerMain / utils
	// 里对应表达式的逐字平移（含默认值、缺键异常行为与每局 reload 时机）。
	// header-only：避免给 Client1/Client2 引入新的 .cpp 与链接依赖。
	class JsonConfigProvider final : public core::IConfigProvider {
	public:
		std::string mode() const override {
			return unool::shell::getServerConfig().value("mode", "normal");
		}
		std::size_t candidateCount(const std::string& rulesKey) const override {
			return unool::shell::getServerConfig()["rules"][rulesKey].value("candidateCount", 5);
		}
		std::size_t initHandCount(const std::string& rulesKey) const override {
			return unool::shell::getServerConfig()["rules"][rulesKey]["initHandCount"].get<std::size_t>();
		}
		std::size_t banCount() const override {
			return unool::shell::getServerConfig()["rules"].value("banCount", 0);
		}
		std::vector<std::string> shieldedCharacters() const override {
			return unool::shell::getServerConfig()["characters"]["shielded"]["characters"].get<std::vector<std::string>>();
		}
		std::vector<std::string> shieldedGroups() const override {
			return unool::shell::getServerConfig()["characters"]["shielded"]["groups"].get<std::vector<std::string>>();
		}
		std::optional<std::vector<std::string>> assignedCharacters() const override {
			auto& config = unool::shell::getServerConfig();
			if (config.contains("characters") && config["characters"].contains("assign")) {
				return config["characters"]["assign"].get<std::vector<std::string>>();
			}
			return std::nullopt;
		}
		int reconnectTimeoutSec() const override {
			return unool::shell::getServerConfig()["network"].value("reconnectTimeoutSec", 600);
		}
		void reload() override {
			unool::shell::reloadServerConfig();
		}
	};
}
