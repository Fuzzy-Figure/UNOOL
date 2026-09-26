#include "../header/PassiveSkill.h"
#include "../header/InstantSkill.h"
#include "../header/TransformSkill.h"
#include "../header/GameLogic.h"
#include <algorithm>
#include <numeric>
#include <optional>

// ==================== 技能：粪怒 ====================
bool 粪怒::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getPlayer().handCount() == 1;
}
bool 粪怒::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getPlayer().draw(
		std::min(carrier.handCount(), 5uz),
		DrawReason::skill
	);
	return true;
}


// ==================== 技能：隐身 ====================
bool 隐身::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2, Card::Name::wild_draw4);
}
bool 隐身::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getCard().cancelEffect();
	trigger.getSource().draw(1, DrawReason::skill);
	return true;
}


// ==================== 技能：顶置 ====================
bool 顶置::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Pile& pile = game.getPile();

	if (pile.empty()) return false;

	const Card& bottomCard = pile.back();
	std::size_t choice = carrier.ask(
		L"牌堆底是" + bottomCard.toWString() + L"，是否顶置？",
		{ L"顶置", L"不顶置" },
		true
	);

	if (choice == 1) {
		std::unique_ptr<Card> card = pile.take_back(game.getDiscardPile());
		pile.push_front(std::move(card));
		std::cout << "<技能> 顶置成功！" << std::endl;
	}
	return true;
}


// ==================== 技能：带派 ====================
bool 带派::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t choice = carrier.ask(
		L"发动[带派]，选择一项：", {
		L"获得一张变色",
		L"获得一张+4",
		L"都获得并失去25体力"
		}, true);
	switch (choice) {
		case 1:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
			break;
		case 2:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
			break;
		case 3:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
			carrier.damage(25, carrier);
			break;
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：寒魄 ====================
bool 寒魄::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handCount() == 1;
}
bool 寒魄::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	carrier.getCardByIndex(0).set(card);
	game.broadcastState();
	return true;
}


// ==================== 技能：割腕 ====================
bool 割腕::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Color::red);
}
bool 割腕::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.damage(unool::random::randomSize_t(1, 5), carrier);
	game.broadcastState();
	return true;
}


// ==================== 技能：丑皇 ====================
bool 丑皇::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 丑皇::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::vector<ref<Card>> card = carrier.chooseToDiscard(
		L"弃置一张非数字牌或点击↓回复10体力", 1, false, &Card::isNotNumber
	);
	if (card.size() == 0) { //回血
		carrier.recover(10);
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：军国 ====================
bool 军国::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	if (player != carrier) //其他角色：失去 1% 最大体力，向上取整
		player.damage(unool::math::ceil(player.getMaxHp() * 0.01), carrier);
	else //自己：失去1体力
		player.damage(1, carrier);
	return true;
}


// ==================== 技能：家暴 ====================
bool 家暴::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return game.playersInclude(
		//有玩家的血量 < 携带者
		[&trigger, &carrier](const Player& p) {
		return p.getHp() < carrier.getHp();
	});
}
bool 家暴::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional targetOpt = carrier.chooseOtherPlayer(L"选择家暴目标：", false, [&carrier](const Player& p) {
		return p.getHp() < carrier.getHp();
	});
	if (!targetOpt.has_value()) return false;

	Player& target = targetOpt.value();
	std::size_t damage = unool::math::ceil(target.getMaxHp() * 0.1);
	target.damage(damage, carrier);
	std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName() << "发动家暴，造成" << damage << "点伤害！" << std::endl;

	game.broadcastState();
	return true;
}


// ==================== 技能：健身 ====================
bool 健身::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.recover(5);
	game.broadcastState();
	return true;
}


// ==================== 技能：做题 ====================
bool 做题::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	Card& card = trigger.getCard();
	if (card.isNumber()) return carrier.handInclude(&Card::isNotNumber);
	else return carrier.handInclude(&Card::isNumber);
}
bool 做题::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	if (card.isNumber()) {
		return carrier.chooseToDiscard(
			L"弃置一张非数字牌", 1,
			false, &Card::isNotNumber
		).size() == 1;
	}
	else {
		return carrier.chooseToDiscard(
			L"弃置一张数字牌", 1,
			false, &Card::isNumber
		).size() == 1;
	}
}


// ==================== 技能：棍击 ====================
bool 棍击::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 棍击::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional targetOpt = carrier.chooseOtherPlayer(L"选择棍击目标：", false);
	if (!targetOpt.has_value()) return false;

	Player& target = targetOpt.value();
	std::size_t damage = unool::math::pow(2, getCount());
	target.damage(damage, carrier);
	std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName()
		<< "发动棍击，造成" << damage << "点伤害！" << std::endl;

	game.broadcastState();
	return true;
}


// ==================== 技能：神木 ====================
bool 神木::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_pal), 9);
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_draw4), 9);
	game.getPile().shuffle();
	return true;
}


// ==================== 技能：雷剑 ====================
bool 雷剑::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_rev);
}
bool 雷剑::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	Card::Color color = card.getColor();

	auto discarded = carrier.chooseToDiscard(L"弃置一张数字牌", 1, false, &Card::isNumber);

	if (!discarded.empty()) {
		std::size_t value = discarded.front().get().value();
		carrier.recover(value);
		std::cout << "<技能> " << carrier.characterName()
			<< "发动雷剑，弃置 [" << discarded.front().get() << "] 并回复" << value << "点体力！！！！！！！！！！！！！" << std::endl;
		game.broadcastState();
		return true;
	}
	return false;
}


// ==================== 技能：买棋 ====================
bool 买棋::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.damage(10 * (getCount() - 1), carrier);
	if (unool::random::probability(0.5)) { //万能
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	}
	else { //+4
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
	}
	return true;
}


// ==================== 技能：卖棋 ====================
bool 卖棋::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isWild);
}
bool 卖棋::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (carrier.chooseToDiscard(L"弃置一张万能牌", 1, false, &Card::isWild).size() == 1) {
		carrier.recover(10);
		return true;
	}
	return false;
}


// ==================== 技能：耐克 ====================
bool 耐克::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	Card& card = trigger.getCard();
	if (!card.is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4))
		return false;
	auto lastOpt = game.lastCard();
	if (!lastOpt.has_value()) return false;
	const Card& last = lastOpt.value();
	return last.is(Card::Color::blue) || last.isWild();
}
bool 耐克::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getCard().cancelEffect();
	std::cout << carrier.characterName() << "触发技能，使此牌无效" << std::endl;
	return true;
}


// ==================== 技能：轰炸 ====================
bool 轰炸::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4);
}
bool 轰炸::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer().next();
	if (trigger.getCard().is(Card::Name::wild_draw4)) {
		target.damage(unool::math::ceil(target.getMaxHp() * 0.04), carrier);
	}
	else if (trigger.getCard().is(Card::Name::action_draw2)) {
		target.damage(unool::math::ceil(target.getMaxHp() * 0.02), carrier);
	}
	else {
		target.damage(unool::math::ceil(target.getMaxHp() * 0.01), carrier);
	}
	return true;
}


// ==================== 技能：爆破 ====================
bool 爆破::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getSource() == carrier;
}
bool 爆破::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	Hand& hand = player.getHand();
	auto card = hand.takeCardByIndex(unool::random::randomSize_t(0, hand.count() - 1));
	player.damage(card->value(), carrier);
	std::cout << "爆破获取了 [" << *card << "]，造成了" << card->value() << "点伤害！" << std::endl;
	carrier.gainCard(std::move(card));
	return true;
}


// ==================== 技能：电音 ====================
bool 电音::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Hand& hand = carrier.getHand();
	hand.forEachIf(&Card::isNumber, [](Card& card) {
		card.setName(unool::random::randomGet(Card::numberCardsFrom0));
	});
	carrier.recover(1);
	game.broadcastState();
	return true;
}


// ==================== 技能：蒙面 ====================
bool 蒙面::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getNumber() = unool::math::ceil(trigger.getNumber() * 0.75);
	return true;
}


// ==================== 技能：锐刻 ====================
bool 锐刻::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (disabled) return false;
	return trigger.getCard().getName() == Card::Name::number_5;
}
bool 锐刻::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t choice = carrier.ask(
		L"发动[锐刻]，选择一项：", {
		L"令一名角色摸1张牌",
		L"令一名角色摸5张牌并失去此技能至本局结束"
		}, false
	);

	if (choice == 0) return false;

	const auto& candidates = game.getPlayers();
	std::vector<std::wstring> options;
	for (const auto& p : candidates) {
		options.push_back(p.get().characterNameW());
	}
	std::size_t targetChoice = carrier.ask(L"选择目标角色：", options, true);
	Player& target = candidates[targetChoice - 1].get();

	if (choice == 1) {
		target.draw(1, DrawReason::skill);
		std::cout << "<技能> " << carrier.characterName() << "发动锐刻，令" << target.characterName() << "摸1张牌" << std::endl;
	}
	else {
		target.draw(5, DrawReason::skill);
		disabled = true;
		std::cout << "<技能> " << carrier.characterName() << "发动锐刻，令" << target.characterName()
			<< "摸5张牌，失去此技能至本局结束" << std::endl;
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：巨富 ====================
bool 巨富::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.draw(4, DrawReason::skill);
	game.broadcastState();
	return true;
}

