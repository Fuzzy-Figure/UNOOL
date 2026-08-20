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

#pragma region 核心数据成员
	std::unique_ptr<Pile> pile;
	std::unique_ptr<Pile> discardPile;
	std::vector<std::unique_ptr<Player>> players;
	std::size_t currentPlayerIndex = 0;
	std::size_t firstPlayerIndex = 0;
	std::vector<std::size_t> seatOrder;
	Card::Color currentColor = Card::Color::no;
	Card::Name currentName = Card::Name::no;
	Direction direction = Direction::increase;
	ServerNetwork& network;
	std::array<bool, 2> charInfoDirty{ true, true };
	std::size_t matchCount = 0;
#pragma endregion

#pragma region 私有辅助方法
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
#pragma endregion

public:
#pragma region 构造与析构
	GameLogic(ServerNetwork& _network);
	~GameLogic();
#pragma endregion

#pragma region 初始化
	void initPlayers();
	void initPlayers(const std::vector<std::string>& chars);
	void determineSeatOrder();
#pragma endregion

#pragma region 回合执行
	bool runTurn();
	void broadcastState();
	void flushCharInfo();
	void markCharInfoDirty(std::size_t playerId);
	std::size_t getMatchCount() const { return matchCount; }
	void nextMatch() { ++matchCount; }
	ServerNetwork& getNetwork();
	Player& currentPlayer() const;
	GameState packStateForPlayer(std::size_t playerId) const;
	std::size_t getCurrentPlayerId() const;
#pragma endregion

#pragma region 基于谓词的查询
	bool playersSatisfy(const std::function<bool(std::vector<std::unique_ptr<Player>>&)>& condition);
	bool playersInclude(const std::function<bool(const Player&)>& condition);
	const std::vector<ref<Player>> getPlayers() const;
	const std::vector<ref<Player>> getPlayersIf(const std::function<bool(const Player&)>& condition) const;
	const std::vector<ref<Player>> getPlayersExcludeId(const std::size_t id) const;
#pragma endregion

#pragma region 遍历迭代
	void forEachPlayer(const std::function<void(Player&)>& operation);
	void forEachOtherPlayer(const Player& self,
							const std::function<void(Player&)>& operation);
	void forEachPlayerIf(const std::function<bool(const Player&)>& condition,
						 const std::function<void(Player&)>& operation);
	void forEachOtherPlayerIf(const Player& self,
							  const std::function<bool(const Player&)>& condition,
							  const std::function<void(Player&)>& operation);
#pragma endregion

#pragma region 调试输出
	void print() const;
#pragma endregion

#pragma region 成员查询与修改
	Pile& getPile();
	Pile& getDiscardPile();
	Player& getPlayerById(const std::size_t id);
	Card::Color getCurrentColor() const;
	void setCurrentColor(const Card::Color newColor);
	Card::Name getCurrentName() const;
	void setCurrentName(const Card::Name newName);
#pragma endregion

#pragma region 回合顺序与方向
	std::size_t nextPlayerIndex(const std::size_t curIndex) const;
	std::size_t prevPlayerIndex(const std::size_t curIndex) const;
	void reverse();
#pragma endregion

#pragma region 技能系统
	void launchPSkills(const PSkill::TriggerTime& currentTriggerTime,
					   opt_ref<Player> player,
					   Card& card,
					   opt_ref<Player> source = std::nullopt,
					   opt_ref<std::size_t> number = std::nullopt);
	void launchPSkills(const PSkill::TriggerTime& triggerTime,
					   opt_ref<Player> player = std::nullopt,
					   std::optional<std::vector<ref<Card>>> cards = std::nullopt,
					   opt_ref<Player> source = std::nullopt,
					   opt_ref<std::size_t> number = std::nullopt);
#pragma endregion

#pragma region 弃牌堆管理
	Card& putCardToDiscardPile(std::unique_ptr<Card> card);
	std::optional<Card> lastCard() const;
#pragma endregion

#pragma region 轮次与游戏状态
	void checkRoundEnd();
	void resetRound();
	bool isGameOver() const;
	std::optional<std::size_t> getWinnerId() const;
#pragma endregion
};
