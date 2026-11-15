#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"
#include <algorithm>
#include <functional>
#include <map>
#include <numeric>
#include <optional>

// ==================== 技能：爆射 ====================
bool 爆射::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	std::size_t myCount = carrier.handCount();
	for (const Player& other : game.getPlayersExcludeId(carrier.getId())) {
		if (myCount == other.handCount() * 2) return true;
	}
	return false;
}

bool 爆射::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	// 1. 弃置至多两张牌（forced=false，玩家可随时取消，已弃的牌保留）
	carrier.chooseToDiscard("【爆射】弃置至多两张牌", 2, false);

	// 2. 选择一名其他角色
	auto targetOpt = carrier.choosePlayer("【爆射】选择一名角色造成6点伤害", true);
	if (!targetOpt.has_value()) {
		std::println("<技能> {}发动爆射，但未选择目标", carrier.displayName());
		game.broadcastState();
		return true;
	}
	Player& target = targetOpt.value().get();

	// 3. 造成6点伤害
	target.damage(6, carrier);
	std::println("<技能> {}发动爆射，对{}造成6点伤害", carrier.displayName(), target.displayName());
	game.broadcastState();
	return true;
}


// ==================== 技能：灵爆 ====================
bool 灵爆::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//拥有幽灵标记期间不计数
	return !carrier.hasMark("幽灵");
}

bool 灵爆::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	++(*playCount);
	std::println("<技能> {}灵爆计数：{}/3", carrier.displayName(), *playCount);
	if (*playCount >= 3) {
		carrier.addMark("幽灵");
		*playCount = 0;
		std::println("<技能> {}灵爆触发，获得\"幽灵\"标记", carrier.displayName());
	}
	game.broadcastState();
	return true;
}

void 灵爆::reset() {
	PassiveSkill::reset();
	*playCount = 0;
}


// ==================== 技能：灵爆_子 ====================
bool 灵爆_子::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	//有目标的牌：封禁/+2/+4
	bool isTargeted = c.isTargeted();
	return isTargeted && carrier.hasMark("幽灵");
}

bool 灵爆_子::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = carrier.next();
	carrier.removeMark("幽灵");
	target.damage(10, carrier);
	std::println("<技能> {}发动灵爆，移去\"幽灵\"标记并对{}造成10点伤害", carrier.displayName(), target.displayName());
	game.broadcastState();
	return true;
}


bool 加速::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isTargeted();
}

// ==================== 技能：加速 ====================
bool 加速::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	const std::size_t choice = carrier.ask(
		std::format("你成为了{}的目标，是否发动加速？", card.toString()),
		{ "发动", "不发动" },
		true
	);
	if (choice == 2) return false;

	//发动一次炫技
	//carrier.getSkill<炫技>().tryActivate(game, carrier);
	carrier.getSkill<炫技>().content(game, carrier);

	//令此牌无效
	card.cancelEffect();
	std::println("<技能> {}发动加速，发动炫技并令此牌无效", carrier.displayName());
	game.broadcastState();
	return true;
}

void 加速::reset() {
	setLimit(0);
	PassiveSkill::reset();
}


// ==================== 技能：返现 ====================
bool 返现::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	for (const auto& c : carrier.getHand()) {
		if (!c->is(Card::Color::yellow)) return true;
	}
	return false;
}

bool 返现::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//收集非黄色牌下标
	std::vector<std::size_t> nonYellowIndices;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		if (!carrier.getCardByIndex(i).is(Card::Color::yellow)) nonYellowIndices.push_back(i);
	}
	if (nonYellowIndices.empty()) return false;

	//从后往前重铸（避免索引偏移）
	std::ranges::sort(nonYellowIndices, std::greater{});
	for (std::size_t idx : nonYellowIndices) {
		carrier.recastByIndex(idx);
	}

	//回复等量体力
	carrier.recover(nonYellowIndices.size());

	std::println("<技能> {}发动返现，重铸了{}张非黄色牌，回复{}点体力", carrier.displayName(), nonYellowIndices.size(), nonYellowIndices.size());
	game.broadcastState();
	return true;
}


// ==================== 技能：尖刺 ====================
bool 尖刺::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.getHp() > 0;
}

bool 尖刺::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t dmg = trigger.getNumber();

	//选一名其他角色造成等量伤害
	auto targetOpt = carrier.chooseOtherPlayer(std::format("【尖刺】选择一名其他角色造成{}点伤害", dmg), true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	target.damage(dmg, carrier);
	carrier.recover(1);

	std::println("<技能> {}发动尖刺，对{}造成{}点伤害，回复1点体力", carrier.displayName(), target.displayName(), dmg);
	game.broadcastState();
	return true;
}


// ==================== 技能：弹暴 ====================
bool 弹暴::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isWild);
}

bool 弹暴::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//弃置一张万能牌；取消则返还可用次数（PassiveSkill::launch 会回退 count）
	auto discarded = carrier.chooseToDiscard("【弹暴】弃置一张万能牌", 1, false, &Card::isWild);
	if (discarded.empty()) return false;

	//X = 此技能本局发动次数（count 已由 launch 在 content 前自增）
	const std::size_t X = getCount();

	//找到【手枪】
	auto sp = carrier.findSkill<手枪>();
	if (!sp.has_value()) return false;
	手枪& handgun = sp.value().get();

	//循环 X 次：选角色 -> 造成伤害（取消则继续循环）
	for (std::size_t i = 0; i < X; ++i) {
		auto targetOpt = carrier.chooseOtherPlayer(
			std::format("【弹暴】选择一名其他角色造成{}点伤害（{}/{}）", handgun.damageValue, i + 1, X),
			false);
		if (!targetOpt.has_value()) continue;  //玩家取消，继续下一次循环
		Player& target = *targetOpt;

		target.damage(handgun.damageValue, carrier);
		std::println("<技能> {}发动弹暴，对{}造成{}点伤害", carrier.displayName(), target.displayName(), handgun.damageValue);
		game.broadcastState();
	}

	//【手枪】本局伤害值+1
	handgun.damageValue += 1;
	std::println("<技能> {}的【手枪】本局伤害值+1，当前={}", carrier.displayName(), handgun.damageValue);
	return true;
}


