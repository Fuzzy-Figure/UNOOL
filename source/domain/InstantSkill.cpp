#include "PassiveSkill.h"
#include "InstantSkill.h"
#include "TransformSkill.h"
#include "GameLogic.h"
#include "utils.h"


bool 装弹::filter(const GameLogic& game, const Player& carrier) const {
	const Hand& hand = carrier.getHand();
	return hand.count() <= 9
		&& hand.exclude([](const Card& c) {
		return c.is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4);
	});
}

bool 装弹::content(GameLogic& game, Player& carrier) {
	Hand& hand = carrier.getHand();
	std::vector<ref<Card>> cards;
	std::ranges::sample(
		hand | std::views::transform([](const auto& cardPtr) { return std::ref(*cardPtr); }),
		std::back_inserter(cards),
		unool::math::floor(hand.count() / 2.0),
		unool::random::rng
	);
	for (Card& c : cards) {
		c.set(Card::randomCard(&Card::isNotNumber));
	}
	return true;
}
bool 徒步::content(GameLogic& game, Player& carrier) {
	Player::RecastResult result = carrier.chooseToRecast("[徒步] 重铸一张牌", 1, false);
	if (result.discarded.size() == 0) return false;

	carrier.recover(1);
	if (result.discarded.front().get().isNumber()) {
		carrier.damage(getCount(), carrier);
	}
	return true;
}


// ==================== 技能：招待 ====================
bool 招待::content(GameLogic& game, Player& carrier) {
	std::optional targetOpt = carrier.chooseOtherPlayer("[招待] 选择一名其他角色", true);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	carrier.chooseToGive(std::format("选择一张手牌交给{}", target.characterName()), target, true);
	return true;
}




bool 摘罩::content(GameLogic& game, Player& carrier) {
	std::string recordStr;
	for (const Card::Name& name : record) {
		recordStr += std::format("{},", name);
	}
	//1. 展示一张未展示过点数的数字牌
	auto cardOpt = carrier.chooseToShow(std::format("[摘罩] 展示一张数字牌\n已展示：{}", recordStr), false, [this](const Card& c) {
		return c.isNumber() && !record.contains(c.getName());
	});
	if (!cardOpt.has_value()) return false;
	Card& card = cardOpt.value().get();
	Card::Name point = card.getName();

	//2. 选一名其他角色，令其展示相同点数的牌
	auto targetOpt = carrier.chooseOtherPlayer("[摘罩] 选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();
	record.insert(point);  //技能确认发动，记录已展示点数

	//3. 检查目标是否有相同点数的牌，有则令其展示
	bool targetShowed = false;
	for (std::size_t i = 0; i < target.handCount(); ++i) {
		if (target.getCardByIndex(i).getName() == point) {
			targetShowed = true;
			break;
		}
	}
	if (targetShowed) {
		auto otherCardOpt = target.chooseToShow(
			std::format("[摘罩] 展示一张{}", point), false,
			[point](const Card& c) { return c.getName() == point; }
		);
		targetShowed = otherCardOpt.has_value();
	}

	//4. 若目标未展示牌，carrier 可任意更改所展示牌的颜色
	if (!targetShowed) {
		auto color = carrier.chooseCardColor("[摘罩] 更改你展示的牌的颜色", false);
		if (color.has_value()) {
			card.setColor(color.value());
			game.broadcastState();
		}
	}
	return true;
}

void 摘罩::reset() {
	ActiveSkill::reset();
	record.clear();
}


// ==================== 技能：还击 ====================
bool 还击::content(GameLogic& game, Player& carrier) {
	//1. 选一名其他角色
	auto targetOpt = carrier.chooseOtherPlayer("[还击] 选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	const std::size_t n = target.handCount();
	if (n == 0) {
		std::println("<还击> 目标无手牌");
		return false;
	}

	//2. 取半数手牌（向上取整，至少保留一张）
	std::size_t takeCount = unool::math::ceil(static_cast<double>(n) * 0.5);
	if (takeCount >= n) takeCount = n - 1;
	if (takeCount == 0) return false;

	//3. 随机拿牌
	for (std::size_t i = 0; i < takeCount; ++i) {
		std::size_t idx = unool::random::randomSize_t(0, target.handCount() - 1);
		carrier.gainCard(target.takeCardByIndex(idx));
	}
	game.broadcastState();

	//4. 交还等量张牌（forced=true，强制完成义务）
	for (std::size_t i = 0; i < takeCount; ++i) {
		carrier.chooseToGive(
			std::format("[还击] 交还一张牌给{}（{}/{}）", target.characterName(), i + 1, takeCount),
			target, true
		);
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：舞爪 ====================
bool 舞爪::content(GameLogic& game, Player& carrier) {
	//弃置一张万能牌
	auto discarded = carrier.chooseCardsToDiscardPile("选择一张万能牌弃置", 1, false, &Card::isWild);
	if (discarded.empty()) return false;

	//逐张重铸所有数字牌
	std::size_t count = 0;
	while (carrier.handInclude(&Card::isNumber)) {
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			if (carrier.getCardByIndex(i).isNumber()) {
				carrier.recastByIndex(i);
				++count;
				break;
			}
		}
	}

	std::println("<技能> {}发动舞爪，弃置一张万能牌，重铸{}张数字牌", carrier.displayName(), count);
	game.broadcastState();
	return true;
}


// ==================== 技能：四霸 ====================
bool 四霸::filter(const GameLogic& game, const Player& carrier) const {
	std::size_t cnt = 0;
	const Hand& hand = carrier.getHand();
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].is(Card::Name::number_4)) ++cnt;
	}
	return cnt >= 3;
}

