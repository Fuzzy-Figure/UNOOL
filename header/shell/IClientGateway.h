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
	// 入站只有 receivePacket：包一旦取走就从网关移除，是否消费由上层
	// ConnectionState 的显式队列决定（P1-3）。原先的 peekMessageType 只服务于
	// 已删除的 PacketDispatcher 的「peek 后不消费」约定，随之一并移除。
	class IClientGateway : public unool::core::IClientSession {
	public:
		virtual bool send(sf::Packet& packet) = 0;
		virtual bool sendShopPurchase(const std::string& purchaseData) = 0;
		virtual void setPlayerId(std::size_t id) = 0;

		virtual std::optional<sf::Packet> receivePacket() = 0;
	};

}
