#pragma once

#include <cstdint>
#include <string_view>
#include "MessageType.h"

namespace unool::protocol {

// Arch-C P1-1：协议版本握手。
// 破坏性换代后旧客户端/旧服务端一律不兼容；LoginRequest 的首字段携带本版本，
// 服务端先读版本再读账号名，不匹配则回 LoginResponse(ok=false) 并给出可读原因。
// 版本不一致时不会进入任何业务分支，避免解析出乱码或崩溃。
inline constexpr std::uint32_t kProtocolVersion = 2;
inline constexpr std::string_view kProtocolVersionMismatch = "协议版本不匹配，请升级客户端";

// 首包版本校验：返回 false 表示版本不匹配（调用方应拒绝并断开）。
constexpr bool isProtocolVersionCompatible(std::uint32_t version) noexcept {
	return version == kProtocolVersion;
}

} // namespace unool::protocol
