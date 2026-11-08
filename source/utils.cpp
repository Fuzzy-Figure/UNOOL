#include "utils.h"
#include "adapters/LegacyServicesAdapter.h"
#include <Windows.h>
#include <cmath>
#include <fstream>
#include <json.hpp>

namespace unool {
	nlohmann::json& getServerConfig() {
		static nlohmann::json config = [] {
			std::ifstream configFile("../server_config.json");
			if (!configFile.is_open()) {
				throw std::runtime_error("无法打开配置文件 server_config.json");
			}
			return nlohmann::json::parse(configFile, nullptr, true, true);
		}();
		return config;
	}

	void reloadServerConfig() {
		std::ifstream configFile("../server_config.json");
		if (!configFile.is_open()) {
			throw std::runtime_error("无法打开配置文件 server_config.json");
		}
		getServerConfig() = nlohmann::json::parse(configFile, nullptr, true, true);
	}

	nlohmann::json& getClientConfig() {
		static nlohmann::json config = [] {
			std::ifstream configFile("../client_config.json");
			if (!configFile.is_open()) {
				throw std::runtime_error("无法打开配置文件 client_config.json");
			}
			return nlohmann::json::parse(configFile, nullptr, true, true);
		}();
		return config;
	}

	void reloadClientConfig() {
		std::ifstream configFile("../client_config.json");
		if (!configFile.is_open()) {
			throw std::runtime_error("无法打开配置文件 client_config.json");
		}
		getClientConfig() = nlohmann::json::parse(configFile, nullptr, true, true);
	}

	namespace string {
		std::wstring to_utf16(const std::string& utf8) {
			if (utf8.empty()) return std::wstring();

			int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
			std::wstring utf16(len, '\0');
			MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], len);
			utf16.pop_back(); // 去掉结尾的空字符
			return utf16;
		}
		std::string to_utf8(const std::wstring& utf16) {
			if (utf16.empty()) return std::string();

			int len = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, nullptr, 0, nullptr, nullptr);
			std::string utf8(len, '\0');
			WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, &utf8[0], len, nullptr, nullptr);
			utf8.pop_back(); // 去掉结尾的空字符
			return utf8;
		}
	}

	namespace random {
		// 真正的 mt19937 已移入 LegacyServicesAdapter；这里只留无状态的端口门面，
		// 使 shuffle / sample / distribution 的所有既有调用点保持源码不变。
		RandomEngine rng;
		int randomInt(const int begin, const int end) {
			return unool::application::services().random.intIn(begin, end);
		}
		std::size_t randomSize_t(const std::size_t begin, const std::size_t end) {
			return unool::application::services().random.sizeIn(begin, end);
		}
		bool probability(const double p) {
			return unool::application::services().random.probability(p);
		}
	}

	namespace math {
		std::size_t ceil(const double num) {
			return static_cast<std::size_t>(std::ceil(num));
		}
		std::size_t floor(const double num) {
			return static_cast<std::size_t>(std::floor(num));
		}
		std::size_t pow(const std::size_t a, const std::size_t b) {
			return static_cast<std::size_t>(std::pow(a, b));
		}
	}
}

// 默认服务定位：三个 exe 工程原本都编译本文件，因此定义放这里不会新增
// 编译单元或链接依赖。静态工具（Character）与全局随机门面（utils）在
// 未装配时自动落到与重构前逐字等价的遗留实现；仅 ServerMain 显式装配。
namespace unool::application {
	namespace detail {
		DomainServices*& installedSlot() {
			static DomainServices* slot = nullptr;
			return slot;
		}
		DomainServices& legacyBundle() {
			static adapters::LegacyServicesAdapter adapter;
			static DomainServices bundle{ adapter, adapter, adapter, adapter, adapter };
			return bundle;
		}
	}

	DomainServices& services() {
		if (DomainServices* installed = detail::installedSlot()) return *installed;
		return detail::legacyBundle();
	}
	void installServices(DomainServices& s) {
		detail::installedSlot() = &s;
	}
}


