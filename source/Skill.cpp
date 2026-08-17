#include "../header/Skill.h"
#include "../header/Player.h"
#include "../header/GameLogic.h"
#include "../header/utils.h"
#include <iostream>
#include <cmath>


Skill::Skill(const std::string& _name, const std::string& _info, const limit_t& _limit)
	:name(_name), info(_info), limit(_limit) {}

void Skill::reset() {
	count = 0;
}




// **********************
//         被动技
// **********************

PSkill::PSkill(const std::string& name, const std::string& description,
			   const limit_t& _limit, const bool _forced,
			   const TriggerPlayer& _triggerPlayer, const TriggerTime& _triggerTime)
	:Skill(name, description, _limit),
	forced(_forced),
	triggerPlayer(_triggerPlayer),
	triggerTime(_triggerTime) {}

bool PSkill::matchTrigger(const TriggerTime& currentTriggerTime,
						  const Trigger& trigger) const {
	return triggerTime == currentTriggerTime && (
		triggerTime == TriggerTime::game_begin ||
		triggerTime == TriggerTime::game_end ||
		triggerPlayer == TriggerPlayer::anybody ||
		(triggerPlayer == TriggerPlayer::self && trigger.getCarrier() == trigger.getPlayer()) ||
		(triggerPlayer == TriggerPlayer::others && trigger.getCarrier() != trigger.getPlayer())
		);
}

void PSkill::launch(Trigger& trigger) {
	//不满足条件，或达到次数限制：不发动
	if ((limit != unlimited && count >= limit) || !this->filter(trigger)) return;
	//如果不是锁定技，询问玩家是否发动
	if (!forced) {
		const std::size_t choice = trigger.getCarrier().ask(
			L"是否发动 [" + getNameW() + L"]？",
			{ L"发动", L"不发动" },
			true
		);
		if (choice == 2) return;
	}
	//发动技能
	count += 1;
	trigger.setCount(count);
	std::cout << "<技能> " << trigger.getCarrier().characterName() << "发动了" << name << "！" << std::endl;
	this->content(trigger);
}

void PSkill::reset() {
	Skill::reset();
}

void PSkill::setForced(const bool newForced) {
	forced = newForced;
}



PSkill::Trigger::Trigger(GameLogic& _game, Player& _carrier,
						 opt_ref<Player> _player,
						 std::optional<std::vector<ref<Card>>> _cards,
						 opt_ref<Player> _source,
						 opt_ref<std::size_t> _number)
	:game(_game), carrier(_carrier), player(_player),
	cards(_cards), source(_source), number(_number) {}


// **********************
//         主动技
// **********************
ASkill::ASkill(const std::string& _name, const std::string& _info, const limit_t& _limit)
	:Skill(_name, _info, _limit) {}


// ==================== 技能：粪怒 ====================
bool 粪怒::filter(const Trigger& trigger) const {
	return trigger.getPlayer().handCount() == 1;
}
void 粪怒::content(Trigger& trigger) {
	trigger.getPlayer().draw(
		std::min(trigger.getCarrier().handCount(), 5ull)
	);
}


// ==================== 技能：隐身 ====================
bool 隐身::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2, Card::Name::wild_draw4);
}
void 隐身::content(Trigger& trigger) {
	trigger.getCard().cancelEffect();
	trigger.getSource().draw(1);
}


// ==================== 技能：顶置 ====================
bool 顶置::filter(const Trigger& trigger) const {
	return true;
}
void 顶置::content(Trigger& trigger) {
	GameLogic& game = trigger.getGame();
	Pile& pile = game.getPile();

	if (pile.empty()) return;

	const Card& bottomCard = pile.back();
	std::size_t choice = trigger.getCarrier().ask(
		L"牌堆底是" + Card::to_wstring(bottomCard.getColor()) + L" " + Card::to_wstring(bottomCard.getName()) + L"，是否顶置？",
		{ L"顶置", L"不顶置" },
		true
	);

	if (choice == 1) {
		std::unique_ptr<Card> card = pile.take_back(game.getDiscardPile());
		pile.push_front(std::move(card));
		std::cout << "<技能> 顶置成功！" << std::endl;
	}
}


