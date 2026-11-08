#include "pch.application.h"

// 纯头文件型静态库：给出一个符号，避免 LNK4221（无公共符号）警告。
namespace unool::application::detail {
	int translation_unit_anchor() noexcept { return 0; }
}
