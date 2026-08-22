#include "../header/Skill.h"
#include "../header/Player.h"
#include "../header/GameLogic.h"
#include "../header/utils.h"
#include <iostream>
#include <cmath>
#include <set>

Skill::Skill(const std::string& _name, const std::string& _info, const limit_t& _limit)
	:name(_name), info(_info), limit(_limit) {}

void Skill::reset() {
	count = 0;
}




// **********************
//         被动技
// **********************


//无子技能
PSkill::PSkill(const std::string& name, const std::string& description,
			   const limit_t& limit, bool forced,
			   const TriggerPlayer& triggerPlayer,
			   const TriggerTime& triggerTime)
	: Skill(name, description, limit),
	forced(forced),
	triggerPlayer(triggerPlayer),
	triggerTime(triggerTime) {}

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
	if ((limit != unlimited && count >= limit) || !filter(trigger)) return;
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

	if (!content(trigger)) {
		count -= 1;
		return;
	}
	std::cout << "<技能> " << trigger.getCarrier().characterName() << "发动了" << name << "！" << std::endl;
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
bool 粪怒::content(Trigger& trigger) {
	trigger.getPlayer().draw(
		std::min(trigger.getCarrier().handCount(), 5ull)
	);
	return true;
}


// ==================== 技能：隐身 ====================
bool 隐身::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2, Card::Name::wild_draw4);
}
bool 隐身::content(Trigger& trigger) {
	trigger.getCard().cancelEffect();
	trigger.getSource().draw(1);
	return true;
}


// ==================== 技能：顶置 ====================
bool 顶置::filter(const Trigger& trigger) const {
	return true;
}
bool 顶置::content(Trigger& trigger) {
	GameLogic& game = trigger.getGame();
	Pile& pile = game.getPile();

	if (pile.empty()) return false;

	const Card& bottomCard = pile.back();
	std::size_t choice = trigger.getCarrier().ask(
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
bool 带派::filter(const Trigger& trigger) const {
	return true;
}
bool 带派::content(Trigger& trigger) {
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
	return true;
}


// ==================== 技能：寒魄 ====================
bool 寒魄::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handCount() == 1;
}
bool 寒魄::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Card& card = trigger.getCard();
	carrier.getCardByIndex(0).set(card);
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：割腕 ====================
bool 割腕::filter(const Trigger& trigger) const {
	return trigger.getCard().getColor() == Card::Color::red;
}
bool 割腕::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	carrier.takeDamage(unool::random::randomSize_t(1, 5), carrier);
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：丑皇 ====================
bool 丑皇::filter(const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 丑皇::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();

	std::vector<ref<Card>> card = carrier.chooseToDiscard(
		L"弃置一张非数字牌或点击↓回复10体力", 1, false, &Card::isNotNumber
	);
	if (card.size() == 0) { //回血
		carrier.recover(10);
	}
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：军国 ====================
bool 军国::filter(const Trigger& trigger) const {
	return true;
}
bool 军国::content(Trigger& trigger) {
	Player& player = trigger.getPlayer();
	Player& carrier = trigger.getCarrier();
	if (player != carrier) //其他角色：失去 1% 最大体力，向上取整
		player.takeDamage(unool::math::ceil(player.getMaxHp() * 0.01), carrier);
	else //自己：失去1体力
		player.takeDamage(1, carrier);
	return true;
}


// ==================== 技能：家暴 ====================
bool 家暴::filter(const Trigger& trigger) const {
	return trigger.getGame().playersInclude(
		//有玩家的血量 < 携带者
		[&trigger](const Player& p) {
		return p.getHp() < trigger.getCarrier().getHp();
	});
}
bool 家暴::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();
	std::optional targetOpt = carrier.chooseOtherPlayer(L"选择家暴目标：", false, [&carrier](const Player& p) {
		return p.getHp() < carrier.getHp();
	});
	if (!targetOpt.has_value()) return false;

	Player& target = targetOpt.value();
	std::size_t damage = unool::math::ceil(target.getMaxHp() * 0.1);
	target.takeDamage(damage, trigger.getCarrier());
	std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName() << "发动家暴，造成" << damage << "点伤害！" << std::endl;

	game.broadcastState();
	return true;
}


// ==================== 技能：健身 ====================
bool 健身::filter(const Trigger& trigger) const {
	return true;
}
bool 健身::content(Trigger& trigger) {
	trigger.getCarrier().recover(5);
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：做题 ====================
bool 做题::filter(const Trigger& trigger) const {
	Card& card = trigger.getCard();
	Player& carrier = trigger.getCarrier();
	if (card.isNumber()) return carrier.handInclude(&Card::isNotNumber);
	else return carrier.handInclude(&Card::isNumber);
}
bool 做题::content(Trigger& trigger) {
	Card& card = trigger.getCard();
	Player& carrier = trigger.getCarrier();

	if (card.isNumber()) {
		return carrier.chooseToDiscard(
			L"弃置一张非数字牌", 1,
			false, [](const Card& c)->bool { return !c.isNumber(); }
		).size() == 1;
	}
	else {
		return carrier.chooseToDiscard(
			L"弃置一张数字牌", 1,
			false, [](const Card& c)->bool { return c.isNumber(); }
		).size() == 1;
	}
}


// ==================== 技能：棍击 ====================
bool 棍击::filter(const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 棍击::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();
	std::optional targetOpt = carrier.chooseOtherPlayer(L"选择棍击目标：", false);
	if (!targetOpt.has_value()) return false;

	Player& target = targetOpt.value();
	std::size_t damage = unool::math::pow(2, trigger.getCount());
	target.takeDamage(damage, trigger.getCarrier());
	std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName()
		<< "发动棍击，造成" << damage << "点伤害！" << std::endl;

	game.broadcastState();
	return true;
}


// ==================== 技能：神木 ====================
bool 神木::filter(const Trigger& trigger) const {
	return true;
}
bool 神木::content(Trigger& trigger) {
	GameLogic& game = trigger.getGame();
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_pal), 9);
	game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_draw4), 9);
	game.getPile().shuffle();
	return true;
}


