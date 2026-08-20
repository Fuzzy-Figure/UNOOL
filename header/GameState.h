#pragma once
#include <vector>
#include "Card.h"

struct PlayerState {
	std::size_t id = -1;
	Hand hand;
	std::string characterName;
	std::string skin;
	std::size_t hp = 0;
	std::size_t maxHp = 0;

	PlayerState() = default;
	PlayerState(const PlayerState& other)
		: id(other.id), characterName(other.characterName), skin(other.skin) {
		for (std::size_t i = 0; i < other.hand.count(); ++i) {
			hand.push_back(Card::make(other.hand[i]));
		}
		hand.setSelectedIndex(other.hand.getSelectedIndex());
	}
	PlayerState& operator=(const PlayerState& other);
	PlayerState(PlayerState&&) = default;
	PlayerState& operator=(PlayerState&&) = default;

	friend sf::Packet& operator>>(sf::Packet& packet, PlayerState& state);
	friend sf::Packet& operator<<(sf::Packet& packet, const PlayerState& state);
};

struct GameState {
	std::size_t currentPlayerIndex = 0;
	Card::Color currentColor = Card::Color::no;
	Card::Name currentName = Card::Name::no;
	int direction = 0;
	std::vector<PlayerState> players;
	std::vector<Card> discardPile;
	std::vector<std::size_t> seatOrder;

	friend sf::Packet& operator>>(sf::Packet& packet, GameState& state);
	friend sf::Packet& operator<<(sf::Packet& packet, const GameState& state);
};

struct CharInfo {
	std::size_t playerIndex = 0;
	std::string levelStr;
	std::string skills;

	friend sf::Packet& operator>>(sf::Packet& packet, CharInfo& info);
	friend sf::Packet& operator<<(sf::Packet& packet, const CharInfo& info);
};
