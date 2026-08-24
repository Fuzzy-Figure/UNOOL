#include "../header/Player.h"
#include "../header/GameLogic.h"
#include <thread>
#include <iterator>

void Player::takeDamage(std::size_t damage, opt_ref<Player> source) {
	game.launchPSkills(PSkill::TriggerTime::damage_begin, *this, std::nullopt, source, damage);
	character->takeDamage(damage);
	game.launchPSkills(PSkill::TriggerTime::damage_end, *this, std::nullopt, source, damage);
}

void Player::recover(std::size_t num) {
	game.launchPSkills(PSkill::TriggerTime::recover_begin, *this, std::nullopt, std::nullopt, num);
	character->recover(num);
	game.launchPSkills(PSkill::TriggerTime::recover_end, *this, std::nullopt, std::nullopt, num);
}


// === 游戏逻辑 ===

std::vector<ref<Card>> Player::draw(std::size_t number, const DrawReason reason) {
	std::cout << "玩家" << id << "(" << characterName() << ")摸了" << number << "张牌" << std::endl;
	game.launchPSkills(PSkill::TriggerTime::draw_begin, *this, std::nullopt, std::nullopt, number);
	if (hasPSkill("巨富") && reason == DrawReason::phase_draw) number += 1;

	std::vector<ref<Card>> drawnCards;
	drawnCards.reserve(number);

	for (std::size_t i = 0; i < number; ++i) {
		auto cardPtr = game.getPile().take_front(game.getDiscardPile());
		hand->push_back(std::move(cardPtr));
		drawnCards.emplace_back(hand->back());
	}

	if (reason == DrawReason::phase_draw) handSelectLast();

	game.launchPSkills(PSkill::TriggerTime::draw_end, *this, drawnCards, std::nullopt, number);
	game.launchPSkills(PSkill::TriggerTime::gain_card_end, *this, drawnCards, std::nullopt, number);
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
	std::cout << "玩家" << id << "打出了：" << card->toString() << std::endl;

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

	//更新客户端显示
	game.broadcastState();

	//技能
	game.launchPSkills(PSkill::TriggerTime::lose_card_end, *this, cardRef, std::nullopt);
	game.launchPSkills(PSkill::TriggerTime::use_card_end, *this, cardRef, std::nullopt);

	return cardRef;
}

void Player::gainCard(std::unique_ptr<Card> card) {
	hand->push_back(std::move(card));
	std::vector<ref<Card>> gainedCards;
	gainedCards.emplace_back(hand->back());
	game.launchPSkills(PSkill::TriggerTime::gain_card_end, *this, gainedCards);
}

