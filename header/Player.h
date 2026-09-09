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
	bool hasUsed = false;
	sf::Keyboard::Scancode currentInput = sf::Keyboard::Scancode::Unknown;

	void setInput(sf::Keyboard::Scancode input) { currentInput = input; }
	sf::Keyboard::Scancode getInput() const { return currentInput; }
	opt_ref<Card> chooseToUse(ASkill::TriggerTime phase = ASkill::TriggerTime::never);
	//收集当前阶段可发动的即时技与转换技
	void collectAvailableSkills(ASkill::TriggerTime phase,
								std::vector<ref<ASkillInstantBase>>& instantRefs,
								std::vector<ref<ASkillTransformBase>>& transformRefs);
	//处理数字键1-9：即时技发动 / 转换技切换；返回true表示已处理（continue）
	bool handleDigitKey(sf::Keyboard::Scancode input,
						const std::vector<ref<ASkillInstantBase>>& instantRefs,
						const std::vector<ref<ASkillTransformBase>>& transformRefs,
						ASkillTransformBase*& activeMode);
	//处理确认选择（Up/W）：返回索引表示出牌成功，nullopt表示继续循环
	std::optional<std::size_t> handleConfirm(const std::function<bool(const Card&)>& condition,
		ASkillTransformBase* activeMode);
	//数字键扫描码转 0-9，非数字键返回 nullopt
	static std::optional<std::size_t> digitFromScancode(sf::Keyboard::Scancode input);

public:
	std::optional<std::size_t> chooseCard(std::function<bool(const Card&)> condition,
										  bool forced, ASkill::TriggerTime phase = ASkill::TriggerTime::never);
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
	std::string getSkillsText() const { return character->getSkillsText(); }
	Character::Level characterLevel() const { return character->getLevel(); }
	std::size_t getHp() const { return character->getHp(); }
	std::size_t getMaxHp() const { return character->getMaxHp(); }
	void damage(std::size_t damage, opt_ref<Player> source);
	void recover(std::size_t num);
	bool isDead() const { return character->isDead(); }
	void resetSkills() { character->resetSkills(); }
	std::size_t getDamageMultiplier() const { return character->getDamageMultiplier(); }
	void setDamageMultiplier(std::size_t m) { character->setDamageMultiplier(m); }
	bool hasMark(const std::string& m) const { return character->hasMark(m); }
	void addMark(const std::string& m) { character->addMark(m); }
	void removeMark(const std::string& m) { character->removeMark(m); }
	const std::unordered_set<std::string>& getMarks() const { return character->getMarks(); }
	void clearMarks() { character->clearMarks(); }
	void addSkill(std::unique_ptr<ASkillInstantBase>   skill) { character->addSkill(std::move(skill)); }
	void addSkill(std::unique_ptr<ASkillTransformBase> skill) { character->addSkill(std::move(skill)); }
	void addSkill(std::unique_ptr<PSkill>              pSkill) { character->addSkill(std::move(pSkill)); }
	void removeSkill(const std::string& name) { character->removeSkill(name); }
	void setCharacter(std::unique_ptr<Character> c) { character = std::move(c); }
#pragma endregion

#pragma region 手牌查询 - 委托到 Hand
	std::size_t handCount() const { return hand->count(); }
	bool handEmpty() const { return hand->empty(); }
	bool getHasUsed() const { return hasUsed; }
	std::size_t handSelectedIndex() const { return hand->getSelectedIndex(); }
	const Card& handSelectedCard() const { return hand->getSelectedCard(); }
	std::size_t handValue() const { return hand->value(); }
	bool handSatisfy(const std::function<bool(const Cards&)>& condition) const { return hand->satisfy(condition); }
	bool handInclude(const std::function<bool(const Card&)>& condition) const { return hand->include(condition); }
	bool handExclude(const std::function<bool(const Card&)>& condition) const { return hand->exclude(condition); }
	bool hasPSkill(const std::string& name) const { return character->hasPSkill(name); }
	opt_ref<PSkill> findPSkill(const std::string& name) { return character->findPSkill(name); }
	std::list<std::unique_ptr<ASkillInstantBase>>& getInstantSkills() { return character->getInstantSkills(); }
	std::list<std::unique_ptr<ASkillTransformBase>>& getTransformSkills() { return character->getTransformSkills(); }