// ==================== 技能：带派 ====================
bool 带派::filter(const Trigger& trigger) const {
	return true;
}
void 带派::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
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
		carrier.takeDamage(25, carrier);
		break;
	}
	trigger.getGame().broadcastState();
}


// ==================== 技能：寒魄 ====================
bool 寒魄::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handCount() == 1;
}
void 寒魄::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Card& card = trigger.getCard();
	carrier.getCardByIndex(0).set(card);
	trigger.getGame().broadcastState();
}


// ==================== 技能：割腕 ====================
bool 割腕::filter(const Trigger& trigger) const {
	return trigger.getCard().getColor() == Card::Color::red;
}
void 割腕::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	carrier.takeDamage(unool::random::randomSize_t(1, 5), carrier);
	trigger.getGame().broadcastState();
}


// ==================== 技能：丑皇 ====================
bool 丑皇::filter(const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
void 丑皇::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();

	std::size_t choice = 1;
	if (carrier.handInclude(&Card::isNotNumber)) { //有非数字牌
		choice = carrier.ask(L"选择一项：", { L"回复10体力", L"弃置一张非数字牌" }, true);
	}
	else {
		carrier.ask(L"手中没有非数字牌，已自动选择回复10体力", { L"确定", }, true);
		choice = 1;
	}
	switch (choice) {
	case 1:
		carrier.recover(10);
		break;
	case 2:
		carrier.chooseToDiscard(L"弃置一张非数字牌", 1,
								true, [](const Card& card)->bool {
			return !card.isNumber();
		});
		break;
	}
	trigger.getGame().broadcastState();
}


// ==================== 技能：军国 ====================
bool 军国::filter(const Trigger& trigger) const {
	return true;
}
void 军国::content(Trigger& trigger) {
	Player& player = trigger.getPlayer();
	Player& carrier = trigger.getCarrier();
	if (player != carrier) //其他角色：失去 1% 最大体力，向上取整
		player.takeDamage(unool::math::ceil(player.getMaxHp() * 0.01), carrier);
	else //自己：失去1体力
		player.takeDamage(1, carrier);
}


// ==================== 技能：家暴 ====================
bool 家暴::filter(const Trigger& trigger) const {
	return trigger.getGame().playersSatisfy(
		//有玩家的血量 < 携带者
		[&trigger](const std::vector<std::unique_ptr<Player>>& players)->bool {
		for (const auto& player : players) {
			if (player->getHp() < trigger.getCarrier().getHp()) return true;
		}
		return false;
	});
}
void 家暴::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();
	const auto& candidates = game.getPlayersIf([&carrier](const Player& p) {
		return p.getHp() < carrier.getHp();
	});

	if (candidates.empty()) {
		game.broadcastState();
		return;
	}

	std::vector<std::wstring> options;
	for (auto& p : candidates) {
		options.push_back(p.get().characterNameW());
	}
	std::size_t choice = carrier.ask(L"选择家暴目标：", options, true);

	Player& target = candidates[choice - 1].get();
	std::size_t damage = unool::math::ceil(target.getMaxHp() * 0.1);
	target.takeDamage(damage, trigger.getCarrier());
	std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName() << "发动家暴，造成" << damage << "点伤害！" << std::endl;

	game.broadcastState();
}


// ==================== 技能：健身 ====================
bool 健身::filter(const Trigger& trigger) const {
	return true;
}
void 健身::content(Trigger& trigger) {
	trigger.getCarrier().recover(5);
	trigger.getGame().broadcastState();
}


// ==================== 技能：做题 ====================
bool 做题::filter(const Trigger& trigger) const {
	Card& card = trigger.getCard();
	Player& carrier = trigger.getCarrier();
	if (card.isNumber()) return carrier.handInclude(&Card::isNotNumber);
	else return carrier.handInclude(&Card::isNumber);
}
void 做题::content(Trigger& trigger) {
	Card& card = trigger.getCard();
	Player& carrier = trigger.getCarrier();
	if (card.isNumber()) {
		carrier.chooseToDiscard(L"弃置一张非数字牌", 1,
								true, [](const Card& c)->bool { return !c.isNumber(); });
	}
	else {
		carrier.chooseToDiscard(L"弃置一张数字牌", 1,
								true, [](const Card& c)->bool { return c.isNumber(); });
	}
}


