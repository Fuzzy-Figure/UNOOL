#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <json.hpp>

#include "MessageType.h"

// 房间协议（协议版本 3）：登录后不再匹配，改为「房间」组队。
//
// 命令（客户端 -> 服务端）都是「消息号 + 少量字段」：
//   RoomListRequest   ：无载荷
//   RoomCreateRequest ：房间名（std::string）
//   RoomJoinRequest   ：房间号（std::uint64_t）
//   RoomLeaveRequest  ：无载荷
//   RoomReadyRequest  ：准备状态（bool）
//
// 事件（服务端 -> 客户端）统一走「消息号 + JSON 字符串」载荷，与商城的
// ShopData / ShopResult 同一套线格式，字段演进不必改二进制布局：
//   RoomListResponse  ：{"rooms":[{id,name,owner,members,capacity,inGame}, ...]}
//   RoomStateResponse ：单房间快照（含成员准备状态；closed=true 表示房间已解散/已离开）
//   RoomErrorResponse ：{"msg":"..."} 或纯字符串载荷
//
// 头文件只前向声明 sf::Packet；JSON 结构用 nlohmann 宏内联定义，
// 由 source/protocol/RoomProtocol.cpp 负责打包与解析。
namespace sf { class Packet; }

namespace unool::protocol {

	// 房间容量：一桌两个座位（玩法规则），房间只容纳这两个人。
	inline constexpr std::size_t kRoomCapacity = 2;

	// 房间号 0 表示「不在任何房间」。
	inline constexpr std::uint64_t kNoRoom = 0;

	struct RoomMember {
		std::string name;      // 账号名
		bool ready = false;    // 是否已准备
		bool owner = false;    // 是否房主

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RoomMember, name, ready, owner)
	};

	// 房间列表里的一行（大厅展示用）
	struct RoomSummary {
		std::uint64_t id = kNoRoom;
		std::string name;          // 房间名
		std::string owner;         // 房主账号名
		std::size_t members = 0;   // 当前人数
		std::size_t capacity = kRoomCapacity;
		bool inGame = false;       // 是否已开局（对局中不可加入）

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RoomSummary, id, name, owner, members, capacity, inGame)
	};

	// 当前所在房间的完整快照
	struct RoomSnapshot {
		std::uint64_t id = kNoRoom;
		std::string name;
		std::string owner;         // 房主账号名（房主退出时服务端会转让）
		std::size_t capacity = kRoomCapacity;
		bool inGame = false;
		// closed=true：本客户端已不在该房间（离开 / 被解散 / 房主转移导致的重建提示）
		bool closed = false;
		std::string reason;        // closed=true 时的可读说明
		std::vector<RoomMember> members;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RoomSnapshot, id, name, owner, capacity, inGame,
									   closed, reason, members)
	};

	struct RoomListBody {
		std::vector<RoomSummary> rooms;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RoomListBody, rooms)
	};

	struct RoomErrorBody {
		std::string msg;

		NLOHMANN_DEFINE_TYPE_INTRUSIVE(RoomErrorBody, msg)
	};

	// ---- 客户端 -> 服务端 ----
	sf::Packet makeRoomListRequest();
	sf::Packet makeRoomCreateRequest(const std::string& roomName);
	sf::Packet makeRoomJoinRequest(std::uint64_t roomId);
	sf::Packet makeRoomLeaveRequest();
	sf::Packet makeRoomReadyRequest(bool ready);

	// ---- 服务端 -> 客户端 ----
	sf::Packet makeRoomListResponse(const std::vector<RoomSummary>& rooms);
	sf::Packet makeRoomStateResponse(const RoomSnapshot& room);
	sf::Packet makeRoomErrorResponse(const std::string& msg);

	// ---- 解析（packet 已消费开头的 int 消息号）----
	std::optional<std::string> parseRoomNameRequest(sf::Packet& packet);
	std::optional<std::uint64_t> parseRoomIdRequest(sf::Packet& packet);
	std::optional<bool> parseRoomReadyRequest(sf::Packet& packet);

	std::optional<std::vector<RoomSummary>> parseRoomListResponse(sf::Packet& packet);
	std::optional<RoomSnapshot> parseRoomStateResponse(sf::Packet& packet);
	std::optional<std::string> parseRoomErrorResponse(sf::Packet& packet);

} // namespace unool::protocol
