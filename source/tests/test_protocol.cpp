// Arch-C P3-5：账号协议与商城 JSON 协议的成对 round-trip 单测。
// 与 test_codec.cpp 的分工：那边覆盖 MessageCodec（游戏态消息），
// 这边覆盖 AccountProtocol（登录 / 注册 / 签到 / 匹配）与 ShopProtocol（商城 JSON），
// 补齐工单 P3-5「协议 round-trip 覆盖全部消息号」剩下的消息号。
#include "test_protocol.h"

#include "AccountEvent.h"
#include "AccountProtocol.h"
#include "MessageCodec.h"
#include "RoomProtocol.h"
#include "ShopData.h"
#include "ShopProtocol.h"
#include <SFML/Network.hpp>

#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

using namespace unool::protocol;

namespace {

	// 读消息号（发送端一律用 int 写，读端也用 int，避免 to_underlying 宽度差异）。
	MessageType takeType(sf::Packet& packet) {
		int raw{};
		packet >> raw;
		return static_cast<MessageType>(raw);
	}

	void testVersionedAccountRequests(unool::tests::TestResult& r) {
		{
			auto packet = makeRegisterRequest("alice", "pw123");
			r.check(takeType(packet) == MessageType::RegisterRequest, "register type");
			const auto req = parseAccountRequest(packet, true);
			r.check(req.has_value(), "register parses");
			r.check(req->username == "alice", "register username");
			r.check(req->password == "pw123", "register password");
			r.check(req->versionMatches, "register version matches");
		}
		{
			auto packet = makeLoginRequest("bob", "secret");
			r.check(takeType(packet) == MessageType::LoginRequest, "login type");
			const auto req = parseAccountRequest(packet, true);
			r.check(req.has_value(), "login parses");
			r.check(req->username == "bob", "login username");
			r.check(req->versionMatches, "login version matches");
		}
		{
			// 版本不匹配：仍解析出账号（服务端要写日志），但 versionMatches=false。
			sf::Packet packet;
			packet << static_cast<int>(MessageType::LoginRequest)
				   << static_cast<std::uint32_t>(kProtocolVersion + 1) << "carol" << "pw";
			r.check(takeType(packet) == MessageType::LoginRequest, "mismatch type");
			const auto req = parseAccountRequest(packet, true);
			r.check(req.has_value(), "mismatch still parses identity");
			r.check(!req->versionMatches, "mismatch flagged");
			r.check(req->username == "carol", "mismatch username");
		}
		{
			// 缺版本字段的旧包：必须被拒绝为坏包，而不是猜出账号。
			sf::Packet packet;
			packet << static_cast<int>(MessageType::LoginRequest);
			r.check(takeType(packet) == MessageType::LoginRequest, "missing version type");
			r.check(!parseAccountRequest(packet, true).has_value(), "missing version rejected");
		}
	}

	void testOneShotRequests(unool::tests::TestResult& r) {
		{
			auto packet = makeCheckUsernameRequest("dave");
			r.check(takeType(packet) == MessageType::CheckUsernameRequest, "check-username type");
			const auto name = parseCheckUsernameRequest(packet);
			r.check(name.has_value(), "check-username parses");
			r.check(name.value_or("") == "dave", "check-username value");
		}
		{
			auto packet = makeCheckinRequest();
			r.check(takeType(packet) == MessageType::CheckinRequest, "checkin type");
			std::string leftover;
			r.check(!(packet >> leftover), "checkin has no payload");
		}
	}