// ==================== 技能：星轨 ====================
bool 星轨::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//三次判定
	Card& c1 = carrier.judge();
	Card& c2 = carrier.judge();
	Card& c3 = carrier.judge();
	const std::string judgeResultStr = std::format("{}，{}，{}", c1.toString(), c2.toString(), c3.toString());

	if (c1.getType() == c2.getType() && c2.getType() == c3.getType()) {
		//类型全相同，引力可用次数+1
		carrier.getSkill<引力>().increaseLimit(1);
		carrier.markCharInfoDirty();
		carrier.hint(std::format("判定结果是：{}\n获得一次【引力】使用次数！", judgeResultStr));
		std::println("<技能> {}星轨判定三次类型相同，【引力】可用次数+1", carrier.displayName());

	}
	else {
		carrier.hint(std::format("判定结果是：{}\n残念だ、未获得【引力】使用次数", judgeResultStr));
		std::println("<技能> {}星轨判定类型不同", carrier.displayName());
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：引力_目标（引力重定向拦截器） ====================
bool 引力_目标::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isNumber();
}

bool 引力_目标::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//从弃牌堆移出这张牌（最新进入的在 front，index=0）
	//若弃牌堆已空或堆顶不是触发本次事件的牌（已被其他引力_目标取走），则跳过
	Pile& discardPile = game.getDiscardPile();
	if (discardPile.empty()) return false;
	if (&discardPile.front() != &trigger.getCard()) return false;
	auto cardPtr = discardPile.takeCardByIndex(0);
	carrier.gainCard(std::move(cardPtr));

	std::println("<技能> 引力将一张数字牌重定向给{}获得", carrier.displayName());
	game.broadcastState();
	return true;
}


// ==================== 技能：引力_清除目标 ====================
bool 引力_清除目标::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//检查是否有玩家身上存在引力_目标
	for (auto& p : game.getPlayers()) {
		if (p.get().findSkill<引力_目标>().has_value()) return true;
	}
	return false;
}

bool 引力_清除目标::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//清除所有玩家身上的引力_目标
	for (auto& p : game.getPlayers()) {
		Player& player = p.get();
		player.removeSkill<引力_目标>();
	}
	game.broadcastState();
	return true;
}


bool 铃铛::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handCount() != 1;
}
bool 铃铛::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//X = (count-1) % 3 + 1，count是launch自增后的值（第一次=1）
	std::size_t X = (getCount() - 1) % 3 + 1;
	std::size_t hc = carrier.handCount();
	if (hc == 0) X = 0;
	else if (hc < X) X = hc;

	//随机重铸X张手牌
	if (X > 0) {
		std::vector<std::size_t> indices(hc);
		std::iota(indices.begin(), indices.end(), std::size_t{ 0 });
		std::ranges::shuffle(indices, unool::random::rng);
		//从大到小排序前X个，从后往前重铸避免索引变化
		std::partial_sort(indices.begin(), indices.begin() + X, indices.end(), std::greater{});
		for (std::size_t i = 0; i < X; ++i) {
			carrier.recastByIndex(indices[i]);
		}
		std::println("<技能> {}铃铛重铸{}张手牌", carrier.displayName(), X);
	}

	//失去其他技能（5个衍生技）
	if (X == 3) {
		carrier.removeSkill<爆糖>();
		carrier.removeSkill<硬糖>();
		carrier.removeSkill<甘草>();
		carrier.removeSkill<跳糖>();
		carrier.removeSkill<薄荷>();

		//随机获得一个衍生技
		switch (const std::size_t r = unool::random::randomInt(0, 4); r) {
			case 0: carrier.addSkill(爆糖::make()); break;
			case 1: carrier.addSkill(硬糖::make()); break;
			case 2: carrier.addSkill(甘草::make()); break;
			case 3: carrier.addSkill(跳糖::make()); break;
			case 4: carrier.addSkill(薄荷::make()); break;
		}
	}
	game.broadcastState();
	return true;
}

//爆糖
bool 爆糖::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	//有目标的牌：封禁/+2/+4
	return c.is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4);
}
bool 爆糖::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = carrier.next();
	//5%最大体力伤害，向上取整
	std::size_t damage = (carrier.getMaxHp() * 5 + 99) / 100;  //向上取整
	target.damage(damage, carrier);
	std::println("<技能> {}爆糖对{}造成{}点伤害", carrier.displayName(), target.displayName(), damage);
	game.broadcastState();
	return true;
}

//薄荷
bool 薄荷::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t heal = unool::random::randomInt(1, 3);
	carrier.recover(heal);
	std::println("<技能> {}薄荷回复{}点体力", carrier.displayName(), heal);
	game.broadcastState();
	return true;
}


