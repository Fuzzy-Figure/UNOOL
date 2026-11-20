// Arch-C P3-5：零依赖单测起点。覆盖本次新增的纯逻辑与不变量。
// 后续可继续在此追加冻结区（牌堆/出牌/技能顺序/摸牌来源/生命上限/积分查表）用例。
#include "Crypto.h"
#include "protocol/Protocol.h"
#include "utils.h"
#include "test_codec.h"
#include "test_connection.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

int g_pass = 0;
int g_fail = 0;

#define CHECK(cond, name)                                                   \
	do {                                                                    \
		if (cond) { ++g_pass; }                                             \
		else { ++g_fail; std::cerr << "FAIL: " << (name) << '\n'; }         \
	} while (false)

std::string hex(std::string_view s) {
	return unool::crypto::toHex(unool::crypto::sha256(s));
}

void testSha256() {
	// 标准 SHA-256 测试向量。
	CHECK(hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	      "sha256(abc)");
	CHECK(hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	      "sha256(empty)");
}

void testPbkdf2() {
	const std::string salt = "0123456789abcdef";
	const auto h1 = unool::crypto::derivePasswordHash("password", salt, 1000);
	const auto h2 = unool::crypto::derivePasswordHash("password", salt, 1000);
	CHECK(h1 == h2, "pbkdf2 deterministic");
	CHECK(h1.size() == 64, "pbkdf2 hash is 64 hex chars");
	CHECK(h1 != unool::crypto::derivePasswordHash("password", "different-salt", 1000),
	      "pbkdf2 salt affects hash");
	CHECK(h1 != unool::crypto::derivePasswordHash("other", salt, 1000),
	      "pbkdf2 password affects hash");
	CHECK(unool::crypto::constantTimeEqualsHex(h1, h2), "hex equals");
	CHECK(!unool::crypto::constantTimeEqualsHex(h1, unool::crypto::derivePasswordHash("x", salt, 1000)),
	      "hex not equals");
	const auto saltA = unool::crypto::makeRandomSaltHex();
	const auto saltB = unool::crypto::makeRandomSaltHex();
	CHECK(saltA.size() == 32 && saltA != saltB, "random salt distinct");
}

void testProtocolVersion() {
	using namespace unool::protocol;
	CHECK(isProtocolVersionCompatible(kProtocolVersion), "current version compatible");
	CHECK(!isProtocolVersionCompatible(kProtocolVersion - 1), "old version rejected");
	CHECK(!isProtocolVersionCompatible(kProtocolVersion + 1), "future version rejected");
	CHECK(!kProtocolVersionMismatch.empty(), "mismatch message readable");
}

void testMessageDirection() {
	// isClientCommand / isServerEvent / isControlMessage 在全局命名空间（MessageType.h）。
	// 命令段 1xx
	CHECK(isClientCommand(MessageType::LoginRequest), "LoginRequest is command");
	CHECK(isClientCommand(MessageType::ClientInput), "ClientInput is command");
	CHECK(!isServerEvent(MessageType::LoginRequest), "command is not server event");
	// 事件段 2xx
	CHECK(isServerEvent(MessageType::LoginResponse), "LoginResponse is event");
	CHECK(isServerEvent(MessageType::GameState), "GameState is event");
	CHECK(!isClientCommand(MessageType::GameStart), "event is not command");
	// 控制段 9xx
	CHECK(isControlMessage(MessageType::Heartbeat), "Heartbeat is control");
	CHECK(!isClientCommand(MessageType::Heartbeat) && !isServerEvent(MessageType::Heartbeat),
	      "control is neither command nor event");
}

void testScoreboard() {
	// 等级序 S/A/B/C/D/F；低等级战胜高等级得分更多。
	CHECK(unool::scoreboard[0][0] == 10, "S beats S = 10");
	CHECK(unool::scoreboard[5][0] > unool::scoreboard[0][5], "F beats S > S beats F");
}

} // namespace

int main() {
	testSha256();
	testPbkdf2();
	testProtocolVersion();
	testMessageDirection();
	testScoreboard();

	// Arch-C P1-2：MessageCodec 成对序列化 round-trip（依赖 SFML，VS Tests 工程运行）。
	const auto codec = runCodecRoundTripTests();
	g_pass += codec.passed;
	g_fail += codec.failed;

	// Arch-C P1-3：ConnectionState 显式入站队列 / 心跳 / 断线单测（fake gateway）。
	const auto conn = runConnectionStateTests();
	g_pass += conn.passed;
	g_fail += conn.failed;

	std::cout << "单测结果: " << g_pass << " 通过, " << g_fail << " 失败\n";
	return g_fail == 0 ? 0 : 1;
}
