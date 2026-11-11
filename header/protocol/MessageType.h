#pragma once

// 线上消息类型枚举：阶段 3 第八批由 header/shell/Socket.h 逐字平移至此。
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
	CheckinResponse,
	// 退出匹配：客户端→服务端请求取消准备；服务端→客户端广播「本场匹配已解散」。
	// 匹配提示：服务端→客户端广播「对手已开始匹配」，让另一名玩家知道该点「开始游戏」。
	// 只追加在末尾，既有枚举值与线上字节不变。
	CancelMatchmaking,
	MatchmakingCancelled,
	MatchmakingStarted
};
