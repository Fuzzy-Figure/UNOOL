#include "Card.h"
#include <SFML/Network.hpp>

// 阶段 2 收官：以下序列化段原位于 source/Card.cpp 尾部（542–591 行），逐字平移至此。
// Card.cpp 只保留领域逻辑；Protocol 库集中持有 Card / Hand 的线上序列化。
// 例外：operator>>(Hand&) 仍调用 Card::make()，其定义仍在由 exe 编译的 Card.cpp 中，
// 因此本库对 exe 对象文件保留反向符号引用（见方案文档第 19.3 / 21 节）。
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
