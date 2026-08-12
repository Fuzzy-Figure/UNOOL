#include "../header/Player.h"
#include "../header/GameLogic.h"
#include <Windows.h>

void Player::takeDamage(std::size_t damage, opt_ref<Player> source) {
	game.launchPSkills(PSkill::TriggerTime::lose_hp_begin, *this, std::nullopt, source, damage);
	character->takeDamage(damage);
	game.launchPSkills(PSkill::TriggerTime::lose_hp_end, *this, std::nullopt, source, damage);
}


// === 游戏逻辑 ===

void Player::draw(std::size_t number) {
	std::cout << "玩家" << id << "(" << characterName() << ")摸了" << number << "张牌" << std::endl;
	game.launchPSkills(PSkill::TriggerTime::draw_begin, *this, std::nullopt, std::nullopt, number);
	for (std::size_t i = 0; i < number; ++i) {
		hand->push_back(game.getPile().take_front(game.getDiscardPile()));
	}
	if (number == 1 && !hand->empty()) {
		Card& lastCard = hand->getCardByIndex(hand->count() - 1);
		game.launchPSkills(PSkill::TriggerTime::draw_end, *this, lastCard, std::nullopt, number);
	} else {
		game.launchPSkills(PSkill::TriggerTime::draw_end, *this, std::nullopt, std::nullopt, number);
	}
}

void Player::drawTo(const std::size_t num) {
	if (const std::size_t _handCount = handCount(); _handCount < num) draw(num - _handCount);
}

//返回使用牌的引用
Card& Player::useCardByIndex(const std::size_t cardIndex) {
	std::unique_ptr<Card> card = hand->takeCardByIndex(cardIndex);
	std::cout << "玩家" << id << "打出了：" << card->nameString() << std::endl;

	game.launchPSkills(PSkill::TriggerTime::use_card_begin, *this, *card);
	game.launchPSkills(PSkill::TriggerTime::card_target_begin, next(), *card, *this);

	//发动卡牌效果
	card->applyEffect(game, *this, next());

	//更改当前颜色
	if (!card->isWild()) game.setCurrentColor(card->getColor());
	//更改当前牌名
	game.setCurrentName(card->getName());

	//牌恢复效果并置入弃牌堆
	card->recoverEffect();
	Card& cardRef = game.putCardToDiscardPile(std::move(card));

	//重置手牌指针
	hand->resetSelectedIndex();

	//更新客户端显示
	game.broadcastState();

	//技能
	game.launchPSkills(PSkill::TriggerTime::lose_card_end, *this, cardRef, std::nullopt);
	game.launchPSkills(PSkill::TriggerTime::use_card_end, *this, cardRef, std::nullopt);

	return cardRef;
}

void Player::discardByIndex(const std::size_t cardIndex) {
	game.putCardToDiscardPile(hand->takeCardByIndex(cardIndex));
}

std::unique_ptr<Card> Player::takeCardByIndex(const std::size_t cardIndex) {
	return hand->takeCardByIndex(cardIndex);
}


bool Player::canUse(const Card& card) {
	if (game.getCurrentColor() == Card::Color::no) return true;
	if (card.getColor() == game.getCurrentColor()
		|| card.getName() == game.getCurrentName()
		|| card.getName() == Card::Name::wild_pal
		|| card.getName() == Card::Name::wild_draw4) {
		return true;
	}
	return false;
}



// === 技能 / 状态 ===

void Player::ban(Player& source, Card& card) {
	game.launchPSkills(PSkill::TriggerTime::ban_begin, *this, card, source);
	banned = true;
	game.launchPSkills(PSkill::TriggerTime::ban_end, *this, card, source);
}


// === 回合流程 ===

void Player::phaseBegin() {
	game.launchPSkills(PSkill::TriggerTime::phase_begin, *this, std::nullopt, std::nullopt);
}

