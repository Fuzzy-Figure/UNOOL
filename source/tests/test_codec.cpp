// Arch-C P1-2：MessageCodec 成对序列化 round-trip 单测。
// 每个消息号 make -> 读消息号 -> parse -> 字段逐项比对，
// 验证收发两端字节自洽（本地 g++ 无法链接 MSVC 的 SFML .lib，故在 VS 的 Tests 工程运行）。
#include "test_codec.h"

#include "MessageCodec.h"
#include "GameState.h"
#include "core/IDecisionPort.h"
#include <SFML/Network.hpp>

#include <iostream>
#include <string>

using namespace unool::protocol;

namespace {

	int g_pass{};
	int g_fail{};

#define CHECK(cond)                                                          \
	do {                                                                     \
		if (cond) { ++g_pass; }                                              \
		else { ++g_fail; std::cerr << "FAIL(codec): " #cond << '\n'; }          \
	} while (false)

#define TEST_CASE(fn)                                                        \
	do {                                                                     \
		const int before = g_fail;                                           \
		fn();                                                                \
		std::cout << "  codec." #fn ": "                                     \
		          << (g_fail == before ? "OK" : "FAIL") << '\n';                \
	} while (false)

	int readWire(sf::Packet& packet) {
		int raw{};
		packet >> raw;
		return raw;
	}

	bool testConnectionInfo() {
		auto packet = makeConnectionInfo(7);
		CHECK(readWire(packet) == static_cast<int>(MessageType::ConnectionInfo));
		auto data = parseConnectionInfo(packet);
		CHECK(data.has_value());
		CHECK(data->playerId == 7u);
		return true;
	}

	bool testGameStartHeartbeat() {
		auto gs = makeGameStart();
		CHECK(readWire(gs) == static_cast<int>(MessageType::GameStart));
		auto hb = makeHeartbeat();
		CHECK(readWire(hb) == static_cast<int>(MessageType::Heartbeat));
		return true;
	}

	bool testCheckinResponse() {
		auto packet = makeCheckinResponse("签到成功", true, 123);
		CHECK(readWire(packet) == static_cast<int>(MessageType::CheckinResponse));
		std::string msg;
		bool ok{};
		int points{};
		CHECK(static_cast<bool>(packet >> msg >> ok >> points));
		CHECK(msg == "签到成功");
		CHECK(ok);
		CHECK(points == 123);
		return true;
	}

	bool testGameEnd() {
		auto withWinner = makeGameEnd(std::size_t{3}, "玩家A");
		CHECK(readWire(withWinner) == static_cast<int>(MessageType::GameEnd));
		auto a = parseGameEnd(withWinner);
		CHECK(a.has_value());
		CHECK(a->hasWinner);
		CHECK(a->winnerId == 3u);
		CHECK(a->winnerName == "玩家A");

		auto noWinner = makeGameEnd(std::nullopt, "");
		CHECK(readWire(noWinner) == static_cast<int>(MessageType::GameEnd));
		auto b = parseGameEnd(noWinner);
		CHECK(b.has_value());
		CHECK(!b->hasWinner);
		return true;
	}

	bool testPointerUpdate() {
		auto packet = makePointerUpdate(2, 9);
		CHECK(readWire(packet) == static_cast<int>(MessageType::PointerUpdate));
		auto data = parsePointerUpdate(packet);
		CHECK(data.has_value());
		CHECK(data->playerId == 2u);
		CHECK(data->selectedIndex == 9u);
		return true;
	}

	bool testClientInput() {
		auto packet = makeClientInput(42, 1, 5);
		CHECK(readWire(packet) == static_cast<int>(MessageType::ClientInput));
		auto data = parseClientInput(packet);
		CHECK(data.has_value());
		CHECK(data->key == 42);
		CHECK(data->playerId == 1u);
		CHECK(data->selectedIndex == 5u);
		return true;
	}

	bool testStringMessages() {
		auto sr = makeStringMessage(MessageType::ShopResult, R"({"ok":true})");
		CHECK(readWire(sr) == static_cast<int>(MessageType::ShopResult));
		auto a = parseStringPayload(sr);
		CHECK(a.has_value());
		CHECK(a->text == R"({"ok":true})");

		auto sp = makeStringMessage(MessageType::ShopPurchase, R"({"type":"item"})");
		CHECK(readWire(sp) == static_cast<int>(MessageType::ShopPurchase));
		auto b = parseStringPayload(sp);
		CHECK(b.has_value());
		CHECK(b->text == R"({"type":"item"})");
		return true;
	}

	bool testChoice() {
		unool::core::ChoicePrompt prompt;
		prompt.title = "请选择";
		prompt.options = {"甲", "乙", "丙"};
		prompt.forced = true;
		prompt.errorMsg = "提示";
		prompt.timeoutMs = std::size_t{5000};
		prompt.currentPage = 1;
		prompt.totalPages = 3;
		unool::core::ChoiceOptionMeta m1{"角色甲", "S", "175", "默认", "images/a.jpg"};
		unool::core::ChoiceOptionMeta m2{"角色乙", "A", "100/200", "皮肤X", "images/b.jpg"};
		prompt.optionMeta = {m1, m2};

		auto packet = makeChoice(prompt);
		CHECK(readWire(packet) == static_cast<int>(MessageType::Choice));
		auto parsed = parseChoice(packet);
		CHECK(parsed.has_value());
		CHECK(parsed->title == prompt.title);
		CHECK(parsed->options == prompt.options);
		CHECK(parsed->forced == prompt.forced);
		CHECK(parsed->errorMsg == prompt.errorMsg);
		CHECK(parsed->timeoutMs.has_value());
		CHECK(parsed->timeoutMs.value() == 5000u);
		CHECK(parsed->currentPage == 1u);
		CHECK(parsed->totalPages == 3u);
		CHECK(parsed->optionMeta.size() == 2u);
		CHECK(parsed->optionMeta[0].characterName == "角色甲");
		CHECK(parsed->optionMeta[0].imagePath == "images/a.jpg");
		CHECK(parsed->optionMeta[1].skinName == "皮肤X");
		return true;
	}

	bool testGameState() {
		GameState state;
		state.currentPlayerIndex = 1;
		state.currentColor = Card::Color::red;
		state.currentName = Card::Name::action_skip;
		state.direction = -1;
		state.gameCount = 4;
		state.operatingPlayerId = std::size_t{0};

		PlayerState player;
		player.id = 0;
		player.hp = 3;
		player.maxHp = 5;
		player.characterNames = {"角色甲"};
		player.skins = {"默认"};
		player.characterImagePaths = {"images/a.jpg"};
		player.marks["标记"] = 2;
		state.players.push_back(player);

		auto packet = makeGameStateMessage(state);
		CHECK(readWire(packet) == static_cast<int>(MessageType::GameState));
		GameState parsed;
		CHECK(parseGameState(packet, parsed));
		CHECK(parsed.currentPlayerIndex == 1u);
		CHECK(parsed.currentName == Card::Name::action_skip);
		CHECK(parsed.direction == -1);
		CHECK(parsed.gameCount == 4u);
		CHECK(parsed.operatingPlayerId.has_value());
		CHECK(parsed.players.size() == 1u);
		CHECK(parsed.players[0].hp == 3);
		CHECK(parsed.players[0].marks.at("标记") == 2u);
		return true;
	}

	bool testCharInfo() {
		CharInfo info;
		info.playerIndex = 5;
		info.fullText = "技能描述文本";
		auto packet = makeCharInfoMessage(info);
		CHECK(readWire(packet) == static_cast<int>(MessageType::CharInfo));
		CharInfo parsed;
		CHECK(parseCharInfo(packet, parsed));
		CHECK(parsed.playerIndex == 5u);
		CHECK(parsed.fullText == "技能描述文本");
		return true;
	}

	bool testReadMessageType() {
		auto packet = makeHeartbeat();
		auto mt = readMessageType(packet);
		CHECK(mt.has_value());
		CHECK(*mt == MessageType::Heartbeat);
		return true;
	}

} // namespace

CodecTestResult runCodecRoundTripTests() {
	g_pass = 0;
	g_fail = 0;
	TEST_CASE(testConnectionInfo);
	TEST_CASE(testGameStartHeartbeat);
	TEST_CASE(testCheckinResponse);
	TEST_CASE(testGameEnd);
	TEST_CASE(testPointerUpdate);
	TEST_CASE(testClientInput);
	TEST_CASE(testStringMessages);
	TEST_CASE(testChoice);
	TEST_CASE(testGameState);
	TEST_CASE(testCharInfo);
	TEST_CASE(testReadMessageType);
	return { g_pass, g_fail };
}
