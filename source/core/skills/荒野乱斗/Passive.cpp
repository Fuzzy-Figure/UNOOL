#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
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

