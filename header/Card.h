#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <deque>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Effect.h"
#include "utils.h"

namespace sf { class Packet; }

class GameLogic;


class Card {
public:
#pragma region 类型定义
	enum class Color {
		no, blue, green, red, yellow, black
	};
	enum class Name {
		no,
		number_0, number_1, number_2, number_3, number_4,
		number_5, number_6, number_7, number_8, number_9,
		action_rev, action_skip, action_draw2, wild_pal, wild_draw4,
		back
	};
	enum class Type {
		unknown, number, action, wild
	};
	enum class DiscardReason {
		none,   //没进入弃牌堆（手中/牌堆/通用场景默认值）
		use,    //打出
		discard,//弃置
		recast, //重铸
		decree, //决议
		judge   //判定
	};
	using ColorName = std::pair<Color, Name>;
	struct TupleHash {
		std::size_t operator()(const ColorName& t) const {
			return std::hash<int>{}(static_cast<int>(std::get<0>(t)) * 100 + static_cast<int>(std::get<1>(t)));
		}
	};
#pragma endregion

private:
	Color color = Color::no;
	Name name = Name::no;
	bool effective = true;
	DiscardReason discardReason = DiscardReason::none;

public:
#pragma region 构造 / 静态工厂
	Card(const Color color = Color::no, const Name name = Name::no);
	Card(const ColorName cn);
	static std::unique_ptr<Card> make(const Color, const Name);
	static std::unique_ptr<Card> make(const Card& other);
	static std::unique_ptr<Card> make(const std::unique_ptr<Card>& otherPtr);
#pragma endregion

#pragma region 随机牌
	using CardMemFn = bool (Card::*)() const;
	// 全牌池（只建一次）
	static const std::vector<Card::ColorName>& getAllCards();
	struct CardMemFnHash {
		std::size_t operator()(CardMemFn fn) const noexcept {
			return std::hash<uintptr_t>{}(std::bit_cast<uintptr_t>(fn));
		}
	};
	static std::unordered_map<CardMemFn, std::vector<Card::ColorName>, CardMemFnHash>& getPoolCache();

	static Card::ColorName randomCard(const std::function<bool(const Card&)>& condition
									  = unool::alwaysTrue);
#pragma endregion


#pragma region 属性查询
	Color getColor() const;
	Name getName() const;
	Type getType() const;
	ColorName getColorName() const;
	DiscardReason getDiscardReason() const { return discardReason; }

	bool sameColorAs(const Card& other) const;
	bool sameNameAs(const Card& other) const;
	bool sameTypeAs(const Card& other) const;

	template<typename... Colors> requires (std::same_as<Colors, Color> && ...)
		bool is(const Colors... colors) const {
		return ((color == colors) || ...);
	}
	template<typename... Names> requires (std::same_as<Names, Name> && ...)
		bool is(const Names... names) const {
		return ((name == names) || ...);
	}
	template<typename... Types> requires (std::same_as<Types, Type> && ...)
		bool is(const Types... types) const {
		const Type type = getType();
		return ((type == types) || ...);
	}

	bool isNumber() const;
	bool isNotNumber() const;
	bool isAction() const;
	bool isNotAction() const;
	bool isWild() const;
	bool isNotWild() const;
	bool isTargeted() const;
	bool isNotTargeted() const;
	std::size_t value() const;
	std::string toString() const;
	std::string getImagePath() const;
	bool operator<(const Card& other) const;
	bool operator==(const Card& other) const;
#pragma endregion

#pragma region 属性设置
	void setColor(const Color newColor);
	void setName(const Name newName);
	void setDiscardReason(DiscardReason r) { discardReason = r; }
	void set(const Card& other);
	void set(const Color color, const Name name);
	void set(const ColorName& cn);
#pragma endregion

#pragma region 效果控制
	void applyEffect(GameLogic& game, Player& source, Player& target);
	void cancelEffect() { effective = false; }
	void recoverEffect() { effective = true; }
	bool isEffective() const { return effective; }
#pragma endregion

#pragma region 静态转换 / 静态方法
	static constexpr std::string_view to_string(const Color& color) {
		switch (color) {
			case Color::blue:   return "蓝";
			case Color::green:  return "绿";
			case Color::red:    return "红";
			case Color::yellow: return "黄";
			case Color::black:  return "黑";
			case Color::no:     return "无";
			default:            return "";
		}
	}
	static constexpr std::string_view to_string(const Name& name) {
		switch (name) {
			// 数字牌
			case Name::number_0: return "0";
			case Name::number_1: return "1";
			case Name::number_2: return "2";
			case Name::number_3: return "3";
			case Name::number_4: return "4";
			case Name::number_5: return "5";
			case Name::number_6: return "6";
			case Name::number_7: return "7";
			case Name::number_8: return "8";
			case Name::number_9: return "9";
				// 功能牌
			case Name::action_skip:   return "封禁";
			case Name::action_rev:   return "反转";
			case Name::action_draw2: return "+2";
				//万能牌
			case Name::wild_pal:     return "变色";
			case Name::wild_draw4:   return "+4";
				//其他
			case Name::back:         return "背面";
			case Name::no:           return "无";
			default:                 return "未知";
		}
	}
	static constexpr std::string_view to_string(const Type& type) {
		switch (type) {
			case Type::number:  return "数字牌";
			case Type::action:  return "功能牌";
			case Type::wild:    return "万能牌";
			case Type::unknown:
			default:            return "未知类型";
		}
	}
	static constexpr std::string_view to_string(const DiscardReason& reason) {
		switch (reason) {
			case DiscardReason::use:     return "打出";
			case DiscardReason::discard: return "弃置";
			case DiscardReason::recast:  return "重铸";
			case DiscardReason::decree:  return "决议";
			case DiscardReason::judge:   return "判定";
			case DiscardReason::none:
			default:                     return "";
		}
	}

