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


// ==================== 新增荒野乱斗角色技能（第二批） ====================

// ==================== 技能：蛮冲 ====================
bool 蛮冲::content(GameLogic& game, Player& carrier) {
	recording = true;
	bannedThisTurn = false;
	turnsLeft = 4;
	++activationCount;
	std::println("<技能> {}发动蛮冲（本局第{}次），开始记录接下来四个回合",
				 carrier.displayName(), activationCount);
	game.broadcastState();
	return true;
}

void 蛮冲::reset() {
	ActiveSkill::reset();
	recording = false;
	bannedThisTurn = false;
	turnsLeft = 0;
	activationCount = 0;
}

// ==================== 技能：爆汽 ====================
bool 爆汽::content(GameLogic& game, Player& carrier) {
	if (!carrier.getHand().include(&Card::isWild)) {
		std::println("<技能> {}手中没有万能牌，爆汽不发动", carrier.displayName());
		return false;
	}

	//逐张选万能牌，↓ 结束；一张未弃则 content 返回 false，tryActivate 回滚次数
	std::size_t discarded{};
	while (carrier.getHand().include(&Card::isWild)) {
		auto index = carrier.chooseCard(
			std::format("【爆汽】选择要弃置的万能牌（已弃{}张，↓结束）", discarded),
			&Card::isWild, false);
		if (!index.has_value()) break;
		carrier.discardByIndex(index.value());
		++discarded;
	}
	if (discarded == 0) {
		std::println("<技能> {}未弃置万能牌，爆汽不发动（不消耗限定技）", carrier.displayName());
		return false;
	}

	for (std::size_t i = 0; i < discarded; ++i)
		carrier.gainCard(Card::make(Card::Color::green, Card::Name::secret_过载));

	std::size_t healed{};
	if (discarded > 2) {
		healed = unool::math::floor((carrier.getMaxHp() - carrier.getHp()) * 0.1);
		carrier.recover(healed);
	}
	std::println("<技能> {}发动爆汽，弃置{}张万能牌获得{}张【过载】，回复{}点体力",
				 carrier.displayName(), discarded, discarded, healed);
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}


// ==================== 新增荒野乱斗角色技能（第三批） ====================

// ==================== 技能：蝙蝠_计数 ====================
bool 蝙蝠_计数::filter(const GameLogic&, const Player&, const Trigger& trigger) const {
	if (trigger.hasDrawReason() && trigger.getDrawReason() == DrawReason::init) return false;
	return trigger.hasCards() && !trigger.getCards().empty();
}

bool 蝙蝠_计数::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	gained += trigger.getCards().size();
	while (gained >= (awarded + 1) * 5) {
		++awarded;
		carrier.getSkill<蝙蝠>().increaseLimit(1);
		carrier.hint(std::format("【蝙蝠】累计获得{}张牌，可发动次数+1", gained));
		std::println("<技能> {}累计获得{}张牌，【蝙蝠】次数+1", carrier.displayName(), gained);
	}
	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}

// ==================== 技能：突击 ====================
bool 突击::filter(const GameLogic& game, const Player& carrier) const {
	for (const auto& other : game.getPlayersExcludeId(carrier.getId())) {
		if (other.get().handCount() > 0) return true;
	}
	return false;
}

bool 突击::content(GameLogic& game, Player& carrier) {
	std::vector<ref<Player>> candidates{};
	for (const auto& other : game.getPlayersExcludeId(carrier.getId())) {
		if (other.get().handCount() > 0) candidates.push_back(other);
	}
	if (candidates.empty()) return false;
	Player& target = unool::random::randomGet(candidates).get();
	const std::size_t index = unool::random::randomSize_t(0, target.handCount() - 1);
	const Card::Name name = target.getCardByIndex(index).getName();
	carrier.hint(std::format("【突击】观看了{}的一张牌：{}",
							  target.displayName(), Card::to_string(name)));
	std::println("<技能> {}发动突击，观看{}的{}",
				 carrier.displayName(), target.displayName(), Card::to_string(name));

	const bool same = carrier.handInclude([name](const Card& c) { return c.getName() == name; });
	if (!same) {
		carrier.gainCard(target.takeCardByIndex(index));
		std::println("<技能> {}手中无此牌名，获得{}的{}",
					 carrier.displayName(), target.displayName(), Card::to_string(name));
	}
	else {
		target.damage(3, carrier);
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			if (carrier.getCardByIndex(i).getName() == name) {
				carrier.discardByIndex(i);
				break;
			}
		}
		std::println("<技能> {}手中已有{}，对{}造成3点伤害并弃置一张同名牌",
					 carrier.displayName(), Card::to_string(name), target.displayName());
	}
	game.broadcastState();
	return true;
}

// ==================== 技能：蝙蝠 ====================
bool 蝙蝠::filter(const GameLogic& game, const Player& carrier) const {
	return !game.getPlayersExcludeId(carrier.getId()).empty();
}