void Player::discardByIndex(const std::size_t cardIndex) {
	game.launchPSkills(PSkill::TriggerTime::lose_card_begin, *this);
	std::unique_ptr<Card> card = hand->takeCardByIndex(cardIndex);
	ref<Card> cardRef = *card;
	game.putCardToDiscardPile(std::move(card));
	game.launchPSkills(PSkill::TriggerTime::lose_card_end, *this, cardRef, std::nullopt);
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

void Player::launchPSkills(const PSkill::TriggerTime& currentTriggerTime, PSkill::Trigger& trigger) {
	character->launchPSkills(currentTriggerTime, trigger);
}

void Player::ban(Player& source, Card& card) {
	game.launchPSkills(PSkill::TriggerTime::ban_begin, *this, card, source);
	banned = true;
	game.launchPSkills(PSkill::TriggerTime::ban_end, *this, card, source);
}


// === 回合流程 ===

void Player::phaseBegin() {
	game.launchPSkills(PSkill::TriggerTime::phase_begin, *this);
}

//返回是否出牌
bool Player::phaseUse1() {
	game.launchPSkills(PSkill::TriggerTime::phase_use1_begin, *this);
	auto card = chooseToUse(ASkill::TriggerTime::phase_use1);
	if (card.has_value())
		game.launchPSkills(PSkill::TriggerTime::phase_use1_end, *this, card.value().get());
	else
		game.launchPSkills(PSkill::TriggerTime::phase_use1_end, *this);
	return card.has_value();
}

void Player::phaseDraw() {
	std::size_t drawCount = 1;
	game.launchPSkills(PSkill::TriggerTime::phase_draw_begin, *this, std::nullopt, std::nullopt, drawCount);
	std::vector<ref<Card>> drawnCards = draw(drawCount, DrawReason::phase_draw);
	game.launchPSkills(PSkill::TriggerTime::phase_draw_end, *this, drawnCards);
}

void Player::phaseUse2() {
	game.launchPSkills(PSkill::TriggerTime::phase_use2_begin, *this);
	chooseToUse(ASkill::TriggerTime::phase_use2);
}

void Player::phaseEnd() {
	game.launchPSkills(PSkill::TriggerTime::phase_end, *this);
}

bool Player::turn() {
	phaseBegin();
	game.broadcastState();
	bool used = false;

	if (banned) {
		std::cout << "玩家" << id << "跳过了他的回合" << std::endl;
		unban();
		goto PhaseEnd;
	}

	if (game.isGameOver()) return handEmpty();

	used = phaseUse1();
	if (game.isGameOver()) return handEmpty();

	if (!used) {
		phaseDraw();
		if (game.isGameOver()) return handEmpty();
		game.broadcastState();
		phaseUse2();
		if (game.isGameOver()) return handEmpty();
	}

PhaseEnd:
	phaseEnd();
	game.broadcastState();
	return handEmpty();
}

// === 导航 ===
Player& Player::next() const { return game.getPlayerById(game.nextPlayerIndex(id)); }
Player& Player::prev() const { return game.getPlayerById(game.prevPlayerIndex(id)); }

// === 交互 ===

std::optional<std::size_t> Player::chooseCard(std::function<bool(const Card&)> condition,
											  bool forced, ASkill::TriggerTime phase) {
	ServerNetwork& network = game.getNetwork();
	game.setOperatingPlayer(id);
	ASkillTransformBase* activeMode = nullptr;  //当前激活的转换型主动技

	//收集当前阶段可发动的两类技能（即时 / 转换）
	std::vector<ref<ASkillInstantBase>>   instantRefs;
	std::vector<ref<ASkillTransformBase>> transformRefs;
	if (phase != ASkill::TriggerTime::never) {
		for (auto& s : getInstantSkills()) {
			if (s->canTriggerAt(phase) && s->canUse()) instantRefs.emplace_back(*s);
		}
		for (auto& s : getTransformSkills()) {
			if (s->canTriggerAt(phase) && s->canUse()) transformRefs.emplace_back(*s);
		}
	}

	while (true) {
		network.update();
		auto inputOpt = network.receiveClientInput();
		if (!inputOpt.has_value()) {
			std::this_thread::sleep_for(16ms);
			continue;
		}

		ClientInput clientInput = inputOpt.value();
		if (clientInput.playerId != id) continue;

		sf::Keyboard::Scancode input = clientInput.key;
		setInput(input);

		//数字1-9：即时技发动 / 转换技切换（仅出牌阶段）
		if (phase != ASkill::TriggerTime::never) {
			std::size_t digit = 0;
			switch (input) {
			case sf::Keyboard::Scancode::Num1: case sf::Keyboard::Scancode::Numpad1: digit = 1; break;
			case sf::Keyboard::Scancode::Num2: case sf::Keyboard::Scancode::Numpad2: digit = 2; break;
			case sf::Keyboard::Scancode::Num3: case sf::Keyboard::Scancode::Numpad3: digit = 3; break;
			case sf::Keyboard::Scancode::Num4: case sf::Keyboard::Scancode::Numpad4: digit = 4; break;
			case sf::Keyboard::Scancode::Num5: case sf::Keyboard::Scancode::Numpad5: digit = 5; break;
			case sf::Keyboard::Scancode::Num6: case sf::Keyboard::Scancode::Numpad6: digit = 6; break;
			case sf::Keyboard::Scancode::Num7: case sf::Keyboard::Scancode::Numpad7: digit = 7; break;
			case sf::Keyboard::Scancode::Num8: case sf::Keyboard::Scancode::Numpad8: digit = 8; break;
			case sf::Keyboard::Scancode::Num9: case sf::Keyboard::Scancode::Numpad9: digit = 9; break;
			default: break;
			}
			if (digit > 0) {
				std::size_t idx = digit - 1;  //0-based
				//前 instantRefs.size() 个键：触发即时技
				if (idx < instantRefs.size()) {
					if (instantRefs[idx].get().tryActivate(game, *this)) {
						game.broadcastState();
					}
				}
				//后续键：切换转换技激活态
				else if (idx - instantRefs.size() < transformRefs.size()) {
					std::size_t tIdx = idx - instantRefs.size();
					ASkillTransformBase& skill = transformRefs[tIdx].get();
					if (activeMode == &skill) {
						activeMode = nullptr;
						network.sendPlayerChoice(id, std::wstring(L""), std::vector<std::wstring>(), false);
					}
					else {
						activeMode = &skill;
						network.sendPlayerChoice(id, skill.getPrompt(), std::vector<std::wstring>(), false);
					}
				}
				continue;
			}
		}

		switch (input) {
		case sf::Keyboard::Scancode::Space:
			hand->setSelectedIndex(clientInput.selectedIndex);
			sortHand();
			game.broadcastState();
			break;
		case sf::Keyboard::Scancode::Up:
		case sf::Keyboard::Scancode::W:
			hand->setSelectedIndex(clientInput.selectedIndex);
			if (!handEmpty()) {
				if (activeMode != nullptr) {
					//转换技：尝试转化选中的牌并打出（暂仅支持单牌转换）
					Card& selected = (*hand)[hand->getSelectedIndex()];
					if (activeMode->canSelect(selected) && activeMode->getCardCount() == 1) {
						Card original = selected;  //备份原牌
						std::vector<ref<Card>> cards;
						cards.emplace_back(selected);
						activeMode->transform(std::move(cards));
						if (canUse(selected)) {
							network.sendPlayerChoice(id, std::wstring(L""), std::vector<std::wstring>(), false);  //清提示
							game.clearOperatingPlayer();
							return hand->getSelectedIndex();
						}
						else {
							selected = original;  //还原
							std::cout << "<" << activeMode->getName() << "> 转化后的牌不符合出牌规则" << std::endl;
						}
					}
					else {
						std::cout << "<" << activeMode->getName() << "> 选中的牌不能转化" << std::endl;
					}
				}
				else if (condition(hand->getSelectedCard())) {
					network.sendPlayerChoice(id, L"", {}, false);  //清提示
					game.clearOperatingPlayer();
					return hand->getSelectedIndex();
				}
			}
			break;
		case sf::Keyboard::Scancode::Down:
		case sf::Keyboard::Scancode::S:
			if (!forced) {
				network.sendPlayerChoice(id, L"", {}, false);  //清提示
				game.clearOperatingPlayer();
				return std::nullopt;
			}
			break;
		default:
			break;
		}
	}
}

opt_ref<Card> Player::chooseToUse(ASkill::TriggerTime phase) {
	auto index = chooseCard([this](const Card& c) { return canUse(c); }, false, phase);
	if (index.has_value()) {
		return useCardByIndex(index.value());
	}
	else {
		std::cout << "玩家" << id << "选择不跟牌" << std::endl;
		game.broadcastState();
		return std::nullopt;
	}
}

std::vector<ref<Card>> Player::chooseToDiscard(const std::wstring& title,
											   const std::size_t num, const bool forced,
											   const std::function<bool(const Card&)>& condition) {
	std::vector<ref<Card>> discardedCards;
	if (num > handCount()) return discardedCards;

	ServerNetwork& network = game.getNetwork();
	std::cout << "玩家" << id << "请选择弃置" << num << "张牌" << std::endl;

	std::size_t discardedCount = 0;
	while (discardedCount < num) {
		std::wstring fullTitle = title + L"（" + std::to_wstring(discardedCount + 1) + L"/" + std::to_wstring(num) + L"）\n"
			+ (forced ? L"（↑确认，不可取消）" : L"（↑确认，↓取消）");
		network.sendPlayerChoice(id, fullTitle, {}, forced);
		auto index = chooseCard(condition, forced);
		if (!index.has_value()) {
			network.sendPlayerChoice(id, L"", {}, false);
			std::cout << "玩家" << id << "取消了弃牌" << std::endl;
			return discardedCards;
		}
		discardedCards.push_back(hand->getCardByIndex(index.value()));
		discardByIndex(index.value());
		discardedCount++;
		std::cout << "玩家" << id << "弃置了一张牌（" << discardedCount << "/" << num << "）" << std::endl;
		game.broadcastState();
	}
	network.sendPlayerChoice(id, L"", {}, false);
	return discardedCards;
}

void Player::chooseToRecast(const std::wstring& title,
							const std::size_t num, const bool forced,
							const std::function<bool(const Card&)>& condition) {
	game.launchPSkills(PSkill::TriggerTime::recast_begin, *this);
	chooseToDiscard(title, num, forced, condition);
	draw(num);
	game.launchPSkills(PSkill::TriggerTime::recast_begin, *this);
}

void Player::decree(const std::wstring& title,
					const std::size_t num, const bool forced,
					const std::function<bool(const Card&)>& condition) {
	game.launchPSkills(PSkill::TriggerTime::decree_begin, *this);
	draw(num);
	chooseToDiscard(title, num, forced, condition);
	game.launchPSkills(PSkill::TriggerTime::decree_end, *this);
}

void Player::inherit(std::unique_ptr<Card>& card) {

}

opt_ref<Card> Player::chooseToOperate(const std::wstring& title, bool forced,
									  const std::function<bool(const Card&)>& condition,
									  const std::function<void(Card&)>& operation) {
	ServerNetwork& network = game.getNetwork();
	if (forced) network.sendPlayerChoice(id, title + L"\n（↑确认，不可取消）", {}, true);
	else network.sendPlayerChoice(id, title + L"\n（↑确认，↓取消）", {}, false);
	std::optional<std::size_t> index = chooseCard(condition, forced);
	network.sendPlayerChoice(id, L"", {}, false);
	if (!index.has_value()) return std::nullopt;
	ref<Card> cardRef = getHand().getCardByIndex(index.value());
	operation(getHand().getCardByIndex(index.value()));
	return cardRef;
}

opt_ref<Card> Player::chooseToGive(const std::wstring& title, Player& target,
								   bool forced, const std::function<bool(const Card&)>& condition) {
	if (handEmpty()) return std::nullopt;

	ServerNetwork& network = game.getNetwork();

	network.sendPlayerChoice(id, title, {}, true, L"", std::nullopt);
	auto index = chooseCard(condition, forced);
	network.sendPlayerChoice(id, L"", {}, false, L"", std::nullopt);

	if (!index.has_value()) {
		std::cout << "玩家" << id << "取消了给" << target.characterName() << "牌" << std::endl;
		return std::nullopt;
	}

	ref<Card> card = hand->getCardByIndex(index.value());

	give(target, takeCardByIndex(index.value()));
	std::cout << "玩家" << id << "给了" << target.characterName() << "一张" << card.get().toString() << std::endl;
	game.broadcastState();

	return card;
}

opt_ref<Player> Player::choosePlayer(const std::wstring& title, bool forced,
									 const std::function<bool(const Player&)>& condition) {
	//选角色
	const auto& candidates = game.getPlayersIf(condition);

	if (candidates.empty()) {
		game.broadcastState();
		return std::nullopt;
	}

	std::vector<std::wstring> options;
	for (auto& p : candidates) {
		options.push_back(p.get().characterNameW());
	}
	std::size_t choice = ask(title, options, forced);

	return candidates[choice - 1];
}
opt_ref<Player> Player::chooseOtherPlayer(const std::wstring& title, bool forced,
										  const std::function<bool(const Player&)>& condition) {
	return choosePlayer(title, forced, [this, &condition](const Player& p) {
		return p != *this && condition(p);
	});
}

std::optional<Card::Color> Player::chooseCardColor(const std::wstring& title, bool forced, const std::vector<Card::Color>& colors) {
	if (colors.empty()) return std::nullopt;
	std::vector<std::wstring> options;
	for (const auto& c : colors) {
		options.push_back(Card::to_wstring(c));
	}
	std::size_t choice = ask(title, options, forced);
	if (choice == 0) return std::nullopt;
	return colors[choice - 1];
}

std::optional<Card::Name> Player::chooseCardName(const std::wstring& title, bool forced, const std::vector<Card::Name>& names) {
	if (names.empty()) return std::nullopt;
	std::vector<std::wstring> options;
	for (const auto& n : names) {
		options.push_back(Card::to_wstring(n));
	}
	std::size_t choice = ask(title, options, forced);
	if (choice == 0) return std::nullopt;
	return names[choice - 1];
}


std::size_t Player::ask(const std::wstring& title, const std::vector<std::wstring>& options,
						bool forced, std::optional<std::chrono::milliseconds> timeoutMs) {
	ServerNetwork& network = game.getNetwork();
	std::wstring errorMsg;

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
			network.sendPlayerChoice(id, title, options, forced, errorMsg, toTimeoutMs(), 0, 1);
			return;
		}
		std::vector<std::wstring> pageOptions;
		const std::size_t start = currentPage * PER_PAGE;
		const std::size_t end = std::min(start + PER_PAGE, options.size());
		for (std::size_t i = start; i < end; ++i) {
			pageOptions.push_back(options[i]);
		}
		network.sendPlayerChoice(id, title, pageOptions, forced, errorMsg, toTimeoutMs(), currentPage, totalPages);
	};

	sendPage();

	sf::Clock clock;
	while (true) {
		if (timeoutMs.has_value()) {
			if (clock.getElapsedTime().asMilliseconds() >= timeoutMs.value().count()) {
				network.sendPlayerChoice(id, L"", {}, false, L"", std::nullopt);
				std::cout << "玩家" << id << "超时未选择" << std::endl;
				return 0;
			}
		}

		network.update();
		auto inputOpt = network.receiveClientInput();
		if (!inputOpt.has_value()) {
			std::this_thread::sleep_for(16ms);
			continue;
		}

		ClientInput clientInput = inputOpt.value();
		if (clientInput.playerId != id) {
			continue;
		}

		sf::Keyboard::Scancode input = clientInput.key;
		setInput(input);

		if (usePaging && (input == sf::Keyboard::Scancode::Left || input == sf::Keyboard::Scancode::Right)) {
			if (input == sf::Keyboard::Scancode::Left) {
				if (currentPage > 0) --currentPage;
			}
			else {
				if (currentPage + 1 < totalPages) ++currentPage;
			}
			errorMsg.clear();
			sendPage();
			continue;
		}

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

		if (usePaging && isValidDigit) {
			if (choice == 0) {
				// 0 = 取消，保持不变
			}
			else {
				const std::size_t realIndex = currentPage * PER_PAGE + (choice - 1);
				if (realIndex >= options.size()) {
					errorMsg = L"超出范围，请输入" + std::wstring(forced ? L"1" : L"0") + L"-" +
						std::to_wstring(std::min(PER_PAGE, options.size() - currentPage * PER_PAGE)) +
						L"范围内的数字（<-->翻页）";
					sendPage();
					continue;
				}
				choice = realIndex + 1;
			}
		}

		if (!isValidDigit) {
			if (usePaging) {
				errorMsg = L"无效输入，请输入数字0-9或使用<-->翻页";
			}
			else {
				errorMsg = L"无效输入，请输入数字0-9";
			}
			sendPage();
			continue;
		}

		if (choice > options.size()) {
			if (usePaging) {
				errorMsg = L"超出范围，请输入" + std::wstring(forced ? L"1" : L"0") + L"-" +
					std::to_wstring(std::min(PER_PAGE, options.size() - currentPage * PER_PAGE)) +
					L"范围内的数字（<-->翻页）";
			}
			else {
				errorMsg = L"超出范围，请输入" + std::wstring(forced ? L"1" : L"0") + L"-" + std::to_wstring(options.size()) + L"范围内的数字";
			}
			sendPage();
			continue;
		}

		if (forced && choice == 0) {
			errorMsg = L"必须选择一个选项，请重新输入";
			sendPage();
			continue;
		}

		network.sendPlayerChoice(id, L"", {}, false, L"", std::nullopt);
		std::cout << "[ask] 标题：“" << unool::string::to_utf8(title) << "”，玩家" << id << "选择了" << choice << ": ";
		if (choice != 0)
			std::cout << unool::string::to_utf8(options[choice - 1]) << std::endl;
		return choice;
	}
}

void Player::hint(const std::wstring& message) {
	(void)ask(message, { L"确认" }, true);
}

Card& Player::judge() {
	game.launchPSkills(PSkill::TriggerTime::judge_begin, *this);
	auto card = game.getPile().takeCardByIndex(0);
	Card& cardRef = *card;
	std::cout << "判定结果：" << cardRef << std::endl;
	game.getDiscardPile().push_front(std::move(card));
	game.launchPSkills(PSkill::TriggerTime::judge_end, *this, cardRef);
	game.broadcastState();
	return cardRef;
}

void Player::showCard(const Card& card) {
	game.forEachOtherPlayer(*this, [this, &card](Player& p) {
		p.hint(unool::string::to_utf16(characterName()) + L"展示了" + card.toWString());
	});
}
