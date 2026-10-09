#include "GameLogic.h"
#include "Player.h"
#include "Character.h"
#include "Card.h"
#include "UserDB.h"

#include <ranges>
#include <algorithm>
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
		players[id]->hint(std::format("你是{}号位", seatOrder[id] + 1));
	}
}

void GameLogic::initPlayers() {
	players.clear();
	//重置增分卡使用状态（每场比赛开始时）
	bonusCardUsed.fill(false);
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
	const std::size_t candidateCount = unool::getServerConfig()["rules"]["normal"].value("candidateCount", 5);
	SelectionState state;

	//获取双方用户名与角色池
	const auto& slots = network.getClientSlots();
	const std::string& user1 = slots[firstSeatId].username;
	const std::string& user2 = slots[secondSeatId].username;
	const auto& pool1 = UserDB::instance().getCharacterPool(user1);
	const auto& pool2 = UserDB::instance().getCharacterPool(user2);

	//选将扩充卡：每张扩充卡增加候选数量（非消耗品，数量即加成）
	const int expandCount1 = UserDB::instance().getItemCount(user1, "选将扩充卡");
	const int expandCount2 = UserDB::instance().getItemCount(user2, "选将扩充卡");
	const std::size_t candCount1 = candidateCount + static_cast<std::size_t>(expandCount1 > 0 ? expandCount1 : 0);
	const std::size_t candCount2 = candidateCount + static_cast<std::size_t>(expandCount2 > 0 ? expandCount2 : 0);

	//双方已抽取的角色名，用于避免重复
	std::unordered_set<std::string> pickedNames;

	//从角色池中随机抽取候选：排除exclude中已有角色名，跳过infos中不存在的名字
	auto pickFromPool = [](const std::vector<std::string>& pool, std::size_t n,
						   const std::unordered_set<std::string>& exclude) {
		std::vector<std::string> available;
		for (const auto& name : pool) {
			if (exclude.contains(name)) continue;
			if (!Character::infos.contains(name)) continue;
			available.push_back(name);
		}
		std::ranges::shuffle(available, unool::random::rng);
		std::vector<Character::Entry> result;
		result.reserve(n < available.size() ? n : available.size());
		for (std::size_t i = 0; i < n && i < available.size(); ++i) {
			result.push_back(*Character::infos.find(available[i]));
		}
		return result;
	};

	//从玩家1角色池抽取候选
	state.cands[firstSeatId] = pickFromPool(pool1, candCount1, pickedNames);
	for (const auto& e : state.cands[firstSeatId]) pickedNames.insert(e.first);

	//从玩家2角色池抽取候选（排除玩家1已抽取的角色）
	state.cands[secondSeatId] = pickFromPool(pool2, candCount2, pickedNames);
	for (const auto& e : state.cands[secondSeatId]) pickedNames.insert(e.first);

	//角色池不足时从全角色补抽（排除已抽取的，randomChooseCharacters已处理被屏蔽角色）
	auto supplement = [&](std::size_t playerId, std::size_t targetCount) {
		const std::size_t current = state.cands[playerId].size();
		if (current >= targetCount) return;
		const std::size_t needed = targetCount - current;
		auto supplemented = Character::randomChooseCharacters(needed, pickedNames);
		for (auto& e : supplemented) {
			state.cands[playerId].push_back(std::move(e));
			pickedNames.insert(state.cands[playerId].back().first);
		}
	};
	supplement(firstSeatId, candCount1);
	supplement(secondSeatId, candCount2);

	//Ban环节：玩家A先连续ban banCount次，再一次性告诉B；然后B同理，最后提示A
	auto formatBanSummary = [&](std::size_t targetId, const std::vector<std::string>& labels) -> std::string {
		if (labels.empty()) return std::format("{}没有禁用你的任何角色", players[targetId]->characterName());
		std::string msg = "对方禁用了你的角色：\n";
		for (std::size_t i = 0; i < labels.size(); ++i) {
			if (i > 0) msg += "\n";
			msg += labels[i];
		}
		return msg;
	};

	const std::size_t banCount = unool::getServerConfig()["rules"].value("banCount", 0);

	std::vector<std::string> bannedByA;
	bannedByA.reserve(banCount);
	for (std::size_t b = 0; b < banCount; ++b) {
		auto label = banPhase(firstSeatId, secondSeatId, b, banCount, state);
		if (label.has_value()) bannedByA.push_back(std::move(*label));
	}
	players[secondSeatId]->hint(formatBanSummary(secondSeatId, bannedByA));

	std::vector<std::string> bannedByB;
	bannedByB.reserve(banCount);
	for (std::size_t b = 0; b < banCount; ++b) {
		auto label = banPhase(secondSeatId, firstSeatId, b, banCount, state);
		if (label.has_value()) bannedByB.push_back(std::move(*label));
	}
	players[firstSeatId]->hint(formatBanSummary(firstSeatId, bannedByB));

	//选角环节：一号位先选，然后二号位选
	selectCharacter(firstSeatId, state);
	selectCharacter(secondSeatId, state);

	//增分卡使用环节：后手先选，先手随后选
	askBonusCard(secondSeatId);
	askBonusCard(firstSeatId);
}

