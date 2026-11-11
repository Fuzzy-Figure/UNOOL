#include "Player.h"
#include "GameLogic.h"
#include <thread>

std::string Player::displayName() const {
	const std::string username = game.usernameOf(id);
	if (username.empty()) return std::format("玩家{}", id);
	if (characterName() == "白板") return username;
	return std::format("{}（{}）", username, characterName());
}

std::size_t Player::damage(std::size_t damageValue, opt_ref<Player> source) {
	//伤害倍率（支持小数，如 +0.1）
	if (source.has_value()) {
		const double multiplier = source.value().get().getDamageMultiplier();
		damageValue = static_cast<std::size_t>(static_cast<double>(damageValue) * multiplier);
	}
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.source = source;
		trigger.number = damageValue;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::damage_begin, trigger);
	}

	//护盾优先抵消伤害
	const std::size_t shieldBefore = shield;
	if (shield > 0 && damageValue > 0) {
		const std::size_t absorbed = (damageValue < shield) ? damageValue : shield;
		shield -= absorbed;
		damageValue -= absorbed;
		std::println("<护盾> {}的护盾抵消{}点伤害（剩余护盾{}）", displayName(), absorbed, shield);
	}

	const std::size_t hpBefore = getHp();
	const std::size_t actualDamageValue = character->damage(damageValue);
	const std::size_t hpLoss = hpBefore - getHp();

	//不死图腾：受到致命伤害时消耗一个，至少保留1点体力
	if (getHp() <= 0 && totemCount > 0) {
		totemCount -= 1;
		character->setHp(1);
		std::println("<道具> {}的不死图腾生效，保留1点体力（剩余{}个）", displayName(), totemCount);
		hint(std::format("不死图腾生效！保留1点体力（剩余{}个）", totemCount));
	}

	//吸血：对其他角色造成伤害后，按实际扣除体力值回复（护盾抵消部分不计）
	if (source.has_value() && source.value().get() != *this) {
		Player& attacker = source.value();
		if (attacker.getLifesteal() > 0 && hpLoss > 0) {
			const std::size_t healed = attacker.getLifesteal() * hpLoss / 100;
			if (healed > 0) {
				attacker.recover(healed);
				std::println("<吸血> {}吸血回复{}点体力（吸血率{}%）", attacker.displayName(), healed, attacker.getLifesteal());
			}
		}
	}

	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.source = source;
		trigger.number = damageValue;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::damage_end, trigger);
	}
	if (shield != shieldBefore) markCharInfoDirty();
	return actualDamageValue;
}

std::size_t Player::loseHp(std::size_t num) {
	const std::size_t hpBefore = getHp();
	const std::size_t actual = character->damage(num);
	std::println("<体力> {}失去{}点体力（{}→{}）", displayName(), actual, hpBefore, getHp());
	game.broadcastState();
	return actual;
}

bool Player::tryUseBombCard() {
	const std::string username = game.usernameOf(id);
	if (username.empty()) return false;
	if (services.users.getItemCount(username, "炸弹卡") <= 0) return false;
	if (bombUsed >= 2) {
		hint("炸弹卡本局已使用2次");
		return false;
	}

	auto targetOpt = chooseOtherPlayer("【炸弹卡】选择一名其他角色，对其造成5点伤害", false);
	if (!targetOpt) return false;

	services.users.useItem(username, "炸弹卡");
	++bombUsed;
	Player& target = targetOpt.value();
	target.damage(5, *this);
	std::println("<道具> {}对{}使用炸弹卡，造成5点伤害（本局第{}次）",
				 displayName(), target.displayName(), bombUsed);
	hint(std::format("炸弹卡生效，对{}造成5点伤害", target.displayName()));
	return true;
}

void Player::recover(std::size_t num) {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.number = num;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::recover_begin, trigger);
	}
	character->recover(num);
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.number = num;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::recover_end, trigger);
	}
}


// === 游戏逻辑 ===

std::vector<ref<Card>> Player::draw(std::size_t number, const DrawReason reason, const DrawPosition position) {
	std::println("{}摸了{}张牌（{}）", displayName(), number, (position == DrawPosition::top ? "顶" : "底")); {
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.number = number;
		trigger.drawReason = reason;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::draw_begin, trigger);
	}

	//摸牌已被技能取消（如【改革】改为失去体力）
	if (cancelDrawRequested) {
		cancelDrawRequested = false;
		std::println("<技能> {}的摸牌被取消", displayName());
		return {};
	}

	std::vector<ref<Card>> drawnCards;
	drawnCards.reserve(number);

	for (std::size_t i = 0; i < number; ++i) {
		std::unique_ptr<Card> cardPtr;
		if (position == DrawPosition::top) {
			cardPtr = game.getPile().take_front(game.getDiscardPile());
		}
		else {
			cardPtr = game.getPile().take_back(game.getDiscardPile());
		}
		hand->push_back(std::move(cardPtr));
		drawnCards.emplace_back(hand->back());
	}

	if (reason == DrawReason::phase_draw) handSelectLast();

	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = drawnCards;
		trigger.number = number;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::draw_end, trigger);
	}
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = drawnCards;
		trigger.number = number;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::gain_card_end, trigger);
	}
	return drawnCards;
}

