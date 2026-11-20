#pragma once

// Arch-C P3-5：GameLogic 测试台入口（fake-port，无网络 / 无窗口）。
// 覆盖 CURRENT.md「不可丢的玩法语义」里必须跑真实对局才能验证的三项：
//   摸牌来源（draw_begin 的 source）、技能触发顺序、生命上限。
struct GameLogicTestResult {
	int passed;
	int failed;
};

GameLogicTestResult runGameLogicTests();
