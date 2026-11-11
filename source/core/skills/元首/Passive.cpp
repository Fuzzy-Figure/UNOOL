#include "skills/Skills.h"
#include "GameLogic.h"
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


// ==================== 技能：望日 ====================
bool 望日::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return carrier.handInclude([](const Card& c) {
		return c.is(Card::Color::yellow) && c.isNumber() && c.value() < 9;
	});
}
bool 望日::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	auto cardRef = carrier.chooseToOperate(
		"【望日】\n选择一张黄色数字牌点数+1", true,
		[](const Card& c) {
		return c.is(Card::Color::yellow) && c.isNumber() && c.value() < 9;
	}, [](Card&) {});
	if (!cardRef.has_value()) return false;
	Card& card = cardRef->get();
	card.setName(Card::numberCardsFrom0[card.value() + 1]);
	std::println("<技能> {}发动望日，将一张{}的点数+1", carrier.displayName(), card);
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
	std::println("<技能> {}发动慈父，获得一张【+4】", carrier.displayName());
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
	std::println("<技能> {}发动朔日，回复1点体力", carrier.displayName());
	game.broadcastState();

	//构建可选项
	bool hasNumber = carrier.handInclude([](const Card& c) { return c.isNumber(); });
	bool hasAction = carrier.handInclude([](const Card& c) { return c.isAction(); });

	std::vector<std::string> opts;
	std::vector<int> branchMap;
	if (hasNumber) {
		opts.push_back("将一张数字牌变为黄色并调整点数");
		branchMap.push_back(1);
	}
	if (hasAction) {
		opts.push_back("将一张功能牌变为黄色随机功能牌");
		branchMap.push_back(2);
	}

	std::size_t choice = carrier.ask("【朔日】选择一项：", opts, false);
	if (choice == 0) return true; //取消，但已回复体力

	int branch = branchMap[choice - 1];

	if (branch == 1) {
		//选择一张数字牌，变黄色
		auto cardRef = carrier.chooseToOperate(
			"选择一张数字牌变为黄色", false,
			[](const Card& c) { return c.isNumber(); },
			[](Card& c) { c.setColor(Card::Color::yellow); });
		if (!cardRef.has_value()) return true;
		Card& card = cardRef->get();
		//选择+1/-1
		std::vector<std::string> adjOpts;
		std::vector<int> adjMap;
		if (card.value() > 0) {
			adjOpts.push_back("-1");
			adjMap.push_back(-1);
		}
		if (card.value() < 9) {
			adjOpts.push_back("+1");
			adjMap.push_back(1);
		}
		if (!adjOpts.empty()) {
			std::size_t adjChoice = carrier.ask("选择调整点数：", adjOpts, false);
			if (adjChoice > 0) {
				int adj = adjMap[adjChoice - 1];
				card.setName(Card::numberCardsFrom0[card.value() + adj]);
			}
		}
		std::println("<技能> {}发动朔日，将一张牌变为{}", carrier.displayName(), card);
	}
	else {
		//选择一张功能牌
		auto cardRef = carrier.chooseToOperate(
			"选择一张功能牌变为黄色随机功能牌", false,
			[](const Card& c) { return c.isAction(); },
			[](Card&) {});
		if (!cardRef.has_value()) return true;
		Card& card = cardRef->get();
		//随机功能牌名
		Card::Name randomName = unool::random::randomGet(Card::actionCards);
		card.setName(randomName);
		card.setColor(Card::Color::yellow);
		std::println("<技能> {}发动朔日，将一张牌变为{}", carrier.displayName(), card);
	}

	game.broadcastState();
	return true;
}

// ==================== 技能：健忘 ====================
bool 健忘::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	std::vector<Card::Color> colorVec(Card::fourColors.begin(), Card::fourColors.end());
	const std::optional colorOpt = carrier.chooseCardColor("【健忘】请选择新的公共颜色", false, colorVec);
	if (!colorOpt.has_value()) return false;

	const Card::Color newColor = colorOpt.value();
	game.setCurrentColor(newColor);
	std::println("<技能> {}发动健忘，将公共颜色改为{}",
				 carrier.displayName(), newColor);
	game.broadcastState();
	return true;
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
	std::vector<std::string> options;
	for (std::size_t i = 1; i <= colorSet.size(); ++i) {
		options.push_back(std::format("弃{}张颜色各不相同的牌", i));
	}
	//选数量，0=取消
	std::size_t count = carrier.ask("【叛党】选择弃牌数量", options, false,
									std::chrono::milliseconds(60000));
	if (count == 0) return false;

	std::unordered_set<Card::Color> usedColors;
	std::size_t discarded = 0;
	for (std::size_t i = 0; i < count; ++i) {
		auto result = carrier.chooseToDiscard(
			std::format("【叛党】选择第{}张牌弃置（颜色各不相同）", i + 1),
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
		std::println("<技能> {}发动叛党，弃置了{}张颜色各不相同的牌", carrier.displayName(), discarded);
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
	std::vector<std::string> options;
	options.push_back(std::format("弃置所有{}色牌", colors[0]));
	options.push_back(std::format("弃置所有{}色牌", colors[1]));
	std::size_t choice = carrier.ask("【清洗】选择弃置哪种颜色的手牌", options, false);
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
	std::println("<技能> {}发动清洗，弃置了{}张{}色牌",
				 carrier.displayName(), discardCount, target);

	game.broadcastState();
	return true;
}


// ==================== 技能：投日 ====================
bool 投日::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	return !carrier.handEmpty();
}