bool 四霸::content(GameLogic& game, Player& carrier) {
	carrier.chooseToDiscard("【四霸】弃置三张【4】", 3, true,
							[](const Card& c) { return c.is(Card::Name::number_4); });
	std::println("<技能> {}发动四霸，弃置了三张【4】", carrier.displayName());
	game.broadcastState();
	return true;
}


// ==================== 技能：装币 ====================
bool 装币::content(GameLogic& game, Player& carrier) {
	auto drawn = carrier.drawTo(18, DrawReason::skill);
	carrier.setDamageMultiplier(carrier.getDamageMultiplier() + 1);
	std::println("<技能> {}发动装币，摸{}张，伤害倍率提升至{}",
				 carrier.displayName(), drawn.size(), carrier.getDamageMultiplier());
	game.broadcastState();
	return true;
}


// ==================== 技能：幽愈 ====================
bool 幽愈::filter(const GameLogic& game, const Player& carrier) const {
	return carrier.hasMark("幽灵");
}

bool 幽愈::content(GameLogic& game, Player& carrier) {
	carrier.removeMark("幽灵");
	const std::size_t recoverValue = unool::math::floor(0.15 * (carrier.getMaxHp() - carrier.getHp()));
	carrier.recover(recoverValue);
	std::println("<技能> {}发动幽愈，移去\"幽灵\"标记并回复{}体力", carrier.displayName(), recoverValue);
	game.broadcastState();
	return true;
}