// ==================== 技能：健体 ====================
bool 健体::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& played = trigger.getCard();
	//打出前手中此牌名≥2张，等价于打出后手中仍有同名牌
	return carrier.handInclude([&](const Card& c) { return c.is(played.getName()); });
}
bool 健体::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.recover(1);
	std::println("<技能> {}健体回复1点体力", carrier.displayName());
	game.broadcastState();
	return true;
}


// ==================== 技能：飞刃 ====================
bool 飞刃::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 飞刃::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto targetOpt = carrier.chooseOtherPlayer("【飞刃】令一名其他角色获得一个\"毒\"标记", false);
	if (!targetOpt.has_value()) return false;
	targetOpt.value().get().addMark("毒");
	std::println("<技能> {}飞刃令{}获得一个毒标记", carrier.displayName(), targetOpt.value().get().displayName());
	return true;
}

// ==================== 技能：淬毒 ====================
bool 淬毒::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return true;
}
bool 淬毒::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//每回合结束 tick+1，作为全局回合计数
	++tick;

	Player& player = trigger.getPlayer();
	if (player.hasMark("毒")) {
		std::size_t dmg = player.getMarkCount("毒");
		player.damage(dmg, carrier);
		std::println("<技能> {}淬毒令{}受到{}点伤害", carrier.displayName(), player.displayName(), dmg);
	}
	return true;
}

void 淬毒::tryDecay(GameLogic& game) {
	game.forEachPlayer([this](Player& p) {
		if (!p.hasMark("毒")) return;
		std::size_t id = p.getId();
		auto it = lastChangeTick.find(id);
		if (it == lastChangeTick.end()) {
			//首次记录，开始计时
			lastChangeTick[id] = tick;
			return;
		}
		if (tick - it->second >= 10) {
			//满五回合未叠加，减1（不重置计时，后续每回合继续减）
			std::size_t before = p.getMarkCount("毒");
			p.removeMark("毒");
			std::println("<淬毒> {}的毒标记10回合未叠加，减少一个（剩余{}）", p.displayName(), (before - 1));
		}
	});
}

void 淬毒::reset() {
	PassiveSkill::reset();
	tick = 0;
	lastChangeTick.clear();
}

// ==================== 子技能：淬毒_感毒 ====================
bool 淬毒_感毒::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.hasMark() && trigger.getMark() == "毒";
}
bool 淬毒_感毒::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (auto qd = carrier.findSkill<淬毒>(); qd.has_value()) {
		qd->get().resetPoisonTimer(trigger.getPlayer().getId());
	}
	return true;
}

// ==================== 子技能：淬毒_衰减 ====================
bool 淬毒_衰减::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (auto qd = carrier.findSkill<淬毒>(); qd.has_value()) {
		qd->get().tryDecay(game);
	}
	return true;
}


// ==================== 技能：重锤 ====================
bool 重锤::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.getHp() > 100;
}
bool 重锤::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//失去10%当前体力（向下取整）
	std::size_t loss = unool::math::floor(carrier.getHp() * 0.1);
	carrier.damage(loss, carrier);

	//造成5%已损体力点伤害（向上取整），已损体力在失去之后计算
	Character::hp_t lostHp = carrier.getMaxHp() - carrier.getHp();
	std::size_t dmg = unool::math::ceil(lostHp * 0.03);

	auto targetOpt = carrier.chooseOtherPlayer(std::format("【重锤】选择一名其他角色造成{}点伤害", dmg), true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	target.damage(dmg, carrier);
	std::println("<技能> {}发动重锤，失去{}点体力，对{}造成{}点伤害", carrier.displayName(), loss, target.displayName(), dmg);
	game.broadcastState();
	return true;
}

//================劲凉================
bool 劲凉::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	Card::Type type = card.getType();
	if (type == lastType) {
		++consecutive;
	}
	else {
		lastType = type;
		consecutive = 1;
	}

	if (consecutive % 3 == 0) {
		carrier.draw(1, DrawReason::skill);
		if (auto skill = carrier.findSkill<绝技>(); skill.has_value()) {
			skill->get().increaseLimit(1);
			carrier.markCharInfoDirty();
		}
		std::println("<技能> {}发动劲凉，连续打出{}张同类型牌，摸一张牌且【绝技】可用次数+1", carrier.displayName(), consecutive);
		game.broadcastState();
	}
	return true;
}

void 劲凉::reset() {
	PassiveSkill::reset();
	lastType = Card::Type::unknown;
	consecutive = 0;
}

//================绝技_额外摸牌================
bool 绝技_额外摸牌::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2);
}
bool 绝技_额外摸牌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.next().draw(1, DrawReason::skill);
	game.broadcastState();
	return true;
}

// ==================== 技能：地雷 ====================
bool 地雷::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return usedColors.size() < 5;
}
bool 地雷::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//计算剩余可选颜色
	std::vector<Card::Color> remaining;
	for (auto c : Card::fiveColors) {
		if (!usedColors.contains(c)) remaining.push_back(c);
	}
	auto colorOpt = carrier.chooseCardColor("选择一种颜色布置地雷", false, remaining);
	if (!colorOpt) return false;

	armedColor = colorOpt.value();
	usedColors.insert(colorOpt.value());
	if (usedColors.size() == 5) usedColors.clear();
	std::println("<技能> {}发动地雷，布置了{}色地雷", carrier.displayName(), colorOpt.value());
	game.broadcastState();
	return true;
}

bool 地雷_引爆::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (trigger.getPlayer() == carrier) return false;
	auto color = carrier.getSkill<地雷>().getArmedColor();
	if (!color.has_value()) return false;
	return trigger.getCard().getColor() == color.value();
}
bool 地雷_引爆::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();
	target.damage(1, carrier);
	std::println("<技能> {}的地雷引爆，{}受到1点伤害", carrier.displayName(), target.displayName());
	game.broadcastState();
	return true;
}

