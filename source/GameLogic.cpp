#include "../header/GameLogic.h"
#include "../header/Player.h"
#include "../header/Character.h"
#include "../header/Card.h"
#include <iostream>
#include <SFML/Graphics.hpp>
#include <ranges>
#include <algorithm>
#include <variant>

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

bool GameLogic::playersInclude(const std::function<bool(const Player&)>& condition) {
	for (const auto& p : players) {
		if (condition(*p)) return true;
	}
	return false;
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

void GameLogic::forEachPlayer(const std::function<void(Player&)>& operation) {
	for (auto& p : players) {
		operation(*p);
	}
}

void GameLogic::forEachOtherPlayer(const Player& self,
								   const std::function<void(Player&)>& operation) {
	for (auto& p : players) {
		if (*p != self) operation(*p);
	}
}

void GameLogic::forEachPlayerIf(const std::function<bool(const Player&)>& condition,
								const std::function<void(Player&)>& operation) {
	for (auto& p : players) {
		if (condition(*p)) operation(*p);
	}
}
void GameLogic::forEachOtherPlayerIf(const Player& self,
									 const std::function<bool(const Player&)>& condition,
									 const std::function<void(Player&)>& operation) {
	for (auto& p : players) {
		if (*p != self && condition(*p)) operation(*p);
	}
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
	std::ranges::shuffle(seatOrder, unool::random::rng);
	for (std::size_t id = 0; id < 2; ++id) {
		players[id]->hint(L"你是" + std::to_wstring(seatOrder[id] + 1) + L"号位");
	}
}

void GameLogic::initPlayers() {
	players.clear();
	//先用"白板"创建两个Player，以便使用ask
	for (std::size_t i = 0; i < 2; ++i) {
		auto p = std::make_unique<Player>(i, *this, Character::make("白板"));
		players.push_back(std::move(p));
	}

	//拼点决定座次
	determineSeatOrder();
	std::size_t firstSeatId = getSeatPlayerId(0);
	std::size_t secondSeatId = getSeatPlayerId(1);

	//选候选角色
	const std::size_t candidateCount = unool::getConfig()["candidateCount"];
	SelectionState state;
	auto allChars = randomChooseCharacters(candidateCount * 2);
	for (std::size_t i = 0; i < 2; ++i) {
		state.cands[i].assign(
			allChars.begin() + i * candidateCount,
			allChars.begin() + (i + 1) * candidateCount
		);
	}

	//Ban环节：一号位先ban对方候选，然后二号位ban
	banPhase(firstSeatId, secondSeatId, state);
	banPhase(secondSeatId, firstSeatId, state);

	//选角环节：一号位先选，然后二号位选
	selectCharacter(firstSeatId, state);
	selectCharacter(secondSeatId, state);

	charInfoDirty = { true, true };
	resetRound();
}

std::size_t GameLogic::getSeatPlayerId(std::size_t seat) const {
	for (const auto& [playerId, seatNumber] : seatOrder | std::views::enumerate) {
		if (seatNumber == seat) return playerId;
	}
	throw std::logic_error("座位号无效");
}

std::wstring GameLogic::formatCharacterLabelW(const CharacterEntry& entry) {
	return unool::string::to_utf16(
		entry.first + "（" + Character::to_string(entry.second.level) + "）"
	);
}

std::vector<GameLogic::CharacterEntry> GameLogic::randomChooseCharacters(std::size_t n) {
	if (n > Character::infos.size() - 1) {
		throw std::invalid_argument("候选角色数量不能超过已有角色数量（不含白板）");
	}

	auto filteredChars = Character::infos | std::views::filter([](const auto& kv) {
		return kv.first != "白板";
	});
	std::vector<CharacterEntry> result;
	result.reserve(n);
	std::ranges::sample(filteredChars, std::back_inserter(result), n, unool::random::rng);
	std::ranges::shuffle(result, unool::random::rng);
	return result;
}

void GameLogic::banPhase(std::size_t bannerId, std::size_t targetId, SelectionState& state) {
	std::vector<std::wstring> banOpts;
	for (const auto& ch : state.cands[targetId]) {
		banOpts.push_back(formatCharacterLabelW(ch));
	}
	std::size_t banChoice = players[bannerId]->ask(
		L"禁用对方的一个角色：", banOpts, false, 30000ms);
	if (banChoice > 0) {
		state.bannedIdx[targetId] = banChoice - 1;
		std::wstring bannedCharLabel = formatCharacterLabelW(state.cands[targetId][banChoice - 1]);
		players[targetId]->hint(L"对方禁用了你的角色：" + bannedCharLabel);
	}
	else {
		players[targetId]->hint(L"对方未禁用你的任何角色");
	}
}

void GameLogic::chooseSkinAndSet(Player& player, const std::string& charName) {
	auto skins = Character::getSkins(charName);
	std::string skin = "默认";
	if (skins.size() > 1) {
		std::vector<std::wstring> skinOpts;
		for (const auto& s : skins) skinOpts.push_back(unool::string::to_utf16(s));
		std::size_t skinChoice = player.ask(L"选择皮肤：", skinOpts, true);
		skin = skins[skinChoice - 1];
	}
	player.setCharacter(Character::make(charName, skin));
}

void GameLogic::selectCharacter(std::size_t playerId, const SelectionState& state) {
	std::vector<std::wstring> opts;
	std::vector<std::size_t> validIndices;
	for (std::size_t i = 0; i < state.cands[playerId].size(); ++i) {
		if (state.bannedIdx[playerId].has_value() && state.bannedIdx[playerId].value() == i) continue;
		opts.push_back(formatCharacterLabelW(state.cands[playerId][i]));
		validIndices.push_back(i);
	}
	std::size_t choice = players[playerId]->ask(L"选择你的角色：", opts, true);
	std::string charName = state.cands[playerId][validIndices[choice - 1]].first;
	chooseSkinAndSet(*players[playerId], charName);
}
void GameLogic::initPlayers(const std::vector<std::string>& chars) {
	players.clear();
	if (chars.size() != 2) throw std::invalid_argument("指定角色时，角色数量必须为2");

	for (std::size_t i = 0; i < 2; ++i) {
		auto p = std::make_unique<Player>(i, *this, Character::make(chars[i]));
		players.push_back(std::move(p));
	}

	//拼点决定座次
	determineSeatOrder();

	resetRound();
}

bool GameLogic::runTurn() {
	std::cout << "玩家" << getCurrentPlayerId() << "的回合" << std::endl;
	bool gameEnded = currentPlayerTurn();

	if (!gameEnded) {
		altPlayer();
	}

	broadcastState();
	return gameEnded;
}

void GameLogic::broadcastState() {
	for (std::size_t i = 0; i < network.getClientCount(); ++i) {
		GameState state = packStateForPlayer(i);
		network.sendGameStateToClient(i, state);
	}
	flushCharInfo();
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

	//弃牌堆只传前4张
	std::size_t discardCount = std::min(discardPile->count(), static_cast<std::size_t>(4));
	state.discardPile.resize(discardCount);
	for (std::size_t i = 0; i < discardCount; ++i) {
		state.discardPile[i] = discardPile->getCardByIndex(i);
	}

	state.operatingPlayerId = operatingPlayerId;

	return state;
}

void GameLogic::setOperatingPlayer(std::size_t playerId) {
	operatingPlayerId = playerId;
	broadcastState();
}

void GameLogic::clearOperatingPlayer() {
	operatingPlayerId = std::nullopt;
	broadcastState();
}

void GameLogic::flushCharInfo() {
	for (std::size_t i = 0; i < players.size(); ++i) {
		if (i < 2 && charInfoDirty[i]) {
			CharInfo info;
			info.playerIndex = i;
			info.fullText = players[i]->characterName() + "（"
				+ Character::to_string(players[i]->characterLevel()) + "）\n"
				+ "技能：\n"
				+ players[i]->getSkillsText();
			network.sendCharInfo(info);
			charInfoDirty[i] = false;
		}
	}
}

void GameLogic::markCharInfoDirty(std::size_t playerId) {
	if (playerId < 2) charInfoDirty[playerId] = true;
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
							  Card& card,
							  opt_ref<Player> source,
							  opt_ref<std::size_t> number) {
	launchPSkills(currentTriggerTime, player, std::vector<ref<Card>>{card}, source, number);
}
void GameLogic::launchPSkills(const PSkill::TriggerTime& currentTriggerTime,
							  opt_ref<Player> player,
							  std::optional<std::vector<ref<Card>>> cards,
							  opt_ref<Player> source,
							  opt_ref<std::size_t> number) {
	for (auto& carrier : players) {
		PSkill::Trigger trigger = { *this, *carrier, player, cards, source, number };
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
	++matchCount;
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
		player->draw(unool::getConfig()["initHandCount"]);
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
		currentPlayerIndex = getSeatPlayerId(0);
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

std::optional<std::size_t> GameLogic::getWinnerId() const {
	for (const auto& player : players) {
		if (!player->isDead()) {
			return player->getId();
		}
	}
	return std::nullopt;
}
