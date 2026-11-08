#pragma once

// UNOOL.Protocol 专用预编译头（阶段 2 / W3）。
// 与 header/pch.h、pch.server.h、pch.adapters.h 完全独立：不共享文件、不共享 .pch 产物。
// 本库只编译 source/GameState.cpp 与 source/AccountProtocol.cpp 两个协议 / DTO 编译单元。
// 内容按 pch.server.h 逐字保留（Server 已成功编译过这两个 .cpp），确保声明可见性一致：
//   Network        : sf::Packet（序列化与协议构造）
//   System         : 由 Network 间接依赖
//   Window/Keyboard: sf::Keyboard::Scancode（Socket.h 中 ClientInput.key 的字节依赖）
//   json.hpp       : adl_serializer<std::optional<T>> 特化的前置声明
// 下面 adl_serializer<std::optional<T>> 特化与 header/pch.h 逐字一致：
// 去掉或改写会改变 JSON 序列化字节，属于存档 / 对外行为变更，因此原样保留。
// 不含 <SFML/Graphics.hpp>：这两个编译单元没有任何图形符号。
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <print>
#include <random>
#include <ranges>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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
