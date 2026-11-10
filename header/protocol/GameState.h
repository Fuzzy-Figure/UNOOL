#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include "Card.h"
// B+：只取角色的标量类型，不引入 Character.h 及其技能类闭包。
#include "contract/CharacterTypes.h"
#include <cstddef>
#include <optional>

// 与 header/domain/Card.h 一致：friend 声明只需 sf::Packet 的类型声明，
// 不引入 SFML 头，使本头可由纯标准库 PCH 的编译单元包含。
namespace sf { class Packet; }

struct PlayerState {
	std::size_t id = -1;
	Hand hand;
	std::vector<std::string> characterNames;
	std::vector<std::string> skins;
	unool::contract::hp_t hp = 0;
	unool::contract::hp_t maxHp = 0;
	std::size_t shield = 0;  //护盾值（显示为“体力：(+护盾)/最大体力”）
	std::unordered_map<std::string, std::size_t> marks;
	//本场内的局胜负计数
	std::size_t wins = 0;
	std::size_t losses = 0;

	PlayerState() = default;
	PlayerState(const PlayerState& other)
		: id(other.id), characterNames(other.characterNames), skins(other.skins),
		  hp(other.hp), maxHp(other.maxHp), shield(other.shield), marks(other.marks),
		  wins(other.wins), losses(other.losses) {
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
	std::size_t gameCount = 0; //本场内当前是第几局（摸初始手牌时+1）

	friend sf::Packet& operator>>(sf::Packet& packet, GameState& state);
	friend sf::Packet& operator<<(sf::Packet& packet, const GameState& state);
};

struct CharInfo {
	std::size_t playerIndex = 0;
	std::string fullText;

	friend sf::Packet& operator>>(sf::Packet& packet, CharInfo& info);
	friend sf::Packet& operator<<(sf::Packet& packet, const CharInfo& info);
};
