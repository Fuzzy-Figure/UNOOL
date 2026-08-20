#include "../header/Card.h"
#include "../header/GameRenderer.h"
#include "../header/ImageManager.h"
#include "../header/utils.h"


// ==================== Card 静态数据 ====================
const std::unordered_map<Card::ColorName, std::string, Card::TupleHash> Card::imagePaths = {
	// 红色牌
	{{Color::red, Name::number_0},     "cards/red/number_0.jpg"},
	{{Color::red, Name::number_1},     "cards/red/number_1.jpg"},
	{{Color::red, Name::number_2},     "cards/red/number_2.jpg"},
	{{Color::red, Name::number_3},     "cards/red/number_3.jpg"},
	{{Color::red, Name::number_4},     "cards/red/number_4.jpg"},
	{{Color::red, Name::number_5},     "cards/red/number_5.jpg"},
	{{Color::red, Name::number_6},     "cards/red/number_6.jpg"},
	{{Color::red, Name::number_7},     "cards/red/number_7.jpg"},
	{{Color::red, Name::number_8},     "cards/red/number_8.jpg"},
	{{Color::red, Name::number_9},     "cards/red/number_9.jpg"},
	{{Color::red, Name::action_ban},   "cards/red/action_ban.jpg"},
	{{Color::red, Name::action_rev},   "cards/red/action_rev.jpg"},
	{{Color::red, Name::action_draw2}, "cards/red/action_draw2.jpg"},

	// 蓝色牌
	{{Color::blue, Name::number_0},     "cards/blue/number_0.jpg"},
	{{Color::blue, Name::number_1},     "cards/blue/number_1.jpg"},
	{{Color::blue, Name::number_2},     "cards/blue/number_2.jpg"},
	{{Color::blue, Name::number_3},     "cards/blue/number_3.jpg"},
	{{Color::blue, Name::number_4},     "cards/blue/number_4.jpg"},
	{{Color::blue, Name::number_5},     "cards/blue/number_5.jpg"},
	{{Color::blue, Name::number_6},     "cards/blue/number_6.jpg"},
	{{Color::blue, Name::number_7},     "cards/blue/number_7.jpg"},
	{{Color::blue, Name::number_8},     "cards/blue/number_8.jpg"},
	{{Color::blue, Name::number_9},     "cards/blue/number_9.jpg"},
	{{Color::blue, Name::action_ban},   "cards/blue/action_ban.jpg"},
	{{Color::blue, Name::action_rev},   "cards/blue/action_rev.jpg"},
	{{Color::blue, Name::action_draw2}, "cards/blue/action_draw2.jpg"},

	// 绿色牌
	{{Color::green, Name::number_0},     "cards/green/number_0.jpg"},
	{{Color::green, Name::number_1},     "cards/green/number_1.jpg"},
	{{Color::green, Name::number_2},     "cards/green/number_2.jpg"},
	{{Color::green, Name::number_3},     "cards/green/number_3.jpg"},
	{{Color::green, Name::number_4},     "cards/green/number_4.jpg"},
	{{Color::green, Name::number_5},     "cards/green/number_5.jpg"},
	{{Color::green, Name::number_6},     "cards/green/number_6.jpg"},
	{{Color::green, Name::number_7},     "cards/green/number_7.jpg"},
	{{Color::green, Name::number_8},     "cards/green/number_8.jpg"},
	{{Color::green, Name::number_9},     "cards/green/number_9.jpg"},
	{{Color::green, Name::action_ban},   "cards/green/action_ban.jpg"},
	{{Color::green, Name::action_rev},   "cards/green/action_rev.jpg"},
	{{Color::green, Name::action_draw2}, "cards/green/action_draw2.jpg"},

	// 黄色牌
	{{Color::yellow, Name::number_0},     "cards/yellow/number_0.jpg"},
	{{Color::yellow, Name::number_1},     "cards/yellow/number_1.jpg"},
	{{Color::yellow, Name::number_2},     "cards/yellow/number_2.jpg"},
	{{Color::yellow, Name::number_3},     "cards/yellow/number_3.jpg"},
	{{Color::yellow, Name::number_4},     "cards/yellow/number_4.jpg"},
	{{Color::yellow, Name::number_5},     "cards/yellow/number_5.jpg"},
	{{Color::yellow, Name::number_6},     "cards/yellow/number_6.jpg"},
	{{Color::yellow, Name::number_7},     "cards/yellow/number_7.jpg"},
	{{Color::yellow, Name::number_8},     "cards/yellow/number_8.jpg"},
	{{Color::yellow, Name::number_9},     "cards/yellow/number_9.jpg"},
	{{Color::yellow, Name::action_ban},   "cards/yellow/action_ban.jpg"},
	{{Color::yellow, Name::action_rev},   "cards/yellow/action_rev.jpg"},
	{{Color::yellow, Name::action_draw2}, "cards/yellow/action_draw2.jpg"},

	// 黑色牌
	{{Color::black, Name::wild_pal},   "cards/black/wild_pal.jpg"},
	{{Color::black, Name::wild_draw4}, "cards/black/wild_draw4.jpg"},

	// 背面牌
	{{Color::no, Name::back}, "cards/back.jpg"}
};