// ==================== 技能：棍击 ====================
bool 棍击::filter(const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
void 棍击::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();
	Player& target = carrier.chooseOtherPlayer(L"选择棍击目标：", true).value();
	std::size_t damage = unool::math::pow(2, trigger.getCount());
	target.takeDamage(damage, trigger.getCarrier());
	std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName()
		<< "发动棍击，造成" << damage << "点伤害！" << std::endl;

	game.broadcastState();
}


// ==================== 技能：神木 ====================
bool 神木::filter(const Trigger& trigger) const {
	return true;
}
void 神木::content(Trigger& trigger) {
	GameLogic& game = trigger.getGame();
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_pal), 9);
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_draw4), 9);
	game.getPile().shuffle();
}


// ==================== 技能：雷剑 ====================
bool 雷剑::filter(const Trigger& trigger) const {
	return trigger.getCard().getName() == Card::Name::action_rev;
}
void 雷剑::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Card& card = trigger.getCard();
	Card::Color color = card.getColor();

	auto discarded = carrier.chooseToDiscard(L"弃置一张" + Card::to_wstring(color) + L"色数字牌", 1,
											 false, [color](const Card& c)->bool {
		return c.isNumber() && c.getColor() == color;
	});

	if (!discarded.empty()) {
		std::size_t value = discarded.front().get().value();
		carrier.recover(value);
		std::cout << "<技能> " << carrier.characterName()
			<< "发动雷剑，弃置 [" << discarded.front().get() << "] 并回复" << value << "点体力！" << std::endl;
		trigger.getGame().broadcastState();
	}
}


// ==================== 技能：买棋 ====================
bool 买棋::filter(const Trigger& trigger) const {
	return true;
}
void 买棋::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	carrier.takeDamage(10 * (trigger.getCount() - 1), trigger.getCarrier());
	if (unool::random::probability(0.5)) { //万能
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	}
	else { //+4
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
	}
}


// ==================== 技能：卖棋 ====================
bool 卖棋::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handInclude(&Card::isWild);
}
void 卖棋::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	carrier.chooseToDiscard(L"弃置一张万能牌", 1, true, [](const Card& card) {
		return card.isWild();
	});
	carrier.recover(10);
}


// ==================== 技能：耐克 ====================
bool 耐克::filter(const Trigger& trigger) const {
	Card& card = trigger.getCard();
	if (!card.is(Card::Name::action_ban, Card::Name::action_draw2, Card::Name::wild_draw4))
		return false;
	auto lastOpt = trigger.getGame().lastCard();
	if (!lastOpt.has_value()) return false;
	const Card& last = lastOpt.value();
	return last.is(Card::Color::blue) || last.isWild();
}
void 耐克::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	trigger.getCard().cancelEffect();
	std::cout << carrier.characterName() << "触发技能，使此牌无效" << std::endl;
}


// ==================== 技能：轰炸 ====================
bool 轰炸::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2, Card::Name::wild_draw4);
}
void 轰炸::content(Trigger& trigger) {
	Player& target = trigger.getPlayer().next();
	if (trigger.getCard().is(Card::Name::action_draw2)) {
		target.takeDamage(unool::math::ceil(target.getMaxHp() * 0.02), trigger.getCarrier());
	}
	else {
		target.takeDamage(unool::math::ceil(target.getMaxHp() * 0.04), trigger.getCarrier());
	}
}


// ==================== 技能：爆破 ====================
bool 爆破::filter(const Trigger& trigger) const {
	return trigger.getSource() == trigger.getCarrier();
}
void 爆破::content(Trigger& trigger) {
	Player& player = trigger.getPlayer();
	Hand& hand = player.getHand();
	auto card = hand.takeCardByIndex(unool::random::randomSize_t(0, hand.count() - 1));
	player.takeDamage(card->value(), trigger.getCarrier());
	std::cout << "爆破获取了 [" << *card << "]，造成了" << card->value() << "点伤害！" << std::endl;
	trigger.getCarrier().gainCard(std::move(card));
}


