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

#include "core/GameLogic.h"
#include "core/Player.h"
#include "core/Skill.h"

#include <cstdio>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

struct Result {
	int passed = 0;
	int failed = 0;
	void check(bool ok, const char* name) {
		if (ok) {
			++passed;
			return;
		}
		++failed;
		std::fprintf(stderr, "FAIL: %s\n", name);
	}
};

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
void testDrawSource(Result& r, unool::tests::FakeWorld& world) {
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
void testTriggerOrder(Result& r, unool::tests::FakeWorld& world) {
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

} // namespace

GameLogicTestResult runGameLogicTests() {
	unool::tests::FakeWorld world;
	// Character / utils 的静态工具与随机门面都经服务定位器取端口，测试台必须先装配。
	unool::core::installServices(world.services);

	Result r;
	testDrawSource(r, world);
	testTriggerOrder(r, world);
	return { r.passed, r.failed };
}