const Card Card::back(Card::Color::no, Card::Name::back);


// ==================== Card 类 ====================

// 构造 / 静态工厂
Card::Card(const Color _color, const Name _name) :color(_color), name(_name) {}

std::unique_ptr<Card> Card::make(const Color _color, const Name _name) {
	return std::make_unique<Card>(_color, _name);
}
std::unique_ptr<Card> Card::make(const Card& other) {
	return std::make_unique<Card>(other.getColor(), other.getName());
}
std::unique_ptr<Card> Card::make(const std::unique_ptr<Card>& otherPtr) {
	return make(*otherPtr);
}

// 属性查询
bool Card::operator<(const Card& other) const {
	if (color != other.color)
		return color < other.color;
	return name < other.name;
}
bool Card::operator==(const Card& other) const {
	return getColorName() == other.getColorName();
}
Card::Color Card::getColor() const {
	return color;
}
Card::Name Card::getName() const {
	return name;
}
Card::ColorName Card::getColorName() const {
	return std::make_pair(color, name);
}
bool Card::isNumber() const {
	return name == Name::number_0 || name == Name::number_1
		|| name == Name::number_2 || name == Name::number_3
		|| name == Name::number_4 || name == Name::number_5
		|| name == Name::number_6 || name == Name::number_7
		|| name == Name::number_8 || name == Name::number_9;
}
bool Card::isNotNumber() const {
	return !isNumber();
}
bool Card::isAction() const {
	return name == Name::action_ban || name == Name::action_draw2
		|| name == Name::action_rev;
}
bool Card::isNotAction() const {
	return !isAction();
}
bool Card::isWild() const {
	return name == Name::wild_pal || name == Name::wild_draw4;
}
bool Card::isNotWild() const {
	return !isWild();
}
int Card::value() const {
	switch (name) {
		//数字
	case Name::number_0: return 0;
	case Name::number_1: return 1;
	case Name::number_2: return 2;
	case Name::number_3: return 3;
	case Name::number_4: return 4;
	case Name::number_5: return 5;
	case Name::number_6: return 6;
	case Name::number_7: return 7;
	case Name::number_8: return 8;
	case Name::number_9: return 9;
		//功能
	case Name::action_ban:
	case Name::action_draw2:
	case Name::action_rev: return 20;
		//万能
	case Name::wild_pal:
	case Name::wild_draw4: return 50;
		//其他
	default: throw std::invalid_argument("无法计算背面/未知牌的价值");
	}
}
std::string Card::toString() const {
	return Card::to_string(color) + Card::to_string(name);
}
std::wstring Card::toWString() const {
	return Card::to_wstring(color) + Card::to_wstring(name);
}
std::string Card::getImagePath() const {
	if (auto it = imagePaths.find(std::tuple(color, name)); it != imagePaths.end())
		return it->second;
	else throw std::invalid_argument("未找到图片路径");
}

// 属性设置
void Card::setColor(const Color newColor) {
	color = newColor;
}
void Card::setName(const Name newName) {
	name = newName;
}
void Card::set(const Card& other) {
	setColor(other.getColor());
	setName(other.getName());
}

