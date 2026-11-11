#include "skills/Skills.h"
#include "GameLogic.h"
#include "utils.h"


// ==================== 技能：伪政 ====================
bool 伪政::filter(const GameLogic& game, const Player& carrier) const {
	//需要从牌堆两侧共取 2×viewCount 张
	const std::size_t available = game.getPile().count() + game.getDiscardPile().count();
	return available >= 2 * viewCount;
}

bool 伪政::content(GameLogic& game, Player& carrier) {
	Pile& pile = game.getPile();

	//取牌堆顶和牌堆底各 viewCount 张
	std::vector<std::unique_ptr<Card>> topCards, bottomCards;
	for (std::size_t i = 0; i < viewCount; ++i) {
		topCards.push_back(pile.take_front(game.getDiscardPile()));
	}
	for (std::size_t i = 0; i < viewCount; ++i) {
		bottomCards.push_back(pile.take_back(game.getDiscardPile()));
	}

	//展示给所有其他玩家
	std::string topStr, bottomStr;
	for (const auto& c : topCards) topStr += std::format("{} ", *c);
	for (const auto& c : bottomCards) bottomStr += std::format("{} ", *c);
	game.forEachOtherPlayer(carrier, [&](Player& p) {
		p.hint(std::format("【伪政】{}展示了牌堆顶：{}，牌堆底：{}", carrier.characterName(), topStr, bottomStr));
	});

	//构建选项：牌堆顶各张 + 牌堆底各张 + 不获得
	std::vector<std::string> options;
	for (const auto& c : topCards) options.push_back(std::format("获得牌堆顶的{}", *c));
	for (const auto& c : bottomCards) options.push_back(std::format("获得牌堆底的{}", *c));
	const std::size_t noneChoice = options.size();
	options.push_back("不获得");

	const std::size_t choice = carrier.ask(
		std::format("【伪政】牌堆顶：{}；牌堆底：{}；选择获得一张", topStr, bottomStr),
		options, false);

	//获得选中的牌
	if (choice != 0 && choice != noneChoice) {
		const std::size_t idx = choice - 1;
		std::unique_ptr<Card> gained = idx < topCards.size()
			? std::move(topCards[idx])
			: std::move(bottomCards[idx - topCards.size()]);
		carrier.gainCard(std::move(gained));
		++gainedTotal;
		//每因此获得十张牌，牌堆两侧观看牌数各增加一张（至多各三张）
		if (gainedTotal % 10 == 0 && viewCount < 3) ++viewCount;
	}

	//弃置其余的牌
	for (auto& c : topCards) {
		if (c) game.putCardToDiscardPile(std::move(c), Card::DiscardReason::discard, carrier);
	}
	for (auto& c : bottomCards) {
		if (c) game.putCardToDiscardPile(std::move(c), Card::DiscardReason::discard, carrier);
	}

	std::println("<技能> {}发动伪政（观看牌堆顶和牌堆底各{}张）", carrier.displayName(), viewCount);
	game.broadcastState();
	return true;
}

// ==================== 技能：伪政_计数 ====================
bool 伪政_计数::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	++totalDiscarded;
	//每有十张牌进入弃牌堆，伪政本局可用次数+1
	if (totalDiscarded % 10 == 0) {
		if (auto skill = carrier.findSkill<伪政>(); skill.has_value()) {
			skill->get().increaseLimit(1);
			carrier.markCharInfoDirty();
			std::println("<技能> {}的伪政可用次数+1（当前{}次）",
						 carrier.displayName(), skill->get().getLimit().value());
		}
	}
	return true;
}


// ==================== 技能：改革 ====================
bool 改革::content(GameLogic& game, Player& carrier) {
	//先选目标，取消则不失去体力
	auto targetOpt = carrier.choosePlayer("【改革】选择一名角色摸1张牌", false);
	if (!targetOpt) return false;
	Player& target = *targetOpt;

	//失去10点体力
	carrier.damage(10, std::nullopt);
	std::println("<技能> {}发动改革，失去10点体力", carrier.displayName());

	//令目标摸一张牌
	target.draw(1, DrawReason::skill);
	std::println("<技能> {}发动改革，令{}摸1张牌", carrier.displayName(), target.displayName());

	carrier.markCharInfoDirty();
	game.broadcastState();
	return true;
}


// ==================== 技能：改革_摸牌替代 ====================
bool 改革_摸牌替代::filter(const GameLogic& game, const Player& carrier, const Trigger& trigger) const {
	//仅在其他角色摸牌阶段外摸牌时触发（摸牌阶段与开局初始手牌除外）
	if (!trigger.hasDrawReason()) return false;
	if (trigger.getDrawReason() == DrawReason::phase_draw) return false;
	if (trigger.getDrawReason() == DrawReason::init) return false;
	if (!trigger.hasNumber() || trigger.getNumber() == 0) return false;
	return trigger.getPlayer().getId() != carrier.getId();
}

bool 改革_摸牌替代::content(GameLogic& game, Player& carrier, Trigger& trigger) {
	Player& target = trigger.getPlayer();
	const std::size_t drawCount = trigger.getNumber();
	//X为摸牌数，失去2X%最大体力（向上取整）
	const std::size_t loss = unool::math::ceil(static_cast<double>(drawCount) * target.getMaxHp() * 0.02);

	const std::size_t choice = carrier.ask(
		std::format("【改革】是否将{}的{}张摸牌改为失去{}点体力？", target.characterName(), drawCount, loss),
		{ "发动", "不发动" }, false);
	if (choice != 1) return false;

	//获取摸牌数并取消摸牌
	trigger.getNumber() = 0;
	target.cancelNextDraw();
	target.damage(loss, std::nullopt);

	std::println("<技能> {}发动改革，将{}的{}张摸牌改为失去{}点体力",
				 carrier.displayName(), target.displayName(), drawCount, loss);
	game.broadcastState();
	return true;
}


