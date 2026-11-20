#pragma once
#include <cstddef>
#include <deque>
#include <initializer_list>
#include <optional>
#include <string>
#include <SFML/Network.hpp>
#include "MessageType.h"

namespace unool::shell {

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

		// 出站转发：保持各调用点发包路径不变（P1-3 不动出站语义）。
		bool send(sf::Packet& packet);
		bool sendShopPurchase(const std::string& purchaseData);
		void setPlayerId(std::size_t id);
		std::size_t getPlayerId() const;
		const std::string& getUsername() const;

	private:
		IClientGateway& gateway_;
		std::deque<Inbound> queue_{};
		bool wasConnected_{};
	};

}
