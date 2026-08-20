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
#pragma region 类型定义
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
	using ColorName = std::pair<Color, Name>;
	struct TupleHash {
		std::size_t operator()(const ColorName& t) const {
			return std::hash<int>{}(static_cast<int>(std::get<0>(t)) * 100 + static_cast<int>(std::get<1>(t)));
		}
	};

	inline static constexpr std::array<Name, 10> numberCardsFrom1 = {
		Name::number_1, Name::number_2, Name::number_3, Name::number_4, Name::number_5,
		Name::number_6, Name::number_7, Name::number_8, Name::number_9, Name::number_0
	};
	inline static constexpr std::array<Name, 10> numberCardsFrom0 = {
		Name::number_0, Name::number_1, Name::number_2, Name::number_3,Name::number_4,
		Name::number_5, Name::number_6, Name::number_7, Name::number_8, Name::number_9,
	};
	inline static constexpr std::array<Name, 3> actionCards = {
		Name::action_rev, Name::action_ban, Name::action_draw2
	};
	inline static constexpr std::array<Name, 2> wildCards = {
		Name::wild_pal, Name::wild_draw4,
	};
#pragma endregion

private:
	Color color = Color::no;
	Name name = Name::no;
	bool effective = true;

public:
#pragma region 构造 / 静态工厂
	Card(const Color color = Color::no, const Name name = Name::no);
	static std::unique_ptr<Card> make(const Color, const Name);
	static std::unique_ptr<Card> make(const Card& other);
	static std::unique_ptr<Card> make(const std::unique_ptr<Card>& otherPtr);
	static const Card back;
#pragma endregion

#pragma region 属性查询
	Color getColor() const;
	Name getName() const;
	ColorName getColorName() const;

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
	bool isNotAction() const;
	bool isWild() const;
	bool isNotWild() const;
	int value() const;
	std::string toString() const;
	std::wstring toWString() const;
	std::string getImagePath() const;
	bool operator<(const Card& other) const;
	bool operator==(const Card& other) const;
#pragma endregion

#pragma region 属性设置
	void setColor(const Color newColor);
	void setName(const Name newName);
	void set(const Card& other);
#pragma endregion

#pragma region 显示
	void display(GameRenderer& renderer, const sf::Vector2f& pos, const sf::Vector2f& cardSize) const;
	void displayInCenter(GameRenderer& renderer, const sf::Vector2f& cardSize) const;
#pragma endregion

#pragma region 效果控制
	void applyEffect(GameLogic& game, Player& source, Player& target);
	void cancelEffect() { effective = false; }
	void recoverEffect() { effective = true; }
	bool isEffective() const { return effective; }
#pragma endregion

#pragma region 静态转换 / 静态数据
	static std::string to_string(const Color& color);
	static std::wstring to_wstring(const Color& color);
	static std::string to_string(const Name& name);
	static std::wstring to_wstring(const Name& name);
	static const std::unordered_map<ColorName, std::string, TupleHash> imagePaths;
#pragma endregion
};

std::ostream& operator<<(std::ostream& ostr, const Card& card);
sf::Packet& operator>>(sf::Packet& packet, Card& card);
sf::Packet& operator<<(sf::Packet& packet, const Card& card);


class Cards {
protected:
	std::deque<std::unique_ptr<Card>> cards = {};

public:
#pragma region 构造 / 赋值
	Cards() = default;
	Cards(const Cards&) = delete;
	Cards& operator=(const Cards&) = delete;
	Cards(Cards&&) = default;
	Cards& operator=(Cards&&) = default;
#pragma endregion

#pragma region 元素访问
	Card& getCardByIndex(const std::size_t index) { return *cards[index]; }
	Card& operator[](const std::size_t pos) { return *cards[pos]; }
	const Card& operator[](const std::size_t pos) const { return *cards[pos]; }
	Card& front() { return *cards.front(); }
	const Card& front() const { return *cards.front(); }
	Card& back() { return *cards.back(); }
	const Card& back() const { return *cards.back(); }
#pragma endregion

#pragma region 修改容器
	void push_front(std::unique_ptr<Card> card, const std::size_t number = 1);
	void push_back(std::unique_ptr<Card> card, const std::size_t number = 1);
	[[nodiscard]] std::unique_ptr<Card> takeCardByIndex(const std::size_t index);
#pragma endregion

#pragma region 容量 / 克隆
	std::size_t count() const { return cards.size(); }
	bool empty() const { return cards.empty(); }
	void clear() { cards.clear(); }
	void resize(const std::size_t newSize) { cards.resize(newSize); }
	void cloneTo(Cards& target) const;
	Cards clone() const;
#pragma endregion

#pragma region 迭代器 / 算法
	template<class _Pr>
	auto find_if(_Pr pred) { return std::ranges::find_if(cards, pred); }

	auto begin() { return cards.begin(); }
	auto end() { return cards.end(); }
	auto begin() const { return cards.begin(); }
	auto end() const { return cards.end(); }
#pragma endregion

#pragma region 条件遍历
	bool satisfy(const std::function<bool(const Cards&)>& condition) const;
	bool include(const std::function<bool(const Card&)>& condition) const;
	bool exclude(const std::function<bool(const Card&)>& condition) const;
	void forEach(const std::function<void(Card&)>& operation) const;
	void forEachIf(const std::function<bool(const Card&)>& condition,
				   const std::function<void(Card&)>& operation) const;
#pragma endregion
};

std::ostream& operator<<(std::ostream& ostr, const Cards& cards);


class Hand :public Cards {
private:
	std::size_t selectedIndex = 0;

public:
#pragma region 指针导航
	void selectLeft();
	void selectRight();
	void selectLast();
	void resetSelectedIndex();
	void setSelectedIndex(std::size_t idx);
#pragma endregion

#pragma region 指针查询
	std::size_t getSelectedIndex() const;
	const Card& getSelectedCard() const;
#pragma endregion

#pragma region 排序 / 输出
	void sort();
	void print() const;
	void display(GameRenderer& renderer, const sf::Vector2f& pos, const sf::Vector2f& cardSize, const sf::Vector2f& pointerSize = { 0,0 }, bool canSelect = false) const;
#pragma endregion

#pragma region 工具方法
	std::size_t value() const;
	Hand clone() const;
#pragma endregion

#pragma region 覆盖 / 序列化
	[[nodiscard]] std::unique_ptr<Card> takeCardByIndex(const std::size_t index);
	friend sf::Packet& operator<<(sf::Packet& packet, const Hand& hand);
	friend sf::Packet& operator>>(sf::Packet& packet, Hand& hand);
#pragma endregion
};


class Pile :public Cards {
public:
#pragma region 工厂 / 克隆
	static std::unique_ptr<Pile> standard();
	Pile clone() const;
#pragma endregion

#pragma region 牌堆操作
	[[nodiscard]] std::unique_ptr<Card> take_front(Pile& discardPile);
	[[nodiscard]] std::unique_ptr<Card> take_back(Pile& discardPile);
	void recycle(Pile& other);
	void shuffle();
#pragma endregion
};
