#pragma once
#include <initializer_list>
#include <optional>
#include <SFML/Network.hpp>
#include "MessageType.h"

class ClientNetwork;

namespace unool::adapters {

	// 阶段 4 · S2：把三处收包循环里重复的「peek 过滤 / 心跳回复 / 包体消息号消费」
	// 收拢到单点。业务判定仍留在各阶段，S3 才会把两个 Scene 纯 View 化并改为订阅回调。
	// 两种重载逐字对应原有语义：
	//   next(net)             → DrainAll：消费队列里所有包，坏包丢弃，心跳内部回复；
	//                           对应 ShopScene::handleNetwork 与 gamePhase 的收包循环。
	//   next(net, { 订阅集 }) → Filtered：先 peek，非订阅包「不消费、留在队列」；
	//                           对应 LoginScene::pollAccountPackets 的跨阶段留包约定。
	// 返回的 packet 已消费掉开头的 int 消息号，与三处原循环在 `>> msgType` 之后的状态一致。
	struct InboundPacket {
		MessageType type{};
		sf::Packet packet{};
	};

	class PacketDispatcher {
	public:
		static std::optional<InboundPacket> next(ClientNetwork& net);
		static std::optional<InboundPacket> next(ClientNetwork& net,
		                                        std::initializer_list<MessageType> subscription);

	private:
		static void replyHeartbeat(ClientNetwork& net);
	};

}