std::vector<ref<Card>> Player::drawTo(const std::size_t num, const DrawReason reason) {
	if (const std::size_t _handCount = handCount(); _handCount < num)
		return draw(num - _handCount, reason);
	else return {};
}

//返回使用牌的引用
Card& Player::useCardByIndex(const std::size_t cardIndex) {
	std::unique_ptr<Card> card = hand->takeCardByIndex(cardIndex);
	hasUsed = true;
	std::println("{}打出了：{}", displayName(), *card);

	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { *card };
		game.launchPassiveSkills(PassiveSkill::TriggerTime::use_card_begin, trigger);
	}
	{
		PassiveSkill::Trigger trigger;
		trigger.player = next();
		trigger.cards = { *card };
		trigger.source = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::card_target_begin, trigger);
	}

	//发动卡牌效果
	card->applyEffect(game, *this, next());

	//更改当前颜色
	if (!card->isWild()) game.setCurrentColor(card->getColor());
	//更改当前牌名
	game.setCurrentName(card->getName());

	//牌恢复效果并置入弃牌堆
	card->recoverEffect();
	Card& cardRef = game.putCardToDiscardPile(std::move(card), Card::DiscardReason::use, *this);

	//饮料限定皮肤：打出万能牌时获得1积分（实时结算）
	if (cardRef.isWild()) {
		const auto& names = character->getNames();
		const auto& skins = character->getSkins();
		for (std::size_t i = 0; i < names.size() && i < skins.size(); ++i) {
			if (Character::getSkinQuality(names[i], skins[i]) == Character::SkinQuality::drink) {
				services.users.addPoints(game.usernameOf(id), 1);
				break;
			}
		}
	}

	//更新客户端显示
	game.broadcastState();

	//技能
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { cardRef };
		game.launchPassiveSkills(PassiveSkill::TriggerTime::lose_card_end, trigger);
	}
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { cardRef };
		game.launchPassiveSkills(PassiveSkill::TriggerTime::use_card_end, trigger);
	}

	return cardRef;
}

void Player::gainCard(std::unique_ptr<Card> card) {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::gain_card_begin, trigger);
	}
	hand->push_back(std::move(card));
	std::vector<ref<Card>> gainedCards;
	gainedCards.emplace_back(hand->back());
	static std::size_t one = 1;
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = gainedCards;
		trigger.number = one;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::gain_card_end, trigger);
	}
}

void Player::addMark(const std::string& m, std::size_t count) {
	character->addMark(m, count);
	markCharInfoDirty();
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.mark = m;
		trigger.number = count;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::add_mark, trigger);
	}
}

void Player::setMark(const std::string& m, std::size_t count) {
	character->setMark(m, count);
	markCharInfoDirty();
}

void Player::removeMark(const std::string& m, std::size_t count) {
	std::size_t actual = character->removeMark(m, count);
	markCharInfoDirty();
	if (actual > 0) {
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.mark = m;
		trigger.number = actual;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::remove_mark, trigger);
	}
}

Card& Player::putCardToDiscardPileByIndex(const std::size_t cardIndex, Card::DiscardReason reason) {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::lose_card_begin, trigger);
	}
	std::unique_ptr<Card> card = hand->takeCardByIndex(cardIndex);
	ref<Card> cardRef = *card;
	game.putCardToDiscardPile(std::move(card), reason, *this);
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { cardRef };
		game.launchPassiveSkills(PassiveSkill::TriggerTime::lose_card_end, trigger);
	}
	return cardRef;
}

Card& Player::discardByIndex(const std::size_t cardIndex) {
	return putCardToDiscardPileByIndex(cardIndex, Card::DiscardReason::discard);
}
Card& Player::recastByIndex(const std::size_t cardIndex) {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::recast_begin, trigger);
	}
	Card& card = putCardToDiscardPileByIndex(cardIndex, Card::DiscardReason::recast);
	draw(1);
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::recast_end, trigger);
	}
	return card;
}

