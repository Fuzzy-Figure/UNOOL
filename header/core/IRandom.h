#pragma once
#include <cstddef>
#include <cstdint>
#include <random>

namespace unool::core {

	// 随机源出站端口。
	// utils.h 的 RandomEngine 门面、以及技能与牌堆里所有
	// std::ranges::shuffle / sample / uniform_int_distribution 最终都经此取数，
	// 因此调用点一行不改即可换成固定种子实现。
	class IRandom {
	public:
		using result_type = std::mt19937::result_type;
		virtual ~IRandom() = default;

		// 引擎原始输出
		virtual result_type next() = 0;
		virtual int intIn(int begin, int end) = 0;
		virtual std::size_t sizeIn(std::size_t begin, std::size_t end) = 0;
		virtual bool probability(double p) = 0;
		virtual void seed(std::uint32_t value) = 0;
	};
}