// ==================== 技能：炫技 ====================
bool 炫技::content(GameLogic& game, Player& carrier) {
	//选牌堆顶或底
	std::size_t pos = carrier.ask("【炫技】从牌堆顶或牌堆底摸两张", { "牌堆顶", "牌堆底" }, false);
	if (pos == 0) return false;  //玩家取消
	DrawPosition dp = (pos == 1) ? DrawPosition::top : DrawPosition::bottom;

	//摸两张（触发draw_begin/end）
	std::vector drawn = carrier.draw(2, DrawReason::skill, dp);

	//记录摸到两张牌的颜色
	Card::Color c1 = drawn[0].get().getColor();
	Card::Color c2 = drawn[1].get().getColor();

	//从所有手牌中选一张置于牌堆底
	auto putOpt = carrier.chooseCard("【炫技】将一张手牌置于牌堆底", [](const Card&) { return true; }, true);
	if (!putOpt.has_value()) return false;
	game.getPile().push_back(carrier.takeCardByIndex(*putOpt));
	std::println("<技能> {}发动炫技，摸2张并置1张手牌于牌堆底", carrier.displayName());

	//若两牌同色且手牌>=2，可弃两张令加速+1
	if (c1 == c2 && carrier.handCount() >= 2) {
		std::size_t choice = carrier.ask("【炫技】两牌同色，是否弃置两张令【加速】+1？",
										 { "弃两张", "不弃" }, false);
		if (choice == 1) {
			carrier.chooseToDiscard("【炫技】弃置两张牌", 2, true);
			if (auto sp = carrier.findSkill<加速>(); sp.has_value()) {
				auto& acc = sp.value().get();
				acc.increaseLimit(1);
				carrier.markCharInfoDirty();
				std::println("<技能> {}的【加速】可发动次数+1，当前={}", carrier.displayName(), acc.getLimit().value());
			}
		}
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：调羹 ====================
bool 调羹::filter(const GameLogic& game, const Player& carrier) const {
	return carrier.handInclude(&Card::isNumber);
}

bool 调羹::content(GameLogic& game, Player& carrier) {
	carrier.draw(1, DrawReason::skill);
	game.broadcastState();

	auto idxOpt = carrier.chooseCard("选择一张数字牌", &Card::isNumber, false);
	if (!idxOpt) return false;

	auto card = carrier.takeCardByIndex(*idxOpt);
	carrier.showCard(*card);
	std::println("<技能> {}展示{}", carrier.displayName(), *card);

	auto targetOpt = carrier.choosePlayer("【调羹】令一名角色获得此牌", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	std::size_t value = card->value();
	carrier.give(target, std::move(card));
	std::println("<技能> {}将牌交给{}", carrier.displayName(), target.displayName());

	target.recover(value);
	std::println("<技能> {}回复{}点体力", target.displayName(), value);

	game.broadcastState();
	return true;
}


// ==================== 技能：挥金 ====================
bool 挥金::filter(const GameLogic& game, const Player& carrier) const {
	if (carrier.handCount() < 3) return false;
	//手中有黄色牌且其类型未用过
	for (const auto& c : carrier.getHand()) {
		if (c->is(Card::Color::yellow) && !usedTypes.contains(c->getType())) return true;
	}
	return false;
}

bool 挥金::content(GameLogic& game, Player& carrier) {
	//选目标
	auto targetOpt = carrier.choosePlayer("【挥金】选择一名角色交牌", false);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//选黄色牌（类型未用过）
	auto cardOpt = carrier.chooseToGive("【挥金】交一张黄色牌", target, false,
										[this](const Card& c) { return c.is(Card::Color::yellow) && !usedTypes.contains(c.getType()); });
	if (!cardOpt) return false;

	//记录已用类型
	usedTypes.insert(cardOpt->get().getType());
	std::println("<技能> {}发动挥金，交出{}", carrier.displayName(), cardOpt.value().get());
	game.broadcastState();
	return true;
}

void 挥金::reset() {
	usedTypes.clear();
	ActiveSkill::reset();
}


// ==================== 技能：再生 ====================
bool 再生::content(GameLogic& game, Player& carrier) {
	Character::hp_t beforeHp = carrier.getHp();

	//弃置所有绿色牌
	std::vector<std::size_t> greenIndices;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		if (carrier.getCardByIndex(i).is(Card::Color::green)) greenIndices.push_back(i);
	}
	std::ranges::sort(greenIndices, std::greater{});
	for (std::size_t idx : greenIndices) {
		carrier.discardByIndex(idx);
	}

	//回复至50点
	if (carrier.getHp() < 50) {
		carrier.recover(static_cast<std::size_t>(50 - carrier.getHp()));
	}

	std::size_t healed = static_cast<std::size_t>(carrier.getHp() - beforeHp);
	std::println("<技能> {}发动再生，弃置{}张绿色牌，回复{}点体力（至50）", carrier.displayName(), greenIndices.size(), healed);
	//若回复≤25，次数改为2
	if (healed <= 25) {
		setLimit(2);
		std::println("<技能> {}的【再生】回复不超过25，可发动次数改为2", carrier.displayName());
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：手枪 ====================
bool 手枪::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.chooseOtherPlayer(std::format("【手枪】选择一名其他角色造成{}点伤害", damageValue), false);
	if (!targetOpt.has_value()) return false;  //取消，返还可用次数（tryActivate 不累加 count）
	Player& target = *targetOpt;

	target.damage(damageValue, carrier);
	std::println("<技能> {}发动手枪，对{}造成{}点伤害",
				 carrier.displayName(), target.displayName(), damageValue);
	game.broadcastState();
	return true;
}

void 手枪::reset() {
	damageValue = 1;
	ActiveSkill::reset();
}


// ==================== 技能：芜湖 ====================
bool 芜湖::content(GameLogic& game, Player& carrier) {
	//先选颜色
	std::size_t colorChoice = carrier.ask("【芜湖】声明颜色", { "红", "黄", "绿", "蓝", "黑" }, true);
	Card::Color targetColor;
	std::vector<Card::Name> nameOpts;
	if (colorChoice <= 4) {
		//基础四色
		switch (colorChoice) {
			case 1: targetColor = Card::Color::red;    break;
			case 2: targetColor = Card::Color::yellow; break;
			case 3: targetColor = Card::Color::green;  break;
			case 4: targetColor = Card::Color::blue;   break;
		}
		nameOpts = {
			Card::Name::number_0, Card::Name::number_1, Card::Name::number_2,
			Card::Name::number_3, Card::Name::number_4, Card::Name::number_5,
			Card::Name::number_6, Card::Name::number_7, Card::Name::number_8,
			Card::Name::number_9, Card::Name::action_skip, Card::Name::action_draw2,
			Card::Name::action_rev
		};
	}
	else {
		//黑色（万能牌）
		targetColor = Card::Color::black;
		nameOpts = { Card::Name::wild_pal, Card::Name::wild_draw4 };
	}

	//选牌名
	std::vector<std::string> nameStrs;
	for (auto n : nameOpts) nameStrs.push_back(std::format("{}", n));
	std::size_t nameChoice = carrier.ask("【芜湖】声明牌名", nameStrs, true);
	Card::Name targetName = nameOpts[nameChoice - 1];

	std::println("<技能> {}发动芜湖，声明{}{}",
				 carrier.displayName(), targetColor, targetName);
	//从牌堆底向牌堆顶检索，找最后一张匹配的
	Pile& pile = game.getPile();
	std::optional<std::size_t> matchIdx;
	for (std::size_t i = 0; i < pile.count(); ++i) {
		const Card& c = pile[i];
		if (c.is(targetColor) && c.getName() == targetName) {
			matchIdx = i;  //记录最后一个匹配的索引
		}
	}

	if (matchIdx.has_value()) {
		//有匹配：获得一张，消耗次数
		auto card = pile.takeCardByIndex(matchIdx.value());
		const Card& cardRef = *card;  //move 前绑定引用，move 后仍可访问
		carrier.gainCard(std::move(card));
		std::println("<技能> {}从牌堆获得{}", carrier.displayName(), cardRef);
		game.broadcastState();
		return true;
	}
	else {
		//无匹配：视为未发动（return false 不消耗次数），可弃一张
		carrier.chooseToDiscard("牌堆无此牌，弃置一张牌", 1, false);
		std::println("<技能> {}声明牌堆无此牌，芜湖视为未发动", carrier.displayName());
		game.broadcastState();
		return false;
	}
}

// ==================== 技能：引力 ====================
bool 引力::content(GameLogic& game, Player& carrier) {
	carrier.markCharInfoDirty();
	game.broadcastState();

	auto targetOpt = carrier.choosePlayer(
		"【引力】选择一名角色，本轮数字牌进入弃牌堆后该角色获得之", false);
	if (!targetOpt.has_value()) return false;  //取消，不消耗次数

	Player& tgt = targetOpt->get();
	//若目标已有引力_目标则不重复添加
	if (!tgt.findSkill<引力_目标>().has_value()) {
		tgt.addSkill(引力_目标::make());
	}
	target = tgt;

	std::println("<技能> {}发动引力，本轮数字牌进弃牌堆后{}获得之",
				 carrier.displayName(), tgt.displayName());
	game.broadcastState();
	return true;
}

void 引力::reset() {
	//本局结束：从目标身上移除引力_目标
	if (target.has_value()) {
		target.value().get().removeSkill<引力_目标>();
		target = std::nullopt;
	}
	ActiveSkill::reset();
}




//甘草
bool 甘草::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.choosePlayer("【甘草】选择一名角色", false);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//收集非数字牌的索引和描述
	std::vector<std::size_t> indices;
	std::vector<std::string> options;
	std::size_t i = 0;
	for (auto it = target.getHand().begin(); it != target.getHand().end(); ++it, ++i) {
		const Card& c = **it;
		if (!c.isNumber()) {
			indices.push_back(i);
			options.push_back(c.toString());
		}
	}
	if (options.empty()) {
		carrier.hint("该角色没有非数字牌");
		return true;
	}
	auto choice = carrier.ask("【甘草】弃置一张非数字牌？", options, false);
	if (choice == 0) return true;  //不弃置
	target.discardByIndex(indices[choice - 1]);
	game.broadcastState();
	return true;
}

//跳糖
bool 跳糖::content(GameLogic& game, Player& carrier) {
	//切斯特指定颜色
	auto colorOpt = carrier.chooseCardColor("【跳糖】指定【+2】的颜色", false);
	if (!colorOpt) return false;

	//选一名其他角色拼点
	auto targetOpt = carrier.chooseOtherPlayer("【跳糖】选择拼点目标", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	auto result = carrier.comparePoint(target, true);
	if (!result) return false;

	std::println("<技能> {}与{}拼点", carrier.displayName(), target.displayName());

	//胜者获得切斯特指定颜色的+2
	auto giveDraw2 = [&](Player& p) {
		auto card = Card::make(colorOpt.value(), Card::Name::action_draw2);
		std::println("<技能> {}拼点获胜，获得{}", p.displayName(), *card);
		p.gainCard(std::move(card));
	};

	if (*result == Player::CompareResult::win) {
		giveDraw2(carrier);
	}
	else if (*result == Player::CompareResult::lose) {
		giveDraw2(target);
	}
	//平局无胜者，都不获得

	game.broadcastState();
	return true;
}


// ==================== 技能：肘击 ====================
bool 肘击::content(GameLogic& game, Player& carrier) {
	carrier.draw(2, DrawReason::skill);
	carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	std::println("<技能> {}发动肘击，摸2张牌并获得2张变色", carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：突袭 ====================
bool 突袭::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.chooseOtherPlayer("【突袭】令一名其他角色的\"毒\"标记数量翻倍", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();
	if (!target.hasMark("毒")) {
		carrier.hint("该角色没有\"毒\"标记");
		return false;
	}
	std::size_t cnt = target.getMarkCount("毒");
	target.addMark("毒", cnt);
	std::println("<技能> {}突袭令{}的毒标记从{}翻倍至{}", carrier.displayName(), target.displayName(), cnt, cnt * 2);
	game.broadcastState();
	return true;
}


// ==================== 技能：猛击 ====================
bool 猛击::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.chooseOtherPlayer("【猛击】选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	//各回复20%已损体力（向下取整）
	std::size_t cRec = unool::math::floor((carrier.getMaxHp() - carrier.getHp()) * 0.2);
	std::size_t tRec = unool::math::floor((target.getMaxHp() - target.getHp()) * 0.2);
	carrier.recover(cRec);
	target.recover(tRec);

	//回复体力较少的角色被封印10轮；相等则不封印
	if (cRec < tRec) {
		carrier.seal(10);
	}
	else if (tRec < cRec) {
		target.seal(10);
	}

	std::println("<技能> {}发动猛击，{}回复{}点体力，{}回复{}点体力",
				 carrier.displayName(),
				 carrier.displayName(), cRec,
				 target.displayName(), tRec);
	game.broadcastState();
	return true;
}

//================绝技================
bool 绝技::content(GameLogic& game, Player& carrier) {
	if (count == 1) {
		//第1次：随机获得一张【封禁】
		carrier.gainCard(std::make_unique<Card>(Card::randomCard([](const Card& c) {
			return c.is(Card::Name::action_skip);
		})));
		std::println("<技能> {}发动绝技（第1次），随机获得一张封禁", carrier.displayName());
	}
	else if (count == 2) {
		//第2次：随机获得一张【+2】，并获得绝技_额外摸牌被动
		carrier.gainCard(std::make_unique<Card>(Card::randomCard([](const Card& c) {
			return c.is(Card::Name::action_draw2);
		})));
		carrier.addSkill(绝技_额外摸牌::make());
		std::println("<技能> {}发动绝技（第2次），随机获得一张+2并获得额外摸牌效果", carrier.displayName());
	}
	else if (count == 3) {
		//第3次：随机获得一张万能牌
		carrier.gainCard(std::make_unique<Card>(Card::randomCard(&Card::isWild)));
		std::println("<技能> {}发动绝技（第3次），随机获得一张万能牌", carrier.displayName());
	}
	else {
		//第4次及以上：将一张手牌变为随机颜色的随机非数字牌，回复发动次数点体力
		auto idxOpt = carrier.chooseCard("选择一张手牌", unool::alwaysTrue, false);
		if (!idxOpt) return false;

		Card::ColorName cn = Card::randomCard(&Card::isNotNumber);
		carrier.getCardByIndex(*idxOpt).set(cn);
		carrier.recover(count);
		std::println("<技能> {}发动绝技（第{}次），将一张手牌变为随机颜色的非数字牌，回复{}点体力", carrier.displayName(), count, count);
	}
	carrier.markCharInfoDirty();

	game.broadcastState();
	return true;
}

// ==================== 技能：过载 ====================
bool 过载::content(GameLogic& game, Player& carrier) {
	carrier.addMark("速度", 5);
	if (carrier.getMarkCount("速度") > 10)
		carrier.setMark("速度", 10);

	std::println("<技能> {}发动过载，增加5个\"速度\"标记", carrier.displayName());
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

// ==================== 技能：炎烈 ====================
bool 炎烈::content(GameLogic& game, Player& carrier) {
	//随机获得两张不同牌名的红色牌
	auto cn1 = Card::randomCard([](const Card& c) { return c.is(Card::Color::red); });
	auto cn2 = Card::randomCard([&cn1](const Card& c) {
		return c.is(Card::Color::red) && c.getName() != cn1.second;
	});
	carrier.gainCard(Card::make(cn1.first, cn1.second));
	carrier.gainCard(Card::make(cn2.first, cn2.second));
	std::println("<技能> {}发动炎烈，获得红色牌{}和{}", carrier.displayName(),
				 cn1.first == Card::Color::red ? "红" : "黄", cn2.first == Card::Color::red ? "红" : "黄");

	//可以弃置其中一张（两张在手牌末尾）
	const std::size_t idx1 = carrier.getHand().count() - 2;
	const std::size_t idx2 = carrier.getHand().count() - 1;
	const std::size_t choice = carrier.ask(
		std::format("【炎烈】弃置其中一张或跳过："),
		{ carrier.getHand().getCardByIndex(idx1).toString(),
		  carrier.getHand().getCardByIndex(idx2).toString() },
		false
	);
	if (choice == 1) {
		carrier.discardByIndex(idx1);
		std::println("<技能> {}弃置了第一张", carrier.displayName());
	}
	else if (choice == 2) {
		carrier.discardByIndex(idx2);
		std::println("<技能> {}弃置了第二张", carrier.displayName());
	}
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

// ==================== 技能：勇进 ====================
bool 勇进::content(GameLogic& game, Player& carrier) {
	//失去1点体力
	carrier.damage(1, carrier);
	std::println("<技能> {}发动勇进，失去1点体力", carrier.displayName());

	//摸一张牌
	auto drawn = carrier.draw(1);
	if (drawn.empty()) return true;
	const Card& drawnCard = drawn[0].get();
	std::println("<技能> {}摸到{}", carrier.displayName(), drawnCard.toString());

	//若为非数字牌，弃置一张其他牌并回复2点体力
	if (drawnCard.isNotNumber()) {
		auto discarded = carrier.chooseToDiscard("【勇进】弃置一张其他牌：", 1, true);
		carrier.recover(2);
		std::println("<技能> {}摸到非数字牌，弃置一张牌并回复2点体力", carrier.displayName());
	}
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

// ==================== 技能：净身 ====================
bool 净身::filter(const GameLogic& game, const Player& carrier) const {
	//手牌需集齐五种颜色
	for (const Card::Color color : Card::fiveColors) {
		if (!carrier.getHand().include([color](const Card& c) { return c.is(color); })) return false;
	}
	return true;
}

bool 净身::content(GameLogic& game, Player& carrier) {
	carrier.chooseToDiscard("【净身】弃置一张牌：", 1, true);
	carrier.recover(1);
	std::println("<技能> {}发动净身，弃置一张牌并回复1点体力", carrier.displayName());
	game.broadcastState();
	return true;
}

// ==================== 技能：修正 ====================
bool 修正::content(GameLogic& game, Player& carrier) {
	//选一张手牌
	auto idxOpt = carrier.chooseCard("选择一张手牌进行修正", unool::alwaysTrue, false);
	if (!idxOpt) return false;
	Card& card = carrier.getCardByIndex(*idxOpt);

	//构建可选牌名：所有非万能牌名，排除已用过的
	std::vector<Card::Name> availableNames;
	for (int i = static_cast<int>(Card::Name::number_0); i <= static_cast<int>(Card::Name::action_draw2); ++i) {
		Card::Name name = static_cast<Card::Name>(i);
		if (!usedNames.contains(name))
			availableNames.push_back(name);
	}
	if (availableNames.empty()) {
		carrier.hint("所有牌名已用完，无法发动修正");
		return false;
	}

	//声明牌名
	auto nameOpt = carrier.chooseCardName("声明要改为的牌名", false, availableNames);
	if (!nameOpt) return false;
	Card::Name targetName = nameOpt.value();

	//改牌
	card.set(card.getColor(), targetName);
	usedNames.insert(targetName);

	//若改为数字牌，失去3%当前体力（向上取整）
	if (card.isNumber()) {
		std::size_t loss = unool::math::ceil(carrier.getHp() * 0.03);
		carrier.damage(loss, std::nullopt);
		std::println("<技能> {}修正将一张手牌改为数字牌，失去{}点体力", carrier.displayName(), loss);
	}
	else {
		std::println("<技能> {}修正将一张手牌改为功能牌", carrier.displayName());
	}

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

// ==================== 技能：伪政 ====================
bool 伪政::filter(const GameLogic& game, const Player& carrier) const {
	//需要从牌堆两侧共取 2×viewCount 张
	const std::size_t available = game.getPile().count() + game.getDiscardPile().count();
	return available >= 2 * viewCount;
}

bool 伪政::content(GameLogic& game, Player& carrier) {
	Pile& pile = game.getPile();

	//取牌堆顶和牌堆底各 viewCount 张
	std::vector<std::unique_ptr<Card>> topCards, bottomCards;
	for (std::size_t i = 0; i < viewCount; ++i) {
		topCards.push_back(pile.take_front(game.getDiscardPile()));
	}
	for (std::size_t i = 0; i < viewCount; ++i) {
		bottomCards.push_back(pile.take_back(game.getDiscardPile()));
	}

	//展示给所有其他玩家
	std::string topStr, bottomStr;
	for (const auto& c : topCards) topStr += std::format("{} ", *c);
	for (const auto& c : bottomCards) bottomStr += std::format("{} ", *c);
	game.forEachOtherPlayer(carrier, [&](Player& p) {
		p.hint(std::format("【伪政】{}展示了牌堆顶：{}，牌堆底：{}", carrier.characterName(), topStr, bottomStr));
	});

	//构建选项：牌堆顶各张 + 牌堆底各张 + 不获得
	std::vector<std::string> options;
	for (const auto& c : topCards) options.push_back(std::format("获得牌堆顶的{}", *c));
	for (const auto& c : bottomCards) options.push_back(std::format("获得牌堆底的{}", *c));
	const std::size_t noneChoice = options.size();
	options.push_back("不获得");

	const std::size_t choice = carrier.ask(
		std::format("【伪政】牌堆顶：{}；牌堆底：{}；选择获得一张", topStr, bottomStr),
		options, false);

	//获得选中的牌
	if (choice != 0 && choice != noneChoice) {
		const std::size_t idx = choice - 1;
		std::unique_ptr<Card> gained = idx < topCards.size()
			? std::move(topCards[idx])
			: std::move(bottomCards[idx - topCards.size()]);
		carrier.gainCard(std::move(gained));
		++gainedTotal;
		//每因此获得十张牌，牌堆两侧观看牌数各增加一张（至多各三张）
		if (gainedTotal % 10 == 0 && viewCount < 3) ++viewCount;
	}

	//弃置其余的牌
	for (auto& c : topCards) {
		if (c) game.putCardToDiscardPile(std::move(c), Card::DiscardReason::discard, carrier);
	}
	for (auto& c : bottomCards) {
		if (c) game.putCardToDiscardPile(std::move(c), Card::DiscardReason::discard, carrier);
	}

	std::println("<技能> {}发动伪政（观看牌堆顶和牌堆底各{}张）", carrier.displayName(), viewCount);
	game.broadcastState();
	return true;
}

// ==================== 技能：伪政_计数 ====================
bool 伪政_计数::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	++totalDiscarded;
	//每有十张牌进入弃牌堆，伪政本局可用次数+1
	if (totalDiscarded % 10 == 0) {
		if (auto skill = carrier.findSkill<伪政>(); skill.has_value()) {
			skill->get().increaseLimit(1);
			carrier.markCharInfoDirty();
			std::println("<技能> {}的伪政可用次数+1（当前{}次）",
						 carrier.displayName(), skill->get().getLimit().value());
		}
	}
	return true;
}


// ==================== 技能：改革 ====================
bool 改革::content(GameLogic& game, Player& carrier) {
	//先选目标，取消则不失去体力
	auto targetOpt = carrier.choosePlayer("【改革】选择一名角色摸1张牌", false);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//失去10点体力
	carrier.damage(10, std::nullopt);
	std::println("<技能> {}发动改革，失去10点体力", carrier.displayName());

	//令目标摸一张牌
	target.draw(1, DrawReason::skill);
	std::println("<技能> {}发动改革，令{}摸1张牌", carrier.displayName(), target.displayName());

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}


// ==================== 技能：改革_摸牌替代 ====================
bool 改革_摸牌替代::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//仅在其他角色摸牌阶段外摸牌时触发（摸牌阶段与开局初始手牌除外）
	if (!trigger.hasDrawReason()) return false;
	if (trigger.getDrawReason() == DrawReason::phase_draw) return false;
	if (trigger.getDrawReason() == DrawReason::init) return false;
	if (!trigger.hasNumber() || trigger.getNumber() == 0) return false;
	return trigger.getPlayer().getId() != carrier.getId();
}

bool 改革_摸牌替代::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();
	const std::size_t drawCount = trigger.getNumber();
	//X为摸牌数，失去2X%最大体力（向上取整）
	const std::size_t loss = unool::math::ceil(static_cast<double>(drawCount) * target.getMaxHp() * 0.02);

	const std::size_t choice = carrier.ask(
		std::format("【改革】是否将{}的{}张摸牌改为失去{}点体力？", target.characterName(), drawCount, loss),
		{ "发动", "不发动" }, false);
	if (choice != 1) return false;

	//获取摸牌数并取消摸牌
	trigger.getNumber() = 0;
	target.cancelNextDraw();
	target.damage(loss, std::nullopt);

	std::println("<技能> {}发动改革，将{}的{}张摸牌改为失去{}点体力",
				 carrier.displayName(), target.displayName(), drawCount, loss);
	game.broadcastState();
	return true;
}


// ==================== 技能：飞手 ====================
bool 飞手_整轮::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto opt = carrier.findSkill<飞手>();
	if (!opt.has_value()) return false;
	飞手& skill = opt.value().get();
	skill.usedLastRound = skill.usedThisRound;
	skill.usedThisRound = false;
	return true;
}

bool 飞手::filter(const GameLogic& game, const Player& carrier) const {
	if (usedLastRound) return false;
	//上/下家至少有一家可以保留一张牌
	return carrier.prev().handCount() >= 2 || carrier.next().handCount() >= 2;
}

bool 飞手::content(GameLogic& game, Player& carrier) {
	//选择上家/下家
	std::vector<std::string> options;
	std::vector<ref<Player>> candidates{};
	Player& prevPlayer = carrier.prev();
	Player& nextPlayer = carrier.next();
	if (prevPlayer.handCount() >= 2) {
		options.push_back(std::format("上家：{}（{}张牌）", prevPlayer.characterName(), prevPlayer.handCount()));
		candidates.push_back(std::ref(prevPlayer));
	}
	if (nextPlayer.handCount() >= 2) {
		options.push_back(std::format("下家：{}（{}张牌）", nextPlayer.characterName(), nextPlayer.handCount()));
		candidates.push_back(std::ref(nextPlayer));
	}
	if (candidates.empty()) {
		carrier.hint("上家与下家都没有可获得的牌（至少保留一张）");
		return false;
	}

	const std::size_t choice = carrier.ask("【飞手】选择获得谁的一张牌", options, false);
	if (choice == 0) return false;
	Player& target{candidates[choice - 1].get()};

	//随机获得一张，然后交还一张
	const std::size_t idx = unool::random::randomSize_t(0, target.handCount() - 1);
	carrier.gainCard(target.takeCardByIndex(idx));
	game.broadcastState();

	carrier.chooseToGive(std::format("【飞手】交还一张牌给{}", target.characterName()), target, true);

	usedThisRound = true;
	std::println("<技能> {}发动飞手，获得{}的一张牌并交还一张", carrier.displayName(), target.displayName());
	game.broadcastState();
	return true;
}

void 飞手::reset() {
	ActiveSkill::reset();
	usedThisRound = false;
	usedLastRound = false;
}


// ==================== 技能：筑丛 ====================
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

bool 筑丛::content(GameLogic& game, Player& carrier) {
	auto subOpt = carrier.findSkill<筑丛_摸牌>();
	if (!subOpt.has_value()) return false;

	//摸牌至摸到绿色牌，回复X点体力
	auto drawn = subOpt.value().get().drawUntilGreen(game, carrier);
	const std::size_t X = drawn.size();
	carrier.recover(X);

	accumulating = true;
	nonGreenCount = 0;

	std::println("<技能> {}发动筑丛，摸了{}张牌并回复{}点体力", carrier.displayName(), X, X);
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

void 筑丛::reset() {
	ActiveSkill::reset();
	accumulating = false;
	nonGreenCount = 0;
}

std::optional<std::string> 筑丛::extraPlaceholders(const std::string& key) const {
	if (key == "nongreen") return std::to_string(nonGreenCount);
	return std::nullopt;
}


// ==================== 技能：星矢 ====================
bool 星矢::filter(const GameLogic& game, const Player& carrier) const {
	return !carrier.handEmpty();
}

bool 星矢::content(GameLogic& game, Player& carrier) {
	//可重铸的颜色：手牌中实际存在的颜色（含黑色）
	std::vector<Card::Color> colors;
	for (const Card::Color c : Card::fiveColors) {
		if (carrier.handInclude([c](const Card& card) { return card.is(c); })) {
			colors.push_back(c);
		}
	}
	if (colors.empty()) {
		carrier.hint("你手中没有可重铸的牌");
		return false;
	}

	auto colorOpt = carrier.chooseCardColor("【星矢】选择一种颜色，重铸该颜色的所有牌", false, colors);
	if (!colorOpt.has_value()) return false;
	const Card::Color color = colorOpt.value();

	//失去5点体力
	carrier.loseHp(5);

	//收集该颜色所有牌的索引（按下标降序重铸，避免下标错位）
	std::vector<std::size_t> indices;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		if (carrier.getCardByIndex(i).is(color)) indices.push_back(i);
	}

	bool gainedSameColor = false;
	for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
		const std::size_t before = carrier.handCount();
		carrier.recastByIndex(*it);
		//成功摸牌后，新牌位于手牌末尾
		if (carrier.handCount() >= before && !carrier.handEmpty()
			&& carrier.getHand().back().is(color)) {
			gainedSameColor = true;
		}
	}

	if (!gainedSameColor) {
		carrier.addDamageMultiplier(0.1);
		carrier.addLifesteal(10);
		std::println("<技能> {}发动星矢，未获得同色牌，伤害倍率+0.1，吸血+10%（当前{}%）",
					 carrier.displayName(), carrier.getLifesteal());
		carrier.hint(std::format("【星矢】未获得同色牌：伤害倍率+0.1，吸血+10%（当前{}%）",
								 carrier.getLifesteal()));
	}

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}


// ==================== 技能：坠机 ====================
bool 坠机::content(GameLogic& game, Player& carrier) {
	Player& other = carrier.next();

	//任选牌名（从0到+4）
	std::vector<Card::Name> names;
	names.append_range(Card::allCards);

	auto nameOpt = carrier.chooseCardName("【坠机】选择一种牌名", false, names);
	if (!nameOpt.has_value()) return false;
	const Card::Name name = nameOpt.value();

	//自己展示（没有则跳过）
	std::optional<std::size_t> selfIdx;
	if (carrier.handInclude([name](const Card& c) { return c.getName() == name; })) {
		selfIdx = carrier.chooseCard("【坠机】展示一张指定牌名的牌",
			[name](const Card& c) { return c.getName() == name; }, true);
		if (selfIdx.has_value()) carrier.showCard(carrier.getCardByIndex(*selfIdx));
	}

	//自动展示对面一张同名牌（没有则跳过）
	std::optional<std::size_t> tgtIdx;
	for (std::size_t i = 0; i < other.handCount(); ++i) {
		if (other.getCardByIndex(i).getName() == name) { tgtIdx = i; break; }
	}
	if (tgtIdx.has_value()) other.showCard(other.getCardByIndex(*tgtIdx));

	const bool selfShowed = selfIdx.has_value();
	const bool tgtShowed = tgtIdx.has_value();

	if (!tgtShowed) {
		//对面未展示：其随机获得一张数字牌，技能视为未发动过
		other.gainCard(std::make_unique<Card>(Card::randomCard(&Card::isNumber)));
		resetCount();
		std::println("<技能> {}发动坠机，{}未展示牌，获得一张数字牌，技能视为未发动",
					 carrier.displayName(), other.displayName());
	}
	else if (!selfShowed) {
		//自己未展示：弃置所有手牌并摸八张牌
		while (!carrier.handEmpty())
			carrier.putCardToDiscardPileByIndex(0, Card::DiscardReason::discard);
		carrier.draw(8, DrawReason::skill);
		std::println("<技能> {}发动坠机，自己未展示牌，弃置所有手牌并摸8张", carrier.displayName());
	}
	else {
		//均展示：可令一名角色获得这两张牌
		const std::size_t choice = carrier.ask("【坠机】是否令一名角色获得这两张牌？", { "是", "否" }, false);
		if (choice == 1) {
			auto pOpt = carrier.choosePlayer("【坠机】选择获得这两张牌的角色", false);
			if (pOpt.has_value()) {
				Player& p = pOpt.value().get();
				//先拿对面的牌，再拿自己的，避免自己的下标变化
				auto tgtCard = other.takeCardByIndex(*tgtIdx);
				auto selfCard = carrier.takeCardByIndex(*selfIdx);
				p.gainCard(std::move(tgtCard));
				p.gainCard(std::move(selfCard));
				std::println("<技能> {}令{}获得两张牌", carrier.displayName(), p.displayName());
			}
		}
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：奔突 ====================
bool 奔突::content(GameLogic& game, Player& carrier) {
	auto tgtOpt = carrier.chooseOtherPlayer("【奔突】选择一名其他角色", false);
	if (!tgtOpt.has_value()) return false;
	Player& target = tgtOpt.value().get();

	//摸一张牌并展示
	auto drawn = target.draw(1, DrawReason::skill);
	if (!drawn.empty()) {
		target.showCard(drawn.back().get());
	}

	//随机弃置一张牌
	if (target.handEmpty()) {
		game.broadcastState();
		return true;
	}
	const std::size_t idx = unool::random::randomSize_t(0, target.handCount() - 1);
	target.putCardToDiscardPileByIndex(idx, Card::DiscardReason::discard);
	std::println("<技能> {}发动奔突，{}摸一张并随机弃置一张牌",
				 carrier.displayName(), target.displayName());

	game.broadcastState();
	return true;
}


// ==================== 技能：炼石 ====================
bool 炼石::content(GameLogic& game, Player& carrier) {
	//多次选一张，取消或选完所有手牌后结束选择
	std::vector<std::size_t> selected;
	while (selected.size() < carrier.handCount()) {
		auto idxOpt = carrier.chooseCard(
			"【炼石】选择一张要重铸的手牌（取消结束选择）",
			[&](const Card& c) {
				for (const std::size_t s : selected)
					if (&carrier.getCardByIndex(s) == &c) return false;
				return true;
			}, false);
		if (!idxOpt.has_value()) break;
		selected.push_back(*idxOpt);
	}
	if (selected.empty()) {
		carrier.hint("未选择任何牌");
		return false;
	}

	yellowGained.clear();
	usedThisTurn = true;

	//降序重铸，统计弃置牌总分值，并记录重铸获得的黄色牌
	std::size_t totalValue = 0;
	for (auto it = selected.rbegin(); it != selected.rend(); ++it) {
		Card& discarded = carrier.getCardByIndex(*it);
		totalValue += discarded.value();
		carrier.recastByIndex(*it);
		if (!carrier.handEmpty() && carrier.getHand().back().is(Card::Color::yellow)) {
			yellowGained.push_back(std::ref(carrier.getHand().back()));
		}
	}

	//重铸光所有手牌时，禁止因手牌为空获胜
	if (carrier.handEmpty()) carrier.setNoWinByEmptyHand(true);

	//扣减等同于弃置牌总分值的体力
	carrier.loseHp(totalValue);
	std::println("<技能> {}发动炼石，重铸{}张牌，扣减{}点体力",
				 carrier.displayName(), selected.size(), totalValue);

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

void 炼石::reset() {
	ActiveSkill::reset();
	usedThisTurn = false;
	yellowGained.clear();
}


// ==================== 技能：成金 ====================
bool 成金::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	auto opt = carrier.findSkill<炼石>();
	return opt.has_value() && opt.value().get().usedThisTurn;
}

bool 成金::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto opt = carrier.findSkill<炼石>();
	if (!opt.has_value()) return false;
	炼石& skill = opt.value().get();

	//弃置本回合获得的所有黄色牌（仍在手牌中的）
	std::size_t discarded = 0;
	for (const ref<Card>& cp : skill.yellowGained) {
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			if (&carrier.getCardByIndex(i) == &cp.get()) {
				carrier.putCardToDiscardPileByIndex(i, Card::DiscardReason::discard);
				++discarded;
				break;
			}
		}
	}

	std::println("<技能> {}发动成金，弃置本回合获得的{}张黄色牌", carrier.displayName(), discarded);
	//重置炼石本回合状态
	skill.usedThisTurn = false;
	skill.yellowGained.clear();

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}


// ==================== 技能：无礼 ====================
bool 无礼_计数::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto opt = carrier.findSkill<无礼>();
	if (!opt.has_value()) return false;
	无礼& skill = opt.value().get();

	const Card::Type type = trigger.getCard().getType();
	if (!skill.usedTypes.contains(type)) {
		skill.usedTypes.insert(type);
		skill.increaseLimit(1);
		std::println("<技能> {}打出了新类型的牌，无礼可用次数+1", carrier.displayName());
		carrier.hint(std::format("【无礼】可用次数+1（当前剩余{}次）",
								 skill.getLimit().value() - skill.getCount()));
		carrier.markCharInfoDirty();
	}
	return true;
}

bool 无礼::filter(const GameLogic& game, const Player& carrier) const {
	//至少有一名角色手牌≥2才能弃置半数
	return carrier.handCount() >= 2 || carrier.next().handCount() >= 2;
}

bool 无礼::content(GameLogic& game, Player& carrier) {
	auto tgtOpt = carrier.choosePlayer("【无礼】选择一名角色，弃置其半数手牌", false);
	if (!tgtOpt.has_value()) return false;
	Player& target = tgtOpt.value().get();

	const std::size_t half = target.handCount() / 2;
	if (half == 0) {
		carrier.hint("该角色手牌不足两张");
		return false;
	}

	//随机弃置半数手牌（向下取整）
	for (std::size_t i = 0; i < half; ++i) {
		const std::size_t idx = unool::random::randomSize_t(0, target.handCount() - 1);
		target.putCardToDiscardPileByIndex(idx, Card::DiscardReason::discard);
	}

	//观看牌堆顶等量张牌并交给其
	auto drawn = target.draw(half, DrawReason::skill);
	for (auto& cardRef : drawn) target.showCard(cardRef.get());

	std::println("<技能> {}发动无礼，弃置{}半数（{}张）手牌并交给其{}张牌",
				 carrier.displayName(), target.displayName(), half, drawn.size());

	game.broadcastState();
	return true;
}

void 无礼::reset() {
	ActiveSkill::reset();
	usedTypes.clear();
}

