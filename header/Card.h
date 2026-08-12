#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <deque>
#include <algorithm>
#include <random>
#include <map>
#include "Effect.h"

class GameRenderer;
class GameLogic;


class Card {
public:
	enum class Color {
		no, blue, green, red, yellow, black
	};
	enum class Name {
		no,
		number_0, number_1, number_2, number_3, number_4,
		number_5, number_6, number_7, number_8, number_9,
		action_ban, action_rev, action_draw2, wild_pal, wild_draw4,
		back
	};
	struct TupleHash {
		std::size_t operator()(const std::tuple<Card::Color, Card::Name>& t) const {
			return std::hash<int>{}(static_cast<int>(std::get<0>(t)) * 100 + static_cast<int>(std::get<1>(t)));
		}
	};
private:
	Color color = Color::no;
	Name name = Name::no;
	bool effective = true;
public:
	bool operator<(const Card& other) const;
	static std::string to_string(const Color& color);
	static std::wstring to_wstring(const Color& color);
	static std::string to_string(const Name& name);
	static std::wstring to_wstring(const Name& name);

	Card(const Color color = Color::no, const Name name = Name::no);
	Color getColor() const;
	void setColor(const Color newColor);
	Name getName() const;
	void setName(const Name newName);
	void set(const Card& other);
	void display(GameRenderer& renderer, const sf::Vector2f& pos, const sf::Vector2f& cardSize) const;
	void displayInCenter(GameRenderer& renderer, const sf::Vector2f& cardSize) const;

	template<typename... Colors> requires (std::same_as<Colors, Color> && ...)
		bool is(const Colors... colors) const {
		return ((color == colors) || ...);
	}

	template<typename... Names> requires (std::same_as<Names, Name> && ...)
		bool is(const Names... names) const {
		return ((name == names) || ...);
	}
	bool isNumber() const;
	bool isNotNumber() const;
	bool isAction() const;
	bool isWild() const;
	int value() const;
	std::string nameString() const;
	std::string getImagePath() const;


	static std::unique_ptr<Card> make(const Color, const Name);
	static std::unique_ptr<Card> make(const Card& other);
	static std::unique_ptr<Card> make(const std::unique_ptr<Card>& otherPtr);
	static const std::unordered_map<std::tuple<Card::Color, Card::Name>, std::string, TupleHash> imagePaths;
	static const Card back;

	void applyEffect(GameLogic& game, Player& source, Player& target);
	void cancelEffect() { effective = false; }
	void recoverEffect() { effective = true; }
	bool isEffective() const { return effective; }
};

std::ostream& operator<<(std::ostream& ostr, const Card& card);
sf::Packet& operator>>(sf::Packet& packet, Card& card);
sf::Packet& operator<<(sf::Packet& packet, const Card& card);

class Cards {
protected:
	std::deque<std::unique_ptr<Card>> cards = {};
public:
	Cards() = default;
	Cards(const Cards&) = delete;
	Cards& operator=(const Cards&) = delete;
	Cards(Cards&&) = default;
	Cards& operator=(Cards&&) = default;

	Card& getCardByIndex(const std::size_t index) { return *cards[index]; }
	[[nodiscard]] std::unique_ptr<Card> takeCardByIndex(const std::size_t index);

	Card& front() { return *cards.front(); }
	const Card& front() const { return *cards.front(); }
	Card& back() { return *cards.back(); }
	const Card& back() const { return *cards.back(); }
	void push_front(std::unique_ptr<Card> card, const std::size_t number = 1);
	void push_back(std::unique_ptr<Card> card, const std::size_t number = 1);

	std::size_t count() const { return cards.size(); }
	bool empty() const { return cards.empty(); }
	void clear() { cards.clear(); }
	void resize(const std::size_t newSize) { cards.resize(newSize); }
	void cloneTo(Cards& target) const;
	Cards clone() const;
	Card& operator[](const std::size_t pos) { return *cards[pos]; }
	const Card& operator[](const std::size_t pos) const { return *cards[pos]; }

	template<class _Pr>
	auto find_if(_Pr pred) { return std::find_if(cards.begin(), cards.end(), pred); }

	auto begin() { return cards.begin(); }
	auto end() { return cards.end(); }
	auto begin() const { return cards.begin(); }
	auto end() const { return cards.end(); }

	bool satisfy(const std::function<bool(const Cards&)>& condition) const;
	bool include(const std::function<bool(const Card&)>& condition) const;
	bool exclude(const std::function<bool(const Card&)>& condition) const;
	void forEach(const std::function<void(Card&)>& operation) const;
	void forEachIf(const std::function<bool(const Card&)>& condition,
				   const std::function<void(Card&)>& operation) const;
};

std::ostream& operator<<(std::ostream& ostr, const Cards& cards);

class Hand :public Cards {
private:
	std::size_t selectedIndex = 0;
public:
	void selectLeft();
	void selectRight();
	void selectLast();
	std::size_t getSelectedIndex() const;
	const Card& getSelectedCard() const;
	void resetSelectedIndex();
	void setSelectedIndex(std::size_t idx);
	void sort();
	void print() const;
	void display(GameRenderer& renderer, const sf::Vector2f& pos, const sf::Vector2f& cardSize, const sf::Vector2f& pointerSize = { 0,0 }) const;
	Hand clone() const;

	std::size_t value() const;
	[[nodiscard]] std::unique_ptr<Card> takeCardByIndex(const std::size_t index);
	friend sf::Packet& operator<<(sf::Packet& packet, const Hand& hand);
	friend sf::Packet& operator>>(sf::Packet& packet, Hand& hand);
};

class Pile :public Cards {
public:
	static std::unique_ptr<Pile> standard();
	[[nodiscard]] std::unique_ptr<Card> take_front(Pile& discardPile);
	[[nodiscard]] std::unique_ptr<Card> take_back(Pile& discardPile);
	void recycle(Pile& other);
	void shuffle();
	Pile clone() const;
};

