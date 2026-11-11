#pragma once
#include <chrono>

namespace unool::core {

	// 单调时钟出站端口。调用方记录起点后自行计算 elapsed，
	// 与原先 sf::Clock::getElapsedTime().asSeconds()/asMilliseconds() 语义一致。
	class IClock {
	public:
		using time_point = std::chrono::steady_clock::time_point;
		virtual ~IClock() = default;

		virtual time_point now() const = 0;
	};
}
