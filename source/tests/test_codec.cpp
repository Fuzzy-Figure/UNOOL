// Arch-C P1-2：MessageCodec 成对序列化 round-trip 单测。
// 每个消息号 make -> 读消息号 -> parse -> 字段逐项比对，
// 验证收发两端字节自洽（本地 g++ 无法链接 MSVC 的 SFML .lib，故在 VS 的 Tests 工程运行）。
#include "test_codec.h"
#include "test_support.h"

#include "MessageCodec.h"
#include "GameState.h"
#include "core/IDecisionPort.h"
#include <SFML/Network.hpp>

#include <string>

using namespace unool::protocol;

namespace {

	int readWire(sf::Packet& packet) {
		int raw{};
		packet >> raw;
		return raw;
	}

	void testConnectionInfo(unool::tests::TestResult& r) {
		auto packet = makeConnectionInfo(7);
		r.check(readWire(packet) == static_cast<int>(MessageType::ConnectionInfo));
		auto data = parseConnectionInfo(packet);
		r.check(data.has_value());
		r.check(data->playerId == 7u);
	}

	void testGameStartHeartbeat(unool::tests::TestResult& r) {
		auto gs = makeGameStart();
		r.check(readWire(gs) == static_cast<int>(MessageType::GameStart));
		auto hb = makeHeartbeat();
		r.check(readWire(hb) == static_cast<int>(MessageType::Heartbeat));
	}

	void testCheckinResponse(unool::tests::TestResult& r) {
		auto packet = makeCheckinResponse("签到成功", true, 123);
		r.check(readWire(packet) == static_cast<int>(MessageType::CheckinResponse));
		std::string msg;
		bool ok{};
		int points{};
		r.check(static_cast<bool>(packet >> msg >> ok >> points));
		r.check(msg == "签到成功");
		r.check(ok);
		r.check(points == 123);
	}

	void testGameEnd(unool::tests::TestResult& r) {
		auto withWinner = makeGameEnd(std::size_t{3}, "玩家A");
		r.check(readWire(withWinner) == static_cast<int>(MessageType::GameEnd));
		auto a = parseGameEnd(withWinner);
		r.check(a.has_value());
		r.check(a->hasWinner);
		r.check(a->winnerId == 3u);
		r.check(a->winnerName == "玩家A");

		auto noWinner = makeGameEnd(std::nullopt, "");
		r.check(readWire(noWinner) == static_cast<int>(MessageType::GameEnd));
		auto b = parseGameEnd(noWinner);
		r.check(b.has_value());
		r.check(!b->hasWinner);
	}

	void testPointerUpdate(unool::tests::TestResult& r) {
		auto packet = makePointerUpdate(2, 9);
		r.check(readWire(packet) == static_cast<int>(MessageType::PointerUpdate));
		auto data = parsePointerUpdate(packet);
		r.check(data.has_value());
		r.check(data->playerId == 2u);
		r.check(data->selectedIndex == 9u);
	}

	void testClientInput(unool::tests::TestResult& r) {
		auto packet = makeClientInput(42, 1, 5);
		r.check(readWire(packet) == static_cast<int>(MessageType::ClientInput));
		auto data = parseClientInput(packet);
		r.check(data.has_value());
		r.check(data->key == 42);
		r.check(data->playerId == 1u);
		r.check(data->selectedIndex == 5u);
	}

	void testStringMessages(unool::tests::TestResult& r) {
		auto sr = makeStringMessage(MessageType::ShopResult, R"({"ok":true})");
		r.check(readWire(sr) == static_cast<int>(MessageType::ShopResult));
		auto a = parseStringPayload(sr);
		r.check(a.has_value());
		r.check(a->text == R"({"ok":true})");

		auto sp = makeStringMessage(MessageType::ShopPurchase, R"({"type":"item"})");
		r.check(readWire(sp) == static_cast<int>(MessageType::ShopPurchase));
		auto b = parseStringPayload(sp);
		r.check(b.has_value());
		r.check(b->text == R"({"type":"item"})");
	}

	void testChoice(unool::tests::TestResult& r) {
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
		r.check(readWire(packet) == static_cast<int>(MessageType::Choice));
		auto parsed = parseChoice(packet);
		r.check(parsed.has_value());
		r.check(parsed->title == prompt.title);
		r.check(parsed->options == prompt.options);
		r.check(parsed->forced == prompt.forced);
		r.check(parsed->errorMsg == prompt.errorMsg);
		r.check(parsed->timeoutMs.has_value());
		r.check(parsed->timeoutMs.value() == 5000u);
		r.check(parsed->currentPage == 1u);
		r.check(parsed->totalPages == 3u);
		r.check(parsed->optionMeta.size() == 2u);
		r.check(parsed->optionMeta[0].characterName == "角色甲");
		r.check(parsed->optionMeta[0].imagePath == "images/a.jpg");
		r.check(parsed->optionMeta[1].skinName == "皮肤X");
	}

	void testGameState(unool::tests::TestResult& r) {
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
		r.check(readWire(packet) == static_cast<int>(MessageType::GameState));
		GameState parsed;
		r.check(parseGameState(packet, parsed));
		r.check(parsed.currentPlayerIndex == 1u);
		r.check(parsed.currentName == Card::Name::action_skip);
		r.check(parsed.direction == -1);
		r.check(parsed.gameCount == 4u);
		r.check(parsed.operatingPlayerId.has_value());
		r.check(parsed.players.size() == 1u);
		r.check(parsed.players[0].hp == 3);
		r.check(parsed.players[0].marks.at("标记") == 2u);
	}

	void testCharInfo(unool::tests::TestResult& r) {
		CharInfo info;
		info.playerIndex = 5;
		info.fullText = "技能描述文本";
		auto packet = makeCharInfoMessage(info);
		r.check(readWire(packet) == static_cast<int>(MessageType::CharInfo));
		CharInfo parsed;
		r.check(parseCharInfo(packet, parsed));
		r.check(parsed.playerIndex == 5u);
		r.check(parsed.fullText == "技能描述文本");
	}

	void testReadMessageType(unool::tests::TestResult& r) {
		auto packet = makeHeartbeat();
		auto mt = readMessageType(packet);
		r.check(mt.has_value());
		r.check(*mt == MessageType::Heartbeat);
	}

} // namespace

unool::tests::TestResult runCodecRoundTripTests() {
	unool::tests::TestResult r;
	r.section("codec.testConnectionInfo", testConnectionInfo);
	r.section("codec.testGameStartHeartbeat", testGameStartHeartbeat);
	r.section("codec.testCheckinResponse", testCheckinResponse);
	r.section("codec.testGameEnd", testGameEnd);
	r.section("codec.testPointerUpdate", testPointerUpdate);
	r.section("codec.testClientInput", testClientInput);
	r.section("codec.testStringMessages", testStringMessages);
	r.section("codec.testChoice", testChoice);
	r.section("codec.testGameState", testGameState);
	r.section("codec.testCharInfo", testCharInfo);
	r.section("codec.testReadMessageType", testReadMessageType);
	return r;
}