	static constexpr bool is_number(const Card::Name name) {
		return name == Name::number_0 || name == Name::number_1
			|| name == Name::number_2 || name == Name::number_3
			|| name == Name::number_4 || name == Name::number_5
			|| name == Name::number_6 || name == Name::number_7
			|| name == Name::number_8 || name == Name::number_9;
	}
	static constexpr bool is_action(const Card::Name name) {
		return name == Name::action_skip || name == Name::action_draw2
			|| name == Name::action_rev;
	}
	static constexpr bool is_wild(const Card::Name name) {
		return name == Name::wild_pal || name == Name::wild_draw4;
	}
#pragma endregion

#pragma region 静态数据
	inline static constexpr std::array<Card::Color, 4> fourColors = {
		Color::blue, Color::green, Color::red, Color::yellow
	};
	inline static constexpr std::array<Card::Color, 5> fiveColors = {
		Color::blue, Color::green, Color::red, Color::yellow, Color::black
	};
	inline static constexpr std::array<Card::Name, 10> numberCardsFrom0 = {
		Name::number_0, Name::number_1, Name::number_2, Name::number_3, Name::number_4,
		Name::number_5, Name::number_6, Name::number_7, Name::number_8, Name::number_9,
	};
	inline static constexpr std::array<Card::Name, 10> numberCardsFrom1 = {
		Name::number_1, Name::number_2, Name::number_3, Name::number_4, Name::number_5,
		Name::number_6, Name::number_7, Name::number_8, Name::number_9, Name::number_0
	};
	inline static constexpr std::array<Card::Name, 3> actionCards = {
		Name::action_rev, Name::action_skip, Name::action_draw2
	};
	inline static constexpr std::array<Card::Name, 2> wildCards = {
		Name::wild_pal, Name::wild_draw4
	};
	inline static constexpr std::array<Card::Name, 15> allCards = {
		Name::number_0, Name::number_1, Name::number_2, Name::number_3,Name::number_4,
		Name::number_5, Name::number_6, Name::number_7, Name::number_8, Name::number_9,
		Name::action_rev, Name::action_skip, Name::action_draw2,
		Name::wild_pal, Name::wild_draw4
	};
	static const Card back;
	static const std::unordered_map<ColorName, std::string, TupleHash> imagePaths;
#pragma endregion
};

sf::Packet& operator>>(sf::Packet& packet, Card& card);
sf::Packet& operator<<(sf::Packet& packet, const Card& card);

template<>
struct std::formatter<Card::Color> : std::formatter<std::string_view> {
	auto format(Card::Color c, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(Card::to_string(c), ctx);
	}
};
template<>
struct std::formatter<Card::Name> : std::formatter<std::string_view> {
	auto format(Card::Name n, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(Card::to_string(n), ctx);
	}
};
template<>
struct std::formatter<Card::Type> : std::formatter<std::string_view> {
	auto format(Card::Type t, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(Card::to_string(t), ctx);
	}
};
template<>
struct std::formatter<Card::DiscardReason> : std::formatter<std::string_view> {
	auto format(const Card::DiscardReason& r, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(Card::to_string(r), ctx);
	}
};
template<>
struct std::formatter<Card> : std::formatter<std::string_view> {
	auto format(const Card& c, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(c.toString(), ctx);
	}
};


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

#pragma region 转字符串
	std::string toString() const;
#pragma endregion

#pragma region 元素访问
	Card& getCardByIndex(const std::size_t index) { return *cards[index]; }
	const Card& getCardByIndex(const std::size_t index) const { return *cards[index]; }
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

#pragma region 迭代器
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

template<>
struct std::formatter<Cards> : std::formatter<std::string_view> {
	auto format(const Cards& cards, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(cards.toString(), ctx);
	}
};




class Hand : public Cards {
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

template<>
struct std::formatter<Hand> : std::formatter<std::string_view> {
	auto format(const Hand& hand, std::format_context& ctx) const {
		return std::formatter<std::string_view>::format(hand.toString(), ctx);
	}
};


class Pile : public Cards {
public:
#pragma region 工厂
	static std::unique_ptr<Pile> standard();
#pragma endregion

#pragma region 牌堆操作
	[[nodiscard]] std::unique_ptr<Card> take_front(Pile& discardPile);
	[[nodiscard]] std::unique_ptr<Card> take_back(Pile& discardPile);
	void recycle(Pile& other);
	void shuffle();
#pragma endregion
};






