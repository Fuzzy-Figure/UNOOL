#pragma once

// 线上消息类型枚举：阶段 3 第八批由 header/adapters/Socket.h 逐字平移至此。
// 目的：协议层（AccountProtocol）不再需要反向包含适配器头 Socket.h。
// 枚举值、顺序、隐式数值与线上字节均未改动；Q3 排除的「拆成 ClientCommand / ServerEvent 两组并改 switch」未做。

//网络
enum class MessageType {
	None,
	ClientInput,
	GameState,
	ConnectionInfo,
	ConnectionRefused,
	GameStart,
	GameEnd,
	Choice,
	PointerUpdate,
	CharInfo,
	RegisterRequest,
	RegisterResponse,
	LoginRequest,
	LoginResponse,
	CheckUsernameRequest,
	CheckUsernameResponse,
	Heartbeat,
	ShopData,
	ShopPurchase,
	ShopResult,
	ReadyToPlay,
	SkillInfoRequest,
	SkillInfoResponse,
	CheckinRequest,
	CheckinResponse
};
