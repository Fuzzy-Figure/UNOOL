#pragma once

#include <cstdint>

namespace unool::protocol {

	// Arch-C P1-1：线上消息号。单一 enum class + 显式赋值 + 按方向分段重编号。
	// 破坏性换代：允许线上字节变化，旧版本不再兼容（版本握手见 protocol/Protocol.h）。
	//
	// 分段约定（数值本身即方向，便于收发两端一眼识别与校验）：
	//   0       保留（非法/未初始化）
	//   100-199 客户端 -> 服务端命令（ClientCommand 段）
	//   200-299 服务端 -> 客户端事件（ServerEvent 段）
	//   900+    双向控制（心跳）
	//
	// 发送端只应产生本方向的消息号；parse 时可用 isClientCommand / isServerEvent
	// 做方向校验，避免把命令当事件解析（旧单枚举靠人工约定，现由数值分段表达）。
	//
	// 房间功能（协议版本 3）：登录后不再匹配，改为「大厅房间列表 → 创建/加入房间 →
	// 房内双方准备 → 开局」。原匹配消息号（105 ReadyToPlay / 106 CancelMatchmaking /
	// 216 MatchmakingCancelled / 217 MatchmakingStarted）已下线，编号留空不复用。
	enum class MessageType : std::uint16_t {
		None = 0,

		// ---- 客户端 -> 服务端（1xx）----
		ClientInput       = 101,
		RegisterRequest   = 102,
		LoginRequest      = 103,
		CheckUsernameRequest = 104,
		ShopPurchase      = 107,
		SkillInfoRequest  = 108,
		CheckinRequest    = 109,
		RoomListRequest   = 110,
		RoomCreateRequest = 111,
		RoomJoinRequest   = 112,
		RoomLeaveRequest  = 113,
		RoomReadyRequest  = 114,

		// ---- 服务端 -> 客户端（2xx）----
		ConnectionInfo    = 201,
		ConnectionRefused = 202,
		RegisterResponse  = 203,
		LoginResponse     = 204,
		CheckUsernameResponse = 205,
		GameStart         = 206,
		GameEnd           = 207,
		GameState         = 208,
		Choice            = 209,
		PointerUpdate     = 210,
		CharInfo          = 211,
		ShopData          = 212,
		ShopResult        = 213,
		SkillInfoResponse = 214,
		CheckinResponse   = 215,
		RoomListResponse  = 218,
		RoomStateResponse = 219,
		RoomErrorResponse = 220,
		// 同账号在另一端登录，本端被顶下线：服务端在断开旧连接前先发这一条，
		// 客户端据此提示原因并退回登录界面（否则只能看到一次静默断线）。
		SessionReplaced   = 221,

		// ---- 双向控制（9xx）----
		Heartbeat         = 901,
	};

	constexpr std::uint16_t toWire(MessageType type) noexcept {
		return static_cast<std::uint16_t>(type);
	}

	constexpr bool isClientCommand(MessageType type) noexcept {
		const auto v = toWire(type);
		return v >= 100 && v < 200;
	}

	constexpr bool isServerEvent(MessageType type) noexcept {
		const auto v = toWire(type);
		return v >= 200 && v < 300;
	}

	constexpr bool isControlMessage(MessageType type) noexcept {
		return toWire(type) >= 900;
	}

} // namespace unool::protocol
