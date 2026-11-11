#include "skills/Skills.h"
#include "GameLogic.h"
#include <algorithm>
#include <numeric>
#include <optional>

// ==================== 技能：顶置 ====================
bool 顶置::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Pile& pile = game.getPile();

	if (pile.empty()) return false;

	const Card& bottomCard = pile.back();
	std::size_t choice = carrier.ask(
		std::format("牌堆底是{}，是否顶置？", bottomCard.toString()),
		{ "顶置", "不顶置" },
		true
	);

	if (choice == 1) {
		std::unique_ptr<Card> card = pile.take_back(game.getDiscardPile());
		pile.push_front(std::move(card));
		std::println("<技能> 顶置成功！");
	}
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
		"弃置一张非数字牌或点击↓回复10体力", 1, false, &Card::isNotNumber
	);
	if (card.size() == 0) { //回血
		carrier.recover(10);
	}
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
			"弃置一张非数字牌", 1,
			false, &Card::isNotNumber
		).size() == 1;
	}
	else {
		return carrier.chooseToDiscard(
			"弃置一张数字牌", 1,
			false, &Card::isNumber
		).size() == 1;
	}
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
	std::println("<技能> {}触发破产，随机弃置一张牌", carrier.displayName());
	game.broadcastState();
	return true;
}



// ==================== 技能：窃观 ====================
bool 窃观::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return trigger.getCards().size() == 1;
}
bool 窃观::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& drawer = trigger.getPlayer();
	const Card& card = trigger.getCard();
	std::string title = std::format("【窃观】{}获得了 {}", drawer.characterName(), card.toString());
	carrier.hint(title);
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
			std::format("弃置一张点数 ≤ {}的{}色数字牌", half, card.getColor()),
			1, true, [&card, &half](const Card& c) {
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


// ==================== 技能：豪赌 ====================
bool 豪赌::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//判定
	Card& card = carrier.judge();
	game.forEachPlayer([&card](Player& p) {
		p.hint(std::format("[豪赌] 判定结果是{}", card.toString()));
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
			carrier.chooseToRecast("【豪赌】重铸一张手牌", 1, true);
		}
	}
	else if (card.is(Card::Color::red)) {
		carrier.damage(5, carrier);
	}

	game.broadcastState();
	return true;
}


// ==================== 技能：迷烟 ====================
bool 迷烟::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::optional cardOpt = carrier.chooseToShow("[迷烟] 选择一张非万能牌", false, &Card::isNotWild);
	if (!cardOpt.has_value()) return false;
	Card& card = cardOpt.value().get();

	std::optional targetOpt = carrier.chooseOtherPlayer("[迷烟] 选择一名其他玩家", false);
	if (!targetOpt.has_value()) return false;
	Player& target = targetOpt.value().get();

	std::string colorStr = std::format("{}", card.getColor());
	std::vector discard = target.chooseToDiscard(
		std::format("[迷烟]\n弃置一张{}色手牌或万能牌，\n或取消并摸一张牌", colorStr), 1, false,
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
	auto nameOpt = carrier.chooseCardName("【创世】选择一个牌名", false, available);
	if (!nameOpt.has_value()) return false;
	Card::Name selectedName = nameOpt.value();

	//选颜色
	Card::Color selectedColor = Card::Color::black;
	if (!Card::is_wild(selectedName)) {
		std::vector<Card::Color> colorVec(Card::fourColors.begin(), Card::fourColors.end());
		auto colorOpt = carrier.chooseCardColor("【创世】选择颜色", false, colorVec);
		if (!colorOpt.has_value()) return false;
		selectedColor = colorOpt.value();
	}

	//选手牌变为此牌
	Card targetCard(selectedColor, selectedName);
	auto cardOpt = carrier.chooseToOperate(
		std::format("【创世】选择一张手牌变为{}", targetCard.toString()), false, unool::alwaysTrue,
		[&targetCard](Card& c) {
		c.set(targetCard);
	});
	if (!cardOpt.has_value()) return false;
	std::println("<技能> {}发动创世，将一张牌变为{}", carrier.displayName(), targetCard);
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
		std::string hintText = "【补天】已记录的牌名：";
		for (std::size_t i = 0; i < sorted.size(); ++i) {
			if (i > 0) hintText += "、";
			hintText += std::format("{}", sorted[i]);
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
			std::format("【补天】牌名是{}，选择颜色", targetName), true);
		if (!colorOpt.has_value()) return false;
		targetColor = colorOpt.value();
	}
	Card targetCard(targetColor, targetName);

	//选择一张手牌变为此牌
	auto cardOpt = carrier.chooseToOperate(
		std::format("【补天】选择一张手牌变为{}", targetCard.toString()), true, unool::alwaysTrue,
		[&targetCard](Card& c) {
		c.set(targetCard);
	});
	if (!cardOpt.has_value()) return false;
	std::println("<技能> {}发动补天，将一张牌变为{}", carrier.displayName(), targetCard);
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
	auto discarded = carrier.chooseToDiscard("[犬子] 选择一张牌弃置", 1, false);
	if (discarded.empty()) return false;

	playCount = 0;
	carrier.markCharInfoDirty();
	std::println("<技能> {}发动犬子，弃置了一张牌", carrier.displayName());
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

