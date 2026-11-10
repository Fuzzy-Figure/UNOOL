#include "platform/Platform.h"

// B+ 平台接缝 · Windows 实现。
//
// 本单元只在 3 个 exe 工程（Client1 / Client2 / Server）里编译，不进 UNOOL.Core：
// 静态库 Core.lib 中被 utils.cpp 引用的 unool::platform::* 符号由外壳解析。
// 因此「Core 的任一共享编译单元都不含 Windows.h」这一安卓前置条件成立，
// 而 Windows 侧实现与重构前 source/utils.cpp 中的代码逐字等价。
//
// 非 Windows 目标请勿把本文件加入编译清单，改由对应平台的实现文件提供同名符号。
#include <Windows.h>
#include <stdexcept>
#include <vector>

namespace unool::platform {
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
				if (len < buffer.size()) {
					return fs::path(std::wstring(buffer.data(), len)).parent_path();
				}
				buffer.resize(buffer.size() * 2);
			}
		}();
		return dir;
	}

	std::wstring toWide(const std::string& utf8) {
		if (utf8.empty()) return std::wstring();

		int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
		std::wstring wide(len, '\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
		wide.pop_back();
		return wide;
	}

	std::string fromWide(const std::wstring& wide) {
		if (wide.empty()) return std::string();

		int len = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string utf8(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], len, nullptr, nullptr);
		utf8.pop_back();
		return utf8;
	}
}