bool 地雷_撤雷::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.getSkill<地雷>().disarm();
	return true;
}

// ==================== 技能：冲撞 ====================
bool 冲撞_加速::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (carrier.getMarkCount("速度") < 10) {
		carrier.addMark("速度", 1);
		std::println("<技能> {}发动冲撞_加速，速度标记+1（当前{}/10）", carrier.displayName(), carrier.getMarkCount("速度"));
	}
	game.broadcastState();
	return true;
}

bool 冲撞_伤害::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isTargeted();
}

bool 冲撞_伤害::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = carrier.next();
	const std::size_t speed = carrier.getMarkCount("速度");
	const std::size_t dmg = 2 * speed;
	target.damage(dmg, carrier);
	std::println("<技能> {}发动冲撞_伤害，对{}造成{}点伤害（速度×2={}×2）", carrier.displayName(), target.displayName(), dmg, speed);
	game.broadcastState();
	return true;
}

bool 冲撞_清标记::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return !carrier.getHasUsed();
}

bool 冲撞_清标记::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.clearMark("速度");
	std::println("<技能> {}本回合未出牌，清空速度标记", carrier.displayName());
	game.broadcastState();
	return true;
}

bool 冲撞::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//主技能本体无事可做，逻辑由子技能承担
	return true;
}

// ==================== 技能：分身 ====================
bool 分身_获取::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//打出万能牌时获得「分身」
	return trigger.getCard().isWild();
}

bool 分身_获取::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.addMark("分身");
	std::println("<技能> {}打出万能牌，获得一个「分身」（当前{}个）",
				 carrier.displayName(), carrier.getMarkCount("分身"));
	game.broadcastState();
	return true;
}

bool 分身_发动::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (!carrier.hasMark("分身")) return false;
	//手牌中存在与刚打出的牌同牌名的牌
	const Card::Name name = trigger.getCard().getName();
	return carrier.handInclude([name](const Card& c) { return c.getName() == name; });
}

bool 分身_发动::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const Card::Name name = trigger.getCard().getName();

	//收集同名牌的索引
	std::vector<std::size_t> indices;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		if (carrier.getCardByIndex(i).getName() == name) indices.push_back(i);
	}
	if (indices.empty()) return false;

	const std::size_t choice = carrier.ask(
		std::format("【分身】是否移去一个「分身」，打出一张【{}】？", name),
		{ "发动", "不发动" }, false);
	if (choice != 1) return false;

	//多张同名牌时自选打出其中一张
	std::size_t chosen = indices.front();
	if (indices.size() > 1) {
		auto idxOpt = carrier.chooseCard(
			std::format("【分身】选择打出一张【{}】", name),
			[name](const Card& c) { return c.getName() == name; }, false);
		if (!idxOpt.has_value()) return false;
		chosen = idxOpt.value();
	}

	carrier.removeMark("分身", 1);
	std::println("<技能> {}移去一个「分身」，打出同名牌【{}】", carrier.displayName(), name);
	//无需符合跟牌规则
	carrier.useCardByIndex(chosen);
	return true;
}

bool 分身::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.addMark("分身");
	std::println("<技能> {}游戏开始时获得一个「分身」", carrier.displayName());
	game.broadcastState();
	return true;
}


// ==================== 技能：飞手_整轮 ====================
bool 飞手_整轮::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto opt = carrier.findSkill<飞手>();
	if (!opt.has_value()) return false;
	飞手& skill = opt.value().get();
	skill.usedLastRound = skill.usedThisRound;
	skill.usedThisRound = false;
	return true;
}

// ==================== 技能：筑丛_摸牌 ====================
std::vector<ref<Card>> 筑丛_摸牌::drawUntilGreen(GameLogic& game, Player& carrier) {
	std::vector<ref<Card>> result;
	while (true) {
		auto drawn = carrier.draw(1, DrawReason::skill);
		if (drawn.empty()) break;  //牌堆耗尽或摸牌被技能取消
		result.insert(result.end(), drawn.begin(), drawn.end());
		if (drawn.back().get().is(Card::Color::green)) break;
	}
	return result;
}

// ==================== 技能：筑丛_累计 ====================
bool 筑丛_累计::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	auto opt = carrier.findSkill<筑丛>();
	return opt.has_value() && opt.value().get().isAccumulating();
}

bool 筑丛_累计::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto opt = carrier.findSkill<筑丛>();
	if (!opt.has_value()) return false;
	筑丛& skill = opt.value().get();

	//绿色牌不计入累计
	if (trigger.getCard().is(Card::Color::green)) return true;

	skill.setNonGreenCount(skill.getNonGreenCount() + 1);
	if (skill.getNonGreenCount() >= 10) {
		skill.setNonGreenCount(0);
		skill.setAccumulating(false);
		skill.resetCount();  //视为未发动过，技能恢复可用
		std::println("<技能> {}累计打出十张非绿色牌，筑丛视为未发动过", carrier.displayName());
		carrier.hint("【筑丛】已累计十张非绿色牌，技能恢复可用");
	}
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}


// ==================== 新增荒野乱斗角色技能 ====================
namespace {
	//随机四色之一：Card::fourColors 是 constexpr 数组，randomGet 需要非 const 引用，故用下标取
	Card::Color randomPlainColor() {
		return Card::fourColors[unool::random::randomSize_t(0, Card::fourColors.size() - 1)];
	}