bool 投日::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	//X为此技能本场发动次数（count 已由 launch 在 content 前自增，首次发动为1）
	const std::size_t X = getCount();
	const std::size_t num = std::min(X, carrier.handCount());
	std::println("<技能> {}发动投日，弃置{}张牌", carrier.displayName(), num);
	carrier.chooseToDiscard(std::format("【投日】弃置{}张牌", num), num, true);
	game.broadcastState();
	return true;
}


// ==================== 技能：熬鹰 ====================

// ==================== 技能：宣战 ====================
bool 宣战::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	const bool played = carrier.getHasUsed();
	const int delta = played ? 1 : 2;
	const int num0 = static_cast<int>(Card::Name::number_0);

	for (std::size_t i = 0; i < carrier.handCount(); ++i) {
		Card& c = carrier.getCardByIndex(i);
		if (!c.isNumber()) continue;
		const int point = static_cast<int>(c.getName()) - num0;
		if (point > 5) {
			int np = point - delta;
			if (np < 0) np = 0;
			c.set(c.getColor(), static_cast<Card::Name>(num0 + np));
		}
		else if (point < 5) {
			int np = point + delta;
			if (np > 9) np = 9;
			c.set(c.getColor(), static_cast<Card::Name>(num0 + np));
		}
	}

	//统计手中【5】的数量
	std::size_t fiveCount = 0;
	for (std::size_t i = 0; i < carrier.handCount(); ++i)
		if (carrier.getCardByIndex(i).is(Card::Name::number_5)) ++fiveCount;

	if (fiveCount >= 4) {
		const std::size_t choice = carrier.ask(
			std::format("【宣战】你手中有{}张【5】，是否全部转化为一张【+4】打出？", fiveCount),
			{ "发动", "不发动" }, false);
		if (choice == 1) {
			//找到第一张【5】改为【+4】
			std::size_t firstIdx = std::string::npos;
			for (std::size_t i = 0; i < carrier.handCount(); ++i) {
				if (carrier.getCardByIndex(i).is(Card::Name::number_5)) { firstIdx = i; break; }
			}
			if (firstIdx != std::string::npos) {
				carrier.getCardByIndex(firstIdx).set(Card::Color::black, Card::Name::wild_draw4);
				//销毁其余的【5】（直接移出游戏，不置入弃牌堆，降序删除避免下标错位）
				std::vector<std::size_t> rest;
				for (std::size_t i = firstIdx + 1; i < carrier.handCount(); ++i)
					if (carrier.getCardByIndex(i).is(Card::Name::number_5)) rest.push_back(i);
				for (auto it = rest.rbegin(); it != rest.rend(); ++it)
					(void)carrier.takeCardByIndex(*it);

				std::println("<技能> {}发动宣战，将{}张【5】转化为【+4】打出", carrier.displayName(), fiveCount);
				carrier.useCardByIndex(firstIdx);
			}
		}
	}

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}


// ==================== 技能：赔款 ====================
bool 赔款::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//仅当自己为封禁来源时触发
	return trigger.hasSource() && &trigger.getSource() == &carrier;
}

bool 赔款::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();
	//若目标手中没有【+2】，随机获得一张四种颜色的【+2】
	if (!target.handInclude([](const Card& c) { return c.is(Card::Name::action_draw2); })) {
		target.gainCard(std::make_unique<Card>(
			unool::random::randomGet(Card::fourColors), Card::Name::action_draw2));
		std::println("<技能> {}发动赔款，{}没有【+2】，获得一张随机色【+2】",
					 carrier.displayName(), target.displayName());
	}
	game.broadcastState();
	return true;
}


