#include "../header/GameLogic.h"
#include "../header/Player.h"
#include "../header/Character.h"
#include "../header/Card.h"
#include <iostream>
#include <SFML/Graphics.hpp>
#include <ranges>
#include <algorithm>

//获取curIndex的下一个玩家的id（curIndex不一定是当前回合玩家的id）
std::size_t GameLogic::nextPlayerIndex(const std::size_t curIndex) const {
	if (direction == Direction::increase) {
		return curIndex < players.size() - 1 ? curIndex + 1 : 0;
	}
	else if (direction == Direction::decrease) {
		return curIndex > 0 ? curIndex - 1 : players.size() - 1;
	}
	throw std::logic_error("无效的direction");
}

//获取curIndex的上一个玩家的id（curIndex不一定是当前回合玩家的id）
std::size_t GameLogic::prevPlayerIndex(const std::size_t curIndex) const {
	if (direction == Direction::decrease) {
		return curIndex < players.size() - 1 ? curIndex + 1 : 0;
	}
	else if (direction == Direction::increase) {
		return curIndex > 0 ? curIndex - 1 : players.size() - 1;
	}
	throw std::logic_error("无效的direction");
}

void GameLogic::altPlayer() {
	currentPlayerIndex = nextPlayerIndex(currentPlayerIndex);
}

Player& GameLogic::currentPlayer() const {
	return *players[currentPlayerIndex];
}

bool GameLogic::currentPlayerTurn() {
	return players[currentPlayerIndex]->turn();
}

std::size_t GameLogic::getCurrentPlayerId() const {
	return currentPlayer().getId();
}

bool GameLogic::playersSatisfy(const std::function<bool(std::vector<std::unique_ptr<Player>>&)>& condition) {
	return condition(players);
}

const std::vector<ref<Player>> GameLogic::getPlayers() const {
	std::vector<ref<Player>> refs;
	for (const auto& pl : players) {
		refs.emplace_back(*pl);
	}
	return refs;
}

const std::vector<ref<Player>> GameLogic::getPlayersIf(const std::function<bool(const Player&)>& condition) const {
	std::vector<ref<Player>> refs;
	for (const auto& pl : players) {
		if (condition(*pl)) refs.emplace_back(*pl);
	}
	return refs;
}

const std::vector<ref<Player>> GameLogic::getPlayersExcludeId(const std::size_t id) const {
	return getPlayersIf([&id](const Player& p) {
		return p.getId() != id;
	});
}

void GameLogic::print() const {
	return;
	for (const auto& pl : players) {
		std::cout << "玩家" << pl->getId() << "的手牌：";
		pl->printHand();
	}
	if (lastCard().has_value()) {
		std::cout << "上一张牌：" << lastCard().value() << std::endl;
	}
	else std::cout << "上一张牌：无" << std::endl;
}

Pile& GameLogic::getPile() { return *pile; }

Pile& GameLogic::getDiscardPile() { return *discardPile; }

GameLogic::GameLogic(ServerNetwork& _network)
	:network(_network) {
	pile = std::make_unique<Pile>();
	discardPile = std::make_unique<Pile>();
}

GameLogic::~GameLogic() {}

void GameLogic::determineSeatOrder() {
	constexpr std::size_t playerCount = 2;
	seatOrder.resize(playerCount);
	std::ranges::iota(seatOrder, 0);
	std::ranges::shuffle(seatOrder, unool::rng);
	for (const auto& [seat, playerId] : seatOrder | std::views::enumerate) {
		std::wstring msg = L"你是" + std::to_wstring(seat + 1) + L"号位";
		players[playerId]->ask(msg, { L"确认" }, true);
	}
}