//返回是否出牌
bool Player::phaseUse1() {
	game.launchPSkills(PSkill::TriggerTime::phase_use1_begin, *this, std::nullopt, std::nullopt);
	auto card = chooseToUse();
	if (card.has_value())
		game.launchPSkills(PSkill::TriggerTime::phase_use1_end, *this, card.value().get(), std::nullopt);
	else
		game.launchPSkills(PSkill::TriggerTime::phase_use1_end, *this, std::nullopt, std::nullopt);
	return card.has_value();
}

void Player::phaseDraw() {
	game.launchPSkills(PSkill::TriggerTime::phase_draw_begin, *this, std::nullopt, std::nullopt);
	draw(hasPSkill("巨富") ? 2 : 1);
	game.launchPSkills(PSkill::TriggerTime::phase_draw_end, *this, std::nullopt, std::nullopt);
}

void Player::phaseUse2() {
	game.launchPSkills(PSkill::TriggerTime::phase_use2_begin, *this, std::nullopt, std::nullopt);
	chooseToUse();
}

void Player::phaseEnd() {
	game.launchPSkills(PSkill::TriggerTime::phase_end, *this, std::nullopt, std::nullopt);
}

bool Player::turn() {
	if (banned) {
		std::cout << "玩家" << id << "跳过了他的回合" << std::endl;
		unban();
		return false;
	}

	phaseBegin();
	bool used = phaseUse1();

	if (!used) {
		phaseDraw();
		game.broadcastState();
		phaseUse2();
	}

	phaseEnd();
	return handEmpty();
}

// === 导航 ===
Player& Player::next() const { return game.getPlayerById(game.nextPlayerIndex(id)); }
Player& Player::prev() const { return game.getPlayerById(game.prevPlayerIndex(id)); }

// === 交互 ===

std::optional<std::size_t> Player::chooseCard(std::function<bool(const Card&)> condition,
											  bool forced) {
	ServerNetwork& network = game.getNetwork();
	while (true) {
		network.update();
		auto inputOpt = network.receiveClientInput();
		if (!inputOpt.has_value()) {
			Sleep(16);
			continue;
		}

		ClientInput clientInput = inputOpt.value();
		if (clientInput.playerId != id) continue;

		sf::Keyboard::Scancode input = clientInput.key;
		setInput(input);

		switch (input) {
		case sf::Keyboard::Scancode::Left:
			handSelectLeft();
			game.broadcastState();
			break;
		case sf::Keyboard::Scancode::Right:
			handSelectRight();
			game.broadcastState();
			break;
		case sf::Keyboard::Scancode::Space:
			sortHand();
			game.broadcastState();
			break;
		case sf::Keyboard::Scancode::Up:
			if (!handEmpty() && condition(hand->getSelectedCard())) {
				return hand->getSelectedIndex();
			}
			break;
		case sf::Keyboard::Scancode::Down:
			if (!forced) {
				return std::nullopt;
			}
			break;
		default:
			break;
		}
	}
}

opt_ref<Card> Player::chooseToUse() {
	auto index = chooseCard([this](const Card& c) { return canUse(c); }, false);
	if (index.has_value()) {
		return useCardByIndex(index.value());
	}
	else {
		std::cout << "玩家" << id << "选择不跟牌" << std::endl;
		game.broadcastState();
		return std::nullopt;
	}
}

std::vector<ref<Card>> Player::chooseToDiscard(std::size_t num, bool forced,
											   std::function<bool(const Card&)> condition) {
	std::vector<ref<Card>> discardedCards;
	if (num > handCount()) return discardedCards;

	std::cout << "玩家" << id << "请选择弃置" << num << "张牌" << std::endl;

	std::size_t discardedCount = 0;
	while (discardedCount < num) {
		auto index = chooseCard(condition, forced);
		if (!index.has_value()) {
			std::cout << "玩家" << id << "取消了弃牌" << std::endl;
			return discardedCards;
		}
		discardedCards.push_back(hand->getCardByIndex(index.value()));
		discardByIndex(index.value());
		discardedCount++;
		std::cout << "玩家" << id << "弃置了一张牌（" << discardedCount << "/" << num << "）" << std::endl;
		game.broadcastState();
	}
	return discardedCards;
}

