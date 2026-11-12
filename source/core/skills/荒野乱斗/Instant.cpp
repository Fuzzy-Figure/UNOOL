#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"


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

// ==================== 技能：飞手 ====================
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