// ==================== 技能：雷剑 ====================
bool 雷剑::filter(const Trigger& trigger) const {
	return trigger.getCard().getName() == Card::Name::action_rev;
}
bool 雷剑::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Card& card = trigger.getCard();
	Card::Color color = card.getColor();

	auto discarded = carrier.chooseToDiscard(
		L"弃置一张" + Card::to_wstring(color) + L"色数字牌", 1,
		false, [color](const Card& c)->bool {
		return c.isNumber() && c.getColor() == color;
	});

	if (!discarded.empty()) {
		std::size_t value = discarded.front().get().value();
		carrier.recover(value);
		std::cout << "<技能> " << carrier.characterName()
			<< "发动雷剑，弃置 [" << discarded.front().get() << "] 并回复" << value << "点体力！" << std::endl;
		trigger.getGame().broadcastState();
		return true;
	}
	return false;
}


// ==================== 技能：买棋 ====================
bool 买棋::filter(const Trigger& trigger) const {
	return true;
}
bool 买棋::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	carrier.takeDamage(10 * (trigger.getCount() - 1), trigger.getCarrier());
	if (unool::random::probability(0.5)) { //万能
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
	}
	else { //+4
		carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
	}
	return true;
}


// ==================== 技能：卖棋 ====================
bool 卖棋::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handInclude(&Card::isWild);
}
bool 卖棋::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	if (carrier.chooseToDiscard(L"弃置一张万能牌", 1, false, &Card::isWild).size() == 1) {
		carrier.recover(10);
		return true;
	}
	return false;
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
bool 耐克::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	trigger.getCard().cancelEffect();
	std::cout << carrier.characterName() << "触发技能，使此牌无效" << std::endl;
	return true;
}


