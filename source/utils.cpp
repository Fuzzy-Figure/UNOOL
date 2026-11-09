#include "utils.h"
#include "application/Services.h"
#include <Windows.h>
#include <cmath>
#include <filesystem>
#include <format>
#include <system_error>

// 阶段 3 第十二批：JSON 配置缓存门面归位到 source/adapters/JsonConfig.cpp，
// 默认服务定位 services() 归位到 source/application/Services.cpp（两者仍由
// UNOOL.Core 编译一次、3 个 exe 共同链接）。本单元只保留路径 / 编码 / 随机 /
// 数学工具，不再包含任何 adapters 头，也不再需要完整 json.hpp。

namespace unool {

	namespace paths {
		namespace fs = std::filesystem;

		const fs::path& exeDir() {
			static const fs::path dir = []() -> fs::path {
				std::vector<wchar_t> buffer(MAX_PATH);
				for (;;) {
					const DWORD len = ::GetModuleFileNameW(nullptr, buffer.data(),
														   static_cast<DWORD>(buffer.size()));
					if (len == 0) {
						throw std::runtime_error("无法获取当前可执行文件路径");
					}
					// 返回值等于缓冲区长度说明可能被截断，扩容后重试。
					if (len < buffer.size()) {
						return fs::path(std::wstring(buffer.data(), len)).parent_path();
					}
					buffer.resize(buffer.size() * 2);
				}
			}();
			return dir;
		}

		fs::path locate(const std::string& fileName) {
			fs::path dir = exeDir();
			for (int level = 0; level < 8; ++level) {
				std::error_code ec;
				const fs::path candidate = dir / fileName;
				if (fs::is_regular_file(candidate, ec)) return candidate;
				const fs::path parent = dir.parent_path();
				if (parent.empty() || parent == dir) break;  //已到盘符根
				dir = parent;
			}
			return {};
		}

		const fs::path& resourceRoot() {
			static const fs::path root = []() -> fs::path {
				fs::path dir = exeDir();
				for (int level = 0; level < 8; ++level) {
					std::error_code ec;
					const bool hasImages = fs::is_directory(dir / "images", ec);
					const bool hasConfig =
						fs::is_regular_file(dir / "server_config.json", ec)
						|| fs::is_regular_file(dir / "client_config.json", ec);
					if (hasImages || hasConfig) return dir;
					const fs::path parent = dir.parent_path();
					if (parent.empty() || parent == dir) break;  //已到盘符根
					dir = parent;
				}
				// 资源缺失时也绝不回退到 current_path()，只落在 exe 目录，
				// 让后续的文件打开失败可诊断，而不是静默读写错误位置。
				return exeDir();
			}();
			return root;
		}
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
		// 真正的 mt19937 已移入适配器的 Mt19937Random；这里只留无状态的端口门面，
		// 使 shuffle / sample / distribution 的所有既有调用点保持源码不变。
		RandomEngine::result_type RandomEngine::operator()() {
			return unool::application::services().random.next();
		}
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