#pragma endregion

#pragma region 手牌操作 - 委托到 Hand
	const Hand& getHand() const { return *hand; }
	Hand& getHand() { return *hand; }
	void clearHand() { hand->clear(); hand->resetSelectedIndex(); }
	void handSelectLeft() { hand->selectLeft(); }
	void handSelectRight() { hand->selectRight(); }
	void handSelectLast() { hand->selectLast(); }
	void sortHand() { hand->sort(); }

	void gainCard(std::unique_ptr<Card> card);
	Card& getCardByIndex(const std::size_t index) { return hand->getCardByIndex(index); }
	void printHand() const { hand->print(); }
#pragma endregion

#pragma region 游戏逻辑
	std::vector<ref<Card>> draw(std::size_t num, const DrawReason reason = DrawReason::unknown);
	std::vector<ref<Card>> drawTo(const std::size_t num, const DrawReason reason = DrawReason::unknown);

	Card& useCardByIndex(const std::size_t cardIndex);
	Card& discardByIndex(const std::size_t cardIndex);
	[[nodiscard]] std::unique_ptr<Card> takeCardByIndex(const std::size_t cardIndex);
	bool canUse(const Card& card);
	void give(Player& other, std::unique_ptr<Card> card) { other.gainCard(std::move(card)); }

#pragma endregion

#pragma region 技能 / 状态
	void launchPSkills(const PSkill::TriggerTime& currentTriggerTime, PSkill::Trigger& trigger);
	void ban(Player& source, Card& card);
	void ban() { banned = true; }
	void unban() { banned = false; }

#pragma endregion

#pragma region 导航
	Player& next() const;
	Player& prev() const;

#pragma endregion

#pragma region 交互
	std::vector<ref<Card>> chooseToDiscard(const std::wstring& title,
										   std::size_t num, const bool forced,
										   const std::function<bool(const Card&)>& condition
										   = unool::alwaysTrue);
	struct RecastResult {
		std::vector<ref<Card>> discarded;
		std::vector<ref<Card>> drawn;
	};
	RecastResult chooseToRecast(const std::wstring& title,
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
	opt_ref<Card> chooseToGive(const std::wstring& title, Player& target,
							   bool forced, const std::function<bool(const Card&)>& condition
							   = unool::alwaysTrue);
	opt_ref<Card> chooseToShow(const std::wstring& title, bool forced,
							   const std::function<bool(const Card&)>& condition);

	[[nodiscard]] opt_ref<Player> choosePlayer(const std::wstring& title, bool forced,
											   const std::function<bool(const Player&)>& condition
											   = unool::alwaysTrue);
	[[nodiscard]] opt_ref<Player> chooseOtherPlayer(const std::wstring& title, bool forced,
													const std::function<bool(const Player&)>& condition
													= unool::alwaysTrue);
	[[nodiscard]] std::optional<Card::Color> chooseCardColor(const std::wstring& title, bool forced,
															 const std::vector<Card::Color>& colors
															 = { Card::Color::blue, Card::Color::green, Card::Color::red, Card::Color::yellow });
	[[nodiscard]] std::optional<Card::Name> chooseCardName(const std::wstring& title, bool forced,
														   const std::vector<Card::Name>& names);
	[[nodiscard]] std::size_t ask(const std::wstring& title, const std::vector<std::wstring>& options,
								  bool forced, std::optional<std::chrono::milliseconds> timeoutMs = std::nullopt);
	void hint(const std::wstring& message);
	[[nodiscard]] Card& judge();
	void showCard(const Card& card);

	//拼点结果
	enum class CompareResult { win, lose, draw };
	//拼点：双方秘密选一张数字牌比点数；发起者无数字牌返回nullopt（不能发动），目标无数字牌判其输
	[[nodiscard]] std::optional<CompareResult> comparePoint(Player& target, bool forced);

#pragma endregion

#pragma region 回合流程
	void phaseBegin();
	bool phaseUse1();
	void phaseDraw();
	void phaseUse2();
	void phaseEnd();
	bool turn();
#pragma endregion

};