// ==================== 技能：轰炸 ====================
bool 轰炸::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2, Card::Name::wild_draw4);
}
bool 轰炸::content(Trigger& trigger) {
	Player& target = trigger.getPlayer().next();
	if (trigger.getCard().is(Card::Name::action_draw2)) {
		target.takeDamage(unool::math::ceil(target.getMaxHp() * 0.02), trigger.getCarrier());
	}
	else {
		target.takeDamage(unool::math::ceil(target.getMaxHp() * 0.04), trigger.getCarrier());
	}
	return true;
}


// ==================== 技能：爆破 ====================
bool 爆破::filter(const Trigger& trigger) const {
	return trigger.getSource() == trigger.getCarrier();
}
bool 爆破::content(Trigger& trigger) {
	Player& player = trigger.getPlayer();
	Hand& hand = player.getHand();
	auto card = hand.takeCardByIndex(unool::random::randomSize_t(0, hand.count() - 1));
	player.takeDamage(card->value(), trigger.getCarrier());
	std::cout << "爆破获取了 [" << *card << "]，造成了" << card->value() << "点伤害！" << std::endl;
	trigger.getCarrier().gainCard(std::move(card));
	return true;
}


// ==================== 技能：电音 ====================
bool 电音::filter(const Trigger& trigger) const {
	return true;
}
bool 电音::content(Trigger& trigger) {
	Hand& hand = trigger.getCarrier().getHand();
	hand.forEachIf(&Card::isNumber, [](Card& card) {
		card.setName(unool::random::randomGet(Card::numberCardsFrom0));
	});
	trigger.getCarrier().recover(1);
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：蒙面 ====================
bool 蒙面::filter(const Trigger& trigger) const {
	return true;
}
bool 蒙面::content(Trigger& trigger) {
	trigger.getNumber() = unool::math::ceil(trigger.getNumber() * 0.75);
	return true;
}


// ==================== 技能：锐刻 ====================
bool 锐刻::filter(const Trigger& trigger) const {
	if (disabled) return false;
	return trigger.getCard().getName() == Card::Name::number_5;
}
bool 锐刻::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();

	std::size_t choice = carrier.ask(L"发动[锐刻]，选择一项：", {
		L"令一名角色摸1张牌",
		L"令一名角色摸5张牌并失去此技能至本局结束"
									 }, false);

	if (choice == 0) return false;

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
	return true;
}


// ==================== 技能：巨富 ====================
bool 巨富::filter(const Trigger& trigger) const {
	return true;
}
bool 巨富::content(Trigger& trigger) {
	trigger.getCarrier().draw(4); //初始8张 + 4张 = 12张
	trigger.getGame().broadcastState();
	return true;
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
bool 破产::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Hand& hand = carrier.getHand();
	carrier.discardByIndex(unool::random::randomSize_t(0, hand.count() - 1));
	std::cout << "<技能> " << carrier.characterName() << "触发破产，随机弃置一张牌" << std::endl;
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：假酒 ====================
bool 假酒::filter(const Trigger& trigger) const {
	return trigger.getCard().isAction();
}
bool 假酒::content(Trigger& trigger) {
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

	if (candidates.empty()) return false;

	std::size_t idx = unool::random::randomSize_t(0, candidates.size() - 1);
	carrier.gainCard(std::move(candidates[idx]));
	std::cout << "<技能> " << carrier.characterName() << "发动假酒，获得了一张不同颜色的非数字牌" << std::endl;
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：窃观 ====================
bool 窃观::filter(const Trigger& trigger) const {
	return trigger.getCards().size() == 1;
}
bool 窃观::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Player& drawer = trigger.getPlayer();
	const Card& card = trigger.getCard();
	std::wstring title = L"【窃观】" + drawer.characterNameW()
		+ L"获得了 " + Card::to_wstring(card.getColor())
		+ L" " + Card::to_wstring(card.getName()) + L"（需确认）";
	carrier.hint(title);
	return true;
}


// ==================== 技能：生存 ====================
bool 生存::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handCount() > 0;
}
bool 生存::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();

	std::size_t x = trigger.getCount();
	if (x > 9) x = 9;
	Card::Name targetName = static_cast<Card::Name>(static_cast<int>(Card::Name::number_0) + x);

	Card::Color targetColor = unool::random::randomGet(Card::colors);

	std::unique_ptr<Card> target = Card::make(targetColor, targetName);
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
	return true;
}


