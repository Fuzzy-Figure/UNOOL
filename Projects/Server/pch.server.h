#pragma once

// UNOOL.Server 专用预编译头（阶段 2）。
// 与 header/pch.h 完全独立，绝不共享 .pch 产物；Client1/Client2 仍使用共享 pch.h。
// 只保留服务端真正用到的第三方模块：
//   Network        : sf::Packet / TcpSocket / TcpListener / SocketSelector / IpAddress
//   System         : sf::Clock / sf::milliseconds / sf::seconds
//   Window/Keyboard: sf::Keyboard::Scancode（线上 ClientInput.key 的字节依赖）
//   json.hpp       : 配置与 UserDB 读写
// 下面 adl_serializer<std::optional<T>> 特化与 header/pch.h 逐字一致：
// 去掉或改写会改变 JSON 序列化字节，属于存档 / 对外行为变更，因此原样保留。
// 去掉 <SFML/Graphics.hpp> 后，显式补齐它原先间接带入的标准库头，
// 保证所有编译单元的声明可见性与重构前一致。
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