// ==================== 技能：电音 ====================
bool 电音::filter(const Trigger& trigger) const {
	return true;
}
void 电音::content(Trigger& trigger) {
	Hand& hand = trigger.getCarrier().getHand();
	hand.forEachIf(&Card::isNumber, [](Card& card) {
		card.setName(Card::numberCardsFrom0[unool::random::randomSize_t(0, 9)]);
	});
	trigger.getCarrier().recover(1);
	trigger.getGame().broadcastState();
}


// ==================== 技能：蒙面 ====================
bool 蒙面::filter(const Trigger& trigger) const {
	return true;
}
void 蒙面::content(Trigger& trigger) {
	trigger.getNumber() = unool::math::ceil(trigger.getNumber() * 0.7);
}


// ==================== 技能：锐刻 ====================
bool 锐刻::filter(const Trigger& trigger) const {
	if (disabled) return false;
	return trigger.getCard().getName() == Card::Name::number_5;
}
void 锐刻::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();

	std::size_t choice = carrier.ask(L"发动[锐刻]，选择一项：", {
		L"令一名角色摸1张牌",
		L"令一名角色摸5张牌并失去此技能至本局结束"
									 }, false);

	if (choice == 0) return;

	const auto& candidates = game.getPlayers();
	std::vector<std::wstring> options;
	for (const auto& p : candidates) {
		options.push_back(p.get().characterNameW());
	}
	std::size_t targetChoice = carrier.ask(L"选择目标角色：", options, true);
	Player& target = candidates[targetChoice - 1].get();

	if (choice == 1) {
		target.draw(1);
		std::cout << "<技能> " << carrier.characterName() << "发动锐刻，令" << target.characterName() << "摸1张牌" << std::endl;
	}
	else {
		target.draw(5);
		disabled = true;
		std::cout << "<技能> " << carrier.characterName() << "发动锐刻，令" << target.characterName()
			<< "摸5张牌，失去此技能至本局结束" << std::endl;
	}
	game.broadcastState();
}


// ==================== 技能：巨富 ====================
bool 巨富::filter(const Trigger& trigger) const {
	return true;
}
void 巨富::content(Trigger& trigger) {
	trigger.getCarrier().draw(4); //初始8张 + 4张 = 12张
	trigger.getGame().broadcastState();
}


// ==================== 技能：破产 ====================
bool 破产::filter(const Trigger& trigger) const {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();
	std::size_t myCount = carrier.handCount();
	if (myCount == 0) return false;
	for (const auto& p : game.getPlayers()) {
		if (p.get().handCount() > myCount) return false;
	}
	return true;
}
void 破产::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Hand& hand = carrier.getHand();
	std::size_t idx = unool::random::randomSize_t(0, hand.count() - 1);
	carrier.discardByIndex(idx);
	std::cout << "<技能> " << carrier.characterName() << "触发破产，随机弃置一张牌" << std::endl;
	trigger.getGame().broadcastState();
}


// ==================== 技能：假酒 ====================
bool 假酒::filter(const Trigger& trigger) const {
	return trigger.getCard().isNotNumber();
}
void 假酒::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Card::Color playedColor = trigger.getCard().getColor();

	static const std::vector nonNumberNames = {
		Card::Name::action_ban, Card::Name::action_rev, Card::Name::action_draw2,
		Card::Name::wild_pal, Card::Name::wild_draw4
	};
	static const std::vector colors = {
		Card::Color::red, Card::Color::yellow, Card::Color::blue, Card::Color::green
	};

	//构建候选：不同颜色的非数字牌
	std::vector<std::unique_ptr<Card>> candidates;
	for (auto n : nonNumberNames) {
		if (n == Card::Name::wild_pal || n == Card::Name::wild_draw4) {
			//万能牌(黑色)：打出牌非黑色时候选
			if (playedColor != Card::Color::black)
				candidates.push_back(Card::make(Card::Color::black, n));
		}
		else {
			//有色功能牌：颜色 != 打出牌颜色
			for (auto c : colors) {
				if (c != playedColor)
					candidates.push_back(Card::make(c, n));
			}
		}
	}

	if (candidates.empty()) return;

	std::size_t idx = unool::random::randomSize_t(0, candidates.size() - 1);
	carrier.gainCard(std::move(candidates[idx]));
	std::cout << "<技能> " << carrier.characterName() << "发动假酒，获得了一张不同颜色的非数字牌" << std::endl;
	trigger.getGame().broadcastState();
}


