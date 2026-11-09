#pragma once
#include <cstddef>
#include <string>
#include "application/IClientSession.h"

namespace sf { class Packet; }

namespace unool::adapters {

	// 阶段 4 · S3a（用户 49.10 口径②A）：客户端出站网关端口。
	// 放在 adapters 层、允许出现 sf::Packet，但只用前向声明，不引入完整 SFML 头；
	// 继承 unool::application::IClientSession 以复用其纯标准库的会话状态 6 方法，
	// IClientSession 本身保持不动（S1 口径：纯标准库、不含 SFML / nlohmann）。
	//
	// 只补齐两个 Scene 实际需要的出站方法：49.2 盘点的 15 处 net.* 调用中，
	// update / isConnected 已由 IClientSession 提供，此处新增 send / sendShopPurchase /
	// setPlayerId 三项即可覆盖全部 5 类。receivePacket / peekMessageType 仍属
	// PacketDispatcher 绑定的具体类型边界，按 49.10「障碍 3 顺延到 S3c 前再判」暂不纳入。
	//
	// S3a 只定义接口并由 ClientNetwork 实现（边界先行）：两个 Scene 仍持 ClientNetwork&，
	// 内部业务判定零改动；S3b / S3c 再把各自的成员逐个切到本端口。
	class IClientGateway : public unool::application::IClientSession {
	public:
		virtual bool send(sf::Packet& packet) = 0;
		virtual bool sendShopPurchase(const std::string& purchaseData) = 0;
		virtual void setPlayerId(std::size_t id) = 0;
	};

}
