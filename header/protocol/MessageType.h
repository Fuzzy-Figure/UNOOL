#pragma once

#include <cstdint>

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
enum class MessageType : std::uint16_t {
	None = 0,

	// ---- 客户端 -> 服务端（1xx）----
	ClientInput       = 101,
	RegisterRequest   = 102,
	LoginRequest      = 103,
	CheckUsernameRequest = 104,
	ReadyToPlay       = 105,
	CancelMatchmaking = 106,
	ShopPurchase      = 107,
	SkillInfoRequest  = 108,
	CheckinRequest    = 109,

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
	MatchmakingCancelled = 216,
	MatchmakingStarted   = 217,

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

// 分组视图：仅用于表达方向语义，取值与 MessageType 同源，不另立数值体系。
namespace ClientCommand {
	constexpr MessageType ClientInput = MessageType::ClientInput;
	constexpr MessageType RegisterRequest = MessageType::RegisterRequest;
	constexpr MessageType LoginRequest = MessageType::LoginRequest;
	constexpr MessageType CheckUsernameRequest = MessageType::CheckUsernameRequest;
	constexpr MessageType ReadyToPlay = MessageType::ReadyToPlay;
	constexpr MessageType CancelMatchmaking = MessageType::CancelMatchmaking;
	constexpr MessageType ShopPurchase = MessageType::ShopPurchase;
	constexpr MessageType SkillInfoRequest = MessageType::SkillInfoRequest;
	constexpr MessageType CheckinRequest = MessageType::CheckinRequest;
}

namespace ServerEvent {
	constexpr MessageType ConnectionInfo = MessageType::ConnectionInfo;
	constexpr MessageType ConnectionRefused = MessageType::ConnectionRefused;
	constexpr MessageType RegisterResponse = MessageType::RegisterResponse;
	constexpr MessageType LoginResponse = MessageType::LoginResponse;
	constexpr MessageType CheckUsernameResponse = MessageType::CheckUsernameResponse;
	constexpr MessageType GameStart = MessageType::GameStart;
	constexpr MessageType GameEnd = MessageType::GameEnd;
	constexpr MessageType GameState = MessageType::GameState;
	constexpr MessageType Choice = MessageType::Choice;
	constexpr MessageType PointerUpdate = MessageType::PointerUpdate;
	constexpr MessageType CharInfo = MessageType::CharInfo;
	constexpr MessageType ShopData = MessageType::ShopData;
	constexpr MessageType ShopResult = MessageType::ShopResult;
	constexpr MessageType SkillInfoResponse = MessageType::SkillInfoResponse;
	constexpr MessageType CheckinResponse = MessageType::CheckinResponse;
	constexpr MessageType MatchmakingCancelled = MessageType::MatchmakingCancelled;
	constexpr MessageType MatchmakingStarted = MessageType::MatchmakingStarted;
}
