#pragma once

// UNOOL.Adapters 专用预编译头：需要具体技术（SFML / nlohmann）。
// 内容与 header/pch.h 等价，但为独立文件、独立 .pch 产物，绝不共享。
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <optional>
#include <json.hpp>

namespace nlohmann {
	template <typename T>
	struct adl_serializer<std::optional<T>> {
		static void from_json(const json& j, std::optional<T>& opt) {
			if (j.is_null()) {
				opt = std::nullopt;
			}
			else {
				opt = j.get<T>();
			}
		}
		static void to_json(json& j, const std::optional<T>& opt) {
			if (opt) {
				j = *opt;
			}
			else {
				j = nullptr;
			}
		}
	};
}
