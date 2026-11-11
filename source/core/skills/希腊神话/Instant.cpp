#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"


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


