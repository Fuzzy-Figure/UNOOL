#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
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
#include <print>


template<typename T>
using ref = std::reference_wrapper<T>;

template<typename T>
using opt_ref = std::optional<ref<T>>;

namespace unool {
	constexpr std::size_t MAX_PLAYERS = 2;

	namespace paths {
		// 当前可执行文件所在目录（绝对路径）。不依赖进程工作目录，
		// 因此从资源管理器双击 exe 或从任意目录启动都得到同一结果。
		const std::filesystem::path& exeDir();
		// 以 exe 所在目录为起点逐级向上查找 fileName，返回第一个存在的绝对路径；
		// 找不到返回空路径。近处优先：部署时把资源放在 exe 同目录即可覆盖默认布局。
		std::filesystem::path locate(const std::string& fileName);
		// 资源根目录（绝对路径）：从 exe 目录起向上找第一个含 images/ 或配置文件的目录。
		// 用作图片路径前缀，以及首次创建 userDatas.json 等数据文件的落点。
		// 找不到时回退到 exeDir()，结果始终与进程工作目录无关。
		const std::filesystem::path& resourceRoot();
	}

	inline constexpr auto alwaysTrue = [](auto&&...) noexcept { return true; };

	namespace string {
		std::wstring to_utf16(const std::string& utf8);
		std::string to_utf8(const std::wstring& utf16);
	}

	namespace random {
		// 随机引擎门面：仍满足 uniform_random_bit_generator，
		// 但每次取数都转发给出站端口 IRandom。
		// 这样 std::ranges::shuffle / sample / 各种 distribution 的调用点
		// 一行都不用改，且仍与重构前共用同一条引擎状态（随机序列逐字等价）。
		class RandomEngine {
		public:
			using result_type = std::mt19937::result_type;
			static constexpr result_type min() { return std::mt19937::min(); }
			static constexpr result_type max() { return std::mt19937::max(); }
			// 定义移到 source/utils.cpp：避免本头为取随机数而包含
			// application/Services.h，使纯领域头闭包不再传递依赖端口层。
			result_type operator()();
		};
		extern RandomEngine rng;
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


