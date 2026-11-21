// Arch-C P3-5：零依赖单测起点。覆盖本次新增的纯逻辑与不变量。
// 后续可继续在此追加冻结区（牌堆/出牌/技能顺序/摸牌来源/生命上限/积分查表）用例。
#include "Crypto.h"
#include "protocol/Protocol.h"
#include "utils.h"
#include "test_codec.h"
#include "test_connection.h"
#include "test_gamelogic.h"
#include "test_loginviewmodel.h"
#include "test_protocol.h"
#include "test_rules.h"
#include "test_shopviewmodel.h"
#include "platform/Platform.h"

#include <cstdlib>
#include <print>
#include <string>
#include <string_view>
#include <utility>

using unool::tests::TestResult;

namespace {

std::string hex(std::string_view s) {
	return unool::crypto::toHex(unool::crypto::sha256(s));
}

void testSha256(TestResult& r) {
	// 标准 SHA-256 测试向量。
	r.check(hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	        "sha256(abc)");
	r.check(hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
	        "sha256(empty)");
}

void testPbkdf2(TestResult& r) {
	const std::string salt = "0123456789abcdef";
	const auto h1 = unool::crypto::derivePasswordHash("password", salt, 1000);
	const auto h2 = unool::crypto::derivePasswordHash("password", salt, 1000);
	r.check(h1 == h2, "pbkdf2 deterministic");
	r.check(h1.size() == 64, "pbkdf2 hash is 64 hex chars");
	r.check(h1 != unool::crypto::derivePasswordHash("password", "different-salt", 1000),
	        "pbkdf2 salt affects hash");
	r.check(h1 != unool::crypto::derivePasswordHash("other", salt, 1000),
	        "pbkdf2 password affects hash");
	r.check(unool::crypto::constantTimeEqualsHex(h1, h2), "hex equals");
	r.check(!unool::crypto::constantTimeEqualsHex(h1, unool::crypto::derivePasswordHash("x", salt, 1000)),
	        "hex not equals");
	const auto saltA = unool::crypto::makeRandomSaltHex();
	const auto saltB = unool::crypto::makeRandomSaltHex();
	r.check(saltA.size() == 32 && saltA != saltB, "random salt distinct");
}

void testProtocolVersion(TestResult& r) {
	using namespace unool::protocol;
	r.check(isProtocolVersionCompatible(kProtocolVersion), "current version compatible");
	r.check(!isProtocolVersionCompatible(kProtocolVersion - 1), "old version rejected");
	r.check(!isProtocolVersionCompatible(kProtocolVersion + 1), "future version rejected");
	r.check(!kProtocolVersionMismatch.empty(), "mismatch message readable");
}

void testMessageDirection(TestResult& r) {
	// MessageType 与 isClientCommand / isServerEvent / isControlMessage 均在 unool::protocol。
	using namespace unool::protocol;
	// 命令段 1xx
	r.check(isClientCommand(MessageType::LoginRequest), "LoginRequest is command");
	r.check(isClientCommand(MessageType::ClientInput), "ClientInput is command");
	r.check(!isServerEvent(MessageType::LoginRequest), "command is not server event");
	// 事件段 2xx
	r.check(isServerEvent(MessageType::LoginResponse), "LoginResponse is event");
	r.check(isServerEvent(MessageType::GameState), "GameState is event");
	r.check(!isClientCommand(MessageType::GameStart), "event is not command");
	// 控制段 9xx
	r.check(isControlMessage(MessageType::Heartbeat), "Heartbeat is control");
	r.check(!isClientCommand(MessageType::Heartbeat) && !isServerEvent(MessageType::Heartbeat),
	        "control is neither command nor event");
}

void testScoreboard(TestResult& r) {
	// 等级序 S/A/B/C/D/F；低等级战胜高等级得分更多。
	r.check(unool::scoreboard[0][0] == 10, "S beats S = 10");
	r.check(unool::scoreboard[5][0] > unool::scoreboard[0][5], "F beats S > S beats F");
}

} // namespace

int main() {
	// 断言名是中文，控制台默认 CP936 会把 UTF-8 输出打成乱码。
	unool::platform::setupConsoleUtf8();

	// 本文件内的纯逻辑断言（Crypto / 协议版本 / 消息方向 / 积分表）。
	TestResult total;
	testSha256(total);
	testPbkdf2(total);
	testProtocolVersion(total);
	testMessageDirection(total);
	testScoreboard(total);

	// 各测试文件的入口，按相加次序 = 执行次序汇总。
	total = total
	      + runCodecRoundTripTests()   // P1-2 MessageCodec 成对序列化 round-trip
	      + runConnectionStateTests()  // P1-3 ConnectionState 入站队列 / 心跳 / 断线
	      + runLoginViewModelTests()   // P1-4 登录状态机（headless）
	      + runShopViewModelTests()    // P1-4 商城状态机 / 导航 / 购买（headless）
	      + runProtocolRoundTripTests() // P3-5 账号 / 商城协议 round-trip
	      + runRulesTests()            // P3-5 冻结区玩法规则
	      + runGameLogicTests();       // P3-5 GameLogic 测试台（fake-port）

	std::println("单测结果: {} 通过, {} 失败", total.passed, total.failed);
	return total.allPassed() ? 0 : 1;
}