// ==================== 子技能：巨富_摸牌 ====================
bool 巨富_摸牌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//摸牌阶段额外摸一张牌
	if (trigger.hasDrawReason() && trigger.getDrawReason() == DrawReason::phase_draw) {
		trigger.getNumber() += 1;
	}
	return true;
}


// ==================== 技能：破产 ====================
bool 破产::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	std::size_t myCount = carrier.handCount();
	if (myCount == 0) return false;
	for (const auto& p : game.getPlayers()) {
		if (p.get().handCount() > myCount) return false;
	}
	return true;
}
bool 破产::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Hand& hand = carrier.getHand();
	carrier.discardByIndex(unool::random::randomSize_t(0, hand.count() - 1));
	std::cout << "<技能> " << carrier.characterName() << "触发破产，随机弃置一张牌" << std::endl;
	game.broadcastState();
	return true;
}



// ==================== 技能：假酒 ====================
bool 假酒::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isAction();
}
bool 假酒::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto card = std::make_unique<Card>(Card::randomCard());
	if (card->isNumber()) number = true;
	else if (card->isAction()) action = true;
	else if (card->isWild()) wild = true;

	carrier.gainCard(std::move(card));

	//弃牌
	if (number && action && wild) {
		carrier.chooseToDiscard(L"[假酒] 弃置两张牌", 2, true);
		limit = 0;
	}
	game.broadcastState();
	return true;
}
void 假酒::reset() {
	number = action = wild = false;
	limit = unlimited;
}

// ==================== 技能：窃观 ====================
bool 窃观::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCards().size() == 1;
}
bool 窃观::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& drawer = trigger.getPlayer();
	const Card& card = trigger.getCard();
	std::wstring title = L"【窃观】" + drawer.characterNameW()
		+ L"获得了 " + card.toWString();
	carrier.hint(title);
	return true;
}


// ==================== 技能：生存 ====================
bool 生存::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handCount() > 0;
}
bool 生存::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t x = getCount();
	if (x > 9) x = 9;
	Card::Name targetName = static_cast<Card::Name>(static_cast<int>(Card::Name::number_0) + x);

	Card::Color targetColor = unool::random::randomGet(Card::fourColors);

	std::unique_ptr<Card> target = Card::make(targetColor, targetName);
	auto opt = carrier.chooseToOperate(
		L"请选择一张牌变为【" + std::to_wstring(x) + L"】",
		true, unool::alwaysTrue,
		[&target](Card& c) {
		c.set(*target);
	});
	if (opt.has_value()) {
		std::cout << "<技能> " << carrier.characterName() << "发动生存，将一张手牌改为【"
			<< *target << "】" << std::endl;
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：创造 ====================
bool 创造::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (c.getName() != Card::Name::number_9) return false;
	if (c.is(Card::Color::black)) return false;
	return usedColors.find(c.getColor()) == usedColors.end();
}
bool 创造::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const Card& nine = trigger.getCard();
	Card::Color targetColor = nine.getColor();

	std::vector<std::wstring> opts = {
		L"1", L"2", L"3", L"4", L"5",
		L"6", L"7", L"8", L"9", L"0",
		L"反转", L"封禁", L"+2"
	};
	std::size_t idx = carrier.ask(L"【创造】选择获得的牌名（" + Card::to_wstring(targetColor) + L"色）：", opts, true);
	Card::Name name;
	if (1 <= idx && idx <= 10) { //数字牌
		name = Card::numberCardsFrom1[idx];
	}
	else if (11 <= idx && idx <= 13) { //功能牌
		name = Card::actionCards[idx - 11];
	}
	else throw std::runtime_error("意外的 idx 的值");

	std::unique_ptr<Card> newCard = Card::make(targetColor, name);
	std::cout << "<技能> " << carrier.characterName() << "发动创造，" <<
		"获得一张【" << *newCard << "】" << std::endl;
	carrier.gainCard(std::move(newCard));
	usedColors.insert(targetColor);
	game.broadcastState();
	return true;
}


// ==================== 技能：炼兵 ====================
std::map<Card::Name, std::size_t> 炼兵::buildPairs(const Player& carrier) const {
	std::map<Card::Name, std::size_t> cnt;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		const Card& c = carrier.getHand().getCardByIndex(i);
		if (c.is(Card::Color::black)) continue;
		if (c.isWild()) continue;
		cnt[c.getName()]++;
	}
	return cnt;
}
bool 炼兵::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (carrier.handCount() < 2) return false;
	auto cnt = buildPairs(carrier);
	for (const auto& kv : cnt) {
		if (kv.second >= 2 && usedNames.find(kv.first) == usedNames.end()) {
			return true;
		}
	}
	return false;
}
bool 炼兵::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto cnt = buildPairs(carrier);
	std::vector<Card::Name> validPairs;
	std::vector<std::wstring> opts;
	for (const auto& kv : cnt) {
		if (kv.second >= 2 && usedNames.find(kv.first) == usedNames.end()) {
			validPairs.push_back(kv.first);
			opts.push_back(Card::to_wstring(kv.first) + L"(" + std::to_wstring(static_cast<unsigned long long>(kv.second)) + L"张)");
		}
	}
	if (validPairs.empty()) return false;

	std::size_t idx = carrier.ask(L"【炼兵】选择要弃的同名牌：", opts, false);
	if (idx == 0) return false;
	if (idx - 1 >= validPairs.size()) return false;

	Card::Name target = validPairs[idx - 1];
	std::size_t i1 = carrier.handCount(), i2 = carrier.handCount();
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		const Card& c = carrier.getHand().getCardByIndex(i);
		if (c.getName() == target) {
			if (i1 >= carrier.handCount()) { i1 = i; }
			else { i2 = i; break; }
		}
	}
	if (i1 >= carrier.handCount() || i2 >= carrier.handCount()) return false;
	if (i1 > i2) std::swap(i1, i2);
	carrier.discardByIndex(i2);
	carrier.discardByIndex(i1);
	Card::Color color = unool::random::randomGet(Card::fourColors);
	carrier.gainCard(Card::make(color, Card::Name::action_draw2));
	usedNames.insert(target);
	std::cout << "<技能> " << carrier.characterName() << "发动炼兵，弃两张"
		<< Card::to_string(target)
		<< "，获得一张" << Card::to_string(color) << "+2" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：好火 ====================
bool 好火::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!c.is(Card::Color::red)) return false;
	Player& player = trigger.getPlayer();
	if (player == carrier) return false;
	if (player.getHp() <= carrier.getHp()) return false;
	if (usedPlayerIds.find(player.getId()) != usedPlayerIds.end()) return false;
	if (carrier.handCount() == 0) return false;
	return true;
}
bool 好火::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();

	auto result = carrier.chooseToGive(L"选择一张手牌交给" + target.characterNameW(),
									   target, false, unool::alwaysTrue);
	if (!result.has_value()) return false;

	usedPlayerIds.insert(target.getId());
	std::cout << "<技能> " << carrier.characterName() << "发动好火，交给"
		<< target.characterName() << "一张" << result.value().get() << std::endl;
	return true;
}


//================== 森罗 =================
bool 森罗::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.getHand().forEachIf(
		[](const Card& c) {
		return c.getColor() != Card::Color::black;
	}, [](Card& c) {
		c.setColor(Card::Color::green);
	});
	return true;
}


bool 大脚::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isWild);
}

//================大脚=====================
bool 大脚::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard(L"弃置一张万能牌", 1, true, &Card::isWild);
	carrier.getHand().forEachIf(
		[](const Card& c) {
		return c.getColor() != Card::Color::black;
	}, [](Card& c) {
		c.setColor(Card::Color::green);
	});
	game.broadcastState();
	return true;
}


//================过江==============
bool 过江::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2);
}
bool 过江::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getSource().draw(2, DrawReason::skill);
	return true;
}


//================大盏==============
void 大盏::randomEnlarge(Card& c) {
	//非数字牌，不能变大
	if (c.isNotNumber()) return;
	//9不能变大
	if (c.is(Card::Name::number_9)) return;
	//变大
	std::size_t currentNumber = c.value();
	c.setName(Card::numberCardsFrom0[unool::random::randomSize_t(currentNumber + 1, 9)]);
}
bool 大盏::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude([](const Card& c) {
		return c.isNumber();
	});
}
bool 大盏::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	// 找到最小数字
	std::size_t min = 100;
	carrier.getHand().forEach([&min](const Card& c) {
		if (c.isNotNumber()) return;
		if (c.value() < min) {
			min = c.value();
		}
	});


	if (min != 9) { //min不是9，存在数字牌点数不是9
		// 让点数最小的数字牌随机变大
		carrier.getHand().forEach([&min](Card& c) {
			if (c.isNumber() && c.value() == min) {
				randomEnlarge(c);
			}
		});
	}
	else { //min == 9，说明数字牌全是9，将一张9变为红色
		carrier.chooseToOperate(
			L"选择一张[9]变成红色", false,
			[](const Card& c) {
			return c.value() == 9;
		}, [](Card& c) {
			c.setColor(Card::Color::red);
		});
		carrier.recover(1);
	}
	game.broadcastState();
	return true;
}


//==============举报=============
bool 举报::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 举报::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	setForced(true);
	Player& player = trigger.getPlayer();
	player.damage(unool::math::ceil(0.1 * player.getHp()), carrier);
	return true;
}
void 举报::reset() {
	PassiveSkill::reset();
	setForced(false);
}