	// 房间功能（协议版本 3）：命令是「消息号 + 少量字段」，事件是「消息号 + JSON 载荷」。
	void testRoomCommands(unool::tests::TestResult& r) {
		{
			auto packet = makeRoomListRequest();
			r.check(takeType(packet) == MessageType::RoomListRequest, "room list request type");
			std::string leftover;
			r.check(!(packet >> leftover), "room list request has no payload");
		}
		{
			auto packet = makeRoomCreateRequest("三分基地");
			r.check(takeType(packet) == MessageType::RoomCreateRequest, "room create type");
			const auto name = parseRoomNameRequest(packet);
			r.check(name.has_value() && name.value_or("") == "三分基地", "room create name");
		}
		{
			auto packet = makeRoomJoinRequest(42);
			r.check(takeType(packet) == MessageType::RoomJoinRequest, "room join type");
			const auto id = parseRoomIdRequest(packet);
			r.check(id.has_value() && id.value_or(0) == 42u, "room join id");
		}
		{
			auto packet = makeRoomLeaveRequest();
			r.check(takeType(packet) == MessageType::RoomLeaveRequest, "room leave type");
		}
		{
			auto packet = makeRoomReadyRequest(true);
			r.check(takeType(packet) == MessageType::RoomReadyRequest, "room ready type");
			const auto ready = parseRoomReadyRequest(packet);
			r.check(ready.has_value() && ready.value_or(false), "room ready value");
		}
		{
			// 坏包不能被当成「空房间列表」：缺字段返回 nullopt。
			sf::Packet broken;
			r.check(!parseRoomListResponse(broken).has_value(), "room list missing payload rejected");
			r.check(!parseRoomNameRequest(broken).has_value(), "room create missing payload rejected");
		}
	}

	void testRoomEvents(unool::tests::TestResult& r) {
		{
			RoomSummary summary;
			summary.id = 7;
			summary.name = "甲的房间";
			summary.owner = "甲";
			summary.members = 2;
			summary.capacity = kRoomCapacity;
			summary.inGame = true;

			auto packet = makeRoomListResponse({ summary });
			r.check(takeType(packet) == MessageType::RoomListResponse, "room list response type");
			const auto rooms = parseRoomListResponse(packet);
			r.check(rooms.has_value(), "room list response parses");
			r.check(rooms.has_value() && rooms->size() == 1u, "room list count");
			r.check(rooms.has_value() && rooms->front().id == 7u, "room list id");
			r.check(rooms.has_value() && rooms->front().name == "甲的房间", "room list name");
			r.check(rooms.has_value() && rooms->front().owner == "甲", "room list owner");
			r.check(rooms.has_value() && rooms->front().members == 2u, "room list members");
			r.check(rooms.has_value() && rooms->front().inGame, "room list inGame");
		}
		{
			RoomSnapshot snapshot;
			snapshot.id = 9;
			snapshot.name = "乙的房间";
			snapshot.owner = "乙";
			snapshot.capacity = kRoomCapacity;
			snapshot.members = { RoomMember{ "乙", true, true }, RoomMember{ "丙", false, false } };

			auto packet = makeRoomStateResponse(snapshot);
			r.check(takeType(packet) == MessageType::RoomStateResponse, "room state response type");
			const auto parsed = parseRoomStateResponse(packet);
			r.check(parsed.has_value(), "room state response parses");
			r.check(parsed.has_value() && parsed->members.size() == 2u, "room state member count");
			r.check(parsed.has_value() && parsed->members[0].ready, "room state ready flag");
			r.check(parsed.has_value() && parsed->members[0].owner, "room state owner flag");
			r.check(parsed.has_value() && !parsed->closed, "room state not closed");
		}
		{
			// 解散 / 离开：closed=true 携带可读原因
			RoomSnapshot snapshot;
			snapshot.id = 9;
			snapshot.name = "乙的房间";
			snapshot.closed = true;
			snapshot.reason = "房主已解散房间";
			auto packet = makeRoomStateResponse(snapshot);
			takeType(packet);
			const auto parsed = parseRoomStateResponse(packet);
			r.check(parsed.has_value() && parsed->closed, "room state closed flag");
			r.check(parsed.has_value() && parsed->reason == "房主已解散房间", "room state reason");
		}
		{
			auto packet = makeRoomErrorResponse("房间已满");
			r.check(takeType(packet) == MessageType::RoomErrorResponse, "room error type");
			const auto msg = parseRoomErrorResponse(packet);
			r.check(msg.has_value() && msg.value_or("") == "房间已满", "room error message");
		}
		{
			sf::Packet garbage;
			garbage << static_cast<int>(MessageType::RoomStateResponse) << std::string("not json");
			takeType(garbage);
			r.check(!parseRoomStateResponse(garbage).has_value(), "room state bad json rejected");
		}
	}