void GameLogic::initPlayers() {
	//先用"白板"创建两个Player，以便使用ask
	for (std::size_t i = 0; i < 2; ++i) {
		auto p = std::make_unique<Player>(i, *this, Character::make("白板"));
		players.push_back(std::move(p));
	}

	//拼点决定座次
	determineSeatOrder();

	//确定一号位和二号位的玩家id
	std::size_t firstSeatId = 0, secondSeatId = 1;
	for (std::size_t i = 0; i < seatOrder.size(); ++i) {
		if (seatOrder[i] == 0) firstSeatId = i;
		else if (seatOrder[i] == 1) secondSeatId = i;
	}

	//从所有角色中随机为每名玩家选5个候选（排除"白板"）
	std::vector<std::tuple<std::string, Character::Level>> allChars;
	for (const auto& [name, info] : Character::infos) {
		if (name != "白板") allChars.push_back(std::tuple{ name, info.level });
	}
	std::shuffle(allChars.begin(), allChars.end(), unool::rng);
	std::vector<std::tuple<std::string, Character::Level>> cands[2];
	cands[0].assign(allChars.begin(), allChars.begin() + 5);
	cands[1].assign(allChars.begin() + 5, allChars.begin() + 10);

	//被ban的候选索引（对每名玩家）
	std::optional<std::size_t> bannedIdx[2];

	//Ban环节：一号位先ban对方候选，然后二号位ban
	auto doBan = [&](std::size_t bannerId, std::size_t targetId) {
		std::vector<std::wstring> banOpts;
		for (const auto& c : cands[targetId]) {
			banOpts.push_back(unool::to_utf16(
				std::get<0>(c) + "（" + Character::levelToString(std::get<1>(c)) + "）")
			);
		}
		std::size_t banChoice = players[bannerId]->ask(
			L"禁用对方的一个角色：", banOpts, false, 10000);
		if (banChoice > 0) {
			bannedIdx[targetId] = banChoice - 1;
			//通知被ban方
			std::wstring bannedName = unool::to_utf16(std::get<0>(cands[targetId][banChoice - 1]) + Character::levelToString(std::get<1>(cands[targetId][banChoice - 1])));
			players[targetId]->ask(L"对方禁用了你的角色：" + bannedName, { L"确认" }, true);
		}
		else {
			//通知被ban方：对方未禁用
			players[targetId]->ask(L"对方未禁用你的任何角色", { L"确认" }, true);
		}
	};
	doBan(firstSeatId, secondSeatId);
	doBan(secondSeatId, firstSeatId);

	//为指定角色选皮肤并设置；仅有"默认"皮肤时跳过询问
	auto chooseSkinAndSet = [&](Player& player, const std::string& charName) {
		auto skins = Character::getSkins(charName);
		std::string skin = "默认";
		if (skins.size() > 1) {
			std::vector<std::wstring> skinOpts;
			for (const auto& s : skins) skinOpts.push_back(unool::to_utf16(s));
			std::size_t skinChoice = player.ask(L"选择皮肤：", skinOpts, true);
			skin = skins[skinChoice - 1];
		}
		player.setCharacter(Character::make(charName, skin));
	};

	//选角环节：一号位先选，然后二号位选
	auto selectCharacter = [&](std::size_t playerId) {
		std::vector<std::wstring> opts;
		std::vector<std::size_t> validIndices;
		for (std::size_t i = 0; i < cands[playerId].size(); ++i) {
			if (bannedIdx[playerId].has_value() && bannedIdx[playerId].value() == i) continue;
			opts.push_back(unool::to_utf16(std::get<0>(cands[playerId][i])));
			validIndices.push_back(i);
		}
		std::size_t choice = players[playerId]->ask(L"选择你的角色：", opts, true);
		std::string charName = std::get<0>(cands[playerId][validIndices[choice - 1]]);
		chooseSkinAndSet(*players[playerId], charName);
	};
	selectCharacter(firstSeatId);
	selectCharacter(secondSeatId);

	resetRound();
}
void GameLogic::initPlayers(const std::vector<std::string>& characters) {
	if (characters.size() != 2) throw std::invalid_argument("参数characters的大小必须为2");

	//先用"白板"创建两个Player，以便使用ask
	for (std::size_t i = 0; i < 2; ++i) {
		auto p = std::make_unique<Player>(i, *this, Character::make(characters[i]));
		players.push_back(std::move(p));
	}

	//拼点决定座次
	determineSeatOrder();

	//确定一号位和二号位的玩家id
	std::size_t firstSeatId = 0, secondSeatId = 1;
	for (std::size_t i = 0; i < seatOrder.size(); ++i) {
		if (seatOrder[i] == 0) firstSeatId = i;
		else if (seatOrder[i] == 1) secondSeatId = i;
	}

	resetRound();
}

bool GameLogic::runTurn() {
	std::cout << "玩家" << getCurrentPlayerId() << "的回合" << std::endl;
	bool gameEnded = currentPlayerTurn();

	if (!gameEnded) {
		altPlayer();
	}

	print();
	broadcastState();
	return gameEnded;
}

void GameLogic::broadcastState() {
	for (std::size_t i = 0; i < network.getClientCount(); ++i) {
		GameState state = packStateForPlayer(i);
		network.sendGameStateToClient(i, state);
	}
}

ServerNetwork& GameLogic::getNetwork() {
	return network;
}

