#pragma once
// B+ 共享契约层：一桌的座位配置。
//
// 口径澄清：seatCount 是「一桌有几个座位」的玩法规则，不是「进程能开几桌」。
// 出牌、Ban/Pick、对手判定（`1 - clientIdx`）、胜负结算都按两座位桌编写，
// 因此这个值是编译期常量且当前固定为 2；把它改成运行期可变容量等于改玩法，
// 不属于「每桌状态显式化」。
//
// 真正支持多桌要的是「N 个 ServerNetwork / GameLogic 实例并存 + 按桌分发
// 连接与输入」，那是组合根与调度层的工作，与本常量无关。
//
// 本头只依赖标准库、不链接规则引擎、不含平台头，可供两端共享编译单元使用。
#include <cstddef>

namespace unool::contract {

	struct TableConfig {
		static constexpr std::size_t seatCount = 2;
	};

}