std::unique_ptr<Card> Player::takeCardByIndex(const std::size_t cardIndex) {
	return hand->takeCardByIndex(cardIndex);
}


bool Player::canUse(const Card& card) {
	//出牌不受限制：任意牌均可打出
	if (unrestrictedPlay) return true;
	if (game.getCurrentColor() == Card::Color::no) return true;
	if (card.is(game.getCurrentColor())
		|| card.is(game.getCurrentName())
		|| card.isWild()) {
		return true;
	}
	return false;
}



// === 技能 / 状态 ===

void Player::ban(Player& source, Card& card) {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { card };
		trigger.source = source;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::ban_begin, trigger);
	}
	banned = true;
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { card };
		trigger.source = source;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::ban_end, trigger);
	}
}

void Player::seal(std::size_t duration) {
	sealed = duration;
	std::println("{}被封印{}轮", displayName(), duration);
	markCharInfoDirty();
}


// === 回合流程 ===

void Player::phaseBegin() {
	//重置所有主动技的阶段内使用次数（每回合开始）
	for (auto& s : getInstantSkills()) s->resetPhaseCount();
	for (auto& s : getTransformSkills()) s->resetPhaseCount();
	//重置被动技的阶段内使用次数（基类仅递归子技能；镇魂等自计数的被动技借此每回合清零）
	for (auto& s : getPassiveSkills()) s->resetPhaseCount();
	//出牌无限制状态仅在当前回合的第二个出牌阶段生效，每回合开始时重置
	unrestrictedPlay = false;
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_begin, trigger);
	}
}

//返回是否出牌
bool Player::phaseUse1() {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_use1_begin, trigger);
	}
	auto card = chooseToUse(ActiveSkill::TriggerTime::phase_use1);
	if (card.has_value()) {
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { card.value().get() };
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_use1_end, trigger);
	}
	else {
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_use1_end, trigger);
	}
	return card.has_value();
}

void Player::phaseDraw() {
	std::size_t drawCount = 1;
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.number = drawCount;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_draw_begin, trigger);
	}
	std::vector<ref<Card>> drawnCards = draw(drawCount, DrawReason::phase_draw);
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = drawnCards;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_draw_end, trigger);
	}
}

void Player::phaseUse2() {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_use2_begin, trigger);
	}
	chooseToUse(ActiveSkill::TriggerTime::phase_use2);
}

void Player::phaseEnd() {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::phase_end, trigger);
	}
	//回合结束，封印剩余轮数-1
	if (sealed > 0) {
		--sealed;
		markCharInfoDirty();
	}
}

bool Player::turn() {
	hasUsed = false;
	noWinByEmptyHand = false;
	phaseBegin();
	game.broadcastState();
	bool used = false;

	if (banned) {
		std::println("{}跳过了他的回合", displayName());
		unban();
		goto PhaseEnd;
	}

	if (game.isGameOver()) return handEmpty();
	if (handEmpty() && !noWinByEmptyHand) return true;

	used = phaseUse1();
	if (game.isGameOver()) return handEmpty();
	if (handEmpty() && !noWinByEmptyHand) return true;

	if (!used) {
		phaseDraw();
		if (game.isGameOver()) return handEmpty();
		if (handEmpty() && !noWinByEmptyHand) return true;
		game.broadcastState();
		phaseUse2();
		if (game.isGameOver()) return handEmpty();
		if (handEmpty() && !noWinByEmptyHand) return true;
	}

PhaseEnd:
	phaseEnd();
	game.broadcastState();
	return handEmpty() && !noWinByEmptyHand;
}

// === 导航 ===
Player& Player::next() const { return game.getPlayerById(game.nextPlayerIndex(id)); }
Player& Player::prev() const { return game.getPlayerById(game.prevPlayerIndex(id)); }

// === 初始化 ===
std::string Player::chooseSkin(const std::string& charName) {
	//只允许选择已拥有的皮肤
	const auto owned = services.users.getOwnedSkins(game.usernameOf(id), charName);
	auto skins = Character::getSkins(charName);
	std::string skin = "默认";
	//按 Character::getSkins 顺序过滤出已拥有的皮肤
	std::vector<std::string> skinOpts;
	for (const auto& s : skins) {
		if (owned.contains(s)) skinOpts.push_back(s);
	}
	if (skinOpts.size() > 1) {
		std::size_t skinChoice = ask("选择皮肤：", skinOpts, true);
		skin = skinOpts[skinChoice - 1];
	}
	else if (skinOpts.size() == 1) {
		skin = skinOpts[0];
	}
	return skin;
}