	void testAccountResponses(unool::tests::TestResult& r) {
		{
			auto packet = makeAccountResponse(MessageType::LoginResponse, true, "登录成功", 120, 3, 1);
			r.check(takeType(packet) == MessageType::LoginResponse, "login response type");
			const auto resp = parseAccountResponse(packet);
			r.check(resp.has_value(), "login response parses");
			r.check(resp->ok, "login response ok");
			r.check(resp->msg == "登录成功", "login response msg");
			r.check(resp->points == 120 && resp->wins == 3 && resp->losses == 1, "login response counters");
		}
		{
			// 两个解析入口必须读出同一份字段（AccountEvent 是 ViewModel 用的那条路）。
			auto packet = makeAccountResponse(MessageType::RegisterResponse, false, "用户名已存在", 0, 0, 0);
			r.check(takeType(packet) == MessageType::RegisterResponse, "register response type");
			const auto ev = parseAccountEvent(MessageType::RegisterResponse, packet);
			r.check(ev.has_value(), "register response event parses");
			r.check(!ev->ok, "register response event ok=false");
			r.check(ev->msg == "用户名已存在", "register response event msg");
		}
		{
			auto packet = makeCheckUsernameResponse(true);
			r.check(takeType(packet) == MessageType::CheckUsernameResponse, "check-username response type");
			const auto exists = parseCheckUsernameResponse(packet);
			r.check(exists.has_value() && exists.value(), "check-username response true");
		}
		{
			auto packet = makeCheckUsernameResponse(false);
			const auto exists = parseCheckUsernameResponse(packet);
			r.check(exists.has_value() && !exists.value(), "check-username response false");
		}
		{
			// 版本不匹配包仍是 LoginResponse(ok=false)，客户端据此展示可读原因。
			auto packet = makeVersionMismatchResponse();
			r.check(takeType(packet) == MessageType::LoginResponse, "mismatch response type");
			const auto resp = parseAccountResponse(packet);
			r.check(resp.has_value(), "mismatch response parses");
			r.check(!resp->ok, "mismatch response ok=false");
			r.check(resp->msg == kProtocolVersionMismatch, "mismatch response message");
		}
		{
			// 签到响应：make 写 msg/ok/points，parseAccountEvent 按同一顺序读。
			auto packet = makeCheckinResponse("签到成功，积分 +50", true, 50);
			r.check(takeType(packet) == MessageType::CheckinResponse, "checkin response type");
			const auto ev = parseAccountEvent(MessageType::CheckinResponse, packet);
			r.check(ev.has_value(), "checkin response event parses");
			r.check(ev->msg == "签到成功，积分 +50", "checkin response msg");
			r.check(ev->ok, "checkin response ok");
			r.check(ev->points == 50, "checkin response points");
		}
	}

	void testShopPurchaseRequest(unool::tests::TestResult& r) {
		struct Case {
			ShopRequestType type;
			const char* typeText;
			const char* name;
			const char* charName;
			const char* skinName;
		};
		const Case cases[] = {
			{ ShopRequestType::Request, "request", "", "", "" },
			{ ShopRequestType::Character, "character", "角色甲", "", "" },
			{ ShopRequestType::Skin, "skin", "", "角色甲", "皮肤乙" },
			{ ShopRequestType::Item, "item", "改名卡", "", "" },
		};
		for (const auto& c : cases) {
			const std::string jsonStr = makeShopPurchaseJson(c.type, c.name, c.charName, c.skinName);
			nlohmann::json j = nlohmann::json::parse(jsonStr, nullptr, false);
			r.check(!j.is_discarded(), "purchase json parses");
			const auto req = j.get<ShopPurchaseRequest>();
			r.check(req.type == c.type, "purchase type field");
			r.check(to_string(req.type) == c.typeText, "purchase type text");
			r.check(req.name == c.name, "purchase name field");
			r.check(req.charName == c.charName, "purchase charName field");
			r.check(req.skinName == c.skinName, "purchase skinName field");
		}
	}

