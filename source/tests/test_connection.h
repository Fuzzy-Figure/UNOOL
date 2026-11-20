#pragma once

// Arch-C P1-3：ConnectionState 单测入口（fake gateway，无需真实网络连接）。
struct ConnectionTestResult {
	int passed;
	int failed;
};

ConnectionTestResult runConnectionStateTests();