// 显示
void Card::display(GameRenderer& renderer, const sf::Vector2f& pos, const sf::Vector2f& cardSize) const {
	renderer.displayImage(getImagePath(), pos, cardSize);
}
void Card::displayInCenter(GameRenderer& renderer, const sf::Vector2f& cardSize) const {
	renderer.displayImageInCenter(getImagePath(), cardSize);
}

// 效果控制
void Card::applyEffect(GameLogic& game, Player& source, Player& target) {
	if (isEffective()) {
		switch (getName()) {
		case Name::action_ban:   Effect::ban(*this, source, target); break;
		case Name::action_rev:   Effect::rev(*this, game); break;
		case Name::action_draw2: Effect::draw2(*this, source, target); break;
		case Name::wild_pal:     Effect::pal(*this, game, source); break;
		case Name::wild_draw4:   Effect::draw4(*this, game, source, target); break;
		}
	}
}

// 静态转换方法
std::string Card::to_string(const Color& color) {
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
std::wstring Card::to_wstring(const Color& color) {
	switch (color) {
	case Color::blue:   return L"蓝";
	case Color::green:  return L"绿";
	case Color::red:    return L"红";
	case Color::yellow: return L"黄";
	case Color::black:  return L"黑";
	case Color::no:     return L"无";
	default:            return L"";
	}
}
std::string Card::to_string(const Name& name) {
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
	case Name::action_ban:   return "封禁";
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
std::wstring Card::to_wstring(const Name& name) {
	switch (name) {
		// 数字牌
	case Name::number_0: return L"0";
	case Name::number_1: return L"1";
	case Name::number_2: return L"2";
	case Name::number_3: return L"3";
	case Name::number_4: return L"4";
	case Name::number_5: return L"5";
	case Name::number_6: return L"6";
	case Name::number_7: return L"7";
	case Name::number_8: return L"8";
	case Name::number_9: return L"9";
		// 功能牌
	case Name::action_ban:   return L"封禁";
	case Name::action_rev:   return L"反转";
	case Name::action_draw2: return L"+2";
		//万能牌
	case Name::wild_pal:     return L"变色";
	case Name::wild_draw4:   return L"+4";
		//其他
	case Name::back:         return L"背面";
	case Name::no:           return L"无";
	default:                 return L"未知";
	}
}

// 友元流输出
std::ostream& operator<<(std::ostream& ostr, const Card& card) {
	return ostr << card.toString();
}


// ==================== Cards 类 ====================

// 修改容器
[[nodiscard]] std::unique_ptr<Card> Cards::takeCardByIndex(std::size_t index) {
	if (index >= cards.size())
		throw std::out_of_range("Cards::takeCardByIndex: index " + std::to_string(index) +
								" out of range, size is " + std::to_string(cards.size()));
	std::unique_ptr<Card> card = std::move(cards[index]);
	cards.erase(cards.begin() + index);
	return card;
}
void Cards::push_front(std::unique_ptr<Card> card, const std::size_t number) {
	for (std::size_t i = 0; i < number - 1; ++i) {
		cards.push_front(Card::make(card));
	}
	cards.push_front(std::move(card));
}
void Cards::push_back(std::unique_ptr<Card> card, const std::size_t number) {
	for (std::size_t i = 0; i < number - 1; ++i) {
		cards.push_back(Card::make(card));
	}
	cards.push_back(std::move(card));
}

// 容量 / 克隆
void Cards::cloneTo(Cards& target) const {
	for (const auto& card : cards) {
		target.push_back(Card::make(*card));
	}
}
Cards Cards::clone() const {
	Cards newCards;
	cloneTo(newCards);
	return newCards;
}

// 条件遍历
bool Cards::satisfy(const std::function<bool(const Cards&)>& condition) const {
	return condition(*this);
}
bool Cards::include(const std::function<bool(const Card&)>& condition) const {
	for (const auto& card : *this) {
		if (condition(*card)) return true;
	}
	return false;
}
bool Cards::exclude(const std::function<bool(const Card&)>& condition) const {
	for (const auto& card : *this) {
		if (condition(*card)) return false;
	}
	return true;
}
void Cards::forEach(const std::function<void(Card&)>& operation) const {
	for (auto& c : cards) {
		operation(*c);
	}
}
void Cards::forEachIf(const std::function<bool(const Card&)>& condition,
					  const std::function<void(Card&)>& operation) const {
	for (auto& c : cards) {
		if (condition(*c)) operation(*c);
	}
}

// 友元流输出
std::ostream& operator<<(std::ostream& ostr, const Cards& cards) {
	for (const auto& card : cards) {
		ostr << *card << "，";
	}
	return ostr;
}


// ==================== Hand 类 ====================

// 指针导航
void Hand::selectLeft() {
	if (count() == 0) return;
	if (selectedIndex == 0) selectedIndex = count() - 1;
	else selectedIndex -= 1;
}
void Hand::selectRight() {
	if (count() == 0) return;
	if (selectedIndex == count() - 1) selectedIndex = 0;
	else selectedIndex += 1;
}
void Hand::selectLast() {
	if (count() > 0) selectedIndex = count() - 1;
}
void Hand::resetSelectedIndex() {
	if (selectedIndex > 0) --selectedIndex;
	if (selectedIndex < 0 || selectedIndex >= count()) selectedIndex = 0;
}
void Hand::setSelectedIndex(std::size_t idx) {
	if (count() == 0) {
		selectedIndex = 0;
		return;
	}
	if (idx >= count()) {
		selectedIndex = count() - 1;
	}
	else {
		selectedIndex = idx;
	}
}

// 指针查询
std::size_t Hand::getSelectedIndex() const {
	return selectedIndex;
}
const Card& Hand::getSelectedCard() const {
	if (count() == 0) throw std::out_of_range("手牌为空");
	return *cards[selectedIndex];
}

// 排序 / 输出
void Hand::sort() {
	std::ranges::sort(cards, [](const std::unique_ptr<Card>& a, const std::unique_ptr<Card>& b) {
		return *a < *b;
	});
}
void Hand::print() const {
	std::cout << *this;
	if (!empty()) std::cout << "；当前选择了第" << getSelectedIndex() << "张牌：" << getSelectedCard();
	std::cout << std::endl;
}
void Hand::display(GameRenderer& renderer, const sf::Vector2f& pos, const sf::Vector2f& cardSize, const sf::Vector2f& pointerSize) const {
	bool displayPointer = pointerSize != sf::Vector2f{ 0, 0 };
	const std::size_t foldCardWidth = static_cast<std::size_t>(cardSize.x / 3);
	std::size_t dx = 0;
	std::size_t selectedPos = 0;
	for (std::size_t i = 0; i < count(); ++i) {
		cards[i]->display(renderer, { pos.x + dx, pos.y }, cardSize);
		if (displayPointer && selectedIndex == i) {
			selectedPos = dx;
			dx += static_cast<std::size_t>(cardSize.x);
		}
		else {
			dx += foldCardWidth;
		}
	}

	if (displayPointer && !empty()) {
		renderer.displayImage(
			"cards/pointer.jpg",
			{ pos.x + selectedPos + cardSize.x / 2 - pointerSize.x / 2,
			pos.y + cardSize.y },
			pointerSize
		);
	}
}

// 工具方法
std::size_t Hand::value() const {
	std::size_t value = 0;
	for (const auto& c : cards) {
		value += c->value();
	}
	return value;
}
Hand Hand::clone() const {
	Hand newHand;
	cloneTo(newHand);
	newHand.selectedIndex = selectedIndex;
	return newHand;
}


[[nodiscard]] std::unique_ptr<Card> Hand::takeCardByIndex(const std::size_t index) {
	auto card = Cards::takeCardByIndex(index);
	if (count() == 0) {
		selectedIndex = 0;
	}
	else if (index < selectedIndex) {
		selectedIndex -= 1;
	}
	else if (index == selectedIndex) {
		if (selectedIndex >= count()) {
			//出的是最右边的牌，指向新的最右边
			selectLast();
		}
		else if (selectedIndex > 0) {
			//出的是中间的牌，指向其左边那张
			selectedIndex -= 1;
		}
		//出的是最左边的牌(index==0)，不变
	}
	return card;
}


// ==================== Pile 类 ====================

// 工厂 / 克隆
std::unique_ptr<Pile> Pile::standard() {
	std::unique_ptr<Pile> standard = std::make_unique<Pile>();
	constexpr std::array<Card::Color, 4> colors = {
		Card::Color::blue, Card::Color::green, Card::Color::red, Card::Color::yellow
	};
	constexpr std::array<Card::Name, 13> names = {
		Card::Name::number_0, Card::Name::number_1, Card::Name::number_2, Card::Name::number_3, Card::Name::number_4,
		Card::Name::number_5, Card::Name::number_6, Card::Name::number_7, Card::Name::number_8, Card::Name::number_9,
		Card::Name::action_rev, Card::Name::action_ban, Card::Name::action_draw2
	};

	//非黑牌
	for (const Card::Color color : colors) {
		for (const Card::Name name : names) {
			auto newCard = std::make_unique<Card>(color, name);
			//数字牌
			if (newCard->isNumber()) {
				//1~9每种颜色4张
				if (newCard->getName() != Card::Name::number_0) standard->push_back(std::move(newCard), 4);
				//0每种颜色3张
				else standard->push_back(std::move(newCard), 3);
			}
			//功能牌
			else if (newCard->isAction()) {
				//反转，封禁每种颜色4张
				if (name == Card::Name::action_rev || name == Card::Name::action_ban)
					standard->push_back(std::move(newCard), 4);
				//+2每种颜色5张
				else if (name == Card::Name::action_draw2)
					standard->push_back(std::move(newCard), 5);
				else throw;
			}
			else throw;
		}
	}
	//黑牌
	auto pal = std::make_unique<Card>(Card::Color::black, Card::Name::wild_pal);
	auto draw4 = std::make_unique<Card>(Card::Color::black, Card::Name::wild_draw4);
	standard->push_back(std::move(pal), 11);
	standard->push_back(std::move(draw4), 10);

	//洗牌
	standard->shuffle();
	return standard;
}
Pile Pile::clone() const {
	Pile newPile;
	cloneTo(newPile);
	return newPile;
}

// 牌堆操作
[[nodiscard]] std::unique_ptr<Card> Pile::take_front(Pile& discardPile) {
	//牌堆里没牌了，回收弃牌堆
	if (cards.empty()) recycle(discardPile);
	if (cards.empty()) throw std::runtime_error("无法摸牌，因为牌堆和弃牌堆里都没有牌了");
	//取牌
	std::unique_ptr<Card> frontCard = std::move(cards.front());
	cards.pop_front();
	if (cards.empty()) recycle(discardPile);
	return frontCard;
}
[[nodiscard]] std::unique_ptr<Card> Pile::take_back(Pile& discardPile) {
	std::unique_ptr<Card> card = std::move(cards.back());
	cards.pop_back();
	return card;
}
void Pile::recycle(Pile& other) {
	cards = std::move(other.cards);
	shuffle();
}
void Pile::shuffle() {
	std::ranges::shuffle(cards, unool::random::rng);
}


// ==================== Packet 序列化 ====================
sf::Packet& operator<<(sf::Packet& packet, const Card& card) {
	packet << static_cast<int>(card.getColor()) << static_cast<int>(card.getName());
	return packet;
}
sf::Packet& operator>>(sf::Packet& packet, Card& card) {
	int colorInt, nameInt;
	if (packet >> colorInt >> nameInt) {
		card.setColor(static_cast<Card::Color>(colorInt));
		card.setName(static_cast<Card::Name>(nameInt));
	}
	return packet;
}

sf::Packet& operator<<(sf::Packet& packet, const Hand& hand) {
	std::size_t validIndex = hand.selectedIndex;
	if (hand.count() > 0 && validIndex >= hand.count()) {
		validIndex = hand.count() - 1;
	}
	else if (hand.count() == 0) {
		validIndex = 0;
	}
	packet << hand.cards.size();
	for (const auto& cardPtr : hand.cards) {
		packet << *cardPtr;
	}
	packet << validIndex;
	return packet;
}
sf::Packet& operator>>(sf::Packet& packet, Hand& hand) {
	hand.cards.clear();
	std::size_t size;
	packet >> size;
	for (std::size_t i = 0; i < size; ++i) {
		Card c;
		packet >> c;
		hand.cards.push_back(Card::make(c));
	}
	packet >> hand.selectedIndex;
	if (hand.count() == 0) {
		hand.selectedIndex = 0;
	}
	else if (hand.selectedIndex >= hand.count()) {
		hand.selectedIndex = hand.count() - 1;
	}
	return packet;
}
