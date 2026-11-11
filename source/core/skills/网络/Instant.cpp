#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"


bool 徒步::content(GameLogic& game, Player& carrier) {
	Player::RecastResult result = carrier.chooseToRecast("[徒步] 重铸一张牌", 1, false);
	if (result.discarded.size() == 0) return false;

	carrier.recover(1);
	if (result.discarded.front().get().isNumber()) {
		carrier.damage(getCount(), carrier);
	}
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


