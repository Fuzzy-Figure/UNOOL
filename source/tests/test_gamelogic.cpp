// Arch-C P3-5：GameLogic 测试台（fake-port）。
//
// 用 test_fakes.h 的假端口构造真实 GameLogic，跑真实的摸牌与技能派发：
//   - 摸牌来源：draw_begin 的 Trigger.source 只在「牌效果摸牌」时存在，
//     且指向打出该牌的角色（【炮击】据此区分「自己打出的牌让别人摸牌」）；
//   - 技能触发顺序：同一时机按载体（players 数组）顺序，载体内部按注册顺序。
//
// 三步就能加新用例：装输入 -> 造 GameLogic -> 断言。技能一律用 forced=true
// 的记录型被动，避免测试台被 ask 卡住；真正需要选牌/选人的用例再预置输入。

#include "test_gamelogic.h"

#include "test_fakes.h"

#include "core/Card.h"
#include "core/GameLogic.h"
#include "core/Player.h"
#include "core/Skill.h"

#include <algorithm>
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using unool::tests::TestResult;

namespace {

// 记录型被动：不改变对局，只把触发时拿到的 player / source 记到日志里。
// forced=true 是关键——否则 launch() 会先 ask() 一次「是否发动」。
class RecordingPassive final : public PassiveSkill {
public:
	RecordingPassive(std::string tag,
					 const PassiveSkill::TriggerTime time,
					 std::vector<std::string>& log,
					 const PassiveSkill::TriggerPlayer who = PassiveSkill::TriggerPlayer::self)
		: PassiveSkill("记录_" + tag, "测试用被动，只记录触发字段",
					   std::nullopt, true, who, time),
		  tag_(std::move(tag)), log_(log) {}

	bool content(GameLogic&, Player&, Trigger& trigger) override {
		log_.get().push_back(std::format("{}|player={}|source={}",
										 tag_, idText(trigger.player), idText(trigger.source)));
		return true;
	}

private:
	static std::string idText(const opt_ref<Player>& who) {
		if (!who.has_value()) return "none";
		return std::to_string(who.value().get().getId());
	}

	std::string tag_;
	std::reference_wrapper<std::vector<std::string>> log_;
};

// 两个白板开局：determineSeatOrder 会对两名玩家各发一次 hint（确认框），
// 因此初始化前预置两次「按 1 确认」。
void initBlankGame(GameLogic& game, unool::tests::FakeWorld& world) {
	world.decision.expectConfirm(0);
	world.decision.expectConfirm(1);
	game.initPlayers({ "白板", "白板" });
}

// 摸牌来源：只有「牌效果摸牌」带 source，普通摸牌必须为空。
void testDrawSource(TestResult& r, unool::tests::FakeWorld& world) {
	GameLogic game(world.view, world.decision, world.services);
	initBlankGame(game, world);

	const auto players = game.getPlayers();
	Player& first = players.at(0).get();
	Player& second = players.at(1).get();

	std::vector<std::string> log;
	first.addSkill(std::make_unique<RecordingPassive>("摸牌", PassiveSkill::TriggerTime::draw_begin, log));

	first.draw(1, DrawReason::phase_draw);
	r.check(log.size() == 1 && log.back() == "摸牌|player=0|source=none",
			"普通摸牌的 draw_begin 不带 source");

	log.clear();
	first.draw(2, DrawReason::card_draw2, DrawPosition::top, second);
	r.check(log.size() == 1 && log.back() == "摸牌|player=0|source=1",
			"牌效果摸牌把出牌者作为 draw_begin 的 source");

	// 摸到的牌确实进手牌，且 draw_begin 只在摸牌前触发一次（不是每张牌一次）。
	log.clear();
	const std::size_t handBefore = first.handCount();
	const auto drawn = first.draw(2, DrawReason::skill);
	r.check(first.handCount() == handBefore + 2, "draw 把牌放进手牌");
	r.check(drawn.size() == 2, "draw 返回摸到的牌");
	r.check(log.size() == 1, "draw_begin 每次摸牌只触发一次");
}

// 技能触发顺序：载体顺序（players 数组）+ 载体内部注册顺序，且只看时机匹配。
void testTriggerOrder(TestResult& r, unool::tests::FakeWorld& world) {
	GameLogic game(world.view, world.decision, world.services);
	initBlankGame(game, world);

	const auto players = game.getPlayers();
	std::vector<std::string> log;
	constexpr auto anybody = PassiveSkill::TriggerPlayer::anybody;
	constexpr auto at = PassiveSkill::TriggerTime::phase_draw_begin;

	players.at(0).get().addSkill(std::make_unique<RecordingPassive>("甲", at, log, anybody));
	players.at(0).get().addSkill(std::make_unique<RecordingPassive>("乙", at, log, anybody));
	players.at(1).get().addSkill(std::make_unique<RecordingPassive>("丙", at, log, anybody));

	game.launchPassiveSkills(at, PassiveSkill::Trigger{});
	r.check(log == std::vector<std::string>{
				 "甲|player=none|source=none",
				 "乙|player=none|source=none",
				 "丙|player=none|source=none" },
			"同一时机按载体顺序、载体内部按注册顺序触发");

	log.clear();
	game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_draw_end, PassiveSkill::Trigger{});
	r.check(log.empty(), "时机不匹配的被动不触发");
}