bool 蝙蝠::content(GameLogic& game, Player& carrier) {
	const auto players = game.getPlayers();
	std::size_t selfIndex = players.size();
	for (std::size_t i = 0; i < players.size(); ++i) {
		if (players[i].get() == carrier) selfIndex = i;
	}
	if (selfIndex == players.size()) return false;

	const auto startOpt = carrier.chooseOtherPlayer("【蝙蝠】选择座位连续区间的起点", false);
	if (!startOpt.has_value()) return false;
	Player& startPlayer{startOpt.value().get()};
	std::size_t startIndex = players.size();
	for (std::size_t i = 0; i < players.size(); ++i) {
		if (players[i].get() == startPlayer) startIndex = i;
	}
	if (startIndex == players.size()) return false;

	//从起点按座位顺序取连续的一段，遇到自己即止（区间内不含自己）
	std::size_t maxCount = 0;
	for (std::size_t k = 0; k < players.size(); ++k) {
		if ((startIndex + k) % players.size() == selfIndex) break;
		++maxCount;
	}
	if (maxCount == 0) return false;

	std::vector<std::string> options{};
	for (std::size_t n = 1; n <= maxCount; ++n) {
		options.push_back(std::format("{}名（吸取{}点体力）", n, 5 * n));
	}
	const std::size_t choice = carrier.ask("【蝙蝠】选择吸取人数", options, true);
	if (choice == 0 || choice > maxCount) return false;

	for (std::size_t k = 0; k < choice; ++k) {
		Player& victim = players[(startIndex + k) % players.size()].get();
		victim.loseHp(5);
	}
	carrier.recover(5 * choice);
	carrier.hint(std::format("【蝙蝠】吸取{}名角色各5点体力，自己回复{}点体力", choice, 5 * choice));
	std::println("<技能> {}发动蝙蝠，吸取{}名座位连续角色各5点体力，回复{}点体力",
				 carrier.displayName(), choice, 5 * choice);
	game.broadcastState();
	return true;
}


// ==================== 新增荒野乱斗角色技能（第四批） ====================

// ==================== 技能：收税 ====================
bool 收税::filter(const GameLogic& game, const Player& carrier) const {
	return !game.getPlayersExcludeId(carrier.getId()).empty();
}

bool 收税::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.chooseOtherPlayer("【收税】选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	const bool hasWild{target.handInclude([](const Card& c) { return c.isWild(); })};
	std::size_t choice{2};
	if (hasWild) {
		choice = target.ask(
			std::format("【收税】{}令你选择：", carrier.displayName()),
			{ "交出一张万能牌（其须弃置一张牌）", "受到1点伤害" },
			true
		);
		if (choice == 0) choice = 2;
	}
	else {
		target.hint("【收税】你手中没有万能牌，只能受到1点伤害");
	}

	if (choice == 1) {
		auto gave = target.chooseToGive("【收税】交出一张万能牌", carrier, true,
			[](const Card& c) { return c.isWild(); });
		if (!gave.has_value()) {
			target.damage(1, carrier);
			std::println("<技能> {}发动收税，{}未能交出万能牌，受到1点伤害",
						 carrier.displayName(), target.displayName());
		}
		else {
			carrier.chooseToDiscard("【收税】弃置一张牌", 1, true);
			std::println("<技能> {}发动收税，{}交出一张万能牌，{}弃置一张牌",
						 carrier.displayName(), target.displayName(), carrier.displayName());
		}
	}
	else {
		target.damage(1, carrier);
		std::println("<技能> {}发动收税，{}选择受到1点伤害",
					 carrier.displayName(), target.displayName());
	}
	game.broadcastState();
	return true;
}

// ==================== 技能：生草_连续 ====================
bool 生草_连续::content(GameLogic&, Player& carrier, Trigger&) {
	auto skillOpt = carrier.findSkill<生草>();
	if (!skillOpt.has_value()) return false;
	生草& skill = skillOpt.value().get();
	if (!skill.usedThisTurn) skill.streak = 0;
	skill.usedThisTurn = false;
	return true;
}

// ==================== 技能：生草 ====================
bool 生草::filter(const GameLogic&, const Player& carrier) const {
	return carrier.handInclude([](const Card& c) { return c.is(Card::Color::green); });
}

bool 生草::content(GameLogic& game, Player& carrier) {
	auto index = carrier.chooseCard("【生草】选择一张绿色牌重铸",
		[](const Card& c) { return c.is(Card::Color::green); }, false);
	if (!index.has_value()) return false;
	carrier.recastByIndex(index.value());
	++streak;
	usedThisTurn = true;
	carrier.recover(streak);
	carrier.hint(std::format("【生草】连续第{}回合发动，回复{}点体力", streak, streak));
	std::println("<技能> {}发动生草，重铸一张绿色牌并回复{}点体力",
				 carrier.displayName(), streak);
	game.broadcastState();
	return true;
}
