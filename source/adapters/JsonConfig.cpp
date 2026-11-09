#include "adapters/JsonConfig.h"
#include "utils.h"
#include <fstream>
#include <format>
#include <json.hpp>
#include <stdexcept>

// JSON 配置缓存门面：实现从 source/utils.cpp 归位到本单元（阶段 3 第十二批）。
// 仍由 UNOOL.Core 静态库编译一次、3 个 exe 共同链接；读取路径、缓存语义与
// 配置字节逐字不变。JsonConfig.h 只 include json_fwd.hpp，本单元显式要完整定义。
namespace unool {
	namespace {
		nlohmann::json readConfig(const std::string& fileName) {
			const std::filesystem::path exe = paths::exeDir();
			const std::filesystem::path path = paths::locate(fileName);
			std::ifstream configFile(path);
			if (!configFile.is_open()) {
				throw std::runtime_error(std::format(
					"无法打开配置文件 {}（已从可执行文件目录 {} 向上查找）",
					fileName, exe.string()));
			}
			// ignore_comments=true：server_config.json 中的 // 注释是受支持的既有格式。
			return nlohmann::json::parse(configFile, nullptr, true, true);
		}
	}

	nlohmann::json& getServerConfig() {
		static nlohmann::json config = readConfig("server_config.json");
		return config;
	}

	void reloadServerConfig() {
		getServerConfig() = readConfig("server_config.json");
	}

	nlohmann::json& getClientConfig() {
		static nlohmann::json config = readConfig("client_config.json");
		return config;
	}

	void reloadClientConfig() {
		getClientConfig() = readConfig("client_config.json");
	}
}
