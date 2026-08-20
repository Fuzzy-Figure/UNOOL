#include "../header/GameState.h"

PlayerState& PlayerState::operator=(const PlayerState& other) {
	if (this != &other) {
		id = other.id;
		characterName = other.characterName;
		skin = other.skin;
		hp = other.hp;
		maxHp = other.maxHp;
		hand = other.hand.clone();
	}
	return *this;
}

sf::Packet& operator>>(sf::Packet& packet, PlayerState& state) {
	return packet >> state.id >> state.hand >> state.characterName >> state.skin
		>> state.hp >> state.maxHp;
}

sf::Packet& operator<<(sf::Packet& packet, const PlayerState& state) {
	return packet << state.id << state.hand << state.characterName << state.skin
		<< state.hp << state.maxHp;
}

sf::Packet& operator>>(sf::Packet& packet, GameState& state) {
	packet >> state.currentPlayerIndex;

	int colorInt;
	packet >> colorInt;
	state.currentColor = static_cast<Card::Color>(colorInt);

	int nameInt;
	packet >> nameInt;
	state.currentName = static_cast<Card::Name>(nameInt);

	packet >> state.direction;

	std::size_t playerCount;
	packet >> playerCount;
	state.players.resize(playerCount);
	for (std::size_t i = 0; i < playerCount; ++i) {
		packet >> state.players[i];
	}

	std::size_t discardCount;
	packet >> discardCount;
	state.discardPile.resize(discardCount);
	for (std::size_t i = 0; i < discardCount; ++i) {
		packet >> state.discardPile[i];
	}

	std::size_t seatOrderCount;
	packet >> seatOrderCount;
	state.seatOrder.resize(seatOrderCount);
	for (std::size_t i = 0; i < seatOrderCount; ++i) {
		packet >> state.seatOrder[i];
	}

	return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const GameState& state) {
	packet << state.currentPlayerIndex
		<< static_cast<int>(state.currentColor)
		<< static_cast<int>(state.currentName)
		<< state.direction;

	packet << state.players.size();
	for (const auto& player : state.players) {
		packet << player;
	}

	packet << state.discardPile.size();
	for (const auto& card : state.discardPile) {
		packet << card;
	}

	packet << state.seatOrder.size();
	for (const auto& seat : state.seatOrder) {
		packet << seat;
	}

	return packet;
}

sf::Packet& operator>>(sf::Packet& packet, CharInfo& info) {
	return packet >> info.playerIndex >> info.levelStr >> info.skills;
}

sf::Packet& operator<<(sf::Packet& packet, const CharInfo& info) {
	return packet << info.playerIndex << info.levelStr << info.skills;
}