// ==================== 技能：创造 ====================
bool 创造::filter(const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (c.getName() != Card::Name::number_9) return false;
	if (c.is(Card::Color::black)) return false;
	return usedColors.find(c.getColor()) == usedColors.end();
}
bool 创造::content(Trigger& trigger) {
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
	return true;
}


// ==================== 技能：炼兵 ====================
std::map<Card::Name, std::size_t> 炼兵::buildPairs(Player& carrier) const {
	std::map<Card::Name, std::size_t> cnt;
	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		const Card& c = carrier.getHand().getCardByIndex(i);
		if (c.getColor() == Card::Color::black) continue;
		if (c.isWild()) continue;
		cnt[c.getName()]++;
	}
	return cnt;
}
bool 炼兵::filter(const Trigger& trigger) const {
	Player& carrier = trigger.getCarrier();
	if (carrier.handCount() < 2) return false;
	auto cnt = buildPairs(carrier);
	for (const auto& kv : cnt) {
		if (kv.second >= 2 && usedNames.find(kv.first) == usedNames.end()) {
			return true;
		}
	}
	return false;
}
bool 炼兵::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();

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
	Card::Color color = unool::random::randomGet(Card::colors);
	carrier.gainCard(Card::make(color, Card::Name::action_draw2));
	usedNames.insert(target);
	std::cout << "<技能> " << carrier.characterName() << "发动炼兵，弃两张"
		<< Card::to_string(target)
		<< "，获得一张" << Card::to_string(color) << "+2" << std::endl;
	game.broadcastState();
	return true;
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
bool 好火::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Player& target = trigger.getPlayer();

	auto result = carrier.chooseToGive(L"选择一张手牌交给" + target.characterNameW(),
									   target, false, unool::alwaysTrue);
	if (!result.has_value()) return false;

	usedPlayerIds.insert(target.getId());
	std::cout << "<技能> " << carrier.characterName() << "发动好火，交给"
		<< target.characterName() << "一张" << result.value().get().toString() << std::endl;
	return true;
}


//================== 森罗 =================
bool 森罗::filter(const Trigger& trigger) const {
	return true;
}
bool 森罗::content(Trigger& trigger) {
	trigger.getCarrier().getHand().forEachIf(
		[](const Card& c) {
		return c.getColor() != Card::Color::black;
	}, [](Card& c) {
		c.setColor(Card::Color::green);
	});
	return true;
}


//================大脚=====================
bool 大脚::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handInclude([](const Card& c) {
		return c.isWild();
	});
}
bool 大脚::content(Trigger& trigger) {
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
	return true;
}


//================过江==============
bool 过江::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_draw2);
}
bool 过江::content(Trigger& trigger) {
	trigger.getSource().draw(2);
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
bool 大盏::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handInclude([](const Card& c) {
		return c.isNumber();
	});
}
bool 大盏::content(Trigger& trigger) {
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
	return true;
}


//==============举报=============
bool 举报::filter(const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 举报::content(Trigger& trigger) {
	setForced(true);
	Player& player = trigger.getPlayer();
	Player& carrier = trigger.getCarrier();
	player.takeDamage(unool::math::ceil(0.1 * player.getHp()), carrier);
	return true;
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
bool 猥琐::content(Trigger& trigger) {
	trigger.getCarrier().recover(1);
	return true;
}



bool 棋王::filter(const Trigger& trigger) const {
	return trigger.getGame().getDiscardPile().count() >= 2
		&& trigger.getCard() == trigger.getGame().getDiscardPile()[1];
}
bool 棋王::content(Trigger& trigger) {
	trigger.getCarrier().chooseToDiscard(L"弃置两张手牌", 2, true);
	return true;
}


bool 金铲::filter(const Trigger& trigger) const {
	return trigger.getNumber() == 1;
}
bool 金铲::content(Trigger& trigger) {
	trigger.getNumber() = 0;
	trigger.getPlayer().takeDamage(1, trigger.getCarrier());
	return true;
}


//==============淘汰=============
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
bool 淘汰::content(Trigger& trigger) {
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
			unool::random::randomGet(Card::actionCards)
		));
	}
	return true;
}


