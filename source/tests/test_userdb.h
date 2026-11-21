#pragma once

// Arch-C P3-5：UserDB（账号存档 / 口令哈希 / 积分 / 商城购买 / 签到）单测入口。
// 全程无窗口无网络；存档路径用 UNOOL_USERDATA 指到系统临时目录，不碰仓库里的 userDatas.json。
#include "test_support.h"

unool::tests::TestResult runUserDbTests();