void Player::chooseSkinAndSet(const std::string& charName) {
	const std::string skin = chooseSkin(charName);
	setCharacter(Character::make(charName, skin));
}

// === 交互 ===

std::optional<std::size_t> Player::chooseCard(const std::string& title, std::function<bool(const Card&)> condition,
											  bool forced, ActiveSkill::TriggerTime phase) {
	game.setOperatingPlayer(id);
	decision.showChoice(id, { title, {}, forced });
	opt_ref<TransformSkill> activeMode;  //当前激活的转换型主动技

	std::vector<ref<InstantSkill>>   instantRefs;
	std::vector<ref<TransformSkill>> transformRefs;
	collectAvailableSkills(phase, instantRefs, transformRefs);

	while (true) {
		decision.poll();
		if (!decision.isConnected(id)) {
			// 玩家掉线，等待重连
			const auto timeoutSec = decision.reconnectTimeoutSec();
			const auto clockStart = services.clock.now();
			bool reconnected = false;
			std::println("[Player] {} 掉线，等待重连（最多 {} 秒）", displayName(), timeoutSec);
			while (std::chrono::duration<double>(services.clock.now() - clockStart).count() < timeoutSec) {
				decision.poll();
				if (decision.isLoggedIn(id)) {
					reconnected = true;
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			if (!reconnected) {
				std::println("[Player] {} 掉线超时，结束选择", displayName());
				return std::nullopt;
			}
			std::println("[Player] {} 重连成功，恢复选择", displayName());
			game.broadcastState();
			decision.showChoice(id, { title, {}, forced });
			continue;
		}

		auto inputOpt = decision.takeInput();
		if (!inputOpt.has_value()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			continue;
		}

		const auto clientInput = inputOpt.value();
		if (clientInput.player != id) continue;

		unool::contract::Key input = clientInput.key;
		setInput(input);
		hand->setSelectedIndex(clientInput.selectedIndex);  //统一同步选中索引

		//数字1-9：即时技发动 / 转换技切换（仅出牌阶段）
		if (phase != ActiveSkill::TriggerTime::never
			&& handleDigitKey(input, instantRefs, transformRefs, activeMode))
			continue;

		//B：使用炸弹卡（对一名其他角色造成5点伤害，每局限两次）
		if (phase != ActiveSkill::TriggerTime::never
			&& input == unool::contract::Key::B) {
			if (tryUseBombCard()) {
				game.setOperatingPlayer(id);
				decision.showChoice(id, { title, {}, forced });
				game.broadcastState();
			}
			continue;
		}

		switch (input) {
			case unool::contract::Key::Space:
				sortHand();
				game.broadcastState();
				break;
			case unool::contract::Key::Up:
			case unool::contract::Key::W:
				if (auto result = handleConfirm(condition, activeMode); result.has_value()) {
					decision.clearChoice(id);
					return result.value();
				}
				break;
			case unool::contract::Key::Down:
			case unool::contract::Key::S:
				if (!forced) {
					decision.clearChoice(id);
					return std::nullopt;
				}
				break;
			default:
				break;
		}
	}
}

void Player::collectAvailableSkills(ActiveSkill::TriggerTime phase,
									std::vector<ref<InstantSkill>>& instantRefs,
									std::vector<ref<TransformSkill>>& transformRefs) {
	if (phase == ActiveSkill::TriggerTime::never) return;
	//封印状态下所有主动技能失效
	if (isSealed()) return;
	for (auto& s : getInstantSkills()) {
		if (s->canTriggerAt(phase) && s->canUse()) instantRefs.emplace_back(*s);
	}
	for (auto& s : getTransformSkills()) {
		if (s->canTriggerAt(phase) && s->canUse()) transformRefs.emplace_back(*s);
	}
}

bool Player::handleDigitKey(unool::contract::Key input,
							const std::vector<ref<InstantSkill>>& instantRefs,
							const std::vector<ref<TransformSkill>>& transformRefs,
							opt_ref<TransformSkill>& activeMode) {
	auto digit = digitFromScancode(input);
	if (!digit.has_value() || digit.value() == 0) return false;

	std::size_t idx = digit.value() - 1;  //0-based
	//前 instantRefs.size() 个键：触发即时技
	if (idx < instantRefs.size()) {
		InstantSkill& skill = instantRefs[idx].get();
		const std::size_t confirm = ask(
			std::format("是否发动【{}】？", skill.getName()),
			{ "是", "否" }, false);
		if (confirm == 1) {
			skill.tryActivate(game, *this);
		}
		game.setOperatingPlayer(id);
		game.broadcastState();
	}
	//后续键：切换转换技激活态
	else if (idx - instantRefs.size() < transformRefs.size()) {
		std::size_t tIdx = idx - instantRefs.size();
		TransformSkill& skill = transformRefs[tIdx].get();
		if (activeMode.has_value() && &activeMode.value().get() == &skill) {
			activeMode.reset();
			decision.clearChoice(id);
		}
		else {
			activeMode = skill;
			decision.showChoice(id, { skill.getPrompt(), {}, false });
		}
	}
	return true;
}

std::optional<std::size_t> Player::handleConfirm(const std::function<bool(const Card&)>& condition,
												 const opt_ref<TransformSkill>& activeMode) {
	if (handEmpty()) return std::nullopt;

	if (activeMode.has_value()) {
		//转换技：尝试转化选中的牌并打出（暂仅支持单牌转换）
		TransformSkill& mode = activeMode.value();
		Card& selected = (*hand)[hand->getSelectedIndex()];
		if (mode.canSelect(selected) && mode.getCardCount() == 1) {
			Card original = selected;  //备份原牌
			std::vector<ref<Card>> cards;
			cards.emplace_back(selected);
			if (mode.transform(game, *this, std::move(cards))) {
				if (canUse(selected)) {
					//转化成功打出：执行附加效果，累加使用次数
					mode.addition(game, *this);
					mode.incrementCount();
					decision.clearChoice(id);
					game.clearOperatingPlayer();
					return hand->getSelectedIndex();
				}
				else {
					selected = original;  //还原
					std::println("<{}> 转化后的牌不符合出牌规则", mode.getName());
				}
			}
			else {
				//玩家在transform交互中取消
				selected = original;
				std::println("<{}> 玩家取消转化", mode.getName());
			}
		}
		else {
			std::println("<{}> 选中的牌不能转化", mode.getName());
		}
	}
	else if (condition(hand->getSelectedCard())) {
		decision.clearChoice(id);
		game.clearOperatingPlayer();
		return hand->getSelectedIndex();
	}
	return std::nullopt;
}

opt_ref<Card> Player::chooseToUse(ActiveSkill::TriggerTime phase) {
	auto index = chooseCard("请选择要打出的牌", [this](const Card& c) { return canUse(c); }, false, phase);
	if (index.has_value()) {
		return useCardByIndex(index.value());
	}
	else {
		std::println("{}选择不跟牌", displayName());
		game.broadcastState();
		return std::nullopt;
	}
}

std::vector<ref<Card>> Player::chooseCardsToDiscardPile(const std::string& title,
														std::size_t num, const bool forced,
														const std::function<bool(const Card&)>& condition,
														Card::DiscardReason reason) {
	std::vector<ref<Card>> discardedCards;
	if (const std::size_t _handCount = handCount(); num > _handCount)
		num = _handCount;

	std::println("{}请选择{}{}张牌", displayName(), reason, num);

	std::size_t discardedCount = 0;
	while (discardedCount < num) {
		std::string fullTitle = std::format(
			"{}（{}/{}）\n{}",
			title, discardedCount + 1, num,
			forced ? "（↑确认，不可取消）" : "（↑确认，↓取消）"
		);
		auto index = chooseCard(fullTitle, condition, forced);
		if (!index.has_value()) {
			std::println("{}取消了{}", displayName(), reason);
			return discardedCards;
		}
		discardedCards.push_back(hand->getCardByIndex(index.value()));
		putCardToDiscardPileByIndex(index.value(), reason);
		discardedCount++;
		std::println("{}{}了一张牌（{}/{}）", displayName(), reason, discardedCount, num);
		game.broadcastState();
	}
	return discardedCards;
}

std::vector<ref<Card>> Player::chooseToDiscard(const std::string& title,
											   std::size_t num, const bool forced,
											   const std::function<bool(const Card&)>& condition) {
	return chooseCardsToDiscardPile(title, num, forced, condition, Card::DiscardReason::discard);
}

Player::RecastResult Player::chooseToRecast(const std::string& title,
											const std::size_t num, const bool forced,
											const std::function<bool(const Card&)>& condition) {

												{
													PassiveSkill::Trigger trigger;
													trigger.player = *this;
													game.launchPassiveSkills(PassiveSkill::TriggerTime::recast_begin, trigger);
												}
												std::vector discarded = chooseCardsToDiscardPile(title, num, forced, condition, Card::DiscardReason::recast);
												std::vector drawn = draw(discarded.size());
												{
													PassiveSkill::Trigger trigger;
													trigger.player = *this;
													game.launchPassiveSkills(PassiveSkill::TriggerTime::recast_end, trigger);
												}
												return RecastResult{ std::move(discarded), std::move(drawn) };
}

void Player::decree(const std::string& title,
					const std::size_t num, const bool forced,
					const std::function<bool(const Card&)>& condition) {
						{
							PassiveSkill::Trigger trigger;
							trigger.player = *this;
							game.launchPassiveSkills(PassiveSkill::TriggerTime::decree_begin, trigger);
						}
						draw(num);
						chooseCardsToDiscardPile(title, num, forced, condition, Card::DiscardReason::decree);
						{
							PassiveSkill::Trigger trigger;
							trigger.player = *this;
							game.launchPassiveSkills(PassiveSkill::TriggerTime::decree_end, trigger);
						}
}

void Player::inherit(std::unique_ptr<Card>& card) {

}

opt_ref<Card> Player::chooseToOperate(const std::string& title, bool forced,
									  const std::function<bool(const Card&)>& condition,
									  const std::function<void(Card&)>& operation) {
	std::string fullTitle = std::format("{}\n{}", title, forced ? "（↑确认，不可取消）" : "（↑确认，↓取消）");
	std::optional<std::size_t> index = chooseCard(fullTitle, condition, forced);
	if (!index.has_value()) return std::nullopt;
	ref<Card> cardRef = getHand().getCardByIndex(index.value());
	operation(getHand().getCardByIndex(index.value()));
	return cardRef;
}

opt_ref<Card> Player::chooseToGive(const std::string& title, Player& target,
								   bool forced, const std::function<bool(const Card&)>& condition) {
	if (handEmpty()) return std::nullopt;

	auto index = chooseCard(title, condition, forced);

	if (!index.has_value()) {
		std::println("{}取消了给{}牌", displayName(), target.displayName());
		return std::nullopt;
	}

	ref<Card> card = hand->getCardByIndex(index.value());

	give(target, takeCardByIndex(index.value()));
	std::println("{}给了{}一张{}", displayName(), target.displayName(), card.get());
	game.broadcastState();

	return card;
}

opt_ref<Card> Player::chooseToShow(const std::string& title, bool forced, const std::function<bool(const Card&)>& condition) {
	return chooseToOperate(title, forced, condition, [this](const Card& c) {
		showCard(c);
	});
}

opt_ref<Player> Player::choosePlayer(const std::string& title, bool forced,
									 const std::function<bool(const Player&)>& condition) {
	//选角色
	const auto& candidates = game.getPlayersIf(condition);

	if (candidates.empty()) {
		game.broadcastState();
		return std::nullopt;
	}

	std::vector<std::string> options;
	for (auto& p : candidates) {
		options.push_back(p.get().characterName());
	}
	std::size_t choice = ask(title, options, forced);

	if (choice == 0) return std::nullopt;
	return candidates[choice - 1];
}
opt_ref<Player> Player::chooseOtherPlayer(const std::string& title, bool forced,
										  const std::function<bool(const Player&)>& condition) {
	return choosePlayer(title, forced, [this, &condition](const Player& p) {
		return p != *this && condition(p);
	});
}

std::optional<Card::Color> Player::chooseCardColor(const std::string& title, bool forced, const std::vector<Card::Color>& colors) {
	if (colors.empty()) return std::nullopt;
	std::vector<std::string> options;
	for (const auto& c : colors) {
		options.push_back(std::format("{}", c));
	}
	std::size_t choice = ask(title, options, forced);
	if (choice == 0) return std::nullopt;
	return colors[choice - 1];
}

std::optional<Card::Name> Player::chooseCardName(const std::string& title, bool forced, const std::vector<Card::Name>& names) {
	if (names.empty()) return std::nullopt;
	std::vector<std::string> options;
	for (const auto& n : names) {
		options.push_back(std::format("{}", n));
	}
	std::size_t choice = ask(title, options, forced);
	if (choice == 0) return std::nullopt;
	return names[choice - 1];
}


std::size_t Player::ask(const std::string& title, const std::vector<std::string>& options,
						bool forced, std::optional<std::chrono::milliseconds> timeoutMs) {
	std::string errorMsg;

	//RAII: 进入/退出操作锁
	struct OperatingGuard {
		GameLogic& g;
		bool active = true;
		OperatingGuard(GameLogic& g_, std::size_t id) : g(g_) { g.setOperatingPlayer(id); }
		~OperatingGuard() { if (active) g.clearOperatingPlayer(); }
	} guard(game, id);

	constexpr std::size_t PER_PAGE = 9;
	const bool usePaging = options.size() > PER_PAGE;
	const std::size_t totalPages = usePaging ? (options.size() + PER_PAGE - 1) / PER_PAGE : 1;
	std::size_t currentPage = 0;

	auto toTimeoutMs = [&]() -> std::optional<std::size_t> {
		if (timeoutMs.has_value()) {
			return static_cast<std::size_t>(timeoutMs.value().count());
		}
		return std::nullopt;
	};

	auto sendPage = [&]() {
		if (!usePaging) {
			decision.showChoice(id, { title, options, forced, errorMsg, toTimeoutMs(), 0, 1 });
			return;
		}
		std::vector<std::string> pageOptions;
		const std::size_t start = currentPage * PER_PAGE;
		const std::size_t end = std::min(start + PER_PAGE, options.size());
		for (std::size_t i = start; i < end; ++i) {
			pageOptions.push_back(options[i]);
		}
		decision.showChoice(id, { title, pageOptions, forced, errorMsg, toTimeoutMs(), currentPage, totalPages });
	};

	sendPage();

	const auto clockStart = services.clock.now();
	while (true) {
		//1. 超时检测
		if (timeoutMs.has_value()
			&& std::chrono::duration_cast<std::chrono::milliseconds>(
				services.clock.now() - clockStart) >= timeoutMs.value()) {
			decision.clearChoice(id);
			std::println("{}超时未选择", displayName());
			return 0;
		}

		//2. 网络更新 + 掉线重连检测
		decision.poll();
		if (!decision.isConnected(id)) {
			std::println("[Player] {} 掉线，等待重连...", displayName());
			if (!waitForReconnect()) {
				std::println("[Player] {} 掉线超时，ask 返回默认值", displayName());
				return 0;
			}
			std::println("[Player] {} 重连成功，恢复选择", displayName());
			game.broadcastState();
			sendPage();
			continue;
		}

		//3. 收包
		auto inputOpt = decision.takeInput();
		if (!inputOpt.has_value()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
			continue;
		}
		const auto clientInput = inputOpt.value();
		if (clientInput.player != id) {
			continue;
		}
		unool::contract::Key input = clientInput.key;
		setInput(input);

		//4. 翻页键
		if (handlePagingKey(input, usePaging, currentPage, totalPages, errorMsg)) {
			sendPage();
			continue;
		}

		//5. 数字键解析
		auto choice = resolveChoice(input, options, forced, usePaging, currentPage, errorMsg);
		if (!choice.has_value()) {
			sendPage();
			continue;
		}

		//6. 返回
		decision.clearChoice(id);
		std::print("[ask] 标题：“{}”，{}选择了{}", title, displayName(), *choice);
		if (*choice != 0) {
			std::println(": {}", options[*choice - 1]);
		}
		else std::println();
		return *choice;
	}
}

bool Player::waitForReconnect() {
	const auto timeoutSec = decision.reconnectTimeoutSec();
	const auto clockStart = services.clock.now();
	std::println("[Player] {} 掉线，等待重连（最多 {} 秒）", displayName(), timeoutSec);
	while (std::chrono::duration<double>(services.clock.now() - clockStart).count() < timeoutSec) {
		decision.poll();
		if (decision.isLoggedIn(id)) {
			return true;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	return false;
}

bool Player::handlePagingKey(unool::contract::Key input, bool usePaging,
							 std::size_t& currentPage, std::size_t totalPages,
							 std::string& errorMsg) {
	if (!usePaging) return false;
	if (!(input == unool::contract::Key::Left || input == unool::contract::Key::Right
		  || input == unool::contract::Key::A || input == unool::contract::Key::D)) {
		return false;
	}
	if (input == unool::contract::Key::Left || input == unool::contract::Key::A) {
		if (currentPage > 0) --currentPage;
	}
	else {
		if (currentPage + 1 < totalPages) ++currentPage;
	}
	errorMsg.clear();
	return true;
}

std::optional<std::size_t> Player::resolveChoice(unool::contract::Key input,
												 const std::vector<std::string>& options,
												 bool forced, bool usePaging,
												 std::size_t currentPage,
												 std::string& errorMsg) {
	constexpr std::size_t PER_PAGE = 9;
	//生成"超出范围"错误提示（分页/非分页复用，消除重复）
	auto rangeError = [&]() -> std::string {
		const std::string minOpt = forced ? "1" : "0";
		if (usePaging) {
			return std::format("超出范围，请输入{}-{}范围内的数字（<-->翻页）",
							   minOpt, std::min(PER_PAGE, options.size() - currentPage * PER_PAGE));
		}
		return std::format("超出范围，请输入{}-{}范围内的数字", minOpt, options.size());
	};

	auto digit = digitFromScancode(input);
	if (!digit.has_value()) {
		errorMsg = usePaging ? "无效输入，请输入数字0-9或使用<-->翻页"
			: "无效输入，请输入数字0-9";
		return std::nullopt;
	}
	std::size_t choice = *digit;

	//分页下换算真实索引（0 表示取消，不换算）
	if (usePaging && choice != 0) {
		const std::size_t realIndex = currentPage * PER_PAGE + (choice - 1);
		if (realIndex >= options.size()) {
			errorMsg = rangeError();
			return std::nullopt;
		}
		choice = realIndex + 1;
	}

	if (choice > options.size()) {
		errorMsg = rangeError();
		return std::nullopt;
	}
	if (forced && choice == 0) {
		errorMsg = "必须选择一个选项，请重新输入";
		return std::nullopt;
	}

	return choice;
}

std::optional<std::size_t> Player::digitFromScancode(unool::contract::Key input) {
	switch (input) {
		case unool::contract::Key::Num0: case unool::contract::Key::Numpad0: return 0;
		case unool::contract::Key::Num1: case unool::contract::Key::Numpad1: return 1;
		case unool::contract::Key::Num2: case unool::contract::Key::Numpad2: return 2;
		case unool::contract::Key::Num3: case unool::contract::Key::Numpad3: return 3;
		case unool::contract::Key::Num4: case unool::contract::Key::Numpad4: return 4;
		case unool::contract::Key::Num5: case unool::contract::Key::Numpad5: return 5;
		case unool::contract::Key::Num6: case unool::contract::Key::Numpad6: return 6;
		case unool::contract::Key::Num7: case unool::contract::Key::Numpad7: return 7;
		case unool::contract::Key::Num8: case unool::contract::Key::Numpad8: return 8;
		case unool::contract::Key::Num9: case unool::contract::Key::Numpad9: return 9;
		default: return std::nullopt;
	}
}

void Player::hint(const std::string& message) {
	(void)ask(message, { "确认" }, true);
}

Card& Player::judge() {
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		game.launchPassiveSkills(PassiveSkill::TriggerTime::judge_begin, trigger);
	}
	auto card = game.getPile().take_front(game.getDiscardPile());
	std::println("判定结果：{}", *card);
	Card& cardRef = game.putCardToDiscardPile(std::move(card), Card::DiscardReason::judge, *this);
	{
		PassiveSkill::Trigger trigger;
		trigger.player = *this;
		trigger.cards = { cardRef };
		game.launchPassiveSkills(PassiveSkill::TriggerTime::judge_end, trigger);
	}
	game.broadcastState();
	return cardRef;
}

void Player::showCard(const Card& card) {
	game.forEachOtherPlayer(*this, [this, &card](Player& p) {
		p.hint(std::format("{}展示了{}", characterName(), card.toString()));
	});
}

std::optional<Player::CompareResult> Player::comparePoint(Player& target, bool forced) {
	//防御性检查：发起者无数字牌则不能发动
	if (!handInclude(&Card::isNumber)) return std::nullopt;

	//目标无数字牌，直接判其输
	if (!target.handInclude(&Card::isNumber)) {
		std::println("<拼点> {}与{}拼点，{}无数字牌，直接判负", displayName(), target.displayName(), target.displayName());
		game.broadcastState();
		return CompareResult::win;
	}

	//发起者选一张数字牌
	auto myIdx = chooseCard("【拼点】选择一张手牌", &Card::isNumber, forced);
	if (!myIdx.has_value()) return std::nullopt;  //发起者取消
	Card& myCard = getHand().getCardByIndex(myIdx.value());

	//目标选一张数字牌（强制）
	auto tgtIdx = target.chooseCard("【拼点】选择一张手牌", &Card::isNumber, true);
	//目标有数字牌且forced=true，必有返回
	Card& tgtCard = target.getHand().getCardByIndex(tgtIdx.value());

	//展示双方结果
	showCard(myCard);
	target.showCard(tgtCard);

	const std::size_t myVal = myCard.value();
	const std::size_t tgtVal = tgtCard.value();
	std::print("<拼点> {}({}) vs {}({})，", displayName(), myVal, target.displayName(), tgtVal);
	CompareResult result;
	if (myVal > tgtVal) {
		result = CompareResult::win;
		std::println("{}赢", displayName());
	}
	else if (myVal < tgtVal) {
		result = CompareResult::lose;
		std::println("{}输", displayName());
	}
	else {
		result = CompareResult::draw;
		std::println("双方平局");
	}
	game.broadcastState();
	return result;
}
