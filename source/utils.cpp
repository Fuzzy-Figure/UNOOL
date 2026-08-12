#include "../header/utils.h"
#include <Windows.h>
#include <cmath>
#include <fstream>

namespace unool {

	const json& getConfig() {
		static json config = []() {
			json j;
			std::ifstream configFile("../config.json");
			if (!configFile.is_open()) {
				throw std::runtime_error("无法打开配置文件 config.json");
			}
			configFile >> j;
			return j;
		}();
		return config;
	}

	std::wstring to_utf16(const std::string& utf8) {
		if (utf8.empty()) return std::wstring();

		int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
		std::wstring utf16(len, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &utf16[0], len);
		utf16.pop_back(); // 去掉结尾的空字符
		return utf16;
	}
	std::string to_utf8(const std::wstring& utf16) {
		if (utf16.empty()) return std::string();

		int len = WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string utf8(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, utf16.c_str(), -1, &utf8[0], len, nullptr, nullptr);
		utf8.pop_back(); // 去掉结尾的空字符
		return utf8;
	}

	std::mt19937 rng(std::random_device{}());
	int randomInt(const int begin, const int end) {
		std::uniform_int_distribution<int> dist(begin, end);
		return dist(rng);
	}
	std::size_t randomSize_t(const std::size_t begin, const std::size_t end) {
		std::uniform_int_distribution<std::size_t> dist(begin, end);
		return dist(rng);
	}
	bool probability(const double p) {
		if (p < 0 || p > 1) throw std::invalid_argument("参数p必须介于0和1之间");
		std::bernoulli_distribution dist(p);
		return dist(rng);
	}

	std::size_t ceil(const double num) {
		return static_cast<std::size_t>(std::ceil(num));
	}
	std::size_t floor(const double num) {
		return static_cast<std::size_t>(std::floor(num));
	}
	std::size_t pow(const std::size_t a, const std::size_t b) {
		return static_cast<std::size_t>(std::pow(a, b));
	}
}
