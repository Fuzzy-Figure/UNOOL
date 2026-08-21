#pragma once
#include <type_traits>
#include <optional>
#include <string>
#include <random>
#include <json.hpp>
#include <chrono>

using nlohmann::json;
using namespace std::chrono_literals;

template<typename T>
using ref = std::reference_wrapper<T>;

template<typename T>
using opt_ref = std::optional<ref<T>>;


namespace unool {
	//全局配置，首次调用时读取 config.json，之后返回缓存引用
	const json& getConfig();

	inline constexpr auto alwaysTrue = [](auto&&...) noexcept { return true; };

	namespace string {
		std::wstring to_utf16(const std::string& utf8);
		std::string to_utf8(const std::wstring& wstr);
	}

	namespace random {
		extern std::mt19937 rng;
		int randomInt(const int begin, const int end);
		std::size_t randomSize_t(const std::size_t begin, const std::size_t end);
		bool probability(const double p);

		//随机取 N 个元素，返回vector<ref<T>>
		template<std::ranges::forward_range R>
		std::vector<ref<std::ranges::range_value_t<R>>>
			randomGet(R& range, std::size_t n) {
			using T = std::ranges::range_value_t<R>;

			if (n > std::ranges::size(range))
				throw std::out_of_range(
					"randomGet：需要选" + std::to_string(n) +
					"个元素，但容器中只有" +
					std::to_string(std::ranges::size(range)) + "个元素");

			if (n == 0 || std::ranges::empty(range)) return {};

			std::vector<ref<T>> all_refs;
			for (auto& elem : range)
				all_refs.emplace_back(elem);

			std::vector<ref<T>> result;
			result.reserve(n);
			std::ranges::sample(all_refs, std::back_inserter(result), n, rng);

			return result;
		}

		//随机取 1 个元素，直接返回原始引用
		template<std::ranges::forward_range R>
		const std::ranges::range_value_t<R>&
			randomGet(R& range) {
			if (std::ranges::empty(range))
				throw std::out_of_range("randomGet on empty container");
			auto offset = randomSize_t(0, std::ranges::size(range) - 1);

			auto it = std::ranges::begin(range);
			std::ranges::advance(it, offset);
			return *it;
		}
	}

	namespace math {
		std::size_t ceil(const double num);
		std::size_t floor(const double num);
		std::size_t pow(const std::size_t a, const std::size_t b);
	}

	namespace input {
		// 安全读取整数（失败返回nullopt）
		std::optional<int> safeReadInt(int minVal, int maxVal);

		// 安全读取字符串（去除首尾空白）
		std::string safeReadLine();

		// 安全读取不含空格的字符串（去除首尾空白，内部含空格则返回空串）
		std::string safeReadNoSpace();
	}
	constexpr std::array<std::array<int, 6>, 6> scoreboard = { {
			//      败者  S   A   B   C   D   F
			//胜者
			/*S*/      {{10,  9,  8,  6,  5,  3}},
			/*A*/      {{12, 10,  9,  8,  6,  5}},
			/*B*/      {{15, 12, 10,  9,  8,  6}},
			/*C*/      {{18, 15, 12, 10,  9,  8}},
			/*D*/      {{25, 18, 15, 12, 10,  9}},
			/*F*/      {{35, 25, 18, 15, 12, 10}}
		} };
}


