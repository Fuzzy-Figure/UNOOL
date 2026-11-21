// Arch-C P3-5：冻结区玩法规则单测。
// 只依赖 Card / Hand / Cards 三个纯值类型，不构造 GameLogic、不接服务定位器，
// 因此可在 Tests 工程里直接跑。规则一旦被改动，这里会先失败。
#include "test_rules.h"

#include "core/Card.h"
#include "core/Character.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

using unool::tests::TestResult;

namespace {

// 牌值表：数字牌 = 点数、功能牌 20、万能牌 50、秘技牌 10。
// 这张表就是「体力扣减 = 手牌分值之和」的乘数，改动等于改玩法。
void testCardValues(TestResult& r) {
	for (std::size_t pip = 0; pip < Card::numberCardsFrom0.size(); ++pip) {
		const auto c = Card::make(Card::Color::red, Card::numberCardsFrom0[pip]);
		r.check(c->value() == pip, "number card value equals its pip");
	}
	for (const Card::Name name : Card::actionCards) {
		const auto c = Card::make(Card::Color::blue, name);
		r.check(c->value() == 20, "action card value is 20");
	}
	for (const Card::Name name : Card::wildCards) {
		const auto c = Card::make(Card::Color::black, name);
		r.check(c->value() == 50, "wild card value is 50");
	}

	// 秘技牌统一 10 点（数值改动会影响败者手牌结算）。
	for (const Card::Name name : { Card::Name::secret_毒酒, Card::Name::secret_药酒,
								   Card::Name::secret_过载, Card::Name::secret_指虎 }) {
		const auto c = Card::make(Card::Color::no, name);
		r.check(c->value() == 10, "secret card value is 10");
	}

	// 背面 / 未知牌没有价值：抛异常而不是返回 0，避免静默算错体力。
	bool threw = false;
	try {
		(void)Card::make(Card::Color::no, Card::Name::back)->value();
	}
	catch (const std::invalid_argument&) {
		threw = true;
	}
	r.check(threw, "back card value throws invalid_argument");
}

// 体力扣减依据：Hand::value() 是逐张牌值求和，空手牌为 0。
void testHandValueIsSum(TestResult& r) {
	Hand hand;
	r.check(hand.value() == 0, "empty hand value is 0");

	hand.push_back(Card::make(Card::Color::red, Card::Name::number_3));
	hand.push_back(Card::make(Card::Color::green, Card::Name::action_skip));
	hand.push_back(Card::make(Card::Color::black, Card::Name::wild_draw4));
	hand.push_back(Card::make(Card::Color::no, Card::Name::secret_药酒));
	r.check(hand.value() == 3 + 20 + 50 + 10, "hand value = sum of card values");
}

// 秘技牌：独立类型、不进标准牌池与随机牌池（只由角色技能发放）。
void testSecretCardClassification(TestResult& r) {
	for (const Card::Name name : { Card::Name::secret_毒酒, Card::Name::secret_药酒,
								   Card::Name::secret_过载, Card::Name::secret_指虎 }) {
		const auto c = Card::make(Card::Color::no, name);
		r.check(c->isSecret(), "secret card reports isSecret");
		r.check(c->getType() == Card::Type::secret, "secret card has Type::secret");
	}

	r.check(Card::make(Card::Color::red, Card::Name::number_5)->getType() == Card::Type::number,
			"number card type");
	r.check(Card::make(Card::Color::red, Card::Name::action_rev)->getType() == Card::Type::action,
			"action card type");
	r.check(Card::make(Card::Color::black, Card::Name::wild_pal)->getType() == Card::Type::wild,
			"wild card type");
	r.check(!Card::make(Card::Color::red, Card::Name::number_5)->isSecret(),
			"number card is not secret");

	// 随机牌池构成：4 个彩色各 10 数字 + 3 功能 = 13，黑色 2 张万能，共 54。
	// 秘技牌与紫色牌（技能专用）都不在池内。
	const auto& pool = Card::getAllCards();
	r.check(pool.size() == 54, "standard card pool has 54 entries");
	std::size_t secretCount = 0;
	std::size_t purpleCount = 0;
	std::size_t blackNonWild = 0;
	for (const auto& [color, name] : pool) {
		if (Card::is_secret(name)) ++secretCount;
		if (color == Card::Color::purple) ++purpleCount;
		if (color == Card::Color::black && name != Card::Name::wild_pal
			&& name != Card::Name::wild_draw4) {
			++blackNonWild;
		}
	}
	r.check(secretCount == 0, "secret cards are not in the random pool");
	r.check(purpleCount == 0, "purple cards are not in the random pool");
	r.check(blackNonWild == 0, "black cards are wild only");
}

// 卡牌容器：批量插入张数、按索引取走、越界抛、克隆是深拷贝。
void testCardsContainer(TestResult& r) {
	Cards cards;
	cards.push_back(Card::make(Card::Color::red, Card::Name::number_7), 3);
	r.check(cards.count() == 3, "push_back(count) inserts that many copies");

	// 0 张是合法入参（调用方算出来的数量可能为 0），必须什么都不做；
	// 早期实现用 number - 1 判断，无符号下溢成 SIZE_MAX，会死循环 + 无限分配。
	Cards none;
	none.push_back(Card::make(Card::Color::red, Card::Name::number_7), 0);
	none.push_front(Card::make(Card::Color::blue, Card::Name::number_1), 0);
	r.check(none.empty(), "push with count 0 inserts nothing");

	auto taken = cards.takeCardByIndex(1);
	r.check(taken != nullptr && taken->getName() == Card::Name::number_7,
			"takeCardByIndex returns the card");
	r.check(cards.count() == 2, "takeCardByIndex removes one card");

	bool threw = false;
	try {
		(void)cards.takeCardByIndex(5);
	}
	catch (const std::out_of_range&) {
		threw = true;
	}
	r.check(threw, "takeCardByIndex out of range throws");

	Cards cloned = cards.clone();
	r.check(cloned.count() == cards.count(), "clone keeps card count");
	cloned[0].setName(Card::Name::number_9);
	r.check(cards[0].getName() == Card::Name::number_7, "clone is a deep copy");
}

// 手牌选择指针：空手牌恒 0、越界夹紧、左右回绕、出牌后跟随修正。
void testHandSelection(TestResult& r) {
	Hand hand;
	r.check(hand.getSelectedIndex() == 0, "empty hand selected index is 0");
	hand.setSelectedIndex(3);
	r.check(hand.getSelectedIndex() == 0, "empty hand clamps selected index to 0");
	bool threw = false;
	try {
		(void)hand.getSelectedCard();
	}
	catch (const std::out_of_range&) {
		threw = true;
	}
	r.check(threw, "empty hand getSelectedCard throws");

	hand.push_back(Card::make(Card::Color::red, Card::Name::number_1));
	hand.push_back(Card::make(Card::Color::blue, Card::Name::number_2));
	hand.push_back(Card::make(Card::Color::green, Card::Name::number_3));

	hand.setSelectedIndex(99);
	r.check(hand.getSelectedIndex() == 2, "selected index clamps to last card");
	hand.setSelectedIndex(0);
	hand.selectLeft();
	r.check(hand.getSelectedIndex() == 2, "selectLeft wraps to last");
	hand.selectRight();
	r.check(hand.getSelectedIndex() == 0, "selectRight wraps to first");

	// 出中间的牌：指针左移一位。
	hand.setSelectedIndex(1);
	(void)hand.takeCardByIndex(1);
	r.check(hand.getSelectedIndex() == 0, "playing middle card moves selection left");

	// 出最右边的牌：指针指向新的最右边。
	hand.setSelectedIndex(hand.count() - 1);
	(void)hand.takeCardByIndex(hand.count() - 1);
	r.check(hand.getSelectedIndex() == hand.count() - 1,
			"playing last card selects new last card");
}

// 弃牌元数据随复制保留：结算与摸牌来源依赖这些字段不丢。
void testCardCopyKeepsMetadata(TestResult& r) {
	auto source = Card::make(Card::Color::red, Card::Name::number_4);
	source->setDiscardReason(Card::DiscardReason::use);
	source->setExtraDamage(2);
	source->setDiscardSourceId(std::size_t{ 1 });

	auto copied = Card::make(*source);
	r.check(copied->getColor() == Card::Color::red, "copy keeps color");
	r.check(copied->getName() == Card::Name::number_4, "copy keeps name");
	r.check(copied->getDiscardReason() == Card::DiscardReason::use, "copy keeps discard reason");
	r.check(copied->getExtraDamage() == 2, "copy keeps extra damage");
	r.check(copied->getDiscardSourceId() == std::optional<std::size_t>{ 1 },
			"copy keeps discard source id");
}

// 生命上限：改上限时当前体力同步压到新上限，提高上限不回复体力。
// 白板只有 1 点体力，所以用「降到 0」「升到 10」两端验证同一条规则，
// 数值大小不影响判定，只是【发射】那种「减上限到 75」的同一段代码。
void testMaxHp(TestResult& r) {
	auto character = Character::make("白板");
	r.check(character->getHp() == 1 && character->getMaxHp() == 1, "max hp defaults to hp");

	character->setMaxHp(0);
	r.check(character->getMaxHp() == 0, "setMaxHp lowers the cap");
	r.check(character->getHp() == 0 && character->isDead(),
			"lowering the cap clamps current hp");

	character->setMaxHp(10);
	r.check(character->getMaxHp() == 10, "raising the cap takes effect");
	r.check(character->getHp() == 0, "raising the cap does not heal");
}

} // namespace

unool::tests::TestResult runRulesTests() {
	unool::tests::TestResult r;
	testCardValues(r);
	testHandValueIsSum(r);
	testSecretCardClassification(r);
	testCardsContainer(r);
	testHandSelection(r);
	testCardCopyKeepsMetadata(r);
	testMaxHp(r);
	return r;
}