bool 光合::filter(const Trigger& trigger) const {
	return trigger.getCard().is(Card::Name::action_ban, Card::Name::action_draw2);
}
bool 光合::content(Trigger& trigger) {
	Card& card = trigger.getCarrier().judge();
	if (card.isNumber()) {
		trigger.getSource().draw(1);
	}
	else if (card.isAction()) {
		trigger.getCard().cancelEffect();
		if (trigger.getCard().is(Card::Name::action_draw2)) trigger.getCarrier().draw(2);
	}
	return true;
}


bool 射门::filter(const Trigger& trigger) const {
	return trigger.getCard().isNumber();
}
bool 射门::content(Trigger& trigger) {
	GameLogic& game = trigger.getGame();
	Player& carrier = trigger.getCarrier();
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
			target.draw(1);
		}
	}
	return true;
}


bool 招待::filter(const Trigger& trigger) const {
	return true;
}
bool 招待::content(Trigger& trigger) {
	//选角色
	Player& carrier = trigger.getCarrier();
	std::optional targetOpt = carrier.chooseOtherPlayer(L"[招待] 选择一名其他角色", true);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value();

	//给牌
	carrier.chooseToGive(L"选择一张手牌交给" + target.characterNameW(), target, true);
	return true;
}


// ==================== 技能：追番 ====================
bool 追番::filter(const Trigger& trigger) const {
	//先检测手中是否有点数≤5的数字牌
	return trigger.getCarrier().handInclude([](const Card& c) {
		return c.isNumber() && c.value() <= 5;
	});
}
bool 追番::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	//选择一张点数≤5的数字牌
	auto cardRef = carrier.chooseToOperate(
		L"选择一张点数≤5的数字牌", false,
		[](const Card& c) {
		return c.isNumber() && c.value() <= 5;
	},
		[](Card&) {});
	if (!cardRef.has_value()) return false; //取消发动
	//自选+1/+2/+3
	const std::size_t addChoice = carrier.ask(L"选择增加的点数：",
											  { L"+1", L"+2", L"+3" }, false);
	const int add = static_cast<int>(addChoice);
	Card& card = cardRef->get();
	card.setName(Card::numberCardsFrom0[card.value() + add]);
	std::cout << "<技能> " << carrier.characterName() << "发动追番，将一张"
		<< card.toString() << "的点数+" << add << std::endl;
	trigger.getGame().broadcastState();
	return true;
}

// ==================== 技能：崩三 ====================
bool 崩三::filter(const Trigger& trigger) const {
	if (trigger.getCard().is(Card::Name::number_3)) ++count3;
	else return false;
	Player& carrier = trigger.getCarrier();
	return count3 % 2 == 0
		&& carrier.handInclude([](const Card& c) { return c.is(Card::Name::number_6); });
}
bool 崩三::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	carrier.chooseToDiscard(
		L"选择一张[6]弃置", 1, false,
		[](const Card& c) { return c.is(Card::Name::number_6); });
	trigger.getGame().broadcastState();
	return true;
}

// ==================== 技能：望日 ====================
bool 望日::filter(const Trigger& trigger) const {
	return trigger.getCarrier().handInclude([](const Card& c) {
		return c.is(Card::Color::yellow) && c.isNumber() && c.value() < 9;
	});
}
bool 望日::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	auto cardRef = carrier.chooseToOperate(
		L"【望日】\n选择一张黄色数字牌点数+1", true,
		[](const Card& c) {
		return c.is(Card::Color::yellow) && c.isNumber() && c.value() < 9;
	}, [](Card&) {});
	if (!cardRef.has_value()) return false;
	Card& card = cardRef->get();
	card.setName(Card::numberCardsFrom0[card.value() + 1]);
	std::cout << "<技能> " << carrier.characterName() << "发动望日，将一张"
		<< card.toString() << "的点数+1" << std::endl;
	trigger.getGame().broadcastState();
	return true;
}

