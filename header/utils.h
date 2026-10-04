#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <format>

using namespace std::chrono_literals;

template<typename T>
using ref = std::reference_wrapper<T>;

template<typename T>
using opt_ref = std::optional<ref<T>>;

namespace unool {
	//服务器专用配置：读取 server_config.json 并缓存；reload 可强制重读
	nlohmann::json& getServerConfig();
	//强制重新读取 server_config.json，刷新缓存（bo 阶段每局前调用）
	void reloadServerConfig();

	//客户端专用配置：读取 client_config.json 并缓存；reload 可强制重读
	nlohmann::json& getClientConfig();
	//强制重新读取 client_config.json，刷新缓存
	void reloadClientConfig();

	inline constexpr auto alwaysTrue = [](auto&&...) noexcept { return true; };

	namespace string {
		std::wstring to_utf16(const std::string& utf8);
		std::string to_utf8(const std::wstring& utf16);
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
				throw std::out_of_range(std::format(
					"randomGet：需要选{}个元素，但容器中只有{}个元素",
					n, std::ranges::size(range)
				));


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


