#pragma once
#include <iostream>
#include <vector>
#include <optional>
#include <string>
#include <functional>
#include "Character.h"
#include "Card.h"
#include "utils.h"

class GameLogic;
class GameRenderer;

class Player {
public:
	enum class DrawReason {
		unknown,
		phase_draw,
		skill
	};
private:
	std::size_t id = 0;
	std::unique_ptr<Hand> hand = std::make_unique<Hand>();
	std::unique_ptr<Character> character = nullptr;
	GameLogic& game;
	bool banned = false;
	sf::Keyboard::Scancode currentInput = sf::Keyboard::Scancode::Unknown;

	void setInput(sf::Keyboard::Scancode input) { currentInput = input; }
	sf::Keyboard::Scancode getInput() const { return currentInput; }
	opt_ref<Card> chooseToUse();

public:
	std::optional<std::size_t> chooseCard(std::function<bool(const Card&)> condition,
										  bool forced);
#pragma region 玩家属性
	Player(const std::size_t _id, GameLogic& _game, std::unique_ptr<Character> _character)
		:id(_id), game(_game), character(std::move(_character)) {}
	std::size_t getId() const { return id; }
	bool operator==(const Player& other) const { return id == other.id; }
#pragma endregion

#pragma region 角色属性 - 委托到 Character
	std::string characterName() const { return character->getName(); }
	std::wstring characterNameW() const { return character->getNameW(); }
	std::string skin() const { return character->getSkin(); }
	std::string skillsName() const { return character->skillsName(); }
	Character::Level characterLevel() const { return character->getLevel(); }
	std::size_t getHp() const { return character->getHp(); }
	std::size_t getMaxHp() const { return character->getMaxHp(); }
	void takeDamage(std::size_t damage, opt_ref<Player> source);
	void recover(std::size_t num);
	bool isDead() const { return character->isDead(); }
	void resetSkills() { character->resetSkills(); }
	void setCharacter(std::unique_ptr<Character> c) { character = std::move(c); }
#pragma endregion

#pragma region 手牌查询 - 委托到 Hand
	std::size_t handCount() const { return hand->count(); }
	bool handEmpty() const { return hand->empty(); }
	std::size_t handSelectedIndex() const { return hand->getSelectedIndex(); }
	const Card& handSelectedCard() const { return hand->getSelectedCard(); }
	std::size_t handValue() const { return hand->value(); }
	bool handSatisfy(const std::function<bool(const Cards&)>& condition) const { return hand->satisfy(condition); }
	bool handInclude(const std::function<bool(const Card&)>& condition) const { return hand->include(condition); }
	bool handExclude(const std::function<bool(const Card&)>& condition) const { return hand->exclude(condition); }
	bool hasPSkill(const std::string& name) const { return character->hasPSkill(name); }
#pragma endregion

#pragma region 手牌操作 - 委托到 Hand
	const Hand& getHand() const { return *hand; }
	Hand& getHand() { return *hand; }
	void clearHand() { hand->clear(); hand->resetSelectedIndex(); }
	void handSelectLeft() { hand->selectLeft(); }
	void handSelectRight() { hand->selectRight(); }
	void sortHand() { hand->sort(); }

	void gainCard(std::unique_ptr<Card> card) { hand->push_back(std::move(card)); }
	Card& getCardByIndex(const std::size_t index) { return hand->getCardByIndex(index); }
	void printHand() const { hand->print(); }
	void displayHand(GameRenderer& renderer,
					 const sf::Vector2f& startPos,
					 const sf::Vector2f& cardSize,
					 const sf::Vector2f& pointerSize = { 0,0 }) const {
		hand->display(renderer, startPos, cardSize, pointerSize);
	}
#pragma endregion


	// === 游戏逻辑 ===
	void draw(std::size_t num, const DrawReason reason = DrawReason::unknown);
	void drawTo(const std::size_t num, const DrawReason reason = DrawReason::unknown);

	Card& useCardByIndex(const std::size_t cardIndex);
	void discardByIndex(const std::size_t cardIndex);
	[[nodiscard]] std::unique_ptr<Card> takeCardByIndex(const std::size_t cardIndex);
	bool canUse(const Card& card);
	void give(Player& other, std::unique_ptr<Card> card) { other.gainCard(std::move(card)); }

	// === 技能 / 状态 ===
	void launchPSkills(const PSkill::TriggerTime& currentTriggerTime, PSkill::Trigger& trigger) { character->launchPSkills(currentTriggerTime, trigger); }
	void ban(Player& source, Card& card);
	void unban() { banned = false; }

	// === 导航 ===
	Player& next() const;
	Player& prev() const;

	// === 交互（网络 / 选择）===
	std::vector<ref<Card>> chooseToDiscard(const std::wstring& title,
										   const std::size_t num, const bool forced,
										   const std::function<bool(const Card&)>& condition
										   = unool::alwaysTrue);
	void chooseToRecast(const std::wstring& title,
						const std::size_t num, const bool forced,
						const std::function<bool(const Card&)>& condition
						= unool::alwaysTrue);
	void decree(const std::wstring& title,
				const std::size_t num, const bool forced,
				const std::function<bool(const Card&)>& condition
				= unool::alwaysTrue);
	void inherit(std::unique_ptr<Card>& card);

	opt_ref<Card> chooseToOperate(const std::wstring& title, bool forced,
								  const std::function<bool(const Card&)>& condition,
								  const std::function<void(Card&)>& operation);
	opt_ref<Card> chooseToGive(Player& target, bool forced,
							   const std::function<bool(const Card&)>& condition
							   = unool::alwaysTrue);


	opt_ref<Player> choosePlayer(const std::wstring& title, bool forced,
								 const std::function<bool(const Player&)>& condition
								 = unool::alwaysTrue);
	opt_ref<Player> chooseOtherPlayer(const std::wstring& title, bool forced,
									  const std::function<bool(const Player&)>& condition
									  = unool::alwaysTrue);
	std::size_t ask(const std::wstring& title, const std::vector<std::wstring>& options,
					bool forced, std::optional<std::chrono::milliseconds> timeoutMs = std::nullopt);
	void hint(const std::wstring& message);
	Card& judge();
	void showCard(const Card& card);

	// === 回合流程 ===
	void phaseBegin();
	bool phaseUse1();
	void phaseDraw();
	void phaseUse2();
	void phaseEnd();
	bool turn();
};