#include "AndroidPlatform.h"
#include "platform/Platform.h"

#include <cstdint>
#include <filesystem>

// B+ 平台接缝的 Android 实现。Core 侧对所有平台一视同仁，只依赖
// Platform.h 的四个函数；这里不引入任何 Windows / SFML 头。

namespace {
	std::filesystem::path dataDir{};

	std::wstring utf8ToWide(const std::string& utf8) {
		std::wstring result;
		result.reserve(utf8.size());
		std::size_t i = 0;
		while (i < utf8.size()) {
			const auto first = static_cast<std::uint8_t>(utf8[i]);
			std::uint32_t code = 0;
			std::size_t extra = 0;
			if (first < 0x80) {
				code = first;
			}
			else if ((first & 0xE0) == 0xC0) {
				code = first & 0x1F;
				extra = 1;
			}
			else if ((first & 0xF0) == 0xE0) {
				code = first & 0x0F;
				extra = 2;
			}
			else if ((first & 0xF8) == 0xF0) {
				code = first & 0x07;
				extra = 3;
			}
			else {
				code = 0xFFFD;
			}

			bool valid = extra <= utf8.size() - i - 1;
			for (std::size_t j = 1; valid && j <= extra; ++j) {
				const auto next = static_cast<std::uint8_t>(utf8[i + j]);
				if ((next & 0xC0) != 0x80) {
					valid = false;
					break;
				}
				code = (code << 6) | (next & 0x3F);
			}
			result.push_back(valid ? static_cast<wchar_t>(code) : static_cast<wchar_t>(0xFFFD));
			i += valid ? extra + 1 : 1;
		}
		return result;
	}

	std::string wideToUtf8(const std::wstring& wide) {
		std::string result;
		result.reserve(wide.size());
		for (const wchar_t value : wide) {
			const auto code = static_cast<std::uint32_t>(value);
			if (code < 0x80) {
				result.push_back(static_cast<char>(code));
			}
			else if (code < 0x800) {
				result.push_back(static_cast<char>(0xC0 | (code >> 6)));
				result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
			else if (code < 0x10000) {
				result.push_back(static_cast<char>(0xE0 | (code >> 12)));
				result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
			else {
				result.push_back(static_cast<char>(0xF0 | (code >> 18)));
				result.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
				result.push_back(static_cast<char>(0x80 | (code & 0x3F)));
			}
		}
		return result;
	}
}

namespace unool::platform {

	void setAndroidDataDir(const std::string& dir) {
		dataDir = std::filesystem::path(dir);
	}

	const std::filesystem::path& exeDir() {
		return dataDir;
	}

	std::wstring toWide(const std::string& utf8) {
		return utf8ToWide(utf8);
	}

	std::string fromWide(const std::wstring& wide) {
		return wideToUtf8(wide);
	}

	void setupConsoleUtf8() {
		// Android 无控制台；日志走 Android logcat 与 JNI 回调。
	}
}