// ==================== 技能：爬竿（主技能） ====================
bool 爬竿::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto targetOpt = carrier.chooseOtherPlayer("[爬竿] 选择一名其他角色", true);
	if (!targetOpt.has_value()) return false;

	targetOpt_ = targetOpt;
	std::println("<技能> {}发动爬竿，选择了{}", carrier.displayName(), targetOpt_->get().displayName());
	return true;
}

// ==================== 技能：爬竿_伤害（子技能） ====================
bool 爬竿_伤害::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	if (auto targetOpt = carrier.getSkill<爬竿>().targetOpt_; targetOpt.has_value()) {
		Player& target = targetOpt.value();
		return trigger.getPlayer() == target
			&& !trigger.getPlayer().getHasUsed();
	}
	return false;
}
bool 爬竿_伤害::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();
	std::size_t damage = target.getMaxHp() / 100; //1%最大体力向下取整
	if (damage > 0) target.damage(damage, carrier);
	std::println("<技能> {}发动爬竿，{}失去{}点体力", carrier.displayName(), target.displayName(), damage);
	game.broadcastState();
	return true;
}

// ==================== 技能：渊涡 ====================
bool 渊涡::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return !carrier.getHasUsed();
}
bool 渊涡::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	carrier.recover(1);
	std::println("<技能> {}发动渊涡，回复1点体力", carrier.displayName());
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
	std::vector<std::string> optionTexts;
	for (std::size_t opt : options) {
		switch (opt) {
			case 1: optionTexts.emplace_back("此牌无效"); break;
			case 2: optionTexts.emplace_back("你弃置一张牌"); break;
			case 3: optionTexts.emplace_back("其摸两张牌"); break;
		}
	}

	std::size_t choiceIdx = carrier.ask(std::format("【治病】对方打出了{}，选择一项：", card.toString()), optionTexts, true);
	std::size_t chosenOpt = options[choiceIdx - 1];

	switch (chosenOpt) {
		case 1: //此牌无效
			card.cancelEffect();
			std::println("<技能> {}发动治病，令{}打出的{}无效", carrier.displayName(), player.displayName(), card);
			break;
		case 2: //你弃置一张牌
			carrier.chooseToDiscard("【治病】弃置一张牌", 1, true);
			std::println("<技能> {}发动治病，弃置一张牌", carrier.displayName());
			break;
		case 3: //其摸两张牌
			player.draw(2, DrawReason::skill);
			std::println("<技能> {}发动治病，令{}摸两张牌", carrier.displayName(), player.displayName());
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




