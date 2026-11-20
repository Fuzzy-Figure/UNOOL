#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Arch-C P2-1：账号口令派生。纯标准库实现 PBKDF2-HMAC-SHA256，
// 不引入新第三方依赖（dep 内 Mbed TLS 仅随 SFML 静态链入，未作为可链接 API 暴露）。
// 落盘只保存 salt（十六进制）与派生出的 hash（十六进制），不保存任何明文口令。
namespace unool::crypto {

	// SHA-256 摘要（32 字节）。
	std::array<std::uint8_t, 32> sha256(std::string_view input);

	// PBKDF2-HMAC-SHA256，输出 32 字节。
	std::array<std::uint8_t, 32> pbkdf2HmacSha256(std::string_view password,
	                                             std::string_view salt,
	                                             int iterations);

	// 生成 n 字节随机盐并转小写十六进制（使用 std::random_device）。
	std::string makeRandomSaltHex(std::size_t byteCount = 16);

	// 字节序列转小写十六进制 / 常量时间相等比较。
	std::string toHex(const std::array<std::uint8_t, 32>& bytes);
	bool constantTimeEqualsHex(std::string_view aHex, std::string_view bHex);

	// 派生参数：迭代次数（破坏性换代后可随时上调，老数据按当时参数校验需记录 iterations）。
	inline constexpr int kPbkdf2Iterations = 120000;

	// 由明文口令 + 盐得到可落盘的哈希十六进制串。
	std::string derivePasswordHash(std::string_view password, std::string_view salt,
	                               int iterations = kPbkdf2Iterations);

} // namespace unool::crypto
