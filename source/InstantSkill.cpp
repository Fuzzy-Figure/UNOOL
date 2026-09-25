#include "../header/PassiveSkill.h"
#include "../header/InstantSkill.h"
#include "../header/TransformSkill.h"
#include "../header/GameLogic.h"
#include "../header/utils.h"


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
	Player::RecastResult result = carrier.chooseToRecast(L"[徒步] 重铸一张牌", 1, false);
	if (result.discarded.size() == 0) return false;

	carrier.recover(1);
	if (result.discarded.front().get().isNumber()) {
		carrier.damage(getCount(), carrier);
	}
	return true;
}


// ==================== 技能：招待 ====================
bool 招待::content(GameLogic& game, Player& carrier) {
	std::optional targetOpt = carrier.chooseOtherPlayer(L"[招待] 选择一名其他角色", true);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	carrier.chooseToGive(L"选择一张手牌交给" + target.characterNameW(), target, true);
	return true;
}




bool 摘罩::content(GameLogic& game, Player& carrier) {
	std::wstring recordStr;
	for (const Card::Name& name : record) {
		recordStr += Card::to_wstring(name) + L',';
	}
	//1. 展示一张未展示过点数的数字牌
	auto cardOpt = carrier.chooseToShow(L"[摘罩] 展示一张数字牌\n已展示：" + recordStr, false, [this](const Card& c) {
		return c.isNumber() && !record.contains(c.getName());
	});
	if (!cardOpt.has_value()) return false;
	Card& card = cardOpt.value().get();
	Card::Name point = card.getName();

	//2. 选一名其他角色，令其展示相同点数的牌
	auto targetOpt = carrier.chooseOtherPlayer(L"[摘罩] 选择一名其他角色", false);
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
			L"[摘罩] 展示一张" + Card::to_wstring(point), false,
			[point](const Card& c) { return c.getName() == point; }
		);
		targetShowed = otherCardOpt.has_value();
	}

	//4. 若目标未展示牌，carrier 可任意更改所展示牌的颜色
	if (!targetShowed) {
		auto color = carrier.chooseCardColor(L"[摘罩] 更改你展示的牌的颜色", false);
		if (color.has_value()) {
			card.setColor(color.value());
			game.broadcastState();
		}
	}
	return true;
}

void 摘罩::reset() {
	Skill::reset();
	record.clear();
}