void GameLogic::initPlayersDouble(std::size_t firstSeatId, std::size_t secondSeatId) {
	//双将模式：无ban，抽 doubleCandidateCount*2 个候选平分各 doubleCandidateCount 个
	const std::size_t doubleCandidateCount = unool::getServerConfig()["rules"]["double"].value("candidateCount", 5);
	auto allChars = Character::randomChooseCharacters(doubleCandidateCount * 2);

	std::vector<Character::Entry> cands1(
		allChars.begin(), allChars.begin() + doubleCandidateCount);
	std::vector<Character::Entry> cands2(
		allChars.begin() + doubleCandidateCount, allChars.end());

	//提取双方候选名集合，供"换一批"时排除对方角色
	std::unordered_set<std::string> names1, names2;
	for (const auto& e : cands1) names1.insert(e.first);
	for (const auto& e : cands2) names2.insert(e.first);

	//按座次每家连续选完2个再下一家
	selectCharacterDouble(firstSeatId, cands1, names2);
	selectCharacterDouble(secondSeatId, cands2, names1);

	//增分卡使用环节：后手先选，先手随后选
	askBonusCard(secondSeatId);
	askBonusCard(firstSeatId);
}

void GameLogic::askBonusCard(std::size_t playerId) {
	const auto& slots = network.getClientSlots();
	const std::string& username = slots[playerId].username;
	if (UserDB::instance().getItemCount(username, "增分卡") <= 0) return;
	std::size_t choice = players[playerId]->ask("是否使用增分卡？", { "使用", "不使用" }, false);
	if (choice == 1) {
		UserDB::instance().useItem(username, "增分卡");
		bonusCardUsed[playerId] = true;
		players[playerId]->hint("已使用增分卡，胜利时积分翻倍！");
	}
}

std::size_t GameLogic::getSeatPlayerId(std::size_t seat) const {
	for (const auto& [playerId, seatNumber] : seatOrder | std::views::enumerate) {
		if (seatNumber == seat) return playerId;
	}
	throw std::logic_error("座位号无效");
}

std::string GameLogic::formatCharacterLabel(const Character::Entry& entry) {
	const Character::Info info = entry.second;
	std::string label = std::format(
		"{}（{}）体力：{}",
		entry.first,
		info.level, info.hp
	);
	if (info.maxHp != info.hp) {
		label += std::format("/{}", info.maxHp);
	}
	return label;
}



