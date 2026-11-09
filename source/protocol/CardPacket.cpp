#include "Card.h"
#include <SFML/Network.hpp>

// 阶段 2 收官：以下序列化段原位于 Card.cpp 尾部（现 source/domain/Card.cpp，542–591 行），逐字平移至此。
// Card.cpp 只保留领域逻辑；UNOOL.Core 静态库集中持有 Card / Hand 的线上序列化。
// operator>>(Hand&) 仍调用 Card::make()，其定义位于同库的 source/domain/Card.cpp；
// 阶段 3 第一批把共享编译单元集中到 Core 后，本库已不再反向引用 exe 的对象文件符号
// （历史说明见方案文档第 19.3 / 21 节，现状见第 23 节）。
// 函数体与线上字节均未改动。

// ==================== Packet 序列化 ====================
sf::Packet& operator<<(sf::Packet& packet, const Card& card) {
	packet << static_cast<int>(card.getColor())
		<< static_cast<int>(card.getName())
		<< static_cast<int>(card.getDiscardReason());
	return packet;
}
sf::Packet& operator>>(sf::Packet& packet, Card& card) {
	int colorInt, nameInt, reasonInt;
	if (packet >> colorInt >> nameInt >> reasonInt) {
		card.setColor(static_cast<Card::Color>(colorInt));
		card.setName(static_cast<Card::Name>(nameInt));
		card.setDiscardReason(static_cast<Card::DiscardReason>(reasonInt));
	}
	return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const Hand& hand) {
	std::size_t validIndex = hand.selectedIndex;
	if (hand.count() > 0 && validIndex >= hand.count()) {
		validIndex = hand.count() - 1;
	}
	else if (hand.count() == 0) {
		validIndex = 0;
	}
	packet << hand.cards.size();
	for (const auto& cardPtr : hand.cards) {
		packet << *cardPtr;
	}
	packet << validIndex;
	return packet;
}
sf::Packet& operator>>(sf::Packet& packet, Hand& hand) {
	hand.cards.clear();
	std::size_t size;
	packet >> size;
	for (std::size_t i = 0; i < size; ++i) {
		Card c;
		packet >> c;
		hand.cards.push_back(Card::make(c));
	}
	packet >> hand.selectedIndex;
	if (hand.count() == 0) {
		hand.selectedIndex = 0;
	}
	else if (hand.selectedIndex >= hand.count()) {
		hand.selectedIndex = hand.count() - 1;
	}
	return packet;
}
