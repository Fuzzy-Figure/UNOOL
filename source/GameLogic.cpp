#include "../header/GameLogic.h"
#include "../header/Player.h"
#include "../header/Character.h"
#include "../header/Card.h"
#include <iostream>
#include <SFML/Graphics.hpp>
#include <ranges>
#include <algorithm>
#include <variant>
#include <format>

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

bool GameLogic::playersInclude(const std::function<bool(const Player&)>& condition) const {
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

void GameLogic::forEachPlayer(const std::function<void(const Player&)>& operation) const {
	for (const auto& p : players) {
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
const Pile& GameLogic::getDiscardPile() const { return *discardPile; }

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

	//读取模式：normal（默认）/ double
	const std::string mode = unool::getServerConfig().value("mode", "normal");
	if (mode == "double") {
		initPlayersDouble(firstSeatId, secondSeatId);
	}
	else {
		initPlayersNormal(firstSeatId, secondSeatId);
	}

	resetGame();
}

void GameLogic::initPlayersNormal(std::size_t firstSeatId, std::size_t secondSeatId) {
	//选候选角色
	const std::size_t candidateCount = unool::getServerConfig()["singleCandidateCount"];
	SelectionState state;
	auto allChars = Character::randomChooseCharacters(candidateCount * 2);
	for (std::size_t i = 0; i < 2; ++i) {
		state.cands[i].assign(
			allChars.begin() + i * candidateCount,
			allChars.begin() + (i + 1) * candidateCount
		);
	}

	//Ban环节：玩家A先连续ban banCount次，再一次性告诉B；然后B同理，最后提示A
	auto formatBanSummary = [&](std::size_t targetId, const std::vector<std::wstring>& labels) -> std::wstring {
		if (labels.empty()) return players[targetId]->characterNameW() + L"没有禁用你的任何角色";
		std::wstring msg = L"对方禁用了你的角色：\n";
		for (std::size_t i = 0; i < labels.size(); ++i) {
			if (i > 0) msg += L"\n";
			msg += labels[i];
		}
		return msg;
	};

	const std::size_t banCount = unool::getServerConfig().value("banCount", 0);

	std::vector<std::wstring> bannedByA;
	bannedByA.reserve(banCount);
	for (std::size_t b = 0; b < banCount; ++b) {
		auto label = banPhase(firstSeatId, secondSeatId, b, banCount, state);
		if (label.has_value()) bannedByA.push_back(std::move(*label));
	}
	players[secondSeatId]->hint(formatBanSummary(secondSeatId, bannedByA));

	std::vector<std::wstring> bannedByB;
	bannedByB.reserve(banCount);
	for (std::size_t b = 0; b < banCount; ++b) {
		auto label = banPhase(secondSeatId, firstSeatId, b, banCount, state);
		if (label.has_value()) bannedByB.push_back(std::move(*label));
	}
	players[firstSeatId]->hint(formatBanSummary(firstSeatId, bannedByB));

	//选角环节：一号位先选，然后二号位选
	selectCharacter(firstSeatId, state);
	selectCharacter(secondSeatId, state);
}

void GameLogic::initPlayersDouble(std::size_t firstSeatId, std::size_t secondSeatId) {
	//双将模式：无ban，抽 doubleCandidateCount*2 个候选平分各 doubleCandidateCount 个
	const std::size_t doubleCandidateCount = unool::getServerConfig().value("doubleCandidateCount", 5);
	auto allChars = Character::randomChooseCharacters(doubleCandidateCount * 2);

	std::vector<Character::Entry> cands1(
		allChars.begin(), allChars.begin() + doubleCandidateCount);
	std::vector<Character::Entry> cands2(
		allChars.begin() + doubleCandidateCount, allChars.end());

	//按座次每家连续选完2个再下一家
	selectCharacterDouble(firstSeatId, cands1);
	selectCharacterDouble(secondSeatId, cands2);
}

std::size_t GameLogic::getSeatPlayerId(std::size_t seat) const {
	for (const auto& [playerId, seatNumber] : seatOrder | std::views::enumerate) {
		if (seatNumber == seat) return playerId;
	}
	throw std::logic_error("座位号无效");
}

std::wstring GameLogic::formatCharacterLabelW(const Character::Entry& entry) {
	return unool::string::to_utf16(
		entry.first + "（" + Character::to_string(entry.second.level) + "）"
	);
}



std::optional<std::wstring> GameLogic::banPhase(std::size_t bannerId, std::size_t targetId, std::size_t banIndex, std::size_t banCount, SelectionState& state) {
	std::vector<std::wstring> banOpts;
	std::vector<std::size_t> validIndices;
	for (std::size_t i = 0; i < state.cands[targetId].size(); ++i) {
		const bool alreadyBanned = std::ranges::contains(state.bannedIdx[targetId], i);
		if (alreadyBanned) continue;
		banOpts.push_back(formatCharacterLabelW(state.cands[targetId][i]));
		validIndices.push_back(i);
	}
	//候选池<=1时无需再ban
	if (validIndices.size() <= 1) return std::nullopt;
	const std::wstring title = std::format(L"禁用对方的角色（{}/{}）：", banIndex + 1, banCount);
	std::size_t banChoice = players[bannerId]->ask(title, banOpts, false, 60s);
	if (banChoice > 0 && banChoice <= validIndices.size()) {
		const std::size_t targetIdx = validIndices[banChoice - 1];
		state.bannedIdx[targetId].push_back(targetIdx);
		return formatCharacterLabelW(state.cands[targetId][targetIdx]);
	}
	return std::nullopt;
}

void GameLogic::selectCharacter(std::size_t playerId, const SelectionState& state) {
	std::vector<std::wstring> opts;
	std::vector<std::size_t> validIndices;
	for (std::size_t i = 0; i < state.cands[playerId].size(); ++i) {
		if (std::ranges::contains(state.bannedIdx[playerId], i)) continue;
		opts.push_back(formatCharacterLabelW(state.cands[playerId][i]));
		validIndices.push_back(i);
	}
	std::size_t choice = players[playerId]->ask(L"选择你的角色：", opts, true);
	std::string charName = state.cands[playerId][validIndices[choice - 1]].first;
	players[playerId]->chooseSkinAndSet(charName);
	broadcastState();
}

void GameLogic::selectCharacterDouble(std::size_t playerId, std::vector<Character::Entry>& cands) {
	//第一轮：5选1
	std::vector<std::wstring> opts1;
	for (const auto& e : cands) opts1.push_back(formatCharacterLabelW(e));
	const std::size_t choice1 = players[playerId]->ask(L"选择你的第1个角色（5选1）：", opts1, true);
	const std::string char1 = cands[choice1 - 1].first;
	const std::string skin1 = players[playerId]->chooseSkin(char1);
	//移除已选
	cands.erase(cands.begin() + (choice1 - 1));

	//第二轮：4选1
	std::vector<std::wstring> opts2;
	for (const auto& e : cands) opts2.push_back(formatCharacterLabelW(e));
	const std::size_t choice2 = players[playerId]->ask(L"选择你的第2个角色（4选1）：", opts2, true);
	const std::string char2 = cands[choice2 - 1].first;
	const std::string skin2 = players[playerId]->chooseSkin(char2);

	//组合
	players[playerId]->setCharacter(Character::makeCombined(char1, skin1, char2, skin2));
	broadcastState();
}
void GameLogic::initPlayers(const std::vector<std::string>& chars) {
	players.clear();
	const std::string mode = unool::getServerConfig().value("mode", "normal");
	if (mode == "double") {
		//双将模式：4 个角色，前 2 个给玩家1，后 2 个给玩家2，各自 makeCombined
		if (chars.size() != 4)
			throw std::invalid_argument("双将模式指定角色时，角色数量必须为4");
		for (std::size_t i = 0; i < 2; ++i) {
			auto p = std::make_unique<Player>(i, *this,
											  Character::makeCombined(chars[i * 2], "默认", chars[i * 2 + 1], "默认"));
			players.push_back(std::move(p));
		}
	}
	else {
		//normal 模式：2 个角色，每家 1 个
		if (chars.size() != 2)
			throw std::invalid_argument("指定角色时，角色数量必须为2");
		for (std::size_t i = 0; i < 2; ++i) {
			auto p = std::make_unique<Player>(i, *this, Character::make(chars[i]));
			players.push_back(std::move(p));
		}
	}

	//拼点决定座次
	determineSeatOrder();

	resetGame();
}

bool GameLogic::runTurn() {
	// 一号位回合开始前触发轮开始
	if (getCurrentPlayerId() == getFirstPlayerId()) {
		PassiveSkill::Trigger trigger;
		trigger.player = *players[currentPlayerIndex];
		launchPassiveSkills(PassiveSkill::TriggerTime::round_begin, trigger);
	}
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
		state.players[i].characterNames = pl->getNames();
		state.players[i].skins = pl->getSkins();
		state.players[i].hp = pl->getHp();
		state.players[i].maxHp = pl->getMaxHp();
		state.players[i].marks = pl->getMarks();

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
		Player& player = *players[i];
		if (i < 2 && player.isCharInfoDirty()) {
			CharInfo info;
			info.playerIndex = i;

			//levelPart
			std::string levelPart;
			if (player.isCombined()) {
				auto levels = player.getLevels();
				levelPart = Character::to_string(levels[0]) + "+" + Character::to_string(levels[1]);
			}
			else {
				levelPart = Character::to_string(player.characterLevel());
			}

			//marksPart
			std::string marksPart;
			for (const auto& [name, count] : player.getMarks()) {
				marksPart += name + "*" + std::to_string(count) + "，";
			}
			//合并
			info.fullText =
				player.characterName() + "（" + levelPart + "）\n"
				+ "标记：" + (marksPart == "" ? "无" : marksPart) + "\n"
				+ "技能：\n" + player.getSkillsText();
			network.sendCharInfo(info);
			player.clearCharInfoDirty();
		}
	}
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

void GameLogic::launchPassiveSkills(const PassiveSkill::TriggerTime& triggerTime, const PassiveSkill::Trigger& trigger) {
	for (auto& carrier : players) {
		PassiveSkill::Trigger t = trigger;
		carrier->launchPassiveSkills(triggerTime, *this, *carrier, t);
	}
}


//返回置入弃牌堆的牌的引用
Card& GameLogic::putCardToDiscardPile(std::unique_ptr<Card> card, Card::DiscardReason reason, Player& player) {
	card->setDiscardReason(reason);
	std::cout << "[" << *card << "](" << unool::string::to_utf8(Card::to_wstring(reason))
		<< ") 进入了弃牌堆" << std::endl;
	discardPile->push_front(std::move(card));
	Card& cardRef = discardPile->front();
	{
		PassiveSkill::Trigger trigger;
		trigger.player = player;
		trigger.cards = { cardRef };
		launchPassiveSkills(PassiveSkill::TriggerTime::card_discard_end, trigger);
	}
	return cardRef;
}

std::optional<Card> GameLogic::lastCard() const {
	if (discardPile->empty()) return std::nullopt;
	else return discardPile->front();
}

void GameLogic::checkRoundEnd() {
	// 查找胜者（手牌为空）与败者
	opt_ref<Player> winner;
	opt_ref<Player> loser;
	for (auto& player : players) {
		if (player->handEmpty()) {
			winner = *player;
		}
		else {
			loser = *player;
		}
	}
	// 正常情况：一胜一败
	if (winner.has_value() && loser.has_value()) {
		Player& w = winner.value();
		Player& l = loser.value();
		w.incrementWins();
		l.incrementLosses();
		const std::size_t actualDamageValue = l.damage(l.handValue(), w);
		std::cout << w.characterName() << "对" << l.characterName()
			<< "造成" << actualDamageValue << "点伤害（败者手牌价值 " << l.handValue()
			<< " * 倍率 " << w.getDamageMultiplier() << "），"
			<< l.characterName() << "剩余" << l.getHp() << "/" << l.getMaxHp() << std::endl;
	}
	else {
		// 兜底：双方都未空手或都已空手，维持原双方各扣自己手牌value的逻辑
		for (auto& player : players) {
			std::size_t damage = player->handValue();
			player->damage(damage, std::nullopt);
			std::cout << "玩家" << player->getId() << "扣除" << damage << "点体力，剩余" << player->getHp() << "/" << player->getMaxHp() << std::endl;
		}
	}
}

void GameLogic::resetGame() {
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
		// 初始手牌：double 模式用 doubleInitHandCount，normal 用 initHandCount
		const std::string mode = unool::getServerConfig().value("mode", "normal");
		const std::string handKey = (mode == "double") ? "doubleInitHandCount" : "singleInitHandCount";
		player->draw(unool::getServerConfig()[handKey]);
		// 重置技能使用次数
		player->resetSkills();
		//取消封禁
		player->unban();
		// 重置伤害倍率
		player->setDamageMultiplier(1);
		// 清空标记
		player->clearAllMarks();
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
	launchPassiveSkills(PassiveSkill::TriggerTime::game_begin, PassiveSkill::Trigger{});
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
