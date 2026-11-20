#pragma once

// Arch-C P1-2：MessageCodec round-trip 单测的入口。
struct CodecTestResult {
	int passed;
	int failed;
};

CodecTestResult runCodecRoundTripTests();
