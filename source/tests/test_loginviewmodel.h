#pragma once

#include "test_support.h"

// Arch-C P1-4：LoginViewModel 单测入口（纯标准库，无窗口 / 无网络 / 无 SFML）。
// 覆盖登录场景三台状态机的迁移规则：输入与焦点、登录 / 注册链、匹配与 3 秒兜底、
// 断线与 GameStart 收尾、签到弹窗、匹配横幅、发包失败回滚。
unool::tests::TestResult runLoginViewModelTests();