	void testShopPayloads(unool::tests::TestResult& r) {
		{
			// 购买结果：服务端 dump -> 客户端 parse（Packet 级重载，消息号已消费）。
			ShopPurchaseResult result;
			result.ok = true;
			result.msg = "购买成功";
			result.points = 88;
			sf::Packet packet;
			packet << static_cast<int>(MessageType::ShopResult)
				   << nlohmann::json(result).dump();
			r.check(takeType(packet) == MessageType::ShopResult, "shop result type");
			const auto parsed = parseShopPurchaseResult(packet);
			r.check(parsed.has_value(), "shop result parses");
			r.check(parsed->ok, "shop result ok");
			r.check(parsed->msg == "购买成功", "shop result msg");
			r.check(parsed->points == 88, "shop result points");
		}
		{
			ShopData data;
			data.points = 320;

			ShopSkinInfo skin;
			skin.name = "星夜";
			skin.unlocked = false;
			skin.price = static_cast<ShopPrice>(150);
			skin.quality = "史诗";
			skin.imagePath = "images/skins/xingye.jpg";

			ShopCharacterInfo chr;
			chr.name = "角色甲";
			chr.level = ShopLevel::S;
			chr.unlocked = true;
			chr.price = std::nullopt;
			chr.skins = { skin };
			chr.defaultImagePath = "images/characters/a.jpg";
			data.characters = { chr };

			ShopItemInfo item;
			item.name = "改名卡";
			item.count = 2;
			item.price = static_cast<ShopPrice>(50);
			item.available = true;
			item.description = "修改昵称";
			item.maxCount = 5;
			data.items = { item };

			sf::Packet packet;
			packet << static_cast<int>(MessageType::ShopData) << nlohmann::json(data).dump();
			r.check(takeType(packet) == MessageType::ShopData, "shop data type");
			const auto parsed = parseShopData(packet);
			r.check(parsed.has_value(), "shop data parses");
			r.check(parsed->points == 320, "shop data points");
			r.check(parsed->characters.size() == 1u, "shop data character count");
			r.check(parsed->characters[0].name == "角色甲", "shop data character name");
			r.check(parsed->characters[0].level == ShopLevel::S, "shop data character level");
			r.check(parsed->characters[0].skins.size() == 1u, "shop data skin count");
			r.check(parsed->characters[0].skins[0].imagePath == "images/skins/xingye.jpg", "shop data skin path");
			r.check(parsed->items.size() == 1u, "shop data item count");
			r.check(parsed->items[0].maxCount.has_value() && parsed->items[0].maxCount.value() == 5, "shop data item max");
			r.check(parsed->items[0].description == "修改昵称", "shop data item description");
		}
		{
			SkillInfoResponseBody body;
			body.skills["角色甲"] = "技能描述";
			sf::Packet packet;
			packet << static_cast<int>(MessageType::SkillInfoResponse) << nlohmann::json(body).dump();
			r.check(takeType(packet) == MessageType::SkillInfoResponse, "skill response type");
			const auto parsed = parseSkillInfoResponse(packet);
			r.check(parsed.has_value(), "skill response parses");
			r.check(parsed->skills.size() == 1u, "skill response count");
			r.check(parsed->skills.at("角色甲") == "技能描述", "skill response text");
		}
		{
			// 请求侧：makeSkillInfoRequestPacket 的包体是可解析的 SkillInfoRequestBody。
			auto packet = makeSkillInfoRequestPacket({ "角色甲", "角色乙" });
			r.check(takeType(packet) == MessageType::SkillInfoRequest, "skill request type");
			std::string jsonStr;
			r.check(static_cast<bool>(packet >> jsonStr), "skill request payload readable");
			nlohmann::json j = nlohmann::json::parse(jsonStr, nullptr, false);
			r.check(!j.is_discarded(), "skill request json parses");
			const auto body = j.get<SkillInfoRequestBody>();
			r.check(body.names == std::vector<std::string>{ "角色甲", "角色乙" }, "skill request names");
		}
		{
			// 坏包不能被当成空数据：解析失败必须返回 nullopt。
			sf::Packet broken;
			r.check(!parseShopData(broken).has_value(), "shop data missing payload rejected");
			sf::Packet garbage;
			garbage << static_cast<int>(MessageType::ShopData) << std::string("not json");
			takeType(garbage);
			r.check(!parseShopData(garbage).has_value(), "shop data bad json rejected");
		}
	}

