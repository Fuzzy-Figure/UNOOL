#pragma once

// Arch-C P3-5：单测共用的断言器（零宏，纯函数）。
//
// 早先各测试文件用 #define CHECK(cond)，唯一不可替代的能力是字符串化 #cond；
// 代价是：失败信息没有 file:line、逗号会被预处理器当参数分隔符
// （std::pair<int, int> 直接编译不过）、断言不进调试器、四个文件各抄一遍计数与打印。
//
// std::source_location 的默认实参在「调用点」求值，普通函数就能自带位置，
// 信息量不低于 #cond（那一行本身就是条件），且签名受类型系统保护：
//
//   r.check(conn.isConnected());               -> test_connection.cpp:110: FAIL
//   r.check(v == 10, "F beats S > S beats F"); -> test_rules.cpp:90: FAIL: F beats S > S beats F
//
// 用例分组（原 TEST_CASE 宏）用 section()：名字显式传入，不再依赖 #fn。
#include <print>
#include <source_location>
#include <string_view>

namespace unool::tests {

	struct TestResult {
		int passed{};
		int failed{};

		// 失败时把 file:line 打到 stderr；name 为空则只打位置。
		void check(const bool ok,
				   const std::string_view name = {},
				   const std::source_location where = std::source_location::current()) {
			if (ok) {
				++passed;
				return;
			}
			++failed;
			if (name.empty()) {
				std::println(stderr, "{}:{}: FAIL", where.file_name(), where.line());
			}
			else {
				std::println(stderr, "{}:{}: FAIL: {}", where.file_name(), where.line(), name);
			}
		}

		// 跑一个用例子函数，按失败数增量打印 "  <name>: OK/FAIL"。
		void section(const std::string_view name, void (*caseFn)(TestResult&)) {
			const int before = failed;
			caseFn(*this);
			std::println("  {}: {}", name, failed == before ? "OK" : "FAIL");
		}

		[[nodiscard]] bool allPassed() const { return failed == 0; }
	};

	// 汇总多个测试文件的结果（保持相加次序 = 用例执行次序）。
	inline TestResult operator+(TestResult lhs, const TestResult& rhs) {
		lhs.passed += rhs.passed;
		lhs.failed += rhs.failed;
		return lhs;
	}

} // namespace unool::tests