	//本次摸牌是否因【+2】/【+4】效果
	bool isPenaltyDraw(const PassiveSkill::Trigger& trigger) {
		return trigger.hasDrawReason()
			&& (trigger.getDrawReason() == DrawReason::card_draw2
				|| trigger.getDrawReason() == DrawReason::card_draw4);
	}
}

// ==================== 麦克【雷管】 ====================
bool 雷管::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && trigger.getCard().isWild();
}

bool 雷管::content(GameLogic& game, Player& carrier, Trigger&) {
	//「可获得」沿用上一批口径：弹确认，取消则不拿牌
	const std::size_t choice = carrier.ask(
		"【雷管】是否获得一张随机颜色的【+2】（额外对目标造成5点伤害）？",
		{ "获得", "不获得" }, false);
	if (choice != 1) return true;

	auto card = Card::make(randomPlainColor(), Card::Name::action_draw2);
	card->setExtraDamage(5);
	std::println("<技能> {}发动雷管，获得{}（附加5点伤害）", carrier.displayName(), *card);
	carrier.gainCard(std::move(card));
	game.broadcastState();
	return true;
}

// ==================== 妮塔【熊灵】 ====================
bool 熊灵_摸前::filter(const GameLogic&, const Player& carrier, const Trigger& trigger) const {
	return isPenaltyDraw(trigger)
		&& trigger.hasNumber()
		&& carrier.getShield() >= 30;
}

bool 熊灵_摸前::content(GameLogic&, Player& carrier, Trigger& trigger) {
	std::size_t& number = trigger.getNumber();
	const std::size_t halved = number / 2;
	std::println("<技能> {}的熊灵护盾已满，本次摸牌数由{}减半为{}",
				 carrier.displayName(), number, halved);
	number = halved;
	return true;
}

bool 熊灵_摸后::filter(const GameLogic&, const Player& carrier, const Trigger& trigger) const {
	return isPenaltyDraw(trigger) && carrier.getShield() < 30;
}

bool 熊灵_摸后::content(GameLogic& game, Player& carrier, Trigger&) {
	const std::size_t gain = carrier.getSkill<熊灵>().shieldGain;
	const std::size_t before = carrier.getShield();
	carrier.setShield(std::min<std::size_t>(30, before + gain));
	carrier.markCharInfoDirty();
	std::println("<技能> {}的熊灵获得{}点护盾（{} -> {}）",
				 carrier.displayName(), gain, before, carrier.getShield());
	game.broadcastState();
	return true;
}

bool 熊灵::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && trigger.getCard().is(Card::Type::action);
}

bool 熊灵::content(GameLogic&, Player& carrier, Trigger&) {
	++shieldGain;
	carrier.markCharInfoDirty();
	std::println("<技能> {}打出功能牌，熊灵本局每次摸牌护盾量提升为{}点",
				 carrier.displayName(), shieldGain);
	return true;
}

// ==================== 布洛克【火箭】 ====================
bool 火箭::filter(const GameLogic& game, const Player&, const Trigger&) const {
	return game.getRoundCount() == 10;
}

