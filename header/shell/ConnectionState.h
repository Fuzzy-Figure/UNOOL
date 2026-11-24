#pragma once
#include <cstddef>
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>
#include <SFML/Network.hpp>
#include "MessageType.h"

namespace unool::shell {

	// MessageType 归 protocol 层：网络上的一切消息号都是协议概念，本层只转发类型名。
	using protocol::MessageType;

	class IClientGateway;

	// Arch-C P1-3：显式入站包（已消费开头 int 消息号，body 为剩余字节）。
	struct Inbound {
		MessageType type{};
		sf::Packet body{};
	};

	// Arch-C P1-3：单一连接状态机。
	// 每帧 pump() 一次，统一完成：驱动网络 IO → 回复心跳 → 把新包打 tag 入显式队列 →
	// 断线时丢弃积压。取代原 peekMessageType() 的隐式跨阶段留包约定：
	// 所有入站包先入队，take(订阅集) 只消费队首命中的包，未命中的留在队列里
	// 交给下一阶段（如 GameStart 从 LoginScene 留到 gamePhase），顺序由队列语义
	// 保证，不再依赖「peek 后不消费」的调用顺序。
	//
	// 只依赖 IClientGateway 端口（不绑 ClientNetwork 具体类型），可用 fake gateway
	// headless 驱动，断线场景行为可枚举。
	class ConnectionState {
	public:
		explicit ConnectionState(IClientGateway& gateway);

		// 每帧调用一次：驱动 IO、回复心跳、入队；若从连接态转为断线则丢弃积压包。
		void pump();

		// 订阅消费：队首类型命中 subscription 才取走，否则返回 nullopt（包留队）。
		std::optional<Inbound> take(std::initializer_list<MessageType> subscription);
		// DrainAll：取走队首任意包。
		std::optional<Inbound> takeAny();
		// 队列中是否存在指定类型的包（只读、不消费）——用于跨阶段 GameStart 判定。
		bool contains(MessageType type) const;
		// 丢弃所有排队包（断线 / 重连前）。
		void clear();

		// 连接状态：收敛原先散在 Scene 与 run() 的 isConnected 判定。
		bool isConnected() const;

		// 本端是否被「同账号在别处登录」顶下线（收到 SessionReplaced）。
		// pump() 收到该消息时就地记录原因并主动断开，各阶段随后按普通断线退出；
		// 组合根据此把用户带回登录界面并展示原因。
		bool wasKickedOffline() const { return kicked_; }
		const std::string& kickedReason() const { return kickedReason_; }
		void clearKickedOffline() { kicked_ = false; kickedReason_.clear(); }

		// 出站转发：保持各调用点发包路径不变（P1-3 不动出站语义）。
		bool send(sf::Packet& packet);
		bool sendShopPurchase(const std::string& purchaseData);
		void setPlayerId(std::size_t id);
		std::size_t getPlayerId() const;
		const std::string& getUsername() const;

		// === P1-4 语义化出站：Scene 不再构造 sf::Packet ===
		// 每个方法内部调用协议层 makeXxx + send，返回 send 结果；字节与线上一致。
		bool sendLogin(const std::string& username, const std::string& password);
		bool sendRegister(const std::string& username, const std::string& password);
		bool sendCheckUsername(const std::string& username);
		bool sendCheckin();
		// 房间语义出站（协议版本 3）：匹配已下线，组队改走房间。
		bool requestRoomList();
		bool createRoom(const std::string& roomName);
		bool joinRoom(std::uint64_t roomId);
		bool leaveRoom();
		bool sendRoomReady(bool ready);
		bool sendSkillInfoRequest(const std::vector<std::string>& names);
		// 商城语义出站：内部构造购买 JSON 并经网关发送（4 类请求）。
		bool sendShopRequest();
		bool sendBuyCharacter(const std::string& name);
		bool sendBuySkin(const std::string& charName, const std::string& skinName);
		bool sendBuyItem(const std::string& itemName);

	private:
		IClientGateway& gateway_;
		std::deque<Inbound> queue_{};
		bool wasConnected_{};
		bool kicked_{};
		std::string kickedReason_{};
	};

}
