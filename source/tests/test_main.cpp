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
#include "test_userdb.h"
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
	// 块边界：64 字节不带 padding，65 字节要多一个块。补位逻辑改动最容易在这里出错。
	r.check(hex(std::string(64, 'x')) == "7ce100971f64e7001e8fe5a51973ecdfe1ced42befe7ee8d5fd6219506b5393c",
	        "sha256(64B no padding)");
	r.check(hex(std::string(65, 'x')) == "9537c5fdf120482f7d58d25e9ed583f52c02b4e304ea814db1633ad565aed7e9",
	        "sha256(65B extra block)");
}

void testPbkdf2(TestResult& r) {
	// RFC 参考向量（PBKDF2-HMAC-SHA256）：验的是「符合标准」而不只是「自己和自己一致」。
	// 这些值一旦变化，存量账号的口令哈希就全部对不上 —— 优化实现时必须逐位保持。
	r.check(unool::crypto::derivePasswordHash("password", "salt", 1)
	            == "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b",
	        "pbkdf2 rfc c=1");
	r.check(unool::crypto::derivePasswordHash("password", "salt", 2)
	            == "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43",
	        "pbkdf2 rfc c=2");
	r.check(unool::crypto::derivePasswordHash("password", "salt", 4096)
	            == "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a",
	        "pbkdf2 rfc c=4096");
	// 长盐 32 字节：U1 的尾部要跨块。
	r.check(unool::crypto::derivePasswordHash("passwordPASSWORDpassword",
	                                          "saltSALTsaltSALTsaltSALTsaltSALTsalt", 4096)
	            == "348c89dbcbd32b2f32d814b8116e84cf2b17347ebc1800181c4e2a1fb8dd53e1",
	        "pbkdf2 rfc long salt");
	// 口令 65 字节（超过 HMAC 的 64 字节密钥块）与 40 / 60 / 61 字节盐：
	// 期望值取自优化前实现（同一份代码已通过上面四条标准向量）。
	r.check(unool::crypto::derivePasswordHash(std::string(65, 'k'), "salt", 5)
	            == "2eb6411f47dbd189beaab65a9abcc1baeabad0b5ee519cc5c4c274daccf2c474",
	        "pbkdf2 65B password");
	r.check(unool::crypto::derivePasswordHash("pw", std::string(40, 's'), 5)
	            == "01de78de362c35c3f9ad8fb19c0ba4d0603ef8ec87d9878b7dd282a6ef8d8a35",
	        "pbkdf2 40B salt");
	r.check(unool::crypto::derivePasswordHash("pw", std::string(60, 's'), 5)
	            == "08b401b757a48a446f0ae0a87cf0bf3320ba248882d3a98162f8c944dc8af756",
	        "pbkdf2 60B salt");
	r.check(unool::crypto::derivePasswordHash("pw", std::string(61, 's'), 5)
	            == "c5b2c687af553635d00d8c05b579025979d1bef11bee9aa728c970e2e5cdc7fd",
	        "pbkdf2 61B salt");
	// 生产参数（12 万次）：落盘格式的最终锚点。
	r.check(unool::crypto::derivePasswordHash("123456", "0123456789abcdef0123456789abcdef",
	                                          unool::crypto::kPbkdf2Iterations)
	            == "c916b5a9128f0045151cffb1068605203fc9febaa303928ddf8f2785f5120869",
	        "pbkdf2 production iterations");

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

	// 各测试文件的入口，逐句累加 = 严格按此顺序执行。
	//
	// 不要写成 total = total + a() + b() + ... ：重载的 operator+ 是函数调用，
	// 两个实参的求值顺序是 unspecified，之前就靠这条链把 runGameLogicTests()
	// 排到了最前面——它装配的 FakeWorld 是栈对象，返回后 services() 里留着
	// 悬空引用，随后 runUserDbTests() 调 checkin / 解锁 S 档取随机数时读到
	// 已销毁对象（调试器里 services().random = 0xFFFFFFFFFFFFFFFF）。
	total = total + runCodecRoundTripTests();    // P1-2 MessageCodec 成对序列化 round-trip
	total = total + runConnectionStateTests();   // P1-3 ConnectionState 入站队列 / 心跳 / 断线
	total = total + runLoginViewModelTests();    // P1-4 登录状态机（headless）
	total = total + runShopViewModelTests();     // P1-4 商城状态机 / 导航 / 购买（headless）
	total = total + runProtocolRoundTripTests(); // P3-5 账号 / 商城协议 round-trip
	total = total + runRulesTests();             // P3-5 冻结区玩法规则
	total = total + runUserDbTests();            // P3-5 账号存档 / 口令哈希 / 积分 / 购买 / 签到
	total = total + runGameLogicTests();         // P3-5 GameLogic 测试台（fake-port）

	std::println("单测结果: {} 通过, {} 失败", total.passed, total.failed);
	return total.allPassed() ? 0 : 1;
}