//=============猥琐====================
bool 猥琐::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//找到最大体力
	Character::hp_t maxHp = 0;
	game.forEachPlayer([&maxHp](const Player& p) {
		maxHp = std::max(maxHp, p.getHp());
	});
	return carrier.getHp() != maxHp;
}
bool 猥琐::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.recover(1);
	return true;
}



bool 棋王::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return game.getDiscardPile().count() >= 2
		&& trigger.getCard() == game.getDiscardPile()[1];
}
bool 棋王::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard(L"弃置两张手牌", 2, true);
	return true;
}


bool 金铲::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getNumber() == 1;
}
bool 金铲::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	trigger.getNumber() = 0;
	trigger.getPlayer().damage(1, carrier);
	return true;
}


//==============淘汰=============
bool 淘汰::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	Card& card = trigger.getCard();
	if (card.isNumber()) {
		int half = static_cast<int>(unool::math::floor(card.value() / 2.0));
		return carrier.handInclude([&card, &half](const Card& c) {
			return c.value() <= half && c.sameColorAs(card) && c.isNumber();
		});
	}
	else return card.isAction();
}
bool 淘汰::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = trigger.getCard();
	if (card.isNumber()) {
		int half = static_cast<int>(unool::math::floor(card.value() / 2.0));
		carrier.chooseToDiscard(
			L"弃置一张点数 <= " + std::to_wstring(half) + L"的" +
			Card::to_wstring(card.getColor()) + L"色数字牌", 1, true, [&card, &half](const Card& c) {
			return c.value() <= half && c.sameColorAs(card) && c.isNumber();
		});
	}
	else if (card.isAction()) {
		carrier.gainCard(Card::make(
			card.getColor(),
			unool::random::randomGet(Card::actionCards)
		));
	}
	return true;
}


bool 光合::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_skip, Card::Name::action_draw2);
}
bool 光合::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& card = carrier.judge();
	if (card.isNumber()) {
		trigger.getSource().draw(1, DrawReason::skill);
	}
	else if (card.isAction()) {
		trigger.getCard().cancelEffect();
		if (trigger.getCard().is(Card::Name::action_draw2)) carrier.draw(2, DrawReason::skill);
	}
	return true;
}


bool 射门::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isNumber();
}
bool 射门::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//选角色
	const auto& targetOpt = carrier.choosePlayer(L"选择射门目标：", true);
	if (!targetOpt.has_value()) return false;

	//判定
	Player& target = targetOpt.value();
	if (target.judge().is(Card::Color::blue, Card::Color::black)) {
		//执行效果
		if (target == carrier) {
			target.chooseToDiscard(L"[射门] 弃置一张牌", 1, true);
		}
		else {
			target.draw(1, DrawReason::skill);
		}
	}
	return true;
}


// ==================== 技能：追番 ====================
bool 追番::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//先检测手中是否有点数≤5的数字牌
	return carrier.handInclude([](const Card& c) {
		return c.isNumber() && c.value() <= 5;
	});
}
bool 追番::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	static auto lessEqual5Number = [](const Card& c) {
		return c.isNumber() && c.value() <= 5;
	};

	//选择一张点数≤5的数字牌
	return carrier.chooseToOperate(
		L"选择一张点数≤5的数字牌", false, lessEqual5Number,
		[&carrier, &game](Card& card) {
		//自选+1/+2/+3
		const std::size_t addChoice = carrier.ask(
			L"选择增加的点数：",
			{ L"+1", L"+2", L"+3" },
			false
		);
		card.setName(Card::numberCardsFrom0[card.value() + addChoice]);
		std::cout << "<技能> " << carrier.characterName() << "发动追番，将一张"
			<< card << "的点数+" << addChoice << std::endl;
		game.broadcastState();
	}
	).has_value();
}

// ==================== 技能：崩三 ====================
bool 崩三::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (trigger.getCard().is(Card::Name::number_3)) ++count3;
	else return false;
	return count3 % 2 == 0
		&& carrier.handInclude([](const Card& c) { return c.is(Card::Name::number_6); });
}
bool 崩三::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard(
		L"选择一张[6]弃置", 1, false,
		[](const Card& c) { return c.is(Card::Name::number_6); });
	game.broadcastState();
	return true;
}

// ==================== 技能：望日 ====================
bool 望日::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude([](const Card& c) {
		return c.is(Card::Color::yellow) && c.isNumber() && c.value() < 9;
	});
}
bool 望日::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto cardRef = carrier.chooseToOperate(
		L"【望日】\n选择一张黄色数字牌点数+1", true,
		[](const Card& c) {
		return c.is(Card::Color::yellow) && c.isNumber() && c.value() < 9;
	}, [](Card&) {});
	if (!cardRef.has_value()) return false;
	Card& card = cardRef->get();
	card.setName(Card::numberCardsFrom0[card.value() + 1]);
	std::cout << "<技能> " << carrier.characterName() << "发动望日，将一张"
		<< card << "的点数+1" << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：慈父_子（望日子技能） ====================
bool 望日_子::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	return c.is(Card::Color::yellow) && c.is(Card::Name::number_9);
}
bool 望日_子::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
	std::cout << "<技能> " << carrier.characterName() << "发动慈父，获得一张【+4】" << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：朔日 ====================
bool 朔日::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!c.is(Card::Color::yellow)) return false;
	bool hasNumber = carrier.handInclude([](const Card& c) { return c.isNumber(); });
	bool hasAction = carrier.handInclude([](const Card& c) { return c.isAction(); });
	return hasNumber || hasAction;
}
bool 朔日::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//回复1点体力
	carrier.recover(1);
	std::cout << "<技能> " << carrier.characterName() << "发动朔日，回复1点体力" << std::endl;
	game.broadcastState();

	//构建可选项
	bool hasNumber = carrier.handInclude([](const Card& c) { return c.isNumber(); });
	bool hasAction = carrier.handInclude([](const Card& c) { return c.isAction(); });

	std::vector<std::wstring> opts;
	std::vector<int> branchMap;
	if (hasNumber) {
		opts.push_back(L"将一张数字牌变为黄色并调整点数");
		branchMap.push_back(1);
	}
	if (hasAction) {
		opts.push_back(L"将一张功能牌变为黄色随机功能牌");
		branchMap.push_back(2);
	}

	std::size_t choice = carrier.ask(L"【朔日】选择一项：", opts, false);
	if (choice == 0) return true; //取消，但已回复体力

	int branch = branchMap[choice - 1];

	if (branch == 1) {
		//选择一张数字牌，变黄色
		auto cardRef = carrier.chooseToOperate(
			L"选择一张数字牌变为黄色", false,
			[](const Card& c) { return c.isNumber(); },
			[](Card& c) { c.setColor(Card::Color::yellow); });
		if (!cardRef.has_value()) return true;
		Card& card = cardRef->get();
		//选择+1/-1
		std::vector<std::wstring> adjOpts;
		std::vector<int> adjMap;
		if (card.value() > 0) {
			adjOpts.push_back(L"-1");
			adjMap.push_back(-1);
		}
		if (card.value() < 9) {
			adjOpts.push_back(L"+1");
			adjMap.push_back(1);
		}
		if (!adjOpts.empty()) {
			std::size_t adjChoice = carrier.ask(L"选择调整点数：", adjOpts, false);
			if (adjChoice > 0) {
				int adj = adjMap[adjChoice - 1];
				card.setName(Card::numberCardsFrom0[card.value() + adj]);
			}
		}
		std::cout << "<技能> " << carrier.characterName() << "发动朔日，将一张牌变为"
			<< card << std::endl;
	}
	else {
		//选择一张功能牌
		auto cardRef = carrier.chooseToOperate(
			L"选择一张功能牌变为黄色随机功能牌", false,
			[](const Card& c) { return c.isAction(); },
			[](Card&) {});
		if (!cardRef.has_value()) return true;
		Card& card = cardRef->get();
		//随机功能牌名
		Card::Name randomName = unool::random::randomGet(Card::actionCards);
		card.setName(randomName);
		card.setColor(Card::Color::yellow);
		std::cout << "<技能> " << carrier.characterName() << "发动朔日，将一张牌变为"
			<< card << std::endl;
	}

	game.broadcastState();
	return true;
}

// ==================== 技能：健忘 ====================
bool 健忘::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::vector<std::wstring> options = {
		Card::to_wstring(Card::Color::blue),
		Card::to_wstring(Card::Color::red),
		Card::to_wstring(Card::Color::green),
		Card::to_wstring(Card::Color::yellow)
	};
	std::size_t choice = carrier.ask(L"【健忘】请选择新的公共颜色", options, true);
	Card::Color newColor;
	switch (choice) {
		case 1: newColor = Card::Color::blue; break;
		case 2: newColor = Card::Color::red; break;
		case 3: newColor = Card::Color::green; break;
		case 4: newColor = Card::Color::yellow; break;
		default: newColor = Card::Color::blue; break;
	}
	game.setCurrentColor(newColor);
	std::cout << "<技能> " << carrier.characterName() << "发动健忘，将公共颜色改为"
		<< Card::to_string(newColor) << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：豪赌 ====================
