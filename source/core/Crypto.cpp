#include "Crypto.h"

#include <algorithm>
#include <random>

namespace unool::crypto {
namespace {

constexpr std::array<std::uint32_t, 64> kK = {
	0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
	0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
	0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
	0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
	0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
	0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
	0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
	0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
};

constexpr std::uint32_t rotr(std::uint32_t x, int n) noexcept {
	return (x >> n) | (x << (32 - n));
}

std::array<std::uint32_t, 8> sha256Core(std::vector<std::uint8_t> msg) {
	const std::uint64_t bitLen = static_cast<std::uint64_t>(msg.size()) * 8;
	msg.push_back(0x80);
	while (msg.size() % 64 != 56) msg.push_back(0);
	for (int i = 7; i >= 0; --i) msg.push_back(static_cast<std::uint8_t>(bitLen >> (i * 8)));

	std::array<std::uint32_t, 8> h = {
		0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
		0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
	};
	for (std::size_t off = 0; off < msg.size(); off += 64) {
		std::array<std::uint32_t, 64> w{};
		for (int i = 0; i < 16; ++i) {
			w[i] = (static_cast<std::uint32_t>(msg[off + i * 4]) << 24)
			     | (static_cast<std::uint32_t>(msg[off + i * 4 + 1]) << 16)
			     | (static_cast<std::uint32_t>(msg[off + i * 4 + 2]) << 8)
			     | static_cast<std::uint32_t>(msg[off + i * 4 + 3]);
		}
		for (int i = 16; i < 64; ++i) {
			const std::uint32_t s0 = rotr(w[i-15],7) ^ rotr(w[i-15],18) ^ (w[i-15] >> 3);
			const std::uint32_t s1 = rotr(w[i-2],17) ^ rotr(w[i-2],19) ^ (w[i-2] >> 10);
			w[i] = w[i-16] + s0 + w[i-7] + s1;
		}
		std::uint32_t a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],hh=h[7];
		for (int i = 0; i < 64; ++i) {
			const std::uint32_t S1 = rotr(e,6) ^ rotr(e,11) ^ rotr(e,25);
			const std::uint32_t ch = (e & f) ^ (~e & g);
			const std::uint32_t t1 = hh + S1 + ch + kK[i] + w[i];
			const std::uint32_t S0 = rotr(a,2) ^ rotr(a,13) ^ rotr(a,22);
			const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
			const std::uint32_t t2 = S0 + maj;
			hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
		}
		h[0]+=a; h[1]+=b; h[2]+=c; h[3]+=d; h[4]+=e; h[5]+=f; h[6]+=g; h[7]+=hh;
	}
	return h;
}

std::array<std::uint8_t,32> hmacSha256(std::string_view key, std::string_view data) {
	constexpr std::size_t block = 64;
	std::array<std::uint8_t,block> k{};
	if (key.size() > block) {
		const auto kh = sha256Core(std::vector<std::uint8_t>(key.begin(), key.end()));
		for (int i = 0; i < 8; ++i) for (int j = 0; j < 4; ++j)
			k[i*4+j] = static_cast<std::uint8_t>(kh[i] >> (24 - j*8));
	} else {
		std::copy(key.begin(), key.end(), k.begin());
	}
	std::array<std::uint8_t,block> ipad{}, opad{};
	for (std::size_t i = 0; i < block; ++i) { ipad[i] = k[i] ^ 0x36; opad[i] = k[i] ^ 0x5c; }

	std::vector<std::uint8_t> inner(ipad.begin(), ipad.end());
	inner.insert(inner.end(), data.begin(), data.end());
	const auto ih = sha256Core(std::move(inner));

	std::vector<std::uint8_t> outer(opad.begin(), opad.end());
	for (int i = 0; i < 8; ++i) for (int j = 0; j < 4; ++j)
		outer.push_back(static_cast<std::uint8_t>(ih[i] >> (24 - j*8)));
	const auto oh = sha256Core(std::move(outer));

	std::array<std::uint8_t,32> out{};
	for (int i = 0; i < 8; ++i) for (int j = 0; j < 4; ++j)
		out[i*4+j] = static_cast<std::uint8_t>(oh[i] >> (24 - j*8));
	return out;
}

char hexDigit(std::uint8_t v) noexcept { return v < 10 ? char('0'+v) : char('a'+(v-10)); }

} // namespace

std::array<std::uint8_t,32> sha256(std::string_view input) {
	const auto h = sha256Core(std::vector<std::uint8_t>(input.begin(), input.end()));
	std::array<std::uint8_t,32> out{};
	for (int i = 0; i < 8; ++i) for (int j = 0; j < 4; ++j)
		out[i*4+j] = static_cast<std::uint8_t>(h[i] >> (24 - j*8));
	return out;
}

std::array<std::uint8_t,32> pbkdf2HmacSha256(std::string_view password, std::string_view salt, int iterations) {
	// 单个 32 字节块：U1 = HMAC(pw, salt || INT(1));  Uj = HMAC(pw, U(j-1));  DK = XOR。
	std::string saltBlock(salt);
	saltBlock.push_back(char(0)); saltBlock.push_back(char(0)); saltBlock.push_back(char(0)); saltBlock.push_back(char(1));
	std::array<std::uint8_t,32> u = hmacSha256(password, saltBlock);
	std::array<std::uint8_t,32> dk = u;
	for (int j = 1; j < iterations; ++j) {
		u = hmacSha256(password, std::string_view(reinterpret_cast<const char*>(u.data()), u.size()));
		for (std::size_t i = 0; i < dk.size(); ++i) dk[i] ^= u[i];
	}
	return dk;
}

std::string makeRandomSaltHex(std::size_t byteCount) {
	std::random_device rd;
	std::string out;
	out.reserve(byteCount * 2);
	for (std::size_t i = 0; i < byteCount; ++i) {
		const std::uint8_t b = static_cast<std::uint8_t>(rd());
		out.push_back(hexDigit(b >> 4));
		out.push_back(hexDigit(b & 0x0f));
	}
	return out;
}

std::string toHex(const std::array<std::uint8_t,32>& bytes) {
	std::string out;
	out.reserve(bytes.size() * 2);
	for (auto b : bytes) { out.push_back(hexDigit(b >> 4)); out.push_back(hexDigit(b & 0x0f)); }
	return out;
}

bool constantTimeEqualsHex(std::string_view aHex, std::string_view bHex) {
	if (aHex.size() != bHex.size()) {
		// 长度不同也走一遍比较，降低长度泄漏（实际哈希串长度固定 64）。
		volatile int acc = 0;
		for (std::size_t i = 0; i < aHex.size(); ++i)
			acc |= (aHex[i] ^ (bHex.empty() ? 0 : static_cast<int>(bHex[i % bHex.size()])));
		return false;
	}
	unsigned diff = 0;
	for (std::size_t i = 0; i < aHex.size(); ++i)
		diff |= static_cast<unsigned char>(aHex[i]) ^ static_cast<unsigned char>(bHex[i]);
	return diff == 0;
}

std::string derivePasswordHash(std::string_view password, std::string_view salt, int iterations) {
	return toHex(pbkdf2HmacSha256(password, salt, iterations));
}

} // namespace unool::crypto