// ==================== 技能：窃观 ====================
bool 窃观::filter(const Trigger& trigger) const {
	return trigger.getCards().size() == 1;
}
void 窃观::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Player& drawer = trigger.getPlayer();
	const Card& card = trigger.getCard();
	std::wstring title = L"【窃观】" + drawer.characterNameW()
		+ L"获得了 " + Card::to_wstring(card.getColor())
		+ L" " + Card::to_wstring(card.getName()) + L"（需确认）";
	carrier.hint(title);
}


// ==================== 技能：生存 ====================
bool 生存::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handCount() > 0;
}
void 生存::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();

	std::size_t x = trigger.getCount();
	if (x > 9) x = 9;
	Card::Name targetName = static_cast<Card::Name>(static_cast<int>(Card::Name::number_0) + x);

	static const std::vector<Card::Color> colors = {
		Card::Color::red, Card::Color::yellow, Card::Color::blue, Card::Color::green
	};
	Card::Color targetColor = colors[unool::random::randomSize_t(0, colors.size() - 1)];

	auto target = Card::make(targetColor, targetName);
	auto opt = carrier.chooseToOperate(
		L"请选择一张牌变为【" + std::to_wstring(x) + L"】",
		true, unool::alwaysTrue,
		[&target](Card& c) {
		c.set(*target);
	});
	if (opt.has_value()) {
		std::cout << "<技能> " << carrier.characterName() << "发动生存，将一张手牌改为【"
			<< target->toString() << "】" << std::endl;
	}
	game.broadcastState();
}


// ==================== 技能：创造 ====================
bool 创造::filter(const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (c.getName() != Card::Name::number_9) return false;
	if (c.is(Card::Color::black)) return false;
	return usedColors.find(c.getColor()) == usedColors.end();
}
void 创造::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
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
		"获得一张【" << newCard->toString() << "】" << std::endl;
	carrier.gainCard(std::move(newCard));
	usedColors.insert(targetColor);
	trigger.getGame().broadcastState();
}


// ==================== 技能：炼兵 ====================
std::map<Card::ColorName, std::size_t> 炼兵::buildPairs(Player& carrier) const {
	std::map<Card::ColorName, std::size_t> cnt;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		const Card& c = carrier.getHand().getCardByIndex(i);
		if (c.getColor() == Card::Color::black) continue;
		cnt[{c.getColor(), c.getName()}]++;
	}
	return cnt;
}
bool 炼兵::filter(const Trigger& trigger) const {
	Player& carrier = trigger.getCarrier();
	if (carrier.handCount() < 2) return false;
	auto cnt = buildPairs(carrier);
	for (const auto& kv : cnt) {
		if (kv.second >= 2 && usedColors.find(kv.first.first) == usedColors.end()) {
			return true;
		}
	}
	return false;
}
void 炼兵::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();

	auto cnt = buildPairs(carrier);
	std::vector<Card::ColorName> validPairs;
	std::vector<std::wstring> opts;
	for (const auto& kv : cnt) {
		if (kv.second >= 2 && usedColors.find(kv.first.first) == usedColors.end()) {
			validPairs.push_back(kv.first);
			opts.push_back(Card::to_wstring(kv.first.first) + L" "
						   + Card::to_wstring(kv.first.second) + L"(" + std::to_wstring(static_cast<unsigned long long>(kv.second)) + L"张)");
		}
	}
	if (validPairs.empty()) return;

	std::size_t idx = carrier.ask(L"【炼兵】选择要弃的同色同名牌对：", opts, false);
	if (idx == 0) return;
	if (idx - 1 >= validPairs.size()) return;

	Card::ColorName target = validPairs[idx - 1];
	std::size_t i1 = carrier.handCount(), i2 = carrier.handCount();
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		const Card& c = carrier.getHand().getCardByIndex(i);
		if (c.getColor() == target.first && c.getName() == target.second) {
			if (i1 >= carrier.handCount()) i1 = i;
			else { i2 = i; break; }
		}
	}
	if (i1 >= carrier.handCount() || i2 >= carrier.handCount()) return;
	if (i1 > i2) std::swap(i1, i2);
	carrier.discardByIndex(i2);
	carrier.discardByIndex(i1);
	carrier.gainCard(Card::make(target.first, Card::Name::action_draw2));
	usedColors.insert(target.first);
	std::cout << "<技能> " << carrier.characterName() << "发动炼兵，弃两张"
		<< Card::to_string(target.first) << Card::to_string(target.second)
		<< "，获得一张" << Card::to_string(target.first) << "+2" << std::endl;
	game.broadcastState();
}


