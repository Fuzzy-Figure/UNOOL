#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace unool::core {

	// 服务端配置出站端口。
	// 每个方法与重构前 GameLogic / Character / ServerMain 直读
	// server_config.json 的表达式一一对应；适配器内部保留原 json 访问写法，
	// 保证默认值、缺键时的异常行为与每局 reload 的时机逐字不变。
	class IConfigProvider {
	public:
		virtual ~IConfigProvider() = default;

		// getServerConfig().value("mode", "normal")
		virtual std::string mode() const = 0;
		// getServerConfig()["rules"][rulesKey].value("candidateCount", 5)
		// rulesKey 为 "normal" / "double"，与重构前传入的字面量一致
		virtual std::size_t candidateCount(const std::string& rulesKey) const = 0;
		// getServerConfig()["rules"][rulesKey]["initHandCount"]
		virtual std::size_t initHandCount(const std::string& rulesKey) const = 0;
		// getServerConfig()["rules"].value("banCount", 0)
		virtual std::size_t banCount() const = 0;
		// getServerConfig()["characters"]["shielded"]["characters"]
		virtual std::vector<std::string> shieldedCharacters() const = 0;
		// getServerConfig()["characters"]["shielded"]["groups"]
		virtual std::vector<std::string> shieldedGroups() const = 0;
		// "characters"/"assign" 存在时返回指定角色，否则 nullopt
		virtual std::optional<std::vector<std::string>> assignedCharacters() const = 0;
		// getServerConfig()["network"].value("reconnectTimeoutSec", 600)
		virtual int reconnectTimeoutSec() const = 0;
		// reloadServerConfig()
		virtual void reload() = 0;
	};
}
