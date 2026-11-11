#pragma once
#include <chrono>

#include "core/IClock.h"

namespace unool::shell {

	// 单调时钟出站端口（基于 std::chrono::steady_clock）。
	// 实现与重构前逐字一致为 std::chrono::steady_clock::now()，
	// 因此本适配器不依赖 SFML，也不引入新的技术依赖；
	// 类名已由原 SfmlClock 改为 ChronoClock，以名实相符。
	class ChronoClock final : public core::IClock {
	public:
		time_point now() const override {
			return std::chrono::steady_clock::now();
		}
	};
}
