#pragma once
// B+ 平台接缝：由「外壳」（各 exe 工程）提供的少量进程级平台能力。
//
// 这是 UNOOL.Core 与操作系统之间唯一的显式边界：Core 内的所有共享编译单元
// 只声明并调用这三个函数，不再 include <Windows.h>；具体平台的实现由
// source/platform/<平台>/ 下的 .cpp 提供，并只在 exe 工程里编译一次。
// 安卓端只需提供同签名的另一份实现（APK assets 根目录 / UTF 转换），
// 无需改动或重编译规则内核。
//
// 声明只使用标准库类型，因此本头本身可在任何平台编译。
#include <filesystem>
#include <string>

namespace unool::platform {

	// 当前可执行文件所在目录（绝对路径）。Windows 实现沿用 GetModuleFileNameW；
	// 安卓实现可返回由 native activity 注入的 assets / 数据根目录。
	const std::filesystem::path& exeDir();

	std::wstring toWide(const std::string& utf8);
	std::string fromWide(const std::wstring& wide);
}
