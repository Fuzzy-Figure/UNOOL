#include "RoomProtocol.h"
#include "MessageCodec.h"

#include <SFML/Network.hpp>

namespace unool::protocol {

	namespace {

		// 统一的事件打包：int 消息号 + JSON 字符串（与商城载荷同构）。
		sf::Packet makeJsonEvent(const MessageType type, const nlohmann::json& body) {
			return makeStringMessage(type, body.dump());
		}

		// 取出事件载荷的 JSON：读不到字符串或解析失败一律 nullopt（坏包静默丢弃）。
		std::optional<nlohmann::json> takeJson(sf::Packet& packet) {
			std::string jsonStr;
			if (!(packet >> jsonStr)) return std::nullopt;
			nlohmann::json j = nlohmann::json::parse(jsonStr, nullptr, false);
			if (j.is_discarded()) return std::nullopt;
			return j;
		}

	}

	// ---- 客户端 -> 服务端 ----

	sf::Packet makeRoomListRequest() {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::RoomListRequest);
		return packet;
	}

	sf::Packet makeRoomCreateRequest(const std::string& roomName) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::RoomCreateRequest);
		packet << roomName;
		return packet;
	}

	sf::Packet makeRoomJoinRequest(const std::uint64_t roomId) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::RoomJoinRequest);
		packet << roomId;
		return packet;
	}

	sf::Packet makeRoomLeaveRequest() {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::RoomLeaveRequest);
		return packet;
	}

	sf::Packet makeRoomReadyRequest(const bool ready) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::RoomReadyRequest);
		packet << ready;
		return packet;
	}

	// ---- 服务端 -> 客户端 ----

	sf::Packet makeRoomListResponse(const std::vector<RoomSummary>& rooms) {
		RoomListBody body;
		body.rooms = rooms;
		return makeJsonEvent(MessageType::RoomListResponse, nlohmann::json(body));
	}

	sf::Packet makeRoomStateResponse(const RoomSnapshot& room) {
		return makeJsonEvent(MessageType::RoomStateResponse, nlohmann::json(room));
	}

	sf::Packet makeRoomErrorResponse(const std::string& msg) {
		RoomErrorBody body;
		body.msg = msg;
		return makeJsonEvent(MessageType::RoomErrorResponse, nlohmann::json(body));
	}

	// ---- 解析 ----

	std::optional<std::string> parseRoomNameRequest(sf::Packet& packet) {
		std::string roomName;
		if (!(packet >> roomName)) return std::nullopt;
		return roomName;
	}

	std::optional<std::uint64_t> parseRoomIdRequest(sf::Packet& packet) {
		std::uint64_t roomId{};
		if (!(packet >> roomId)) return std::nullopt;
		return roomId;
	}

	std::optional<bool> parseRoomReadyRequest(sf::Packet& packet) {
		bool ready{};
		if (!(packet >> ready)) return std::nullopt;
		return ready;
	}

	std::optional<std::vector<RoomSummary>> parseRoomListResponse(sf::Packet& packet) {
		auto j = takeJson(packet);
		if (!j) return std::nullopt;
		try {
			return j->get<RoomListBody>().rooms;
		}
		catch (const std::exception&) {
			return std::nullopt;
		}
	}

	std::optional<RoomSnapshot> parseRoomStateResponse(sf::Packet& packet) {
		auto j = takeJson(packet);
		if (!j) return std::nullopt;
		try {
			return j->get<RoomSnapshot>();
		}
		catch (const std::exception&) {
			return std::nullopt;
		}
	}

	std::optional<std::string> parseRoomErrorResponse(sf::Packet& packet) {
		auto j = takeJson(packet);
		if (!j) return std::nullopt;
		try {
			return j->get<RoomErrorBody>().msg;
		}
		catch (const std::exception&) {
			return std::nullopt;
		}
	}

} // namespace unool::protocol