GameState GameLogic::packStateForPlayer(std::size_t playerId) const {
	GameState state;

	state.currentPlayerIndex = currentPlayerIndex;
	state.currentColor = currentColor;
	state.currentName = currentName;
	state.direction = direction == Direction::increase ? 0 : 1;
	state.seatOrder = seatOrder;

	state.players.resize(players.size());
	for (const auto& [i, pl] : players | std::views::enumerate) {
		state.players[i].id = pl->getId();
		state.players[i].characterName = pl->characterName();
		state.players[i].skin = pl->skin();
		state.players[i].hp = pl->getHp();
		state.players[i].maxHp = pl->getMaxHp();

		if (pl->getId() == playerId) {
			for (const auto& card : pl->getHand()) {
				state.players[i].hand.push_back(Card::make(card));
			}
		}
		else {
			for (const auto& card : pl->getHand()) {
				state.players[i].hand.push_back(Card::make(Card::back));
			}
		}

		state.players[i].hand.setSelectedIndex(pl->handSelectedIndex());
	}

	state.discardPile.resize(discardPile->count());
	for (const auto& [i, card] : *discardPile | std::views::enumerate) {
		state.discardPile[i] = *card;
	}

	return state;
}

Player& GameLogic::getPlayerById(const std::size_t id) {
	return *players[id];
}

void GameLogic::setCurrentColor(const Card::Color newColor) {
	currentColor = newColor;
}

Card::Color GameLogic::getCurrentColor() const {
	return currentColor;
}

void GameLogic::setCurrentName(const Card::Name newName) {
	currentName = newName;
}

Card::Name GameLogic::getCurrentName() const {
	return currentName;
}

void GameLogic::reverse() {
	if (direction == Direction::increase) direction = Direction::decrease;
	else direction = Direction::increase;
}

void GameLogic::launchPSkills(const PSkill::TriggerTime& currentTriggerTime,
							  opt_ref<Player> player,
							  opt_ref<Card> card,
							  opt_ref<Player> source,
							  opt_ref<std::size_t> number) {
	for (auto& carrier : players) {
		PSkill::Trigger trigger = { *this, *carrier, player, card, source, number };
		carrier->launchPSkills(currentTriggerTime, trigger);
	}
}

//返回置入弃牌堆的牌的引用
Card& GameLogic::putCardToDiscardPile(std::unique_ptr<Card> card) {
	std::cout << "[" << *card << "] 进入了弃牌堆" << std::endl;
	discardPile->push_front(std::move(card));
	return discardPile->front();
}

std::optional<Card> GameLogic::lastCard() const {
	if (discardPile->empty()) return std::nullopt;
	else return discardPile->front();
}

void GameLogic::checkRoundEnd() {
	for (auto& player : players) {
		std::size_t damage = player->handValue();
		player->takeDamage(damage, std::nullopt);
		std::cout << "玩家" << player->getId() << "扣除" << damage << "点体力，剩余" << player->getHp() << "/" << player->getMaxHp() << std::endl;
	}
}

void GameLogic::resetRound() {
	// 重置牌堆
	pile = Pile::standard();
	discardPile->clear();

	// 重置玩家
	for (auto& player : players) {
		// 打印手牌
		player->printHand();
		// 重置手牌
		player->clearHand();
		// 初始手牌
		player->draw(8);
		// 重置技能使用次数
		player->resetSkills();
		//取消封禁
		player->unban();
	}
	// 重置当前颜色
	currentColor = Card::Color::no;
	// 重置当前牌名
	currentName = Card::Name::no;
	// 重置当前玩家（一号位始终先手）
	if (!seatOrder.empty()) {
		for (std::size_t i = 0; i < seatOrder.size(); ++i) {
			if (seatOrder[i] == 0) {
				currentPlayerIndex = i;
				break;
			}
		}
	}
	else {
		currentPlayerIndex = firstPlayerIndex;
	}
	// 重置方向
	direction = Direction::increase;
	broadcastState();
	launchPSkills(PSkill::TriggerTime::game_begin);
	std::cout << "[Server] 新一局开始！玩家" << currentPlayerIndex << "先手" << std::endl;
}

bool GameLogic::isGameOver() const {
	for (const auto& player : players) {
		if (player->isDead()) {
			return true;
		}
	}
	return false;
}

std::size_t GameLogic::getWinnerId() const {
	for (const auto& player : players) {
		if (!player->isDead()) {
			return player->getId();
		}
	}
	return -1;
}