bool 豪赌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//判定
	Card& card = carrier.judge();
	game.forEachPlayer([&card](Player& p) {
		p.hint(L"[豪赌] 判定结果是" + card.toWString());
	});

	if (card.is(Card::Color::green, Card::Color::black)) {
		//从弃牌堆顶取回这张牌，加入手牌
		std::unique_ptr<Card> cardU = game.getDiscardPile().takeCardByIndex(0);
		carrier.gainCard(std::move(cardU));
	}
	else if (card.is(Card::Color::yellow)) {
		carrier.ban();
	}
	else if (card.is(Card::Color::blue)) {
		if (!carrier.handEmpty()) {
			carrier.chooseToRecast(L"【豪赌】重铸一张手牌", 1, true);
		}
	}
	else if (card.is(Card::Color::red)) {
		carrier.damage(5, carrier);
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：黑帮 ====================
bool 黑帮::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t X = game.getMatchCount();
	for (std::size_t i = 0; i < X; ++i) {
		carrier.gainCard(Card::make(Card::Color::black,
									unool::random::randomGet(Card::wildCards)));
	}
	std::cout << "<技能> " << carrier.characterName() << "发动黑帮，获得了" << X << "张万能牌" << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：有活 ====================
bool 有活::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t X = game.getMatchCount();

	carrier.draw(2, DrawReason::skill);
	std::cout << "<技能> " << carrier.characterName() << "发动有活，摸了2张牌" << std::endl;

	carrier.chooseToDiscard(L"[有活] 弃置" + std::to_wstring(X) + L"张牌", X, true);
	std::cout << "<技能> " << carrier.characterName() << "发动有活，弃置了" << X << "张牌" << std::endl;

	game.broadcastState();
	return true;
}

// ==================== 技能：拖拉 ====================
bool 拖拉::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 拖拉::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card::Name targetName = trigger.getCard().getName();

	std::vector<std::size_t> matchingIndices;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		if (carrier.getCardByIndex(i).getName() == targetName) {
			matchingIndices.push_back(i);
		}
	}

	for (auto it = matchingIndices.rbegin(); it != matchingIndices.rend(); ++it) {
		carrier.discardByIndex(*it);
	}

	std::size_t count = matchingIndices.size();
	if (count > 0) {
		carrier.recover(count);
	}
	std::cout << "<技能> " << carrier.characterName() << "发动拖拉，弃置了" << count << "张牌，回复" << count << "点体力" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：互质 ====================
bool 互质::areCoprime(const std::size_t a, const std::size_t b) {
	if (a == 0 || b == 0) {
		return false;  // 0 与任何数（包括 0）都不互质
	}
	return std::gcd(a, b) == 1;
}
bool 互质::isPairwiseCoprime(const std::vector<std::size_t>& nums) {
	// 空集或单元素集视为两两互质
	if (nums.size() <= 1) {
		return true;
	}
	// 检查是否存在 0（有 0 且不止一个元素则必然不互质）
	if (std::ranges::any_of(nums, [](std::size_t x) { return x == 0; })) {
		return false;
	}
	// 使用索引视图生成所有数对并检查
	for (auto i : std::views::iota(0uz, nums.size())) {
		for (auto j : std::views::iota(i + 1, nums.size())) {
			if (!areCoprime(nums[i], nums[j])) {
				return false;
			}
		}
	}
	return true;
}
bool 互质::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Hand& hand = carrier.getHand();
	std::vector<std::size_t> nums;
	for (const auto& c : hand) {
		if (c->isNumber()) nums.push_back(c->value());
	}
	return isPairwiseCoprime(nums);
}
bool 互质::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t product = 1;
	carrier.getHand().forEachIf(
		&Card::isNumber,
		[&product](const Card& c) {
		product *= c.value();
	}
	);
	carrier.damage(product, carrier);
	return true;
}


// ==================== 技能：难题_变牌（子技能） ====================
bool 难题_变牌::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return !record->empty();
}
bool 难题_变牌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::wstring recordStr = L"已记录的点数：";
	for (const auto& x : *record) {
		recordStr += Card::to_wstring(x) + L",";
	}
	auto cardRef = carrier.chooseToOperate(
		L"【难题】\n" + recordStr + L"\n" +
		L"选择一张非万能牌变为随机已记录点数的同色数字牌", false,
		&Card::isNotWild,
		[this](Card& c) { c.setName(unool::random::randomGet(*record)); }
	);
	if (!cardRef.has_value()) return false;
	std::cout << "<技能> " << carrier.characterName() << "发动难题，将一张牌变为"
		<< cardRef.value().get() << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：难题 ====================
bool 难题::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	for (const auto& c : trigger.getCards()) {
		if (c.get().isNumber()) return true;
	}
	return false;
}
bool 难题::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	for (const auto& c : trigger.getCards()) {
		if (c.get().isNumber()) {
			Card::Name val = c.get().getName();
			if (std::ranges::find(*record, val) == record->end()) {
				record->push_back(val);
				std::cout << "<技能> " << carrier.characterName() << "发动难题，"
					"记录点数" << Card::to_string(val) << std::endl;
			}
		}
	}
	return true;
}


// ==================== 技能：迷烟 ====================
bool 迷烟::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional cardOpt = carrier.chooseToShow(L"[迷烟] 选择一张非万能牌", false, &Card::isNotWild);
	if (!cardOpt.has_value()) return false;
	Card& card = cardOpt.value().get();

	std::optional targetOpt = carrier.chooseOtherPlayer(L"[迷烟] 选择一名其他玩家", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	std::wstring colorStr = Card::to_wstring(card.getColor());
	std::vector discard = target.chooseToDiscard(
		L"[迷烟]\n弃置一张" + colorStr + L"色手牌或万能牌，\n或取消并摸一张牌", 1, false,
		[&card](const Card& c) {
		return c.sameColorAs(card) || c.isWild();
	});
	if (discard.size() == 0) { //没弃牌，摸一张
		target.draw(1, DrawReason::skill);
	}
	return true;
}


bool 创世::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//收集手牌中已有的牌名
	std::unordered_set<Card::Name> handNames;
	for (const auto& x : carrier.getHand()) {
		handNames.insert(x->getName());
	}
	//筛选手牌中没有的牌名
	std::vector<Card::Name> available;
	for (const auto& name : Card::allCards) {
		if (!handNames.contains(name)) {
			available.push_back(name);
		}
	}

	//选牌名
	auto nameOpt = carrier.chooseCardName(L"【创世】选择一个牌名", false, available);
	if (!nameOpt.has_value()) return false;
	Card::Name selectedName = nameOpt.value();

	//选颜色
	Card::Color selectedColor = Card::Color::black;
	if (!Card::is_wild(selectedName)) {
		std::vector<Card::Color> colorVec(Card::fourColors.begin(), Card::fourColors.end());
		auto colorOpt = carrier.chooseCardColor(L"【创世】选择颜色", false, colorVec);
		if (!colorOpt.has_value()) return false;
		selectedColor = colorOpt.value();
	}

	//选手牌变为此牌
	Card targetCard(selectedColor, selectedName);
	auto cardOpt = carrier.chooseToOperate(
		L"【创世】选择一张手牌变为" + targetCard.toWString(), false, unool::alwaysTrue,
		[&targetCard](Card& c) {
		c.set(targetCard);
	});
	if (!cardOpt.has_value()) return false;
	std::cout << "<技能> " << carrier.characterName() << "发动创世，将一张牌变为"
		<< targetCard << std::endl;
	game.broadcastState();
	return true;
}

bool 补天::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const auto& hand = carrier.getHand();
	if (hand.empty()) return false;
	//打出的牌是手中该牌名的唯一一张（剩余手牌中无此牌名）
	Card::Name playedName = trigger.getCard().getName();
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].getName() == playedName) return false;
	}
	//该牌名未被记录
	if (std::ranges::find(record, playedName) != record.end()) return false;
	//至少有一个未记录的牌名（排除即将记录的playedName）
	for (const auto& name : Card::allCards) {
		if (name != playedName && std::ranges::find(record, name) == record.end()) {
			return true;
		}
	}
	return false;
}
bool 补天::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card::Name recordedName = trigger.getCard().getName();

	//记录牌名
	record.push_back(recordedName);

	//提示已记录的牌名（按分值从小到大）
	if (!record.empty()) {
		std::vector<Card::Name> sorted(record.begin(), record.end());
		std::ranges::sort(sorted, [](Card::Name a, Card::Name b) {
			return static_cast<int>(a) < static_cast<int>(b);
		});
		std::wstring hintText = L"【补天】已记录的牌名：";
		for (std::size_t i = 0; i < sorted.size(); ++i) {
			if (i > 0) hintText += L"、";
			hintText += Card::to_wstring(sorted[i]);
		}
		carrier.hint(hintText);
	}

	//算出所有可选的牌名（排除已记录的），从中随机选一个牌名
	std::vector<Card::Name> namePool;
	for (const auto& name : Card::allCards) {
		if (std::ranges::find(record, name) != record.end()) continue;
		namePool.push_back(name);
	}
	if (namePool.empty()) return false;
	Card::Name targetName = unool::random::randomGet(namePool);

	//万能牌固定黑色，其他牌自选颜色
	Card::Color targetColor;
	if (Card::is_wild(targetName)) {
		targetColor = Card::Color::black;
	}
	else {
		auto colorOpt = carrier.chooseCardColor(
			L"【补天】牌名是" + Card::to_wstring(targetName) + L"，选择颜色", true);
		if (!colorOpt.has_value()) return false;
		targetColor = colorOpt.value();
	}
	Card targetCard(targetColor, targetName);

	//选择一张手牌变为此牌
	auto cardOpt = carrier.chooseToOperate(
		L"【补天】选择一张手牌变为" + targetCard.toWString(), true, unool::alwaysTrue,
		[&targetCard](Card& c) {
		c.set(targetCard);
	});
	if (!cardOpt.has_value()) return false;
	std::cout << "<技能> " << carrier.characterName() << "发动补天，将一张牌变为"
		<< targetCard << std::endl;
	game.broadcastState();
	return true;
}

