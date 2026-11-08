#pragma once

// UNOOL.Domain 专用预编译头：纯标准库。
// 与 header/pch.h 完全独立（不共享文件、不共享 .pch 产物），
// 不含 SFML / Windows.h / json.hpp / UserDB.h / Socket.h。
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>
