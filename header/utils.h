#pragma once
#include <type_traits>
#include <optional>
#include <string>
#include <random>
#include <json.hpp>
#include <chrono>

using nlohmann::json;
using namespace std::chrono_literals;

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

//别名

template<typename T>
using ref = std::reference_wrapper<T>;

template<typename T>
using opt_ref = std::optional<ref<T>>;

