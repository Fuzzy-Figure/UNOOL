#pragma once
#include <chrono>

#include "application/IClock.h"

namespace unool::adapters {

	// 单调时钟出站端口。类名沿用方案 §3.2 的端口命名；
	// 实现与重构前逐字一致为 std::chrono::steady_clock::now()，
	// 因此本适配器不依赖 SFML，也不引入新的技术依赖。
	class SfmlClock final : public application::IClock {
	public:
		time_point now() const override {
			return std::chrono::steady_clock::now();
		}
	};
}