// ==================== 技能：好火 ====================
bool 好火::filter(const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!c.is(Card::Color::red)) return false;
	Player& player = trigger.getPlayer();
	Player& carrier = trigger.getCarrier();
	if (player == carrier) return false;
	if (player.getHp() <= carrier.getHp()) return false;
	if (usedPlayerIds.find(player.getId()) != usedPlayerIds.end()) return false;
	if (carrier.handCount() == 0) return false;
	return true;
}
void 好火::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Player& target = trigger.getPlayer();

	auto result = carrier.chooseToGive(target, false);
	if (!result.has_value()) return;

	usedPlayerIds.insert(target.getId());
	std::cout << "<技能> " << carrier.characterName() << "发动好火，交给"
		<< target.characterName() << "一张" << result.value().get().toString() << std::endl;
}


//================== 森罗 =================
bool 森罗::filter(const Trigger& trigger) const {
	return true;
}
void 森罗::content(Trigger& trigger) {
	trigger.getCarrier().getHand().forEachIf(
		[](const Card& c) {
		return c.getColor() != Card::Color::black;
	}, [](Card& c) {
		c.setColor(Card::Color::green);
	});
}


//================大脚=====================
bool 大脚::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handInclude([](const Card& c) {
		return c.isWild();
	});
}
void 大脚::content(Trigger& trigger) {
	trigger.getCarrier().chooseToDiscard(L"弃置一张万能牌", 1, true, [](const Card& c) {
		return c.isWild();
	});
	trigger.getCarrier().getHand().forEachIf(
		[](const Card& c) {
		return c.getColor() != Card::Color::black;
	}, [](Card& c) {
		c.setColor(Card::Color::green);
	});
	trigger.getGame().broadcastState();
}


//================过江==============
bool 过江::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2);
}
void 过江::content(Trigger& trigger) {
	trigger.getSource().draw(2);
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
bool 大盏::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handInclude([](const Card& c) {
		return c.isNumber();
	});
}
void 大盏::content(Trigger& trigger) {
	// 找到最小数字
	int min = 100;
	trigger.getCarrier().getHand().forEach([&min](const Card& c) {
		if (c.isNotNumber()) return;
		if (c.value() < min) {
			min = c.value();
		}
	});


	if (min != 9) { //min不是9，存在数字牌点数不是9
		// 让点数最小的数字牌随机变大
		trigger.getCarrier().getHand().forEach([&min](Card& c) {
			if (c.isNumber() && c.value() == min) {
				randomEnlarge(c);
			}
		});
	}
	else { //min == 9，说明数字牌全是9，将一张9变为红色
		trigger.getCarrier().chooseToOperate(
			L"选择一张[9]变成红色", false,
			[](const Card& c) {
			return c.value() == 9;
		}, [](Card& c) {
			c.setColor(Card::Color::red);
		});
		trigger.getCarrier().recover(1);
	}
	trigger.getGame().broadcastState();
}