// ==================== 技能：慈父 ====================
bool 慈父::filter(const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	return c.is(Card::Color::yellow) && c.is(Card::Name::number_9);
}
bool 慈父::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
	std::cout << "<技能> " << carrier.characterName() << "发动慈父，获得一张【+4】" << std::endl;
	trigger.getGame().broadcastState();
	return true;
}

// ==================== 技能：朔日 ====================
bool 朔日::filter(const Trigger& trigger) const {
	const Card& c = trigger.getCard();
	if (!c.is(Card::Color::yellow)) return false;
	Player& carrier = trigger.getCarrier();
	bool hasNumber = carrier.handInclude([](const Card& c) { return c.isNumber(); });
	bool hasAction = carrier.handInclude([](const Card& c) { return c.isAction(); });
	return hasNumber || hasAction;
}
bool 朔日::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	//回复1点体力
	carrier.recover(1);
	std::cout << "<技能> " << carrier.characterName() << "发动朔日，回复1点体力" << std::endl;
	trigger.getGame().broadcastState();

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
			<< card.toString() << std::endl;
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
			<< card.toString() << std::endl;
	}

	trigger.getGame().broadcastState();
	return true;
}

// ==================== 技能：徒步 ====================
bool 徒步::filter(const Trigger& trigger) const {
	return !trigger.getCard().isNumber();
}
bool 徒步::content(Trigger& trigger) {
	setForced(true);
	Player& carrier = trigger.getCarrier();
	std::size_t x = trigger.getCount();
	carrier.takeDamage(x, carrier);
	carrier.chooseToRecast(L"【徒步】重铸一张手牌", 1, true);
	trigger.getGame().broadcastState();
	return true;
}

// ==================== 技能：健忘 ====================
bool 健忘::filter(const Trigger& trigger) const {
	return true;
}
bool 健忘::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();
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
bool 豪赌::filter(const Trigger& trigger) const {
	(void)trigger;
	return true;
}
bool 豪赌::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	GameLogic& game = trigger.getGame();

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
		carrier.takeDamage(5, carrier);
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：黑帮 ====================
bool 黑帮::filter(const Trigger& trigger) const {
	return true;
}
bool 黑帮::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	std::size_t X = trigger.getGame().getMatchCount();
	for (std::size_t i = 0; i < X; ++i) {
		carrier.gainCard(Card::make(Card::Color::black,
									unool::random::randomGet(Card::wildCards)));
	}
	std::cout << "<技能> " << carrier.characterName() << "发动黑帮，获得了" << X << "张万能牌" << std::endl;
	trigger.getGame().broadcastState();
	return true;
}

// ==================== 技能：拖拉 ====================
bool 拖拉::filter(const Trigger& trigger) const {
	return trigger.getCard().isWild();
}
bool 拖拉::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
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
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：互质 ====================
bool 互质::areCoprime(const int a, const int b) {
	if (a == 0 || b == 0) {
		return false;  // 0 与任何数（包括 0）都不互质
	}
	return std::gcd(a, b) == 1;
}
bool 互质::isPairwiseCoprime(const std::vector<int>& nums) {
	// 空集或单元素集视为两两互质
	if (nums.size() <= 1) {
		return true;
	}
	// 检查是否存在 0（有 0 且不止一个元素则必然不互质）
	if (std::ranges::any_of(nums, [](int x) { return x == 0; })) {
		return false;
	}
	// 使用索引视图生成所有数对并检查
	for (auto i : std::views::iota(0ull, nums.size())) {
		for (auto j : std::views::iota(i + 1, nums.size())) {
			if (!areCoprime(nums[i], nums[j])) {
				return false;
			}
		}
	}
	return true;
}
bool 互质::filter(const Trigger& trigger) const {
	Hand& hand = trigger.getCarrier().getHand();
	std::vector<int> nums;
	for (const auto& c : hand) {
		if (c->isNumber()) nums.push_back(c->value());
	}
	return isPairwiseCoprime(nums);
}
bool 互质::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	std::size_t product = 1;
	carrier.getHand().forEachIf(
		&Card::isNumber,
		[&product](const Card& c) {
		product *= c.value();
	}
	);
	carrier.takeDamage(product, carrier);
	return true;
}