bool 火箭::content(GameLogic& game, Player& carrier, Trigger&) {
	//凭空生成，不从牌堆取：第十轮时牌堆可能已无足够的【变色】
	carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	carrier.gainCard(Card::make(randomPlainColor(), Card::Name::action_skip));
	carrier.gainCard(Card::make(randomPlainColor(), Card::Name::action_skip));
	std::println("<技能> 第十轮结束，{}发动火箭，获得两张【变色】与两张随机颜色的【封禁】",
				 carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 巴利【掷酒】 ====================
bool 掷酒::content(GameLogic& game, Player& carrier, Trigger&) {
	carrier.gainCard(Card::make(Card::Color::green, Card::Name::secret_毒酒));
	carrier.gainCard(Card::make(Card::Color::green, Card::Name::secret_药酒));
	std::println("<技能> {}发动掷酒，获得秘技牌【毒酒】和【药酒】各一张",
				 carrier.displayName());
	game.broadcastState();
	return true;
}


// ==================== 新增荒野乱斗角色技能（第二批） ====================

// ==================== 技能：烧炉_过载判定 ====================
bool 烧炉_过载判定::filter(const GameLogic&, const Player& carrier, const Trigger&) const {
	const std::size_t total{carrier.handCount()};
	std::size_t overloads{};
	carrier.getHand().forEach([&](Card& c) {
		if (c.getName() == Card::Name::secret_过载) ++overloads;
	});
	//「此类牌」=【过载】：至少有一张可弃，且其数量不少于手中其他牌
	return overloads > 0 && overloads >= total - overloads;
}

bool 烧炉_过载判定::content(GameLogic& game, Player& carrier, Trigger&) {
	std::optional<std::size_t> index{};
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		if (carrier.getHand().getCardByIndex(i).getName() == Card::Name::secret_过载) {
			index = i;
			break;
		}
	}
	if (!index.has_value()) return false;
	carrier.discardByIndex(index.value());
	std::println("<技能> {}弃置一张【过载】", carrier.displayName());

	auto targetOpt = carrier.chooseOtherPlayer("【过载】选择一名其他角色", false);
	if (!targetOpt.has_value()) {
		std::println("<技能> {}未选择【过载】目标，不造成伤害", carrier.displayName());
		game.broadcastState();
		return true;
	}
	Player& target = targetOpt.value().get();
	target.damage(10, carrier);
	std::println("<技能> {}的【过载】对{}造成10点伤害", carrier.displayName(), target.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：烧炉 ====================
bool 烧炉::filter(const GameLogic& game, const Player&, const Trigger&) const {
	const std::size_t round{game.getRoundCount()};
	return round > 0 && round % 5 == 0;
}

bool 烧炉::content(GameLogic& game, Player& carrier, Trigger&) {
	carrier.gainCard(Card::make(Card::Color::green, Card::Name::secret_过载));
	std::println("<技能> {}发动烧炉（第{}轮），获得一张秘技牌【过载】",
				 carrier.displayName(), game.getRoundCount());
	game.broadcastState();
	return true;
}

// ==================== 技能：飞弹 ====================
bool 飞弹::filter(const GameLogic&, const Player& carrier, const Trigger& trigger) const {
	//只放大以自己为来源的伤害；trigger.number 在 damage_begin 绑定到实际伤害值
	return trigger.hasSource() && trigger.hasNumber()
		&& trigger.getSource() == carrier;
}

bool 飞弹::content(GameLogic&, Player& carrier, Trigger& trigger) {
	trigger.getNumber() += 1;
	std::println("<技能> {}的飞弹令此伤害+1", carrier.displayName());
	return true;
}

// ==================== 技能：冲爆 ====================
bool 冲爆::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	if (!trigger.hasCards()) return false;
	for (const auto& cardRef : trigger.getCards()) {
		if (cardRef.get().isNumber()) return true;
	}
	return false;
}

bool 冲爆::content(GameLogic& game, Player& carrier, Trigger&) {
	auto others = game.getPlayersExcludeId(carrier.getId());
	if (others.empty()) return false;
	Player& target = unool::random::randomGet(others).get();
	target.damage(1, carrier);
	std::println("<技能> {}发动冲爆，随机对{}造成1点伤害", carrier.displayName(), target.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：功夫 ====================
bool 功夫::filter(const GameLogic&, const Player& carrier, const Trigger& trigger) const {
	if (!trigger.hasCards()) return false;
	if (!trigger.getCard().is(Card::Color::red)) return false;
	//打出红牌之后手牌数>10 时整个技能不触发
	return carrier.handCount() <= 10;
}

bool 功夫::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& played = trigger.getCard();
	Player& down = carrier.next();
	down.ban(carrier, played);
	carrier.draw(1, DrawReason::skill);
	std::println("<技能> {}发动功夫，红牌附带封禁封住{}并摸一张牌",
				 carrier.displayName(), down.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：连踢 ====================
bool 连踢::content(GameLogic& game, Player& carrier, Trigger&) {
	if (!carrier.getHasUsed()) {
		streak = 0;
		return false;
	}
	++streak;
	if (streak < 4) return false;
	streak = 0;

	//「可」字效果：满足条件时才弹确认
	const std::size_t choice{carrier.ask("是否发动 [连踢]？", { "发动", "不发动" }, true)};
	if (choice == 2) return false;
	auto targetOpt = carrier.chooseOtherPlayer("【连踢】选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	carrier.draw(2, DrawReason::skill);
	for (std::size_t i = 0; i < 4; ++i) target.damage(2, carrier);
	std::println("<技能> {}发动连踢，摸两张牌并对{}造成4次2点伤害",
				 carrier.displayName(), target.displayName());
	game.broadcastState();
	return true;
}

void 连踢::reset() {
	PassiveSkill::reset();
	streak = 0;
}

// ==================== 技能：蛮冲_记录 ====================
bool 蛮冲_记录::filter(const GameLogic&, const Player& carrier, const Trigger&) const {
	const auto skill = carrier.findSkill<蛮冲>();
	return skill.has_value() && skill.value().get().recording;
}

bool 蛮冲_记录::content(GameLogic&, Player& carrier, Trigger&) {
	const auto skill = carrier.findSkill<蛮冲>();
	if (!skill.has_value()) return false;
	//phase_begin 时 ban() 造成的封禁尚未被 turn() 的 unban() 清除
	skill.value().get().bannedThisTurn = carrier.isBanned();
	return true;
}

// ==================== 技能：蛮冲_结算 ====================
bool 蛮冲_结算::filter(const GameLogic&, const Player& carrier, const Trigger&) const {
	const auto skill = carrier.findSkill<蛮冲>();
	return skill.has_value() && skill.value().get().recording;
}

bool 蛮冲_结算::content(GameLogic& game, Player& carrier, Trigger&) {
	auto skillOpt = carrier.findSkill<蛮冲>();
	if (!skillOpt.has_value()) return false;
	蛮冲& skill = skillOpt.value().get();

	//空过（本回合未出牌）或被封禁：记录作废，限定技保持已消耗
	if (skill.bannedThisTurn || !carrier.getHasUsed()) {
		skill.recording = false;
		std::println("<技能> {}的蛮冲记录中断（空过或被封禁），本局限定技已消耗", carrier.displayName());
		return true;
	}

	if (skill.turnsLeft > 0) --skill.turnsLeft;
	if (skill.turnsLeft > 0) {
		std::println("<技能> {}的蛮冲累计{}回合，还需{}回合", carrier.displayName(),
					 4 - skill.turnsLeft, skill.turnsLeft);
		return true;
	}

	//四个回合全部达标
	carrier.draw(4, DrawReason::skill);
	auto targetOpt = carrier.chooseOtherPlayer("【蛮冲】选择一名其他角色", false);
	if (targetOpt.has_value()) {
		const std::size_t dmg{10 * skill.activationCount};
		Player& target = targetOpt.value().get();
		target.damage(dmg, carrier);
		std::println("<技能> {}的蛮冲对{}造成{}点伤害", carrier.displayName(), target.displayName(), dmg);
	}
	skill.resetCount();  //视为未发动过，本局可再次发动；X 单独累计不清零
	skill.recording = false;
	std::println("<技能> {}的蛮冲结算完成，此技能视为未发动过", carrier.displayName());
	game.broadcastState();
	return true;
}


// ==================== 新增荒野乱斗角色技能（第三批） ====================

// ==================== 技能：指虎_弃牌 ====================
bool 指虎_弃牌::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	if (!trigger.hasCards()) return false;
	const Card& card = trigger.getCard();
	return card.getName() == Card::Name::secret_指虎 || card.isWild();
}

bool 指虎_弃牌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	auto& main = carrier.getSkill<指虎>();
	if (card.getName() == Card::Name::secret_指虎) {
		main.unlocked = true;
		carrier.hint("【指虎】已解锁：本局每打出一张万能牌可弃置一张手牌（最多3张）");
		std::println("<技能> {}打出【指虎】，解锁万能牌弃牌效果", carrier.displayName());
		game.broadcastState();
		return true;
	}
	if (!main.unlocked || main.discarded >= 3 || carrier.handCount() == 0) return false;
	const auto index = carrier.chooseCard(
		std::format("【指虎】选择一张手牌弃置（↓取消），本局还可弃{}张", 3 - main.discarded),
		[](const Card&) { return true; }, false);
	if (!index.has_value()) {
		std::println("<技能> {}取消【指虎】弃牌", carrier.displayName());
		return false;
	}
	carrier.discardByIndex(index.value());
	++main.discarded;
	std::println("<技能> {}因【指虎】弃置一张牌（本局{}/3）", carrier.displayName(), main.discarded);
	game.broadcastState();
	return true;
}

// ==================== 技能：指虎 ====================
bool 指虎::content(GameLogic& game, Player& carrier, Trigger&) {
	carrier.gainCard(Card::make(Card::Color::green, Card::Name::secret_指虎));
	std::println("<技能> {}发动指虎，获得一张专属牌【指虎】", carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：镐机 ====================
bool 镐机::content(GameLogic& game, Player& carrier, Trigger&) {
	setForced(true);  //契定技：发动一次后变为锁定技
	std::map<Card::Color, std::vector<std::size_t>> byColor{};
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		byColor[carrier.getCardByIndex(i).getColor()].push_back(i);
	}
	if (byColor.size() <= 1) {
		const std::size_t count = carrier.handCount();
		for (std::size_t k = count; k > 0; --k) {
			carrier.recastByIndex(k - 1);
		}
		carrier.recover(5);
		std::println("<技能> {}发动镐机：仅剩一种颜色，重铸全部{}张手牌并回复5点体力",
					 carrier.displayName(), count);
	}
	else {
		std::vector<Card::Color> colors{};
		for (const auto& [color, indices] : byColor) colors.push_back(color);
		const auto picked = unool::random::randomGet(colors, 2);
		const std::size_t first = unool::random::randomGet(byColor[picked[0].get()]);
		const std::size_t second = unool::random::randomGet(byColor[picked[1].get()]);
		std::vector<std::size_t> indices{first, second};
		std::ranges::sort(indices, std::greater{});
		for (const std::size_t idx : indices) carrier.recastByIndex(idx);
		std::println("<技能> {}发动镐机：随机重铸两张颜色不同的牌", carrier.displayName());
	}
	game.broadcastState();
	return true;
}

// ==================== 技能：旋转 ====================
bool 旋转::filter(const GameLogic& game, const Player&, const Trigger&) const {
	const std::size_t round{game.getRoundCount()};
	return round == 3 || round == 4 || round == 5;
}

bool 旋转::content(GameLogic& game, Player& carrier, Trigger&) {
	carrier.getSkill<镐机>().increaseLimit(1);
	carrier.markCharInfoDirty();
	carrier.hint(std::format("【旋转】第{}轮，【镐机】可发动次数+1", game.getRoundCount()));
	std::println("<技能> {}在第{}轮发动旋转，【镐机】次数+1",
				 carrier.displayName(), game.getRoundCount());
	game.broadcastState();
	return true;
}


// ==================== 新增荒野乱斗角色技能（第四批） ====================

// ==================== 技能：炮台_轰击 ====================
bool 炮台_轰击::filter(const GameLogic& game, const Player& carrier, const Trigger&) const {
	return carrier.hasMark("炮台") && !game.getPlayersExcludeId(carrier.getId()).empty();
}

bool 炮台_轰击::content(GameLogic& game, Player& carrier, Trigger&) {
	auto targetOpt = carrier.chooseOtherPlayer("【炮台】选择一名其他角色开火", true);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();
	const std::size_t times{static_cast<std::size_t>(unool::random::randomInt(2, 3))};
	for (std::size_t i = 0; i < times; ++i) target.damage(1, carrier);
	std::println("<技能> {}的炮台对{}造成{}次1点伤害",
				 carrier.displayName(), target.displayName(), times);
	game.broadcastState();
	return true;
}

// ==================== 技能：炮台_无效 ====================
bool 炮台_无效::filter(const GameLogic&, const Player& carrier, const Trigger& trigger) const {
	if (!carrier.hasMark("炮台") || !trigger.hasCards()) return false;
	return trigger.getCard().isWild();
}

bool 炮台_无效::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	const std::size_t choice{player.ask(
		std::format("{}拥有\"炮台\"，是否令此万能牌无效并销毁\"炮台\"？", carrier.displayName()),
		{ "令此牌无效并销毁炮台", "正常结算" },
		true
	)};
	if (choice != 1) return false;
	trigger.getCard().cancelEffect();
	carrier.clearMark("炮台");
	carrier.hint("【炮台】已被销毁");
	std::println("<技能> {}选择令自己打出的万能牌无效，并销毁{}的炮台",
				 player.displayName(), carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：炮台 ====================
bool 炮台::content(GameLogic& game, Player& carrier, Trigger&) {
	if (!carrier.getHasUsed()) {
		streak = 0;
		return false;
	}
	++streak;
	if (streak < 5) return false;
	streak = 0;
	if (carrier.hasMark("炮台")) return false;
	carrier.addMark("炮台");
	carrier.hint("【炮台】已部署：回合开始时开火，他人可用万能牌将其销毁");
	std::println("<技能> {}连续五回合出牌，获得一个\"炮台\"", carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：征讨 ====================
bool 征讨::filter(const GameLogic&, const Player& carrier, const Trigger& trigger) const {
	return trigger.hasSource() && trigger.hasNumber()
		&& trigger.getSource() == carrier
		&& trigger.getNumber() > 0;
}

bool 征讨::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();
	const int hp{target.getHp()};
	const std::size_t currentHp{hp > 0 ? static_cast<std::size_t>(hp) : 0};
	const std::size_t dmg{(currentHp * 33 + 99) / 100};   //当前体力33%，向上取整
	const std::size_t heal{(dmg + 1) / 2};                //此伤害50%，向上取整
	//先结算额外伤害；本技能 limit=1，嵌套的 damage_begin 因已达次数不会再次触发
	target.damage(dmg, carrier);
	carrier.recover(heal);
	carrier.hint(std::format("【征讨】额外造成{}点伤害并回复{}点体力", dmg, heal));
	std::println("<技能> {}发动征讨，先对{}造成{}点伤害并回复{}点体力",
				 carrier.displayName(), target.displayName(), dmg, heal);
	game.broadcastState();
	return true;
}

// ==================== 新增荒野乱斗角色技能（第五批） ====================

// ==================== 技能：溯流 ====================
bool 溯流::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards();
}

bool 溯流::content(GameLogic& game, Player& carrier, Trigger&) {
	++played;
	if (played < 2) return false;
	played -= 2;   //每累计两张牌给一次机会

	const auto players = game.getPlayers();
	Pile& discardPile = game.getDiscardPile();
	std::vector<std::string> options{};
	std::vector<std::size_t> indices{};
	for (std::size_t i = 0; i < discardPile.count(); ++i) {
		const Card& card = discardPile.getCardByIndex(i);
		if (card.getDiscardReason() == Card::DiscardReason::discard) continue;
		const auto source = card.getDiscardSourceId();
		if (source.has_value() && *source < players.size()
			&& players[*source].get() == carrier) continue;   //来源是自己
		options.push_back(card.toString());
		indices.push_back(i);
	}
	if (options.empty()) return false;

	const std::size_t choice = carrier.ask(
		"【溯流】选择弃牌堆中一张牌获得（↓取消）", options, false);
	if (choice == 0 || choice > indices.size()) return false;

	auto card = discardPile.takeCardByIndex(indices[choice - 1]);
	const std::string obtained = card->toString();
	carrier.gainCard(std::move(card));
	//获得后必须弃置一张牌（不可取消）
	carrier.chooseToDiscard("【溯流】弃置一张手牌", 1, true);
	carrier.hint(std::format("【溯流】获得{}并弃置一张牌", obtained));
	std::println("<技能> {}发动溯流，从弃牌堆获得{}并弃置一张牌", carrier.displayName(), obtained);
	game.broadcastState();
	return true;
}

// ==================== 技能：指虎_销毁 ====================
bool 指虎_销毁::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	return trigger.hasCards() && trigger.getCard().getName() == Card::Name::secret_指虎;
}

bool 指虎_销毁::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	Pile& pile = game.getDiscardPile();
	for (std::size_t i = 0; i < pile.count(); ++i) {
		if (&pile.getCardByIndex(i) != &card) continue;
		//不直接释放：putCardToDiscardPile 返回的引用与后续 use_card_end 仍指向此对象，
		//先移出弃牌堆并寄存在主技能里，每局 reset 时统一释放。
		carrier.getSkill<指虎>().destroyed.push_back(pile.takeCardByIndex(i));
		std::println("<技能> 专属牌【指虎】进入弃牌堆后销毁");
		game.broadcastState();
		return true;
	}
	return false;
}

// ==================== 技能：指虎_专属 ====================
bool 指虎_专属::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	if (!trigger.hasCards() || !trigger.hasPlayer()) return false;
	for (const auto& rc : trigger.getCards()) {
		if (rc.get().getName() == Card::Name::secret_指虎) return true;
	}
	return false;
}

bool 指虎_专属::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional<ref<Card>> found{};
	for (const auto& rc : trigger.getCards()) {
		if (rc.get().getName() == Card::Name::secret_指虎) { found = rc; break; }
	}
	if (!found.has_value()) return false;
	Card& target = found->get();
	Player& owner = trigger.getPlayer();
	for (std::size_t i = 0; i < owner.handCount(); ++i) {
		if (&owner.getCardByIndex(i) != &target) continue;
		carrier.getSkill<指虎>().destroyed.push_back(owner.takeCardByIndex(i));
		std::println("<技能> 专属牌【指虎】被{}获得后销毁", owner.displayName());
		game.broadcastState();
		return true;
	}
	return false;
}