// ==================== 技能：还击 ====================
bool 还击::content(GameLogic& game, Player& carrier) {
	//1. 选一名其他角色
	auto targetOpt = carrier.chooseOtherPlayer(L"[还击] 选择一名其他角色", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	const std::size_t n = target.handCount();
	if (n == 0) {
		std::cout << "<还击> 目标无手牌" << std::endl;
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
			L"[还击] 交还一张牌给" + target.characterNameW()
			+ L"（" + std::to_wstring(i + 1) + L"/" + std::to_wstring(takeCount) + L"）",
			target, true
		);
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：舞爪 ====================
bool 舞爪::content(GameLogic& game, Player& carrier) {
	auto drawn = carrier.drawTo(10, DrawReason::skill);
	carrier.recover(2 * drawn.size());
	carrier.removeSkill("暗忍");
	carrier.addSkill(暗忍_改::make());
	std::cout << "<技能> " << carrier.characterName() << "发动舞爪，摸"
		<< drawn.size() << "张，回复" << (2 * drawn.size()) << "点体力，【暗忍】移除失体力效果" << std::endl;
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
	carrier.chooseToDiscard(L"【四霸】弃置三张【4】", 3, true,
							[](const Card& c) { return c.is(Card::Name::number_4); });
	std::cout << "<技能> " << carrier.characterName() << "发动四霸，弃置了三张【4】" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：装币 ====================
bool 装币::content(GameLogic& game, Player& carrier) {
	auto drawn = carrier.drawTo(18, DrawReason::skill);
	carrier.setDamageMultiplier(carrier.getDamageMultiplier() + 1);
	std::cout << "<技能> " << carrier.characterName() << "发动装币，摸"
		<< drawn.size() << "张，伤害倍率提升至" << carrier.getDamageMultiplier() << std::endl;
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
	std::cout << "<技能> " << carrier.characterName() << "发动幽愈，移去\"幽灵\"标记并回复" << recoverValue << "体力" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：炫技 ====================
bool 炫技::content(GameLogic& game, Player& carrier) {
	//选牌堆顶或底
	std::size_t pos = carrier.ask(L"【炫技】从牌堆顶或牌堆底摸两张", { L"牌堆顶", L"牌堆底" }, false);
	if (pos == 0) return false;  //玩家取消
	DrawPosition dp = (pos == 1) ? DrawPosition::top : DrawPosition::bottom;

	//摸两张（触发draw_begin/end）
	carrier.draw(2, DrawReason::skill, dp);

	//两张在hand末尾，记录颜色
	std::size_t idx1 = carrier.getHand().count() - 2;
	std::size_t idx2 = carrier.getHand().count() - 1;
	Card::Color c1 = carrier.getHand()[idx1].getColor();
	Card::Color c2 = carrier.getHand()[idx2].getColor();

	//选一张置于牌堆底
	std::size_t put = carrier.ask(L"【炫技】将一张牌置于牌堆底",
								  { carrier.getHand()[idx1].toWString(), carrier.getHand()[idx2].toWString() }, true);
	std::size_t putIdx = (put == 1) ? idx1 : idx2;
	game.getPile().push_back(carrier.takeCardByIndex(putIdx));
	std::cout << "<技能> " << carrier.characterName() << "发动炫技，摸2张并置1张于牌堆底" << std::endl;

	//若两牌同色且手牌>=2，可弃两张令加速+1
	if (c1 == c2 && carrier.handCount() >= 2) {
		std::size_t choice = carrier.ask(L"【炫技】两牌同色，是否弃置两张令【加速】+1？",
										 { L"弃两张", L"不弃" }, false);
		if (choice == 1) {
			carrier.chooseToDiscard(L"【炫技】弃置两张牌", 2, true);
			if (auto sp = carrier.findSkill<加速>(); sp.has_value()) {
				auto& acc = sp.value().get();
				acc.increaseLimit(1);
				std::cout << "<技能> " << carrier.characterName() << "的【加速】可发动次数+1，当前="
					<< acc.getLimit().value() << std::endl;
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

	auto idxOpt = carrier.chooseCard(&Card::isNumber, false);
	if (!idxOpt) return false;

	auto card = carrier.takeCardByIndex(*idxOpt);
	carrier.showCard(*card);
	std::cout << "<技能> " << carrier.characterName() << "展示" << *card << std::endl;

	auto targetOpt = carrier.chooseOtherPlayer(L"【调羹】令一名角色获得此牌", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	std::size_t value = card->value();
	carrier.give(target, std::move(card));
	std::cout << "<技能> " << carrier.characterName() << "将牌交给" << target.characterName() << std::endl;

	target.recover(value);
	std::cout << "<技能> " << target.characterName() << "回复" << value << "点体力" << std::endl;

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
	auto targetOpt = carrier.chooseOtherPlayer(L"【挥金】选择一名角色交牌", false);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//选黄色牌（类型未用过）
	auto cardOpt = carrier.chooseToGive(L"【挥金】交一张黄色牌", target, false,
										[this](const Card& c) { return c.is(Card::Color::yellow) && !usedTypes.contains(c.getType()); });
	if (!cardOpt) return false;

	//记录已用类型
	usedTypes.insert(cardOpt->get().getType());
	std::cout << "<技能> " << carrier.characterName() << "发动挥金，交出"
		<< cardOpt.value().get() << std::endl;

	game.broadcastState();
	return true;
}

void 挥金::reset() {
	usedTypes.clear();
	ActiveSkill::reset();
}


// ==================== 技能：再生 ====================
bool 再生::content(GameLogic& game, Player& carrier) {
	std::size_t beforeHp = carrier.getHp();

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
		carrier.recover(50 - carrier.getHp());
	}

	std::size_t healed = carrier.getHp() - beforeHp;
	std::cout << "<技能> " << carrier.characterName() << "发动再生，弃置"
		<< greenIndices.size() << "张绿色牌，回复" << healed << "点体力（至50）" << std::endl;

	//若回复≤25，次数改为2
	if (healed <= 25) {
		setLimit(2);
		std::cout << "<技能> " << carrier.characterName() << "的【再生】回复不超过25，可发动次数改为2" << std::endl;
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：手枪 ====================
bool 手枪::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.chooseOtherPlayer(L"【手枪】选择一名其他角色造成" + std::to_wstring(damageValue) + L"点伤害", false);
	if (!targetOpt.has_value()) return false;  //取消，返还可用次数（tryActivate 不累加 count）
	Player& target = *targetOpt;

	target.damage(damageValue, carrier);
	std::cout << "<技能> " << carrier.characterName() << "发动手枪，对"
		<< target.characterName() << "造成" << damageValue << "点伤害" << std::endl;
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
	std::size_t colorChoice = carrier.ask(L"【芜湖】声明颜色", { L"红", L"黄", L"绿", L"蓝", L"黑" }, true);
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
	std::vector<std::wstring> nameStrs;
	for (auto n : nameOpts) nameStrs.push_back(Card::to_wstring(n));
	std::size_t nameChoice = carrier.ask(L"【芜湖】声明牌名", nameStrs, true);
	Card::Name targetName = nameOpts[nameChoice - 1];

	std::cout << "<技能> " << carrier.characterName() << "发动芜湖，声明"
		<< Card::to_string(targetColor) << Card::to_string(targetName) << std::endl;

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
		std::cout << "<技能> " << carrier.characterName() << "从牌堆获得"
			<< cardRef << std::endl;
		game.broadcastState();
		return true;
	}
	else {
		//无匹配：视为未发动（return false 不消耗次数），可弃一张
		carrier.chooseToDiscard(L"牌堆无此牌，弃置一张牌", 1, false);
		std::cout << "<技能> " << carrier.characterName() << "声明牌堆无此牌，芜湖视为未发动" << std::endl;
		game.broadcastState();
		return false;
	}
}

// ==================== 技能：引力 ====================
bool 引力::content(GameLogic& game, Player& carrier) {
	carrier.markCharInfoDirty();
	game.broadcastState();

	auto targetOpt = carrier.chooseOtherPlayer(
		L"【引力】选择一名角色，本轮数字牌进入弃牌堆后该角色获得之", false);
	if (!targetOpt.has_value()) return false;  //取消，不消耗次数

	Player& tgt = targetOpt->get();
	//若目标已有引力_目标则不重复添加
	if (!tgt.findSkill<引力_目标>().has_value()) {
		tgt.addSkill(引力_目标::make());
	}
	target = tgt;

	std::cout << "<技能> " << carrier.characterName() << "发动引力，本轮数字牌进弃牌堆后"
		<< tgt.characterName() << "获得之" << std::endl;
	game.broadcastState();
	return true;
}

void 引力::reset() {
	//本局结束：从目标身上移除引力_目标
	if (target.has_value()) {
		target.value().get().removeSkill("引力_目标");
		target = std::nullopt;
	}
	ActiveSkill::reset();
}




//甘草
bool 甘草::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.chooseOtherPlayer(L"【甘草】选择一名角色", false);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//收集非数字牌的索引和描述
	std::vector<std::size_t> indices;
	std::vector<std::wstring> options;
	std::size_t i = 0;
	for (auto it = target.getHand().begin(); it != target.getHand().end(); ++it, ++i) {
		const Card& c = **it;
		if (!c.isNumber()) {
			indices.push_back(i);
			options.push_back(c.toWString());
		}
	}
	if (options.empty()) {
		carrier.hint(L"该角色没有非数字牌");
		return true;
	}
	auto choice = carrier.ask(L"【甘草】弃置一张非数字牌？", options, false);
	if (choice == 0) return true;  //不弃置
	target.discardByIndex(indices[choice - 1]);
	game.broadcastState();
	return true;
}

//跳糖
bool 跳糖::content(GameLogic& game, Player& carrier) {
	//切斯特指定颜色
	auto colorOpt = carrier.chooseCardColor(L"【跳糖】指定【+2】的颜色", false);
	if (!colorOpt) return false;

	//选一名其他角色拼点
	auto targetOpt = carrier.chooseOtherPlayer(L"【跳糖】选择拼点目标", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	auto result = carrier.comparePoint(target, true);
	if (!result) return false;

	std::cout << "<技能> " << carrier.characterName() << "与" << target.characterName() << "拼点" << std::endl;

	//胜者获得切斯特指定颜色的+2
	auto giveDraw2 = [&](Player& p) {
		auto card = Card::make(colorOpt.value(), Card::Name::action_draw2);
		std::cout << "<技能> " << p.characterName() << "拼点获胜，获得"
			<< *card << std::endl;
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
	std::cout << "<技能> " << carrier.characterName() << "发动肘击，摸2张牌并获得2张变色" << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：突袭 ====================
bool 突袭::content(GameLogic& game, Player& carrier) {
	auto targetOpt = carrier.chooseOtherPlayer(L"【突袭】令一名其他角色的\"毒\"标记数量翻倍", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();
	if (!target.hasMark("毒")) {
		carrier.hint(L"该角色没有\"毒\"标记");
		return false;
	}
	std::size_t cnt = target.getMarkCount("毒");
	target.addMark("毒", cnt);
	std::cout << "<技能> " << carrier.characterName() << "突袭令" << target.characterName() << "的毒标记从" << cnt << "翻倍至" << cnt * 2 << std::endl;
	game.broadcastState();
	return true;
}