	void testMessageInventory(unool::tests::TestResult& r) {
		// 每个消息号必须且只能落在一个方向段里；新增枚举值忘了分段会在这里暴露。
		const MessageType all[] = {
			MessageType::ClientInput, MessageType::RegisterRequest, MessageType::LoginRequest,
			MessageType::CheckUsernameRequest, MessageType::ShopPurchase, MessageType::SkillInfoRequest,
			MessageType::CheckinRequest, MessageType::RoomListRequest, MessageType::RoomCreateRequest,
			MessageType::RoomJoinRequest, MessageType::RoomLeaveRequest, MessageType::RoomReadyRequest,
			MessageType::ConnectionInfo, MessageType::ConnectionRefused, MessageType::RegisterResponse,
			MessageType::LoginResponse, MessageType::CheckUsernameResponse, MessageType::GameStart,
			MessageType::GameEnd, MessageType::GameState, MessageType::Choice,
			MessageType::PointerUpdate, MessageType::CharInfo, MessageType::ShopData,
			MessageType::ShopResult, MessageType::SkillInfoResponse, MessageType::CheckinResponse,
			MessageType::RoomListResponse, MessageType::RoomStateResponse, MessageType::RoomErrorResponse,
			MessageType::Heartbeat,
		};
		int commands = 0;
		int events = 0;
		int controls = 0;
		for (const auto type : all) {
			const int hits = static_cast<int>(isClientCommand(type)) + static_cast<int>(isServerEvent(type))
						   + static_cast<int>(isControlMessage(type));
			r.check(hits == 1, "message classified exactly once");
			if (isClientCommand(type)) ++commands;
			else if (isServerEvent(type)) ++events;
			else if (isControlMessage(type)) ++controls;
		}
		r.check(std::size(all) == 31u, "message inventory size");
		r.check(commands == 12, "12 client commands");
		r.check(events == 18, "18 server events");
		r.check(controls == 1, "1 control message");
		// None 不属于任何方向段，接收端不应把它当业务消息。
		r.check(!isClientCommand(MessageType::None) && !isServerEvent(MessageType::None)
				&& !isControlMessage(MessageType::None), "None unclassified");
		// 读到 0 或空包时的行为（接收端据此丢弃）：空包 nullopt，0 归一为 None。
		sf::Packet empty;
		r.check(!readMessageType(empty).has_value(), "empty packet rejected");
		sf::Packet zero;
		zero << 0;
		const auto mt = readMessageType(zero);
		r.check(mt.has_value() && *mt == MessageType::None, "zero reads as None");
	}

} // namespace

unool::tests::TestResult runProtocolRoundTripTests() {
	unool::tests::TestResult r;
	r.section("protocol.testVersionedAccountRequests", testVersionedAccountRequests);
	r.section("protocol.testOneShotRequests", testOneShotRequests);
	r.section("protocol.testRoomCommands", testRoomCommands);
	r.section("protocol.testRoomEvents", testRoomEvents);
	r.section("protocol.testAccountResponses", testAccountResponses);
	r.section("protocol.testShopPurchaseRequest", testShopPurchaseRequest);
	r.section("protocol.testShopPayloads", testShopPayloads);
	r.section("protocol.testMessageInventory", testMessageInventory);
	return r;
}
