#pragma once
#include <cstdint>
#include <cstddef>
#include <random>
#include <stdexcept>

#include "core/IRandom.h"

namespace unool::shell {

	// 随机源出站端口：唯一真实引擎。
	// 原 utils.cpp 的全局 std::mt19937 变成 utils.h 里的 RandomEngine 门面后，
	// 所有 shuffle / sample / distribution 仍共用这一条状态。
	// 硬约束：每个进程内 DomainServices::random 与 RandomEngine 门面必须指向
	// 同一实例，否则随机序列会改变。ServerMain 显式构造一个实例并注入，
	// 未装配的客户端由 LegacyServicesAdapter 默认组合持有唯一实例。
	class Mt19937Random final : public core::IRandom {
	public:
		result_type next() override {
			return engine_();
		}
		int intIn(int begin, int end) override {
			std::uniform_int_distribution<int> dist(begin, end);
			return dist(engine_);
		}
		std::size_t sizeIn(std::size_t begin, std::size_t end) override {
			std::uniform_int_distribution<std::size_t> dist(begin, end);
			return dist(engine_);
		}
		bool probability(double p) override {
			if (p < 0 || p > 1) throw std::invalid_argument("参数p必须介于0和1之间");
			std::bernoulli_distribution dist(p);
			return dist(engine_);
		}
		void seed(std::uint32_t value) override {
			engine_.seed(value);
		}

	private:
		std::mt19937 engine_{ std::random_device{}() };
	};
}