// ==================== 技能：难题_变牌（子技能） ====================
bool 难题_变牌::filter(const Trigger& trigger) const {
	return !record->empty();
}
bool 难题_变牌::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
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
		<< cardRef->get().toString() << std::endl;
	trigger.getGame().broadcastState();
	return true;
}


// ==================== 技能：难题 ====================
bool 难题::filter(const Trigger& trigger) const {
	for (const auto& c : trigger.getCards()) {
		if (c.get().isNumber()) return true;
	}
	return false;
}
bool 难题::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
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
bool 迷烟::filter(const Trigger& trigger) const {
	trigger.getGame().broadcastState();
	return true;
}

bool 迷烟::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	Card& card = trigger.getCard();
	carrier.showCard(card);
	std::optional targetOpt = carrier.chooseOtherPlayer(L"[迷烟] 选择一名其他玩家", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	std::wstring colorStr = Card::to_wstring(card.getColor());
	std::vector discard = target.chooseToDiscard(
		L"[迷烟]\n弃置一张" + colorStr + L"色手牌或万能牌，\n或取消并摸一张牌", 1, false,
		[&card](const Card& c) {
		return c.getColor() == card.getColor() || c.isWild();
	});
	if (discard.size() == 0) { //没弃牌，摸一张
		target.draw(1);
	}
	return true;
}

bool 创世::filter(const Trigger& trigger) const {
	return true;
}
bool 创世::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
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
		std::vector<Card::Color> colorVec(Card::colors.begin(), Card::colors.end());
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
	trigger.getGame().broadcastState();
	return true;
}

bool 补天::filter(const Trigger& trigger) const {
	const Player& carrier = trigger.getCarrier();
	const auto& hand = carrier.getHand();
	if (hand.empty()) return false;
	//手牌中只有一种牌名
	Card::Name firstName = hand[0].getName();
	for (std::size_t i = 1; i < hand.count(); ++i) {
		if (hand[i].getName() != firstName) return false;
	}
	//该牌名未被记录
	if (std::ranges::find(record, firstName) != record.end()) return false;
	//至少有一个未记录的牌名（排除即将记录的firstName）
	for (const auto& name : Card::allCards) {
		if (name != firstName && std::ranges::find(record, name) == record.end()) {
			return true;
		}
	}
	return false;
}

bool 补天::content(Trigger& trigger) {
	Player& carrier = trigger.getCarrier();
	const auto& hand = carrier.getHand();
	Card::Name recordedName = hand[0].getName();

	//记录牌名
	record.push_back(recordedName);

	//算出所有可选的牌名颜色组合（排除已记录的牌名）
	std::vector<Card::ColorName> available;
	for (const auto& name : Card::allCards) {
		if (std::ranges::find(record, name) != record.end()) continue;
		for (const auto& color : Card::colors) {
			available.emplace_back(color, name);
		}
	}

	//随机选一个
	Card targetCard(unool::random::randomGet(available));

	//选择一张手牌变为此牌
	auto cardOpt = carrier.chooseToOperate(
		L"【补天】选择一张手牌变为" + targetCard.toWString(), true, unool::alwaysTrue,
		[&targetCard](Card& c) {
		c.set(targetCard);
	});
	if (!cardOpt.has_value()) return false;
	std::cout << "<技能> " << carrier.characterName() << "发动补天，将一张牌变为"
		<< targetCard << std::endl;
	trigger.getGame().broadcastState();
	return true;
}
void 补天::reset() {
	PSkill::reset();
	record.clear();
}