bool 水鬼::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//至少一张获得的牌是蓝色
	std::size_t blueCount = 0;
	for (const auto& c : trigger.getCards()) {
		if (c.get().is(Card::Color::blue)) ++blueCount;
	}
	if (blueCount == 0) return false;
	//弃置蓝色牌后手牌非空（能弃置一张其他牌）
	return carrier.handCount() > blueCount;
}
bool 水鬼::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//找出获得的蓝色牌在手牌中的下标
	std::vector<std::size_t> blueIndices;
	for (const auto& c : trigger.getCards()) {
		if (c.get().getColor() != Card::Color::blue) continue;
		const Card* ptr = &c.get();
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			if (&carrier.getCardByIndex(i) == ptr) {
				blueIndices.push_back(i);
				break;
			}
		}
	}
	//从大到小弃置（避免下标偏移）
	std::ranges::sort(blueIndices, std::greater{});
	for (std::size_t idx : blueIndices) {
		carrier.discardByIndex(idx);
	}
	std::cout << "<技能> " << carrier.characterName() << "发动水鬼，弃置了"
		<< blueIndices.size() << "张蓝色牌" << std::endl;
	//发状态包，确保客户端渲染前有最新数据
	game.broadcastState();
	//弃置一张其他牌
	carrier.chooseToDiscard(L"【水鬼】弃置一张其他牌", 1, true);
	game.broadcastState();
	return true;
}

bool 爆缸::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Hand& hand = carrier.getHand();
	return std::ranges::all_of(hand, &Card::isNumber);
}
bool 爆缸::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	setForced(true);
	std::optional card = carrier.chooseToOperate(
		L"[爆缸] 选择一张数字牌变为同色的随机功能牌", true,
		&Card::isNumber, [](Card& c) {
		c.setName(unool::random::randomGet(Card::actionCards));
	});
	return true;
}
void 爆缸::reset() {
	setForced(false);
}

bool 叛党::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Hand& hand = carrier.getHand();
	//手牌至少一张有色牌（颜色 != no）
	for (const auto& c : hand) {
		if (c->getColor() != Card::Color::no) return true;
	}
	return false;
}
bool 叛党::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Hand& hand = carrier.getHand();
	//统计手牌中的不同颜色
	std::unordered_set<Card::Color> colorSet;
	for (const auto& c : hand) {
		if (c->getColor() != Card::Color::no) {
			colorSet.insert(c->getColor());
		}
	}
	if (colorSet.empty()) return false;

	//构造选项：1.弃1张 2.弃2张 ...
	std::vector<std::wstring> options;
	for (std::size_t i = 1; i <= colorSet.size(); ++i) {
		options.push_back(L"弃" + std::to_wstring(i) + L"张颜色各不相同的牌");
	}
	//选数量，0=取消
	std::size_t count = carrier.ask(L"【叛党】选择弃牌数量", options, false,
									std::chrono::milliseconds(60000));
	if (count == 0) return false;

	std::unordered_set<Card::Color> usedColors;
	std::size_t discarded = 0;
	for (std::size_t i = 0; i < count; ++i) {
		auto result = carrier.chooseToDiscard(
			L"【叛党】选择第" + std::to_wstring(i + 1) + L"张牌弃置（颜色各不相同）",
			1, false,
			[&](const Card& c) {
			return c.getColor() != Card::Color::no
				&& !usedColors.contains(c.getColor());
		}
		);
		if (result.empty()) break; //玩家取消
		usedColors.insert(result[0].get().getColor());
		++discarded;
	}
	if (discarded > 0) {
		std::cout << "<技能> " << carrier.characterName() << "发动叛党，弃置了"
			<< discarded << "张颜色各不相同的牌" << std::endl;
		game.broadcastState();
	}
	return discarded > 0;
}

bool 清洗::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Hand& hand = carrier.getHand();
	//手牌仅有两种颜色（排除 Color::no）
	std::unordered_set<Card::Color> colorSet;
	for (const auto& c : hand) {
		if (c->getColor() != Card::Color::no) {
			colorSet.insert(c->getColor());
		}
	}
	return colorSet.size() == 2;
}
bool 清洗::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Hand& hand = carrier.getHand();
	//收集出现过的两种颜色
	std::vector<Card::Color> colors;
	std::unordered_set<Card::Color> colorSet;
	for (const auto& c : hand) {
		Card::Color col = c->getColor();
		if (col != Card::Color::no && !colorSet.contains(col)) {
			colorSet.insert(col);
			colors.push_back(col);
		}
	}
	if (colors.size() != 2) return false;

	//ask 让玩家二选一：弃哪种颜色
	std::vector<std::wstring> options;
	options.push_back(L"弃置所有" + Card::to_wstring(colors[0]) + L"色牌");
	options.push_back(L"弃置所有" + Card::to_wstring(colors[1]) + L"色牌");
	std::size_t choice = carrier.ask(L"【清洗】选择弃置哪种颜色的手牌", options, false);
	if (choice == 0) return false; //0取消
	Card::Color target = colors[choice - 1];

	//收集该颜色牌的下标
	std::vector<std::size_t> indices;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].is(target)) indices.push_back(i);
	}
	std::ranges::sort(indices, std::greater{});
	for (std::size_t idx : indices) {
		carrier.discardByIndex(idx);
	}

	std::size_t discardCount = indices.size();
	std::cout << "<技能> " << carrier.characterName() << "发动清洗，弃置了"
		<< discardCount << "张" << Card::to_string(target) << "色牌" << std::endl;

	//若弃置了蓝色牌，回复两倍弃牌数点体力
	if (target == Card::Color::blue && discardCount > 0) {
		std::size_t heal = 2 * discardCount;
		carrier.recover(heal);
		std::cout << "<技能> " << carrier.characterName() << "回复了"
			<< heal << "点体力" << std::endl;
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：落水 ====================
bool 落水::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().is(Card::Color::blue);
}
bool 落水::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	setForced(true);
	Player& player = trigger.getPlayer();
	Pile& pile = game.getPile();

	//其他三种颜色
	std::vector<Card::Color> otherColors;
	for (auto c : Card::fourColors) {
		if (c != Card::Color::blue) otherColors.push_back(c);
	}

	for (Card::Color targetColor : otherColors) {
		//在牌堆中搜索此颜色的牌
		std::vector<std::size_t> indices;
		for (std::size_t i = 0; i < pile.count(); ++i) {
			if (pile[i].is(targetColor)) indices.push_back(i);
		}
		if (indices.empty()) continue;
		std::size_t pick = unool::random::randomSize_t(0, indices.size() - 1);
		auto card = pile.takeCardByIndex(indices[pick]);
		player.gainCard(std::move(card));
	}

	std::cout << "<技能> " << carrier.characterName()
		<< "发动落水，" << player.characterName() << "从牌堆获得了其他三色牌各一张" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：骚扰 ====================
bool 骚扰::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return !disabled;
}
bool 骚扰::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Card& result = carrier.judge();
	if (result.is(Card::Color::blue)) {
		//判定为蓝色：失去此技能至本局结束
		disabled = true;
		std::cout << "<技能> " << carrier.characterName()
			<< "发动骚扰，判定为蓝色，失去此技能至本局结束" << std::endl;
		game.broadcastState();
		return true;
	}

	//判定非蓝色：回复1点体力
	carrier.recover(1);
	//重置【落水】使用次数（不改是否锁定）
	if (auto opt = carrier.findSkill<落水>()) {
		opt->get().resetCount();
	}
	std::cout << "<技能> " << carrier.characterName()
		<< "发动骚扰，判定非蓝色，回复1点体力并重置落水次数" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：犬子 ====================
bool 犬子::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	++playCount;
	carrier.markCharInfoDirty();
	return playCount >= count + 1;
}
bool 犬子::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto discarded = carrier.chooseToDiscard(L"[犬子] 选择一张牌弃置", 1, false);
	if (discarded.empty()) return false;

	playCount = 0;
	carrier.markCharInfoDirty();
	std::cout << "<技能> " << carrier.characterName() << "发动犬子，弃置了一张牌" << std::endl;
	game.broadcastState();
	return true;
}
void 犬子::reset() {
	PassiveSkill::reset();
	playCount = 0;
}
std::optional<std::string> 犬子::extraPlaceholders(const std::string& key) const {
	if (key == "playCount") return std::to_string(playCount);
	if (key == "count+1") return std::to_string(count + 1);
	return std::nullopt;
}

