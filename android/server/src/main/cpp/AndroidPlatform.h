#pragma once
#include <string>

namespace unool::platform {
	// 由 JNI 在进入组合根之前注入 App 的 filesDir；exeDir() 会返回它。
	void setAndroidDataDir(const std::string& dir);
}