// 标准牌堆构成：牌堆数量表本身就是玩法的一部分，改一张就等于改玩法。
void testStandardPile(TestResult& r, unool::tests::FakeWorld& world) {
	GameLogic game(world.view, world.decision, world.services);
	const auto pile = Pile::standard(game);

	// 每色：0 两张、1~9 各三张、rev/skip 各三张、draw2 四张 = 39，四色 156；
	// 黑牌：万能 9 + +4 7 = 16。合计 172。
	r.check(pile->count() == 172, "标准牌堆 172 张");
	for (const Card::Color color : Card::fourColors) {
		const auto n = std::ranges::count_if(
			*pile, [color](const std::unique_ptr<Card>& c) { return c.get()->getColor() == color; });
		r.check(n == 39, "每种颜色 39 张");
	}
	const auto zeros = std::ranges::count_if(
		*pile, [](const std::unique_ptr<Card>& c) { return c.get()->getName() == Card::Name::number_0; });
	r.check(zeros == 8, "数字 0 每种颜色两张");

	// 秘技牌与紫牌只由角色技能发放，绝不能出现在标准牌堆里。
	bool hasSecret = false;
	bool hasPurple = false;
	for (const auto& card : *pile) {
		if (card.get()->isSecret()) hasSecret = true;
		if (card.get()->getColor() == Card::Color::purple) hasPurple = true;
	}
	r.check(!hasSecret && !hasPurple, "标准牌堆不含秘技牌与紫牌");
}

// 弃牌堆回收：牌堆摸空时先回收再取，回收要洗牌并重置弃牌来源。
void testPileRecycle(TestResult& r, unool::tests::FakeWorld& world) {
	GameLogic game(world.view, world.decision, world.services);
	Pile pile(game);
	Pile discard(game);

	// 牌堆空、弃牌堆 3 张：take_front 先把弃牌堆回收成新牌堆，再取出 1 张。
	for (int i = 0; i < 3; ++i) {
		auto card = Card::make(Card::Color::red, Card::Name::number_5);
		card->setDiscardReason(Card::DiscardReason::discard);
		discard.push_back(std::move(card));
	}
	const auto taken = pile.take_front(discard);
	r.check(taken != nullptr, "牌堆空时先回收弃牌堆再取牌");
	r.check(discard.empty(), "回收后弃牌堆清空");
	r.check(pile.count() == 2, "回收 3 张、取出 1 张，牌堆剩 2 张");

	bool reasonsCleared = true;
	for (const auto& card : pile) {
		if (card.get()->getDiscardReason() != Card::DiscardReason::none) reasonsCleared = false;
	}
	r.check(reasonsCleared, "回收进牌堆的牌重置弃牌来源");

	// 两边都空：必须抛错，而不是返回空指针让调用方解引用。
	Pile emptyPile(game);
	Pile emptyDiscard(game);
	bool threw = false;
	try {
		(void)emptyPile.take_back(emptyDiscard);
	}
	catch (const std::runtime_error&) {
		threw = true;
	}
	r.check(threw, "牌堆与弃牌堆都空时取牌抛 runtime_error");
}

// 出牌合法性：颜色 / 牌名 / 万能牌三条放行规则 + 秘技牌特例。
void testCanUse(TestResult& r, unool::tests::FakeWorld& world) {
	GameLogic game(world.view, world.decision, world.services);
	initBlankGame(game, world);
	Player& player = game.getPlayers().at(0).get();

	game.setCurrentColor(Card::Color::red);
	game.setCurrentName(Card::Name::number_5);
	r.check(player.canUse(*Card::make(Card::Color::red, Card::Name::number_7)), "同颜色可出");
	r.check(player.canUse(*Card::make(Card::Color::blue, Card::Name::number_5)), "同牌名可出");
	r.check(!player.canUse(*Card::make(Card::Color::blue, Card::Name::number_7)),
			"颜色与牌名都不同不可出");
	r.check(player.canUse(*Card::make(Card::Color::black, Card::Name::wild_pal)), "万能牌始终可出");

	// 当前颜色为「无」（开局或万能牌结算前）：任意牌都可出。
	game.setCurrentColor(Card::Color::no);
	r.check(player.canUse(*Card::make(Card::Color::blue, Card::Name::number_7)),
			"当前颜色为无时任意牌可出");

	// 秘技牌可跟任意数字牌；【过载】是唯一恒不可打出的牌。
	game.setCurrentColor(Card::Color::red);
	game.setCurrentName(Card::Name::number_3);
	r.check(player.canUse(*Card::make(Card::Color::no, Card::Name::secret_毒酒)),
			"秘技牌可跟任意数字牌");
	r.check(!player.canUse(*Card::make(Card::Color::no, Card::Name::secret_过载)),
			"秘技牌【过载】不可打出");
}

} // namespace

unool::tests::TestResult runGameLogicTests() {
	unool::tests::FakeWorld world;
	// Character / utils 的静态工具与随机门面都经服务定位器取端口，测试台必须先装配。
	unool::core::installServices(world.services);

	unool::tests::TestResult r;
	testDrawSource(r, world);
	testTriggerOrder(r, world);
	testStandardPile(r, world);
	testPileRecycle(r, world);
	testCanUse(r, world);
	return r;
}