bool 黑洞::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//前4张牌有未被记录的
	const Pile& discardPile = game.getDiscardPile();
	for (const auto& card : discardPile | std::views::take(4)) {
		if (!record.contains(card->getName())) return true;
	}
	return false;
}
bool 黑洞::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Pile& discardPile = game.getDiscardPile();
	std::vector<std::wstring> options;
	std::vector<std::size_t> validIndices;
	for (std::size_t i = 0; const auto& card : discardPile | std::views::take(4)) {
		if (!record.contains(card->getName())) {
			options.push_back(card->toWString());
			validIndices.push_back(i);
		}
		++i;
	}
	const std::size_t choice = carrier.ask(
		L"[黑洞] 选择一张牌获得", options, false
	);
	if (choice == 0) return false;

	std::unique_ptr<Card> card = discardPile.takeCardByIndex(validIndices[choice - 1]);
	record.insert(card->getName());
	carrier.gainCard(std::move(card));

	game.broadcastState();
	return true;
}
void 黑洞::reset() {
	PassiveSkill::reset();
	record.clear();
}

bool 好事::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 好事::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.showCard(trigger.getCard());
	carrier.recover(5);
	return true;
}

bool 压抑::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 压抑::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional targetOpt = carrier.choosePlayer(
		L"[压抑] 选择一名角色", false
	);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value();

	//找出目标手牌中点数最大的数字牌
	Hand& hand = target.getHand();
	std::optional<Card::Name> maxName;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		Card& card = hand[i];
		if (card.isNumber()) {
			if (!maxName.has_value() || card.getName() > *maxName) {
				maxName = card.getName();
			}
		}
	}
	if (!maxName.has_value()) {
		std::cout << "<技能> " << carrier.characterName() << "发动压抑，"
			<< target.characterName() << "手中无数字牌" << std::endl;
		game.broadcastState();
		return true;
	}

	//收集所有最大点数的牌的索引
	std::vector<std::size_t> indices;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].isNumber() && hand[i].getName() == *maxName) {
			indices.push_back(i);
		}
	}

	//从后往前弃牌，避免索引偏移
	for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
		target.discardByIndex(*it);
	}

	std::cout << "<技能> " << carrier.characterName() << "发动压抑，"
		<< target.characterName() << "弃置了" << indices.size() << "张点数最大的数字牌" << std::endl;

	//弃置牌数≥2张：失去10%最大体力（向上取整）
	if (indices.size() >= 2) {
		const std::size_t damage = unool::math::ceil(target.getMaxHp() * 0.1);
		target.damage(damage, carrier);
		std::cout << target.characterName() << "失去" << damage << "点体力" << std::endl;
	}

	game.broadcastState();
	return true;
}


bool 捉奸_弃牌::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 捉奸_弃牌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	if (carrier.discardByIndex(unool::random::randomSize_t(0, carrier.handCount() - 1)).is(Card::Color::red)) {
		carrier.damage(unool::math::ceil(carrier.getHp() * 0.05), carrier);
	}
	game.broadcastState();
	return true;
}

bool 捉奸::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Hand& hand = carrier.getHand();
	//寻找非红的非万能牌下标
	std::vector<std::size_t> notRedIndex;
	for (const auto [i, c] : hand | std::views::enumerate) {
		if (c->getColor() != Card::Color::red && c->isNotWild())
			notRedIndex.push_back(i);
	}

	//寻找红牌下标
	std::vector<std::size_t> redIndex;
	for (const auto [i, c] : hand | std::views::enumerate) {
		if (c->is(Card::Color::red))
			redIndex.push_back(i);
	}

	//变色
	for (const std::size_t index : notRedIndex) {
		hand[index].setColor(Card::Color::red);
	}

	static const std::array<Card::Color, 3> colors = {
		Card::Color::blue, Card::Color::green, Card::Color::yellow
	};
	for (const std::size_t index : redIndex) {
		hand[index].setColor(unool::random::randomGet(colors));
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：爬竿_伤害（子技能） ====================
bool 爬竿_伤害::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getPlayer().getId() == *targetId
		&& !trigger.getPlayer().getHasUsed();
}
bool 爬竿_伤害::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();
	std::size_t damage = target.getMaxHp() / 100; //1%最大体力向下取整
	if (damage > 0) target.damage(damage, carrier);
	std::cout << "<技能> " << carrier.characterName()
		<< "发动爬竿，" << target.characterName() << "失去" << damage << "点体力" << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：爬竿（主技能） ====================
bool 爬竿::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto targetOpt = carrier.chooseOtherPlayer(L"[爬竿] 选择一名其他角色", true);
	if (!targetOpt.has_value()) return false;
	*targetId = targetOpt->get().getId();
	std::cout << "<技能> " << carrier.characterName() << "发动爬竿，选择了"
		<< targetOpt->get().characterName() << std::endl;
	return true;
}

// ==================== 技能：渊涡 ====================
bool 渊涡::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return !carrier.getHasUsed();
}
bool 渊涡::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.recover(1);
	std::cout << "<技能> " << carrier.characterName() << "发动渊涡，回复1点体力" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：没座 ====================
bool 没座::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const auto& hand = carrier.getHand();
	const bool hasRed = hand.include([](const Card& c) {
		return c.is(Card::Color::red);
	});
	//没红且手牌为1时不触发；其余情况触发
	return hasRed || carrier.handCount() != 1;
}
bool 没座::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const auto& hand = carrier.getHand();
	const bool hasRed = hand.include([](const Card& c) {
		return c.is(Card::Color::red);
	});
	if (!hasRed) {
		//没红牌：随机获得一张红色牌
		carrier.gainCard(std::make_unique<Card>(Card::randomCard([](const Card& c) {
			return c.is(Card::Color::red);
		})));
		std::cout << "<技能> " << carrier.characterName() << "发动没座，随机获得了一张红牌" << std::endl;
	}
	else {
		//有红牌：可弃一张非红色牌
		auto discarded = carrier.chooseToDiscard(L"[没座] 弃置一张非红色牌", 1, false,
												 [](const Card& c) { return !c.is(Card::Color::red); });
		if (discarded.empty()) return false; //玩家取消
		std::cout << "<技能> " << carrier.characterName() << "发动没座，弃置了一张非红色牌" << std::endl;
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：空空 ====================
bool 空空::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isAction);
}
bool 空空::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto cardRef = carrier.chooseToOperate(
		L"【空空】选择一张功能牌变为红色的【封禁】", false,
		&Card::isAction, [](Card&) {});
	if (!cardRef.has_value()) return false;
	Card& card = cardRef->get();
	card.setColor(Card::Color::red);
	card.setName(Card::Name::action_skip);
	std::cout << "<技能> " << carrier.characterName() << "发动空空，将一张功能牌变为红色的【封禁】" << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：暗忍 ====================
bool 暗忍::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isNotWild);
}

bool 暗忍::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.damage(1, carrier);
	//收集非万能牌索引（颜色不变，仅改牌名为封禁）
	Hand& hand = carrier.getHand();
	std::vector<std::size_t> nonWildIndices;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].isNotWild()) nonWildIndices.push_back(i);
	}
	if (!nonWildIndices.empty()) {
		std::size_t pick = nonWildIndices[unool::random::randomSize_t(0, nonWildIndices.size() - 1)];
		Card& c = carrier.getCardByIndex(pick);
		c.setName(Card::Name::action_skip);
		std::cout << "<技能> " << carrier.characterName() << "发动暗忍，失去1点体力并将一张非万能牌变为【封禁】" << std::endl;
	}
	else {
		std::cout << "<技能> " << carrier.characterName() << "发动暗忍，失去1点体力（无非万能牌可变）" << std::endl;
	}
	game.broadcastState();
	return true;
}

// ==================== 技能：暗忍_改 ====================
bool 暗忍_改::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//收集非万能牌索引（颜色不变，仅改牌名为封禁）
	Hand& hand = carrier.getHand();
	std::vector<std::size_t> nonWildIndices;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		if (hand[i].isNotWild()) nonWildIndices.push_back(i);
	}
	if (!nonWildIndices.empty()) {
		std::size_t pick = nonWildIndices[unool::random::randomSize_t(0, nonWildIndices.size() - 1)];
		Card& c = carrier.getCardByIndex(pick);
		c.setName(Card::Name::action_skip);
		std::cout << "<技能> " << carrier.characterName() << "发动暗忍_改，将一张非万能牌变为【封禁】" << std::endl;
	}
	else {
		std::cout << "<技能> " << carrier.characterName() << "发动暗忍_改（无非万能牌可变）" << std::endl;
	}
	game.broadcastState();
	return true;
}


// ==================== 技能：治病 ====================
bool 治病::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!c.isNotNumber()) return false;
	return !playedNames.contains(c.getName());
}

