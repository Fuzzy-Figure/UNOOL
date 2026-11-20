#pragma once

// Arch-C P3-5：冻结区玩法规则单测入口（纯逻辑，不建 GameLogic / 不开窗口）。
// 覆盖 CURRENT.md「不可丢的玩法语义」中可脱离对局独立验证的部分：
//   牌值表、体力扣减依据（手牌分值之和）、秘技牌归类与牌池隔离、
//   卡牌容器语义、手牌选择指针、弃牌元数据随复制保留、生命上限夹紧。
// 摸牌来源与技能触发顺序要真实 GameLogic，见 test_gamelogic.h。
struct RulesTestResult {
	int passed;
	int failed;
};

RulesTestResult runRulesTests();
