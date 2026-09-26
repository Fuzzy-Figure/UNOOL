#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "Card.h"
#include "Character.h"

struct PlayerState {
	std::size_t id = -1;
	Hand hand;
	std::vector<std::string> characterNames;
	std::vector<std::string> skins;
	Character::hp_t hp = 0;
	Character::hp_t maxHp = 0;
	std::unordered_map<std::string, std::size_t> marks;

	PlayerState() = default;
	PlayerState(const PlayerState& other)
		: id(other.id), characterNames(other.characterNames), skins(other.skins),
		  hp(other.hp), maxHp(other.maxHp), marks(other.marks) {
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
	std::optional<std::size_t> operatingPlayerId; //当前正在选牌/操作的玩家，nullopt=无人操作

	friend sf::Packet& operator>>(sf::Packet& packet, GameState& state);
	friend sf::Packet& operator<<(sf::Packet& packet, const GameState& state);
};

struct CharInfo {
	std::size_t playerIndex = 0;
	std::string fullText;

	friend sf::Packet& operator>>(sf::Packet& packet, CharInfo& info);
	friend sf::Packet& operator<<(sf::Packet& packet, const CharInfo& info);
};