bool 治病::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	Card& card = trigger.getCard();

	//记录该牌名已首次触发
	playedNames.insert(card.getName());

	//构建当前剩余选项文字
	std::vector<std::wstring> optionTexts;
	for (std::size_t opt : options) {
		switch (opt) {
			case 1: optionTexts.emplace_back(L"此牌无效"); break;
			case 2: optionTexts.emplace_back(L"你弃置一张牌"); break;
			case 3: optionTexts.emplace_back(L"其摸两张牌"); break;
		}
	}

	std::size_t choiceIdx = carrier.ask(L"【治病】对方打出了" + card.toWString() + L"，选择一项：", optionTexts, true);
	std::size_t chosenOpt = options[choiceIdx - 1];

	switch (chosenOpt) {
		case 1: //此牌无效
			card.cancelEffect();
			std::cout << "<技能> " << carrier.characterName() << "发动治病，令"
				<< player.characterName() << "打出的" << card << "无效" << std::endl;
			break;
		case 2: //你弃置一张牌
			carrier.chooseToDiscard(L"【治病】弃置一张牌", 1, true);
			std::cout << "<技能> " << carrier.characterName() << "发动治病，弃置一张牌" << std::endl;
			break;
		case 3: //其摸两张牌
			player.draw(2, DrawReason::skill);
			std::cout << "<技能> " << carrier.characterName() << "发动治病，令"
				<< player.characterName() << "摸两张牌" << std::endl;
			break;
	}

	//若此时剩余多个选项，移除本次所选
	if (options.size() > 1) {
		options.erase(options.begin() + (choiceIdx - 1));
	}

	game.broadcastState();
	return true;
}

void 治病::reset() {
	PassiveSkill::reset();
	playedNames.clear();
	options = { 1, 2, 3 };
}




bool 连营::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (!trigger.hasPlayer() || !trigger.hasCards()) return false;
	if (carrier.getId() != trigger.getPlayer().getId()) return false;
	const Card& c = trigger.getCard();
	Card::Type type = c.getType();
	if (triggered.count(type)) return false;  //该类别已触发过
	//失去后手牌中该类别牌数为0（最后一张）
	return !carrier.handInclude(
		[type](const Card& hc) { return hc.is(type); });
}

bool 连营::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const Card& lost = trigger.getCard();
	Card::Type lostType = lost.getType();

	//弃置另一类别的一张牌
	auto discarded = carrier.chooseToDiscard(
		L"【连营】弃置另一类别的一张牌", 1, false,
		[lostType](const Card& c) { return c.getType() != lostType; });
	if (discarded.empty()) return false;  //玩家取消

	//从游戏外获得该类别一张牌
	Card::ColorName cn = Card::randomCard([lostType](const Card& c) {
		return c.is(lostType);
	});
	carrier.gainCard(Card::make(cn));

	triggered.insert(lostType);
	std::cout << "<技能> " << carrier.characterName() << "发动连营，弃置一张牌并获得一张"
		<< (lostType == Card::Type::wild ? "万能" : lostType == Card::Type::action ? "功能" : "数字") << "牌" << std::endl;
	game.broadcastState();
	return true;
}

void 连营::reset() {
	PassiveSkill::reset();
	triggered.clear();
}

std::set<Card::Type>& 困界::getTriggered(const Player& carrier) const {
	if (!triggeredCache.has_value()) {
		auto skillOpt = carrier.findSkill<连营>();
		if (!skillOpt.has_value()) throw std::runtime_error("没有找到\"连营\"技能");
		triggeredCache = skillOpt.value().get().triggered;
	}
	return triggeredCache.value().get();
}
bool 困界::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return getTriggered(carrier).size() >= 3;
}

bool 困界::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//1. 选目标角色
	auto targetOpt = carrier.choosePlayer(L"【困界】选择一名角色重铸其手中一种类别的所有牌", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	//重铸某类别所有牌的局部函数（每张单独走 recastByIndex，保证 reason 与触发时机正确）
	auto recastAll = [](Player& p, Card::Type type_) {
		for (std::size_t i = p.handCount(); i-- > 0; ) {
			if (p.getCardByIndex(i).is(type_)) p.recastByIndex(i);
		}
	};

	//2. 选目标重铸的类别
	std::size_t typeAIdx = carrier.ask(
		L"【困界】选择" + target.characterNameW() + L"重铸的类别",
		{ L"数字牌", L"功能牌", L"万能牌" }, true);
	Card::Type typeA = static_cast<Card::Type>(typeAIdx);
	recastAll(target, typeA);

	//3. 选自己重铸的类别（必须不同于 typeA）
	std::vector<std::wstring> otherNames;
	std::vector<Card::Type> otherTypes;
	for (int i = 1; i <= 3; ++i) {
		Card::Type c = static_cast<Card::Type>(i);
		if (c != typeA) {
			otherNames.push_back(Card::to_wstring(c));
			otherTypes.push_back(c);
		}
	}
	std::size_t typeBIdx = carrier.ask(L"【困界】选择自己重铸的类别", otherNames, true);
	Card::Type typeB = otherTypes[typeBIdx - 1];
	recastAll(carrier, typeB);

	std::cout << "<技能> " << carrier.characterName() << "发动困界" << std::endl;
	game.broadcastState();
	return true;
}

// ==================== 技能：四麻 ====================
bool 四麻::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handCount() != 4;
}

bool 四麻::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t cnt = carrier.handCount();
	if (cnt > 4) {
		carrier.chooseToDiscard(L"【四麻】弃置牌调整至四张", cnt - 4, true);
		std::cout << "<技能> " << carrier.characterName() << "发动四麻，弃置了"
			<< (cnt - 4) << "张牌" << std::endl;
	}
	else if (cnt < 4) {
		carrier.draw(4 - cnt, DrawReason::skill);
		std::cout << "<技能> " << carrier.characterName() << "发动四麻，摸了"
			<< (4 - cnt) << "张牌" << std::endl;
	}
	game.broadcastState();
	return true;
}


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
	carrier.chooseToDiscard(L"【爆射】弃置至多两张牌", 2, false);

	// 2. 选择一名其他角色
	auto targetOpt = carrier.chooseOtherPlayer(L"【爆射】选择一名角色造成6点伤害", true);
	if (!targetOpt.has_value()) {
		std::cout << "<技能> " << carrier.characterName() << "发动爆射，但未选择目标" << std::endl;
		game.broadcastState();
		return true;
	}
	Player& target = targetOpt.value().get();

	// 3. 造成6点伤害
	target.damage(6, carrier);
	std::cout << "<技能> " << carrier.characterName() << "发动爆射，对"
		<< target.characterName() << "造成6点伤害" << std::endl;
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
	std::cout << "<技能> " << carrier.characterName() << "灵爆计数：" << *playCount << "/3" << std::endl;
	if (*playCount >= 3) {
		carrier.addMark("幽灵");
		*playCount = 0;
		std::cout << "<技能> " << carrier.characterName() << "灵爆触发，获得\"幽灵\"标记" << std::endl;
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
	bool isTargeted = c.is(Card::Name::action_skip, Card::Name::action_draw2, Card::Name::wild_draw4);
	return isTargeted && carrier.hasMark("幽灵");
}

bool 灵爆_子::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = carrier.next();
	carrier.removeMark("幽灵");
	target.damage(10, carrier);
	std::cout << "<技能> " << carrier.characterName() << "发动灵爆，移去\"幽灵\"标记并对"
		<< target.characterName() << "造成10点伤害" << std::endl;
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
		L"你成为了" + card.toWString() + L"的目标，是否发动加速？",
		{ L"发动", L"不发动" },
		true
	);
	if (choice == 2) return false;

	//发动一次炫技
	carrier.getSkill<炫技>().tryActivate(game, carrier);

	//令此牌无效
	card.cancelEffect();
	std::cout << "<技能> " << carrier.characterName() << "发动加速，发动炫技并令此牌无效" << std::endl;
	game.broadcastState();
	return true;
}

void 加速::reset() {
	setLimit(0);
	PassiveSkill::reset();
}


// ==================== 技能：走位 ====================
bool 走位::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//X = 已输局数，至多3
	std::size_t X = std::min(carrier.getLosses(), std::size_t{ 3 });

	//未输过（X=0）：失去走位+获得芜湖
	if (X == 0) {
		carrier.removeSkill("走位");
		carrier.addSkill(芜湖::make());
		std::cout << "<技能> " << carrier.characterName() << "未输过局，失去【走位】并获得【芜湖】" << std::endl;
		game.broadcastState();
		return true;
	}

	//询问决议几张（0..X）
	std::vector<std::wstring> opts;
	opts.push_back(L"不决议");
	for (std::size_t i = 1; i <= X; ++i) {
		opts.push_back(L"决议" + std::to_wstring(i) + L"张");
	}
	std::size_t choice = carrier.ask(L"【走位】决议至多" + std::to_wstring(X) + L"张牌", opts, false);

	//玩家取消或选0张：失去走位+获得芜湖
	if (choice == 0 || choice == 1) {
		carrier.removeSkill("走位");
		carrier.addSkill(芜湖::make());
		std::cout << "<技能> " << carrier.characterName() << "未决议牌，失去【走位】并获得【芜湖】" << std::endl;
		game.broadcastState();
		return true;
	}

	//决议 N 张
	std::size_t N = choice - 1;  //choice=2对应N=1
	carrier.decree(L"【走位】决议牌", N, true);
	std::cout << "<技能> " << carrier.characterName() << "发动走位，决议了" << N << "张牌" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：九一 ====================
bool 九一::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!(c.is(Card::Name::number_9, Card::Name::number_1))) return false;
	return carrier.handInclude(&Card::isNumber);
}

bool 九一::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.chooseToDiscard(L"弃置一张【9】或【1】", 1, false,
							[](const Card& c) { return c.is(Card::Name::number_9, Card::Name::number_1); });
	std::cout << "<技能> " << carrier.characterName() << "发动九一" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：白虎 ====================
bool 白虎::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	const Player& target = trigger.getPlayer();
	if (triggeredPlayers.contains(target.getId())) return false;
	return target.handCount() == 1;
}