//==============举报=============
bool 举报::filter(const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
void 举报::content(Trigger& trigger) {
	setForced(true);
	Player& player = trigger.getPlayer();
	Player& carrier = trigger.getCarrier();
	player.takeDamage(unool::math::ceil(0.1 * player.getHp()), carrier);
}
void 举报::reset() {
	PSkill::reset();
	setForced(false);
}


//=============猥琐====================
bool 猥琐::filter(const Trigger& trigger) const {
	//找到最大体力
	std::size_t maxHp = 0;
	trigger.getGame().forEachPlayer([&maxHp](Player& p) {
		maxHp = std::max(maxHp, p.getHp());
	});
	return trigger.getCarrier().getHp() != maxHp;
}
void 猥琐::content(Trigger& trigger) {
	trigger.getCarrier().recover(1);
}



bool 棋王::filter(const Trigger& trigger) const {
	return trigger.getGame().getDiscardPile().count() >= 2
		&& trigger.getCard() == trigger.getGame().getDiscardPile()[1];
}
void 棋王::content(Trigger& trigger) {
	trigger.getCarrier().chooseToDiscard(L"弃置一张手牌", 1, true);
}


bool 金铲::filter(const Trigger& trigger) const {
	return trigger.getNumber() == 1;
}
void 金铲::content(Trigger& trigger) {
	trigger.getNumber() = 0;
	trigger.getPlayer().takeDamage(1, trigger.getCarrier());
}


//==============淘汰=============
/*"每局游戏共限五次，"
"你打出数字牌后，可弃置一张点数小于等于该牌一半（向下取整）的同色数字牌；"
"你打出功能牌后，可从游戏外随机获得一张同色的功能牌。",*/
bool 淘汰::filter(const Trigger& trigger) const {
	Card& card = trigger.getCard();
	if (card.isNumber()) {
		int half = static_cast<int>(unool::math::floor(card.value() / 2.0));
		return trigger.getCarrier().handInclude([&card, &half](const Card& c) {
			return c.value() <= half && c.getColor() == card.getColor() && c.isNumber();
		});
	}
	else return card.isAction();
}
void 淘汰::content(Trigger& trigger) {
	Card& card = trigger.getCard();
	Player& carrier = trigger.getCarrier();
	if (card.isNumber()) {
		int half = static_cast<int>(unool::math::floor(card.value() / 2.0));
		carrier.chooseToDiscard(
			L"弃置一张点数 <= " + std::to_wstring(half) + L"的" +
			Card::to_wstring(card.getColor()) + L"色数字牌", 1, true, [&card, &half](const Card& c) {
			return c.value() <= half && c.getColor() == card.getColor() && c.isNumber();
		});
	}
	else if (card.isAction()) {
		trigger.getCarrier().gainCard(Card::make(
			card.getColor(),
			Card::actionCards[unool::random::randomSize_t(0, 2)]
		));
	}
}


bool 光合::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_ban, Card::Name::action_draw2);
}
void 光合::content(Trigger& trigger) {
	Card& card = trigger.getCarrier().judge();
	if (card.isNumber()) {
		trigger.getSource().draw(1);
	}
	else if (card.isAction()) {
		trigger.getCard().cancelEffect();
	}
}


bool 射门::filter(const Trigger& trigger) const {
	return trigger.getCard().isNumber();
}
void 射门::content(Trigger& trigger) {
	GameLogic& game = trigger.getGame();
	Player& carrier = trigger.getCarrier();
	//选角色
	const auto& targetOpt = carrier.choosePlayer(L"选择射门目标：", true);
	if (!targetOpt.has_value()) return;

	//判定
	Player& target = targetOpt.value();
	if (target.judge().getColor() != Card::Color::blue) return;

	//执行效果
	if (target == carrier) {
		target.chooseToDiscard(L"[射门] 弃置一张牌", 1, true);
	}
	else {
		target.draw(1);
	}
}


bool 招待::filter(const Trigger& trigger) const {
	return false;
}

void 招待::content(Trigger& trigger) {

}




bool test::filter(const Trigger& trigger) const {
	return true;
}
void test::content(Trigger& trigger) {
	std::cout << "判定：" << trigger.getCarrier().judge() << std::endl;
}

