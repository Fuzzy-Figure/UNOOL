#pragma once

// 预编译头：只放第三方库（永不改动），让所有 .cpp 复用同一份编译缓存
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <json.hpp>   // ← 先让 json.hpp 把 adl_serializer 声明出来

// 然后才能特化
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