bool 白虎::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& caster = carrier;   //唐伯虎（技能持有者）
	Player& target = trigger.getPlayer();    //回合开始的角色
	//caster 选颜色
	auto colorOpt = caster.chooseCardColor(L"发动【白虎】，选择一个颜色", true,
										   { Card::Color::red, Card::Color::yellow, Card::Color::green, Card::Color::blue });
	if (!colorOpt) return false;
	Card::Color color = *colorOpt;

	//caster 选牌名（9或1）
	auto nameOpt = caster.chooseCardName(L"选择一个牌名", true,
										 { Card::Name::number_9, Card::Name::number_1 });
	if (!nameOpt) return false;
	Card::Name name = *nameOpt;

	//新建一张牌给目标
	auto card = Card::make(color, name);
	std::cout << "<技能> " << caster.characterName() << "发动白虎，令" << target.characterName()
		<< "获得" << *card << std::endl;
	target.gainCard(std::move(card));

	triggeredPlayers.insert(target.getId());
	game.broadcastState();
	return true;
}

void 白虎::reset() {
	triggeredPlayers.clear();
	PassiveSkill::reset();
}


// ==================== 技能：易主 ====================
bool 易主::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (phaseCount >= 1) return false;
	if (!trigger.hasNumber()) return false;
	return trigger.getNumber() == 2;
}

bool 易主::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//弃置刚获得的两张牌
	std::vector<ref<Card>> gainedCards = trigger.getCards();
	for (auto& cardRef : gainedCards) {
		//找到手中对应的牌并弃置
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			if (&carrier.getCardByIndex(i) == &cardRef.get()) {
				carrier.discardByIndex(i);
				break;
			}
		}
	}
	std::cout << "<技能> " << carrier.characterName() << "发动易主，弃置获得的两张牌" << std::endl;

	//渡荆可发动次数+1
	if (auto sp = carrier.findSkill<渡荆>(); sp.has_value()) {
		auto& dj = sp.value().get();
		dj.increaseLimit(1);
		std::cout << "<技能> " << carrier.characterName() << "的【渡荆】可发动次数+1，当前="
			<< dj.getLimit().value() << std::endl;
	}

	++phaseCount;
	game.broadcastState();
	return true;
}

void 易主::reset() {
	phaseCount = 0;
	PassiveSkill::reset();
}


// ==================== 技能：渡荆 ====================
bool 渡荆::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//检查手牌数全场最多
	std::size_t myCount = carrier.handCount();
	for (const auto& p : game.getPlayers()) {
		if (&p.get() == &carrier) continue;
		if (p.get().handCount() > myCount) return false;
	}
	return true;
}

bool 渡荆::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//选一名其他角色拼点
	auto targetOpt = carrier.chooseOtherPlayer(L"【渡荆】选择一名角色拼点", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//拼点（carrier是发起者）
	auto result = carrier.comparePoint(target, true);
	if (!result) return false;

	std::cout << "<技能> " << carrier.characterName() << "与" << target.characterName() << "拼点" << std::endl;

	//没赢的（lose或draw）获得随机+2
	auto giveRandomDraw2 = [&game](Player& p) {
		Card::Color color = unool::random::randomGet(Card::fourColors);
		auto card = Card::make(color, Card::Name::action_draw2);
		std::cout << "<技能> " << p.characterName() << "拼点未赢，从游戏外获得" << *card << std::endl;
		p.gainCard(std::move(card));
	};

	if (*result == Player::CompareResult::lose) {
		//carrier没赢
		giveRandomDraw2(carrier);
	}
	else if (*result == Player::CompareResult::draw) {
		//平局，双方都没赢
		giveRandomDraw2(carrier);
		giveRandomDraw2(target);
	}
	else {
		//carrier赢，target没赢
		giveRandomDraw2(target);
	}

	game.broadcastState();
	return true;
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

	std::cout << "<技能> " << carrier.characterName() << "发动返现，重铸了"
		<< nonYellowIndices.size() << "张非黄色牌，回复" << nonYellowIndices.size() << "点体力" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：尖刺 ====================
bool 尖刺::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.hasNumber();
}

bool 尖刺::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t dmg = trigger.getNumber();

	//选一名其他角色造成等量伤害
	auto targetOpt = carrier.chooseOtherPlayer(L"【尖刺】选择一名角色造成" + std::to_wstring(dmg) + L"点伤害", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	target.damage(dmg, carrier);
	carrier.recover(1);

	std::cout << "<技能> " << carrier.characterName() << "发动尖刺，对"
		<< target.characterName() << "造成" << dmg << "点伤害，回复1点体力" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：弹暴 ====================
bool 弹暴::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude(&Card::isWild);
}

bool 弹暴::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//弃置一张万能牌；取消则返还可用次数（PassiveSkill::launch 会回退 count）
	auto discarded = carrier.chooseToDiscard(L"【弹暴】弃置一张万能牌", 1, false, &Card::isWild);
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
			L"【弹暴】选择一名其他角色造成" + std::to_wstring(handgun.damageValue)
			+ L"点伤害（" + std::to_wstring(i + 1) + L"/" + std::to_wstring(X) + L"）",
			false);
		if (!targetOpt.has_value()) continue;  //玩家取消，继续下一次循环
		Player& target = *targetOpt;

		target.damage(handgun.damageValue, carrier);
		std::cout << "<技能> " << carrier.characterName() << "发动弹暴，对"
			<< target.characterName() << "造成" << handgun.damageValue << "点伤害" << std::endl;
		game.broadcastState();
	}

	//【手枪】本局伤害值+1
	handgun.damageValue += 1;
	std::cout << "<技能> " << carrier.characterName() << "的【手枪】本局伤害值+1，当前="
		<< handgun.damageValue << std::endl;

	return true;
}


// ==================== 技能：星轨 ====================
bool 星轨::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//三次判定
	Card& c1 = carrier.judge();
	Card& c2 = carrier.judge();
	Card& c3 = carrier.judge();
	const std::wstring judgeResultStr = c1.toWString() + L"，" + c2.toWString() + L"，" + c3.toWString();

	if (c1.getType() == c2.getType() && c2.getType() == c3.getType()) {
		//类型全相同，引力可用次数+1
		carrier.getSkill<引力>().increaseLimit(1);
		carrier.markCharInfoDirty();
		carrier.hint(L"判定结果是：" + judgeResultStr + L"\n获得一次【引力】使用次数！");
		std::cout << "<技能> " << carrier.characterName() << "星轨判定三次类型相同，【引力】可用次数+1" << std::endl;

	}
	else {
		carrier.hint(L"判定结果是：" + judgeResultStr + L"\n残念だ、未获得【引力】使用次数");
		std::cout << "<技能> " << carrier.characterName() << "星轨判定类型不同" << std::endl;
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

	std::cout << "<技能> 引力将一张数字牌重定向给"
		<< carrier.characterName() << "获得" << std::endl;
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
		player.removeSkill("引力_目标");
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
		std::cout << "<技能> " << carrier.characterName() << "铃铛重铸" << X << "张手牌" << std::endl;
	}

	//失去其他技能（5个衍生技）
	if (X == 3) {
		static const std::vector<std::string> derivedNames = { "爆糖", "硬糖", "甘草", "跳糖", "薄荷" };
		for (const std::string& name : derivedNames) {
			carrier.removeSkill(name);
		}

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
	std::cout << "<技能> " << carrier.characterName() << "爆糖对"
		<< target.characterName() << "造成" << damage << "点伤害" << std::endl;
	game.broadcastState();
	return true;
}

//薄荷
bool 薄荷::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::size_t heal = unool::random::randomInt(1, 3);
	carrier.recover(heal);
	std::cout << "<技能> " << carrier.characterName() << "薄荷回复" << heal << "点体力" << std::endl;
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
	std::cout << "<技能> " << carrier.characterName() << "健体回复1点体力" << std::endl;
	game.broadcastState();
	return true;
}


// ==================== 技能：飞刃 ====================
bool 飞刃::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 飞刃::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto targetOpt = carrier.chooseOtherPlayer(L"【飞刃】令一名其他角色获得一个\"毒\"标记", false);
	if (!targetOpt.has_value()) return false;
	targetOpt.value().get().addMark("毒");
	std::cout << "<技能> " << carrier.characterName() << "飞刃令" << targetOpt.value().get().characterName() << "获得一个毒标记" << std::endl;
	return true;
}

// ==================== 技能：淬毒 ====================
bool 淬毒::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getPlayer().hasMark("毒");
}
bool 淬毒::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& player = trigger.getPlayer();
	std::size_t dmg = player.getMarkCount("毒");
	player.damage(dmg, carrier);
	std::cout << "<技能> " << carrier.characterName() << "淬毒令" << player.characterName() << "受到" << dmg << "点伤害" << std::endl;
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
	std::size_t dmg = unool::math::ceil(lostHp * 0.05);

	auto targetOpt = carrier.chooseOtherPlayer(L"【重锤】选择一名其他角色造成" + std::to_wstring(dmg) + L"点伤害", true);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	target.damage(dmg, carrier);
	std::cout << "<技能> " << carrier.characterName() << "发动重锤，失去" << loss << "点体力，对"
		<< target.characterName() << "造成" << dmg << "点伤害" << std::endl;
	game.broadcastState();
	return true;
}