std::optional<std::size_t> Player::chooseToChange(const Card& targetCard) {
	auto index = chooseCard([](const Card&) { return true; }, false);
	if (!index.has_value()) return std::nullopt;
	getHand().getCardByIndex(index.value()).set(targetCard);
	std::cout << "玩家" << id << "将手牌第" << index.value() << "张改为 [" << targetCard.nameString() << "]" << std::endl;
	return index;
}

std::size_t Player::ask(const std::wstring& title, const std::vector<std::wstring>& options, bool forced) {
	return ask(title, options, forced, std::nullopt);
}

std::size_t Player::ask(const std::wstring& title, const std::vector<std::wstring>& options, bool forced, std::optional<std::size_t> timeoutMs) {
	ServerNetwork& network = game.getNetwork();
	std::wstring errorMsg;

	network.sendPlayerChoice(id, title, options, forced, errorMsg, timeoutMs);

	sf::Clock clock;
	while (true) {
		if (timeoutMs.has_value()) {
			if (clock.getElapsedTime().asMilliseconds() >= static_cast<int>(timeoutMs.value())) {
				network.sendPlayerChoice(id, L"", {}, false, L"", std::nullopt);
				std::cout << "玩家" << id << "超时未选择" << std::endl;
				return 0;
			}
		}

		network.update();
		auto inputOpt = network.receiveClientInput();
		if (!inputOpt.has_value()) {
			Sleep(16);
			continue;
		}

		ClientInput clientInput = inputOpt.value();
		if (clientInput.playerId != id) {
			continue;
		}

		sf::Keyboard::Scancode input = clientInput.key;
		setInput(input);

		std::size_t choice = 0;
		bool isValidDigit = false;

		switch (input) {
		case sf::Keyboard::Scancode::Num0:
		case sf::Keyboard::Scancode::Numpad0:
			choice = 0;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num1:
		case sf::Keyboard::Scancode::Numpad1:
			choice = 1;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num2:
		case sf::Keyboard::Scancode::Numpad2:
			choice = 2;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num3:
		case sf::Keyboard::Scancode::Numpad3:
			choice = 3;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num4:
		case sf::Keyboard::Scancode::Numpad4:
			choice = 4;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num5:
		case sf::Keyboard::Scancode::Numpad5:
			choice = 5;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num6:
		case sf::Keyboard::Scancode::Numpad6:
			choice = 6;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num7:
		case sf::Keyboard::Scancode::Numpad7:
			choice = 7;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num8:
		case sf::Keyboard::Scancode::Numpad8:
			choice = 8;
			isValidDigit = true;
			break;
		case sf::Keyboard::Scancode::Num9:
		case sf::Keyboard::Scancode::Numpad9:
			choice = 9;
			isValidDigit = true;
			break;
		default:
			isValidDigit = false;
			break;
		}

		if (!isValidDigit) {
			errorMsg = L"无效输入，请输入数字0-9";
			network.sendPlayerChoice(id, title, options, forced, errorMsg, timeoutMs);
			continue;
		}

		if (choice > options.size()) {
			errorMsg = L"超出范围，请输入" + std::wstring(forced ? L"1" : L"0") + L"-" + std::to_wstring(options.size()) + L"范围内的数字";
			network.sendPlayerChoice(id, title, options, forced, errorMsg, timeoutMs);
			continue;
		}

		if (forced && choice == 0) {
			errorMsg = L"必须选择一个选项，请重新输入";
			network.sendPlayerChoice(id, title, options, forced, errorMsg, timeoutMs);
			continue;
		}

		network.sendPlayerChoice(id, L"", {}, false, L"", std::nullopt);
		std::cout << "玩家" << id << "选择了" << choice << std::endl;
		return choice;
	}
}