std::optional<std::string> GameLogic::banPhase(std::size_t bannerId, std::size_t targetId, std::size_t banIndex, std::size_t banCount, SelectionState& state) {
	std::vector<std::string> banOpts;
	std::vector<std::size_t> validIndices;
	for (std::size_t i = 0; i < state.cands[targetId].size(); ++i) {
		const bool alreadyBanned = std::ranges::contains(state.bannedIdx[targetId], i);
		if (alreadyBanned) continue;
		banOpts.push_back(formatCharacterLabel(state.cands[targetId][i]));
		validIndices.push_back(i);
	}
	//候选池<=1时无需再ban
	if (validIndices.size() <= 1) return std::nullopt;
	const std::string title = std::format("禁用对方的角色（{}/{}）：", banIndex + 1, banCount);
	std::size_t banChoice = players[bannerId]->ask(title, banOpts, false, std::chrono::seconds(60));
	if (banChoice > 0 && banChoice <= validIndices.size()) {
		const std::size_t targetIdx = validIndices[banChoice - 1];
		state.bannedIdx[targetId].push_back(targetIdx);
		return formatCharacterLabel(state.cands[targetId][targetIdx]);
	}
	return std::nullopt;
}

void GameLogic::selectCharacter(std::size_t playerId, const SelectionState& state) {
	//选将时只允许选择用户 ownedCharacters 中已解锁的角色
	const auto& slots = network.getClientSlots();
	const std::string& username = slots[playerId].username;
	const auto& owned = UserDB::instance().getOwnedCharacters(username);

	std::vector<std::string> opts;
	std::vector<std::size_t> validIndices;
	for (std::size_t i = 0; i < state.cands[playerId].size(); ++i) {
		if (std::ranges::contains(state.bannedIdx[playerId], i)) continue;
		const std::string& charName = state.cands[playerId][i].first;
		if (!owned.contains(charName)) continue; //未解锁的角色不可选
		opts.push_back(formatCharacterLabel(state.cands[playerId][i]));
		validIndices.push_back(i);
	}
	//兜底：若所有候选均未解锁（理论上不应发生），则允许全部非ban候选
	if (opts.empty()) {
		for (std::size_t i = 0; i < state.cands[playerId].size(); ++i) {
			if (std::ranges::contains(state.bannedIdx[playerId], i)) continue;
			opts.push_back(formatCharacterLabel(state.cands[playerId][i]));
			validIndices.push_back(i);
		}
	}
	std::size_t choice = players[playerId]->ask("选择你的角色：", opts, true);
	std::string charName = state.cands[playerId][validIndices[choice - 1]].first;
	players[playerId]->chooseSkinAndSet(charName);
	broadcastState();
}

