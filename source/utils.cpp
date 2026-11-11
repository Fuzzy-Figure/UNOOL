#include "utils.h"
#include "core/Services.h"
#include "platform/Platform.h"
#include <cmath>
#include <filesystem>
#include <format>
#include <system_error>

// B+：平台 IO 剥离。本单元是 UNOOL.Core 的共享编译单元，被客户端与服务端
// 共同链接，因此不再包含 <Windows.h>，也不再直接调用任何 Win32 API。
// 可执行文件路径与编码转换转发给 header/platform/Platform.h 声明的平台接缝，
// 由各 exe 外壳提供实现；路径查找 / 资源根定位 / 随机 / 数学逻辑保持原样。
// 规则：Core 内任何编译单元都不得 include 平台头，平台实现只出现在 exe 工程。

namespace unool {

	namespace paths {
		namespace fs = std::filesystem;

		// 平台接缝：具体实现由 exe 外壳（source/platform/WindowsPlatform.cpp）提供。
		const fs::path& exeDir() {
			return platform::exeDir();
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
		// 平台接缝：Windows 侧实现与重构前的 MultiByteToWideChar /
		// WideCharToMultiByte 逐字等价，调用方与线上字节均不变。
		std::wstring to_utf16(const std::string& utf8) {
			return platform::toWide(utf8);
		}
		std::string to_utf8(const std::wstring& utf16) {
			return platform::fromWide(utf16);
		}
	}

	namespace random {
		// 真正的 mt19937 已移入适配器的 Mt19937Random；这里只留无状态的端口门面，
		// 使 shuffle / sample / distribution 的所有既有调用点保持源码不变。
		RandomEngine::result_type RandomEngine::operator()() {
			return unool::core::services().random.next();
		}
		RandomEngine rng;
		int randomInt(const int begin, const int end) {
			return unool::core::services().random.intIn(begin, end);
		}
		std::size_t randomSize_t(const std::size_t begin, const std::size_t end) {
			return unool::core::services().random.sizeIn(begin, end);
		}
		bool probability(const double p) {
			return unool::core::services().random.probability(p);
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
