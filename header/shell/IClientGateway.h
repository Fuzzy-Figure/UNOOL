#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <SFML/Network.hpp>
#include "core/IClientSession.h"

namespace unool::shell {

	// 阶段 4 · S3a（用户 49.10 口径②A）：客户端出站网关端口。
	// 放在 adapters 层、允许出现 sf::Packet；继承 unool::core::IClientSession
	// 以复用其纯标准库的会话状态 6 方法，IClientSession 本身保持不动（S1 口径：
	// 纯标准库、不含 SFML / nlohmann）。
	//
	// 阶段 4 · S3b1（用户 51.11 口径②A）：新增两个入站纯虚 receivePacket /
	// peekMessageType，使 PacketDispatcher 的 next() / replyHeartbeat() 可以从具体
	// ClientNetwork& 泛化到本端口。std::optional<sf::Packet> 要求 sf::Packet 为完整
	// 类型，因此本头显式包含 <SFML/Network.hpp>（②A 原文允许端口「可含 sf::Packet」，
	// S3a 的仅前向声明轻量化在此让位于完整类型要求）。
	//
	// 端口现在覆盖两个 Scene 的 8 处 net.* 调用与 PacketDispatcher 的收包动作；
	// 三处 PacketDispatcher::next(ClientNetwork&) 调用点因派生→基类隐式转换零改动。
	// 协议构造 / 响应解析仍留在 LoginScene.cpp，按 51.5 ①A 归 S3b2 再上移。
	class IClientGateway : public unool::core::IClientSession {
	public:
		virtual bool send(sf::Packet& packet) = 0;
		virtual bool sendShopPurchase(const std::string& purchaseData) = 0;
		virtual void setPlayerId(std::size_t id) = 0;

		virtual std::optional<sf::Packet> receivePacket() = 0;
		virtual std::optional<int> peekMessageType() const = 0;
	};

}
