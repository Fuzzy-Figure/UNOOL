#pragma once
#include <SFML/Network.hpp>
#include <vector>
#include "Card.h"
#include "Player.h"
#include "GameState.h"
#include "Socket.h"
#include "utils.h"

class GameLogic {
private:
	enum class Direction {
		increase, decrease
	};

	std::unique_ptr<Pile> pile;
	std::unique_ptr<Pile> discardPile;
	std::vector<std::unique_ptr<Player>> players;
	std::size_t currentPlayerIndex = 0;
	std::size_t firstPlayerIndex = 0;
	std::vector<std::size_t> seatOrder; // seatOrder[playerId] = 座位号（0=一号位，1=二号位）
	Card::Color currentColor = Card::Color::no;
	Card::Name currentName = Card::Name::no;
	Direction direction = Direction::increase;
	ServerNetwork& network;

	void altPlayer();

	bool currentPlayerTurn();

	using CharacterEntry = std::pair<std::string, Character::Info>;
	struct SelectionState {
		std::vector<CharacterEntry> cands[2];
		std::optional<std::size_t> bannedIdx[2];
	};

	static std::wstring formatCharacterLabelW(const CharacterEntry& entry);
	static std::vector<CharacterEntry> randomChooseCharacters(std::size_t n);
	static void chooseSkinAndSet(Player& player, const std::string& charName);
	std::size_t getSeatPlayerId(std::size_t seat) const;
	void banPhase(std::size_t bannerId, std::size_t targetId, SelectionState& state);
	void selectCharacter(std::size_t playerId, const SelectionState& state);

public:
	GameLogic(ServerNetwork& _network);
	~GameLogic();

	void initPlayers();
	void initPlayers(const std::vector<std::string>& chars);
	void determineSeatOrder();

	bool runTurn();
	void broadcastState();
	ServerNetwork& getNetwork();
	Player& currentPlayer() const;
	GameState packStateForPlayer(std::size_t playerId) const;
	std::size_t getCurrentPlayerId() const;
	bool playersSatisfy(const std::function<bool(std::vector<std::unique_ptr<Player>>&)>& condition);
	const std::vector<ref<Player>> getPlayers() const;
	const std::vector<ref<Player>> getPlayersIf(const std::function<bool(const Player&)>& condition) const;
	const std::vector<ref<Player>> getPlayersExcludeId(const std::size_t id) const;
	void forEachPlayer(const std::function<void(Player&)>& operation);
	void forEachPlayerIf(const std::function<bool(const Player&)>& condition,
						 const std::function<void(Player&)>& operation);
	Card& judge();

	void print() const;

	Pile& getPile();
	Pile& getDiscardPile();
	Player& getPlayerById(const std::size_t id);
	void setCurrentColor(const Card::Color newColor);
	Card::Color getCurrentColor() const;
	void setCurrentName(const Card::Name newName);
	Card::Name getCurrentName() const;
	std::size_t nextPlayerIndex(const std::size_t curIndex) const;
	std::size_t prevPlayerIndex(const std::size_t curIndex) const;
	void reverse();
	void launchPSkills(const PSkill::TriggerTime& triggerTime,
					   opt_ref<Player> player = std::nullopt,
					   opt_ref<Card> card = std::nullopt,
					   opt_ref<Player> source = std::nullopt,
					   opt_ref<std::size_t> number = std::nullopt);
	Card& putCardToDiscardPile(std::unique_ptr<Card> card);
	std::optional<Card> lastCard() const;

	void checkRoundEnd();
	void resetRound();
	bool isGameOver() const;
	std::optional<std::size_t> getWinnerId() const;
};
