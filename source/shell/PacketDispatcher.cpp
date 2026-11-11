#include "PacketDispatcher.h"

#include "IClientGateway.h"
#include <SFML/Network.hpp>
#include <utility>

// 阶段 4 · S2：三处收包循环原本各写一份的收包动作，收拢到这里。
// 心跳回复统一在本单元完成（用户 46.5 口径②A），`MessageType` 值、包体与回复字节逐字不变。
// S3b1（51.11 口径②A）：参数由 ClientNetwork& 泛化为 IClientGateway&，函数体逐字未改。

namespace unool::shell {

	void PacketDispatcher::replyHeartbeat(IClientGateway& net) {
		sf::Packet hb;
		hb << static_cast<int>(MessageType::Heartbeat);
		net.send(hb);
	}

	std::optional<InboundPacket> PacketDispatcher::next(IClientGateway& net) {
		// 显式 for 循环 + 末尾 return，避免 Debug|x64 的 /we4715 把「控制路径未全部返回值」判为错误
		for (auto packetOpt = net.receivePacket(); packetOpt; packetOpt = net.receivePacket()) {
			sf::Packet packet = std::move(*packetOpt);
			int msgType{};
			if (!(packet >> msgType)) continue;  // 坏包已消费丢弃，与原循环一致
			auto mt = static_cast<MessageType>(msgType);
			if (mt == MessageType::Heartbeat) {
				replyHeartbeat(net);
				continue;
			}
			return InboundPacket{ mt, std::move(packet) };
		}
		return std::nullopt;
	}

	std::optional<InboundPacket> PacketDispatcher::next(IClientGateway& net,
														std::initializer_list<MessageType> subscription) {
		while (auto peek = net.peekMessageType()) {
			// 队列空或队首包读不出消息号：原 LoginScene 直接 break，坏包留在队列，这里保持一致
			auto mt = static_cast<MessageType>(*peek);

			if (mt == MessageType::Heartbeat) {
				net.receivePacket();             // 消费心跳包
				replyHeartbeat(net);
				continue;
			}

			// 非账号包：不消费、留在队列里给 gamePhase，逐字保留跨阶段留包约定
			bool accepted{};
			for (auto type : subscription) {
				if (type == mt) { accepted = true; break; }
			}
			if (!accepted) break;

			auto packetOpt = net.receivePacket();
			if (!packetOpt) break;
			sf::Packet packet = std::move(*packetOpt);
			int msgType{};
			if (!(packet >> msgType)) continue;
			return InboundPacket{ mt, std::move(packet) };
		}
		return std::nullopt;
	}

}