void GameLogic::selectCharacterDouble(std::size_t playerId, std::vector<Character::Entry>& cands,
									  const std::unordered_set<std::string>& opponentNames) {
	Player& player = *players[playerId];
	const std::size_t candidateCount = cands.size();

	//本方所有出现过的候选角色名（含历史批次），确保换一批不重复
	std::unordered_set<std::string> seenNames;
	for (const auto& e : cands) seenNames.insert(e.first);
	bool hasSwapped = false;

	//"换一批"处理：扣除5积分，生成全新候选（排除对方+本方历史），重置选角进度
	auto doSwap = [&]() -> bool {
		if (hasSwapped) {
			player.hint("每局限换一批一次");
			return false;
		}
		if (!network.trySpendPoints(playerId, 5)) {
			player.hint("积分不足，无法换一批");
			return false;
		}
		std::unordered_set<std::string> exclude = opponentNames;
		exclude.insert(seenNames.begin(), seenNames.end());
		cands = Character::randomChooseCharacters(candidateCount, exclude);
		for (const auto& e : cands) seenNames.insert(e.first);
		hasSwapped = true;
		player.hint("换一批成功！");
		broadcastState();
		return true;
	};

	std::string char1, skin1;
	bool round1Done = false;

	while (true) {
		//第一轮：N选1
		if (!round1Done) {
			std::vector<std::string> opts1;
			for (const auto& e : cands) opts1.push_back(formatCharacterLabel(e));
			const std::string title1 = hasSwapped
				? std::format("选择你的第1个角色（{}选1）：", opts1.size())
				: std::format("选择你的第1个角色（{}选1，按0消耗5积分换一批，仅一次）：", opts1.size());
			const std::size_t choice1 = player.ask(title1, opts1, false);
			if (choice1 == 0) {
				if (doSwap()) continue;
				continue;
			}
			char1 = cands[choice1 - 1].first;
			skin1 = player.chooseSkin(char1);
			cands.erase(cands.begin() + (choice1 - 1));
			round1Done = true;
		}

		//第二轮：M选1
		std::vector<std::string> opts2;
		for (const auto& e : cands) opts2.push_back(formatCharacterLabel(e));
		const std::string title2 = hasSwapped
			? std::format("选择你的第2个角色（{}选1）：", opts2.size())
			: std::format("选择你的第2个角色（{}选1，按0消耗5积分换一批，仅一次）：", opts2.size());
		const std::size_t choice2 = player.ask(title2, opts2, false);
		if (choice2 == 0) {
			if (doSwap()) {
				//换一批后丢弃第一轮选择，重新从第一轮选起
				round1Done = false;
				char1.clear();
				skin1.clear();
			}
			continue;
		}
		const std::string char2 = cands[choice2 - 1].first;
		const std::string skin2 = player.chooseSkin(char2);

		//组合
		player.setCharacter(Character::makeCombined(char1, skin1, char2, skin2));
		broadcastState();
		return;
	}
}
void GameLogic::initPlayers(const std::vector<std::string>& chars) {
	players.clear();
	bonusCardUsed.fill(false);
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
	std::println("玩家{}的回合", getCurrentPlayerId());
	bool gameEnded = currentPlayerTurn();

	if (!gameEnded) {
		altPlayer();
	}

	broadcastState();
	return gameEnded;
}

void GameLogic::broadcastState() {
	for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
		if (!network.isClientConnected(i)) continue;
		GameState state = packStateForPlayer(i);
		bool ok = network.sendGameStateToClient(i, state);
		if (!ok) {
			std::println("[Warning] broadcastState发给玩家{}失败！", i);
		}
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
				levelPart = std::format("{}+{}", levels[0], levels[1]);
			}
			else {
				levelPart = std::format("{}", player.characterLevel());
			}

			//marksPart
			std::string marksPart = "标记：";
			for (const auto& [name, count] : player.getMarks()) {
				marksPart += std::format("{}*{}，", name, count);
			}

			//skillsPart
			std::string skillsPart;
			if (!player.isSealed()) skillsPart = "技能：\n";
			else skillsPart = std::format("技能（已被封印，{}回合后解除）：\n", player.getSealed());
			skillsPart += player.getSkillsText();

			//合并
			info.fullText = std::format(
				"{}（{}）\n{}\n{}",
				player.characterName(), levelPart, marksPart, skillsPart
			);

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
	std::println("[{}]({}) 进入了弃牌堆", *card, reason);
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
		std::println("{}对{}造成{}点伤害（败者手牌价值 {} * 倍率 {}），{}剩余{}/{}", w.characterName(), l.characterName(), actualDamageValue, l.handValue(), w.getDamageMultiplier(), l.characterName(), l.getHp(), l.getMaxHp());
	}
	else {
		// 兜底：双方都未空手或都已空手，维持原双方各扣自己手牌value的逻辑
		for (auto& player : players) {
			std::size_t damage = player->handValue();
			player->damage(damage, std::nullopt);
			std::println("玩家{}扣除{}点体力，剩余{}/{}", player->getId(), damage, player->getHp(), player->getMaxHp());
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
		const std::string handKey = (mode == "double") ? "double" : "normal";
		player->draw(unool::getServerConfig()["rules"][handKey]["initHandCount"]);
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
	std::println("[Server] 新一局开始！玩家{}先手", currentPlayerIndex);
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
