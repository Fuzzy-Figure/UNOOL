#include "Socket.h"
#include "AccountProtocol.h"
#include "UserDB.h"
#include "GameState.h"
#include "ShopData.h"

#include <thread>
#include <algorithm>
#include <array>
#include <utility>

ServerNetwork::~ServerNetwork() {
	disconnect();
}

bool ServerNetwork::start(unsigned short port) {
	listener = std::make_unique<sf::TcpListener>();
	sf::Socket::Status listenStatus = listener->listen(port);

	if (listenStatus != sf::Socket::Status::Done) {
		std::println(stderr, "[ServerNetwork] 启动失败");
		listener.reset();
		return false;
	}

	listener->setBlocking(false);
	selector.add(*listener);

	std::println("[ServerNetwork] 已启动，监听端口：{}", port);
	return true;
}

void ServerNetwork::disconnect() {
	for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
		removeClient(i);
	}
	if (pendingSocket) {
		selector.remove(*pendingSocket);
		pendingSocket->disconnect();
		pendingSocket.reset();
	}
	if (listener) {
		listener->close();
		listener.reset();
	}
	serverReady = false;
	std::println("[ServerNetwork] 已断开所有连接");
}

void ServerNetwork::removeClient(std::size_t clientIndex) {
	if (clientIndex >= unool::MAX_PLAYERS) return;
	if (clientSockets[clientIndex]) {
		selector.remove(*clientSockets[clientIndex]);
		clientSockets[clientIndex]->disconnect();
		clientSockets[clientIndex].reset();
	}
	clientSlots_[clientIndex].loggedIn = false;
	clientSlots_[clientIndex].disconnected = true;
	// 保留 username，供重连时识别身份
	//重置心跳计时，避免新连接顶替进来后因残留旧值被误判超时
	lastRecvClocks[clientIndex].restart();
}

std::size_t ServerNetwork::getClientCount() const {
	std::size_t count = 0;
	for (const auto& s : clientSockets) {
		if (s) ++count;
	}
	return count;
}

bool ServerNetwork::isClientConnected(std::size_t clientIndex) const {
	return clientIndex < unool::MAX_PLAYERS && clientSockets[clientIndex] != nullptr;
}

bool ServerNetwork::isClientLoggedIn(std::size_t clientIndex) const {
	return isClientConnected(clientIndex) && clientSlots_[clientIndex].loggedIn;
}

bool ServerNetwork::trySpendPoints(std::size_t clientIndex, int amount) {
	if (clientIndex >= clientSlots_.size()) return false;
	const std::string& username = clientSlots_[clientIndex].username;
	if (!UserDB::instance().trySpendPoints(username, amount)) return false;
	clientSlots_[clientIndex].points -= amount;
	return true;
}

void ServerNetwork::update() {
	if (selector.wait(sf::milliseconds(10))) {
		handleNewConnections();
		handleClientPackets();
		handlePendingSocket();
	}
	sendHeartbeat();
	checkTimeouts();
}

void ServerNetwork::handleNewConnections() {
	if (selector.isReady(*listener)) {
		std::unique_ptr<sf::TcpSocket> newSocket = std::make_unique<sf::TcpSocket>();
		if (listener->accept(*newSocket) == sf::Socket::Status::Done) {
			//检查是否有 disconnected 槽位（等待重连）
			bool hasDisconnectedSlot = false;
			for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
				if (clientSlots_[i].disconnected) {
					hasDisconnectedSlot = true;
					break;
				}
			}

			if (hasDisconnectedSlot && !pendingSocket) {
				//有掉线槽位：收下 socket 作为 pendingSocket，等 LoginRequest 识别 username 匹配后再分配槽位
				newSocket->setBlocking(false);
				selector.add(*newSocket);
				pendingSocket = std::move(newSocket);
				std::println("[ServerNetwork] 检测到掉线槽位，新连接进入待登录队列，等待 LoginRequest 匹配 username...");
			}
			else {
				//无掉线槽位或 pendingSocket 已被占用：按原逻辑分配第一个空槽位
				std::size_t newPlayerId = unool::MAX_PLAYERS;
				for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
					if (!clientSockets[i]) {
						newPlayerId = i;
						break;
					}
				}

				if (newPlayerId < unool::MAX_PLAYERS) {
					newSocket->setBlocking(false);
					selector.add(*newSocket);
					clientSockets[newPlayerId] = std::move(newSocket);

					sendConnectionInfo(newPlayerId);

					std::println("[ServerNetwork] 客户端{}已连接，等待登录...", newPlayerId);
				}
				else if (!pendingSocket) {
					//槽位已满：先收下，等收到 LoginRequest 后识别重连意图顶替旧槽位
					newSocket->setBlocking(false);
					selector.add(*newSocket);
					pendingSocket = std::move(newSocket);
					std::println("[ServerNetwork] 槽位已满，新连接进入待登录队列，等待重连识别...");
				}
				else {
					//pendingSocket 已被占用：直接拒绝
					newSocket->disconnect();
					std::println("[ServerNetwork] 新连接被拒（pendingSocket 已占用）");
				}
			}
		}
	}
}

void ServerNetwork::handleClientPackets() {
	for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
		if (!clientSockets[i]) continue;

		sf::TcpSocket& socket = *clientSockets[i];
		if (selector.isReady(socket)) {
			sf::Packet packet;
			sf::Socket::Status status = socket.receive(packet);
			if (status == sf::Socket::Status::Done) {
				//收到任何包都说明客户端活着，重置心跳计时
				lastRecvClocks[i].restart();
				// 先 peek 类型，账号包内部处理，其他包入队列
				sf::Packet peeked = packet;
				int msgType;
				if (peeked >> msgType) {
					if (msgType == static_cast<int>(MessageType::Heartbeat)) {
						//心跳回包：不入队，已通过 lastRecvClocks 重置完成
					}
					else if (msgType == static_cast<int>(MessageType::RegisterRequest) ||
							 msgType == static_cast<int>(MessageType::LoginRequest) ||
							 msgType == static_cast<int>(MessageType::CheckUsernameRequest)) {
						handleAccountPacket(i, static_cast<MessageType>(msgType), std::move(packet));
					}
					else if (msgType == static_cast<int>(MessageType::ShopPurchase)) {
						//商城购买请求：内联处理，不入队
						int discardedType;
						std::string purchaseData;
						if (packet >> discardedType >> purchaseData) {
							handleShopPurchase(i, purchaseData);
						}
					}
					else {
						receivedPackets.push(packet);
					}
				}
			}
			else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
				std::println("[ServerNetwork] 客户端{} 断开连接（status={}）", i, static_cast<int>(status));
				removeClient(i);
			}
		}
	}
}

void ServerNetwork::handlePendingSocket() {
	//处理待登录队列的 socket：识别重连意图后顶替同账号的旧连接
	if (pendingSocket && selector.isReady(*pendingSocket)) {
		sf::Packet packet;
		sf::Socket::Status status = pendingSocket->receive(packet);
		if (status == sf::Socket::Status::Done) {
			sf::Packet peek = packet;
			int msgType;
			std::string reqUsername, reqPassword;
			if (peek >> msgType
				&& msgType == static_cast<int>(MessageType::LoginRequest)
				&& peek >> reqUsername >> reqPassword) {
				//找匹配的 loggedIn 或 disconnected 槽位（同账号）
				std::size_t targetIdx = unool::MAX_PLAYERS;
				for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
					if ((clientSlots_[i].loggedIn || clientSlots_[i].disconnected)
						&& clientSlots_[i].username == reqUsername) {
						targetIdx = i;
						break;
					}
				}
				if (targetIdx < unool::MAX_PLAYERS) {
					if (clientSlots_[targetIdx].loggedIn) {
						//旧连接还活着：踢旧让新顶替
						std::println("[ServerNetwork] 检测到重连意图：踢掉旧连接 {}（账号 {}），让新连接顶替",
									 targetIdx, reqUsername);
						removeClient(targetIdx);  //selector.remove 旧 socket + reset + 标记 disconnected=true
					}
					//disconnected 槽位：socket 已空，不需要 removeClient，直接 move
					//pendingSocket 仍在 selector 中，直接 move 到 targetIdx 槽位
					clientSockets[targetIdx] = std::move(pendingSocket);
					sendConnectionInfo(targetIdx);  //让客户端拿到正确的 playerId
					handleAccountPacket(targetIdx, MessageType::LoginRequest, std::move(packet));
				}
				else {
					//没找到匹配的 loggedIn/disconnected 槽位：尝试找空槽位（非 disconnected）分配之
					std::size_t emptyIdx = unool::MAX_PLAYERS;
					for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
						if (!clientSockets[i] && !clientSlots_[i].disconnected) {
							emptyIdx = i;
							break;
						}
					}
					if (emptyIdx < unool::MAX_PLAYERS) {
						//有非 disconnected 的空槽位：分配之（覆盖首次登录新账号场景）
						clientSockets[emptyIdx] = std::move(pendingSocket);
						sendConnectionInfo(emptyIdx);
						handleAccountPacket(emptyIdx, MessageType::LoginRequest, std::move(packet));
					}
					else {
						//无匹配也无空槽位：拒绝（防止陌生人顶替）
						auto resp = AccountProtocol::makeAccountResponse(
							MessageType::LoginResponse, false,
							"服务器已满，且无匹配的重连槽位");
						if (pendingSocket->send(resp) != sf::Socket::Status::Done) {
							std::println("[ServerNetwork] 待登录队列的拒绝响应发送失败（账号 {}）", reqUsername);
						}
						selector.remove(*pendingSocket);
						pendingSocket->disconnect();
						pendingSocket.reset();
						std::println("[ServerNetwork] 待登录队列的连接被拒绝（无匹配槽位，账号 {}）", reqUsername);
					}
				}
			}
			else {
				//首包非 LoginRequest：拒绝
				selector.remove(*pendingSocket);
				pendingSocket->disconnect();
				pendingSocket.reset();
				std::println("[ServerNetwork] 待登录队列的连接被拒绝（首包非登录请求）");
			}
		}
		else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
			std::println("[ServerNetwork] 待登录队列的 socket 断开（status={}）", static_cast<int>(status));
			selector.remove(*pendingSocket);
			pendingSocket->disconnect();
			pendingSocket.reset();
		}
	}
}

void ServerNetwork::sendHeartbeat() {
	//心跳机制：每秒向所有 loggedIn 客户端发 Heartbeat
	if (heartbeatClock.getElapsedTime().asSeconds() >= 1.0f) {
		heartbeatClock.restart();
		sf::Packet hb;
		hb << static_cast<int>(MessageType::Heartbeat);
		for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
			if (isClientLoggedIn(i)) {
				sendPacketToClient(i, hb);
			}
		}
	}
}

void ServerNetwork::checkTimeouts() {
	//超时检测：loggedIn 客户端连续 3 秒未收到任何回包视为掉线
	for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
		if (isClientLoggedIn(i) && lastRecvClocks[i].getElapsedTime().asSeconds() >= 3.0f) {
			std::println("[ServerNetwork] 客户端{}（账号 {}）已 3 秒未响应，判定为掉线，主动断开",
						 i, clientSlots_[i].username);
			removeClient(i);
		}
	}
}

void ServerNetwork::handleAccountPacket(std::size_t clientIdx, MessageType type, sf::Packet packet) {
	if (!isClientConnected(clientIdx)) return;

	// 跳过 msgType（调用方已在 update() 中 peek 过，但 packet 内部仍保留完整数据）
	int discardedMsgType;
	packet >> discardedMsgType;

	if (type == MessageType::RegisterRequest) {
		auto req = AccountProtocol::parseAccountRequest(packet);
		if (!req) return;
		std::string errMsg;
		bool ok = UserDB::instance().registerUser(req->username, req->password, errMsg);
		std::string msg = ok ? "注册成功" : errMsg;
		auto resp = AccountProtocol::makeAccountResponse(MessageType::RegisterResponse, ok, msg);
		sendPacketToClient(clientIdx, resp);
	}
	else if (type == MessageType::LoginRequest) {
		auto req = AccountProtocol::parseAccountRequest(packet);
		if (!req) return;
		std::string errMsg;
		auto userInfo = UserDB::instance().login(req->username, req->password, errMsg);

		bool ok = userInfo.has_value();

		// 检查该槽位是否是掉线玩家的：若有残留 username 且不匹配当前登录账号，拒绝占用
		if (ok && clientSlots_[clientIdx].disconnected && !clientSlots_[clientIdx].username.empty()
			&& clientSlots_[clientIdx].username != req->username) {
			ok = false;
			errMsg = "该座位已被其他玩家占用，请等待";
			//清理该 socket，避免占住槽位但未登录成功导致后续重连连锁失败
			if (clientSockets[clientIdx]) {
				selector.remove(*clientSockets[clientIdx]);
				clientSockets[clientIdx]->disconnect();
				clientSockets[clientIdx].reset();
			}
		}

		// 检查另一端是否已用同一账号登录
		if (ok) {
			std::size_t other = 1 - clientIdx;
			if (other < unool::MAX_PLAYERS
				&& clientSlots_[other].loggedIn
				&& clientSlots_[other].username == req->username) {
				ok = false;
				errMsg = "该账号已在另一端登录";
			}
		}

		int pts = ok ? userInfo->points : 0;
		int w = ok ? userInfo->wins : 0;
		int l = ok ? userInfo->losses : 0;
		std::string msg = ok ? "登录成功" : errMsg;

		auto resp = AccountProtocol::makeAccountResponse(MessageType::LoginResponse, ok, msg, pts, w, l);
		sendPacketToClient(clientIdx, resp);

		if (ok) {
			const bool isReconnect = clientSlots_[clientIdx].disconnected;
			clientSlots_[clientIdx].loggedIn = true;
			clientSlots_[clientIdx].disconnected = false;
			clientSlots_[clientIdx].username = req->username;
			clientSlots_[clientIdx].points = pts;
			clientSlots_[clientIdx].wins = w;
			clientSlots_[clientIdx].losses = l;
			//登录成功时重置心跳计时，避免重连场景下因 removeClient 至 login 之间的耗时被误判超时
			lastRecvClocks[clientIdx].restart();

			std::println("[ServerNetwork] 客户端{} {}: {}（积分 {}）", clientIdx, (isReconnect ? "重连" : "登录"), req->username, pts);
			// 两玩家都登录后开局（重连顶替场景下 serverReady 已为 true，不重复发 GameStart）
			if (clientSlots_[0].loggedIn && clientSlots_[1].loggedIn && !serverReady) {
				serverReady = true;
				sendGameStart();
				std::println("[ServerNetwork] 两个客户端都已登录，游戏开始");
			}
		}
	}
	else if (type == MessageType::CheckUsernameRequest) {
		auto username = AccountProtocol::parseCheckUsernameRequest(packet);
		if (!username) return;
		bool exists = UserDB::instance().exists(*username);
		auto resp = AccountProtocol::makeCheckUsernameResponse(exists);
		sendPacketToClient(clientIdx, resp);
	}
}

std::optional<ClientInput> ServerNetwork::receiveClientInput() {
	while (!receivedPackets.empty()) {
		sf::Packet packet = receivedPackets.front();
		receivedPackets.pop();

		int msgType;
		if (packet >> msgType && msgType == static_cast<int>(MessageType::ClientInput)) {
			ClientInput input;
			int keyCode;
			if (packet >> keyCode >> input.playerId >> input.selectedIndex) {
				input.key = static_cast<sf::Keyboard::Scancode>(keyCode);
				return input;
			}
		}
	}
	return std::nullopt;
}

bool ServerNetwork::sendGameState(const GameState& state) {
	sf::Packet packet;
	packet << static_cast<int>(MessageType::GameState);
	packet << state;
	return sendPacketToAll(packet);
}

bool ServerNetwork::sendGameStateToClient(std::size_t clientIndex, const GameState& state) {
	if (!isClientConnected(clientIndex)) return false;

	sf::Packet packet;
	packet << static_cast<int>(MessageType::GameState);
	packet << state;
	return sendPacketToClient(clientIndex, packet);
}

bool ServerNetwork::sendConnectionInfo(std::size_t newPlayerId) {
	if (!isClientConnected(newPlayerId)) return false;

	sf::Packet packet;
	packet << static_cast<int>(MessageType::ConnectionInfo) << newPlayerId;
	return sendPacketToClient(newPlayerId, packet);
}

bool ServerNetwork::sendGameStart() {
	sf::Packet packet;
	packet << static_cast<int>(MessageType::GameStart);
	return sendPacketToAll(packet);
}

bool ServerNetwork::sendGameEnd(std::optional<std::size_t> winnerId) {
	sf::Packet packet;
	const bool hasWinner = winnerId.has_value();
	packet << static_cast<int>(MessageType::GameEnd) << hasWinner;
	if (hasWinner) packet << winnerId.value();
	return sendPacketToAll(packet);
}

bool ServerNetwork::sendCharInfo(const CharInfo& info) {
	sf::Packet packet;
	packet << static_cast<int>(MessageType::CharInfo) << info;
	return sendPacketToAll(packet);
}

bool ServerNetwork::sendPlayerChoice(std::size_t clientIndex,
									 const std::string& title,
									 const std::vector<std::string>& options,
									 bool forced,
									 const std::string& errorMsg,
									 std::optional<std::size_t> timeoutMs,
									 std::size_t currentPage,
									 std::size_t totalPages) {
	if (!isClientConnected(clientIndex)) return false;

	sf::Packet packet;
	packet << std::to_underlying(MessageType::Choice);
	packet << title;
	packet << options.size();
	for (const auto& option : options) {
		packet << option;
	}
	packet << forced;
	packet << errorMsg;
	bool hasTimeout = timeoutMs.has_value();
	packet << hasTimeout;
	if (hasTimeout) {
		packet << timeoutMs.value();
	}
	packet << currentPage;
	packet << totalPages;

	return sendPacketToClient(clientIndex, packet);
}

bool ServerNetwork::clearPlayerChoice(std::size_t clientIndex) {
	return sendPlayerChoice(clientIndex, "", {}, false);
}

bool ServerNetwork::sendPacketToAll(sf::Packet& packet) {
	bool allOk = true;
	for (std::size_t i = 0; i < unool::MAX_PLAYERS; ++i) {
		if (clientSockets[i] && !sendPacketToClient(i, packet)) {
			allOk = false;
		}
	}
	return allOk;
}

bool ServerNetwork::sendPacketToClient(std::size_t clientIndex, sf::Packet& packet) {
	if (!isClientConnected(clientIndex)) return false;

	sf::TcpSocket& socket = *clientSockets[clientIndex];
	for (int attempt = 0; attempt < 3; ++attempt) {
		sf::Socket::Status status = socket.send(packet);
		if (status == sf::Socket::Status::Done) return true;
		if (status == sf::Socket::Status::Disconnected) {
			std::println("[ServerNetwork] 发送时检测到客户端{} 断开", clientIndex);
			removeClient(clientIndex);
			return false;
		}
		if (attempt < 2) std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	return false;
}

bool ServerNetwork::sendShopResult(std::size_t clientIndex, const std::string& resultJson) {
	if (!isClientConnected(clientIndex)) return false;
	sf::Packet packet;
	packet << static_cast<int>(MessageType::ShopResult);
	packet << resultJson;
	return sendPacketToClient(clientIndex, packet);
}

bool ServerNetwork::sendShopData(std::size_t clientIndex, const std::string& username) {
	if (!isClientConnected(clientIndex)) return false;

	// 从 UserDB 获取 UserInfo
	const UserInfo& userInfo = UserDB::instance().getUserInfo(username);
	// 是否集齐 ABCDF 所有角色（决定 S 档角色可购买性）
	const bool allNonSCollected = UserDB::instance().hasAllNonSRank(username);

	ShopData shopData;
	shopData.points = userInfo.points;

	// 遍历 Character::infos，跳过"白板"，为每个角色构建 ShopCharacterInfo
	for (const auto& [name, info] : Character::infos) {
		if (name == "白板") continue;

		ShopCharacterInfo charInfo;
		charInfo.name = name;
		charInfo.level = info.level;
		charInfo.unlocked = userInfo.ownedCharacters.contains(name);
		charInfo.price = Character::getCharacterPrice(name);
		// S 档角色：集齐 ABCDF 后售价 488，否则不可购买（-1）
		if (info.level == Character::Level::S && allNonSCollected) {
			charInfo.price = 488;
		}

		// 获取皮肤列表，跳过"默认"皮肤
		auto skins = Character::getSkins(name);
		for (const auto& skinName : skins) {
			if (skinName == "默认") continue;

			ShopSkinInfo skinInfo;
			skinInfo.name = skinName;
			// 检查皮肤是否已解锁（从 ownedCharacters[charName] 的皮肤集合中查找）
			auto charIt = userInfo.ownedCharacters.find(name);
			if (charIt != userInfo.ownedCharacters.end()) {
				skinInfo.unlocked = charIt->second.contains(skinName);
			}
			skinInfo.price = Character::getSkinPrice(name, skinName);
			skinInfo.quality = Character::to_string(Character::getSkinQuality(name, skinName));
			charInfo.skins.push_back(std::move(skinInfo));
		}

		shopData.characters.push_back(std::move(charInfo));
	}

	// 按 Level F→S 排序（F=0, D=1, C=2, B=3, A=4, S=5）
	std::ranges::sort(shopData.characters, [](const ShopCharacterInfo& a, const ShopCharacterInfo& b) {
		return std::to_underlying(a.level) < std::to_underlying(b.level);
	});

	// 构建道具列表
	// 选将扩充卡
	{
		ShopItemInfo itemInfo;
		itemInfo.name = "选将扩充卡";
		itemInfo.count = UserDB::instance().getItemCount(username, "选将扩充卡");
		itemInfo.price = (itemInfo.count < 5) ? (88 + 100 * itemInfo.count) : -1;
		itemInfo.available = itemInfo.count < 5;
		shopData.items.push_back(std::move(itemInfo));
	}
	// 增分卡
	{
		ShopItemInfo itemInfo;
		itemInfo.name = "增分卡";
		itemInfo.count = UserDB::instance().getItemCount(username, "增分卡");
		itemInfo.price = 10;
		itemInfo.available = true;
		shopData.items.push_back(std::move(itemInfo));
	}

	// 序列化 ShopData 为 JSON 并通过 sf::Packet 发送
	nlohmann::json j = shopData;
	std::string jsonStr = j.dump();

	sf::Packet packet;
	packet << static_cast<int>(MessageType::ShopData);
	packet << jsonStr;

	std::println("[ServerNetwork] 发送商城数据给客户端{}（账号 {}）：积分{}，角色{}个，道具{}个",
				 clientIndex, username, shopData.points, shopData.characters.size(), shopData.items.size());

	return sendPacketToClient(clientIndex, packet);
}

void ServerNetwork::handleShopPurchase(std::size_t clientIndex, const std::string& purchaseData) {
	if (!isClientConnected(clientIndex)) return;

	const std::string& username = clientSlots_[clientIndex].username;

	// 解析购买请求 JSON
	ShopPurchaseRequest req;
	try {
		nlohmann::json j = nlohmann::json::parse(purchaseData);
		req = j.get<ShopPurchaseRequest>();
	}
	catch (const std::exception& e) {
		ShopPurchaseResult result;
		result.ok = false;
		result.msg = std::format("购买数据解析失败：{}", e.what());
		result.points = UserDB::instance().getPoints(username);
		nlohmann::json rj = result;
		sendShopResult(clientIndex, rj.dump());
		return;
	}

	// type="request"：客户端请求刷新商城数据，直接回送 ShopData
	if (req.type == "request") {
		sendShopData(clientIndex, username);
		return;
	}

	// 根据类型调用 UserDB 的 purchase 方法（价格由服务端计算，不信任客户端）
	bool ok = false;
	std::string errMsg;
	std::string sUnlockMsg;	// 集齐档位触发 S 档解锁的附加消息

	if (req.type == "character") {
		// 判断是否为 S 档角色
		auto charIt = Character::infos.find(req.name);
		const bool isSRank = (charIt != Character::infos.end()
							 && charIt->second.level == Character::Level::S);

		int price = -1;
		if (isSRank) {
			// S 档角色：需集齐 ABCDF 所有角色，售价 488
			if (UserDB::instance().hasAllNonSRank(username)) {
				price = 488;
			} else {
				errMsg = "需集齐 ABCDF 所有角色后才可购买 S 档角色";
			}
		} else {
			price = Character::getCharacterPrice(req.name);
		}

		if (price >= 0 && errMsg.empty()) {
			// 购买前记录各非 S 档集齐状态，用于判断本次购买是否新集齐某档
			const std::array<std::pair<Character::Level, bool>, 5> beforeStates = {{
				{Character::Level::F, UserDB::instance().hasAllCharactersOfLevel(username, Character::Level::F)},
				{Character::Level::D, UserDB::instance().hasAllCharactersOfLevel(username, Character::Level::D)},
				{Character::Level::C, UserDB::instance().hasAllCharactersOfLevel(username, Character::Level::C)},
				{Character::Level::B, UserDB::instance().hasAllCharactersOfLevel(username, Character::Level::B)},
				{Character::Level::A, UserDB::instance().hasAllCharactersOfLevel(username, Character::Level::A)},
			}};

			ok = UserDB::instance().purchaseCharacter(username, req.name, price);
			if (!ok) {
				errMsg = "积分不足或购买失败";
			} else {
				// 检查是否新集齐某档，触发 S 档随机解锁
				bool newCollected = false;
				for (const auto& [lv, before] : beforeStates) {
					if (!before && UserDB::instance().hasAllCharactersOfLevel(username, lv)) {
						newCollected = true;
						break;
					}
				}
				if (newCollected) {
					if (auto unlocked = UserDB::instance().checkAndUnlockSRank(username)) {
						sUnlockMsg = std::format("，集齐档位解锁 S 档角色：{}", *unlocked);
					}
				}
			}
		}
	}
	else if (req.type == "skin") {
		int price = Character::getSkinPrice(req.charName, req.skinName);
		if (price < 0) {
			errMsg = "该皮肤不可购买";
		}
		else {
			ok = UserDB::instance().purchaseSkin(username, req.charName, req.skinName, price);
			if (!ok) errMsg = "积分不足或购买失败";
		}
	}
	else if (req.type == "item") {
		int price = 0;
		int count = UserDB::instance().getItemCount(username, req.name);

		if (req.name == "选将扩充卡") {
			if (count >= 5) {
				errMsg = "已达购买上限";
			}
			else {
				price = 88 + 100 * count;
			}
		}
		else if (req.name == "增分卡") {
			price = 10;
		}
		else {
			errMsg = "未知道具";
		}

		if (errMsg.empty()) {
			ok = UserDB::instance().purchaseItem(username, req.name, price);
			if (!ok) errMsg = "积分不足";
		}
	}
	else {
		errMsg = std::format("未知购买类型：{}", req.type);
	}

	// 构建购买结果并发送
	ShopPurchaseResult result;
	result.ok = ok;
	result.msg = ok ? ("购买成功" + sUnlockMsg) : errMsg;
	result.points = UserDB::instance().getPoints(username);

	// 购买成功时同步更新 ClientSlot.points
	if (ok) {
		clientSlots_[clientIndex].points = result.points;
	}

	nlohmann::json rj = result;
	sendShopResult(clientIndex, rj.dump());

	std::println("[ServerNetwork] 客户端{}（账号 {}）购买 {}：{}（剩余积分 {}）",
				 clientIndex, username, req.type, (ok ? "成功" : "失败"), result.points);

	// 购买成功后自动刷新商城数据给该客户端
	if (ok) {
		sendShopData(clientIndex, username);
	}
}

ClientNetwork::~ClientNetwork() {
	disconnect();
}

bool ClientNetwork::connect(const std::string& ip, unsigned short port) {
	serverIp = ip;
	serverPort = port;

	socket = std::make_unique<sf::TcpSocket>();

	sf::Socket::Status connectStatus = socket->connect(sf::IpAddress::fromString(ip).value(), port, sf::seconds(3));

	if (connectStatus != sf::Socket::Status::Done) {
		std::println(stderr, "[ClientNetwork] 连接服务器失败");
		socket.reset();
		return false;
	}

	socket->setBlocking(false);
	selector.add(*socket);

	std::println("[ClientNetwork] 已连接到服务器：{}:{}", ip, port);
	return true;
}

bool ClientNetwork::reconnect() {
	if (serverIp.empty() || username.empty()) {
		std::println(stderr, "[ClientNetwork] 无法重连：缺少服务器地址或凭证");
		return false;
	}

	// 清空旧的接收队列
	while (!receivedPackets.empty()) receivedPackets.pop();

	if (!connect(serverIp, serverPort)) {
		return false;
	}

	// 发送登录请求
	sf::Packet req = AccountProtocol::makeLoginRequest(username, password);
	if (!send(req)) {
		std::println(stderr, "[ClientNetwork] 重连：发送登录请求失败");
		return false;
	}

	// 等待登录响应（最多等 5 秒）
	// 暂存非登录包（如 GameState），登录成功后放回队列供主循环处理
	std::queue<sf::Packet> savedPackets;
	sf::Clock clock;
	while (clock.getElapsedTime().asSeconds() < 5.f) {
		update();
		while (auto packetOpt = receivePacket()) {
			sf::Packet packet = *packetOpt;
			sf::Packet peek = packet;  // 用副本判断类型，不移动原始 packet 的读指针
			int msgType;
			if (!(peek >> msgType)) continue;

			if (msgType == static_cast<int>(MessageType::ConnectionInfo)) {
				std::size_t pid;
				if (peek >> pid) setPlayerId(pid);
				continue;
			}
			if (msgType == static_cast<int>(MessageType::LoginResponse)) {
				auto resp = AccountProtocol::parseAccountResponse(peek);
				if (resp && resp->ok) {
					std::println("[ClientNetwork] 重连成功：{}", resp->msg);
					// 把暂存的包放回队列
					while (!savedPackets.empty()) {
						receivedPackets.push(savedPackets.front());
						savedPackets.pop();
					}
					return true;
				}
				std::println(stderr, "[ClientNetwork] 重连登录失败：{}", (resp ? resp->msg : "解析失败"));
				return false;
			}
			// 其他包暂存（原始 packet 读指针未移动）
			savedPackets.push(packet);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	std::println(stderr, "[ClientNetwork] 重连：等待登录响应超时");
	return false;
}

void ClientNetwork::disconnect() {
	if (socket) {
		selector.remove(*socket);
		socket->disconnect();
		socket.reset();
	}
	playerId = 0;
	std::println("[ClientNetwork] 已断开连接");
}

void ClientNetwork::update() {
	//1ms 超时：避免主渲染线程被网络 IO 长时间卡住（注意 SFML 的 wait(0) 是无限阻塞，不能用 0）
	if (socket && selector.wait(sf::milliseconds(1))) {
		if (selector.isReady(*socket)) {
			sf::Packet packet;
			sf::Socket::Status status = socket->receive(packet);
			if (status == sf::Socket::Status::Done) {
				receivedPackets.push(packet);
			}
			else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
				std::println("[ClientNetwork] 与服务器断开连接（status={}）", static_cast<int>(status));
				disconnect();
			}
		}
	}
}

bool ClientNetwork::sendClientInput(sf::Keyboard::Scancode key, std::size_t selectedIndex) {
	if (!socket) return false;

	sf::Packet packet;
	packet << static_cast<int>(MessageType::ClientInput)
		<< static_cast<int>(key)
		<< playerId
		<< selectedIndex;

	return sendPacket(packet);
}

bool ClientNetwork::send(sf::Packet& packet) {
	if (!socket) return false;
	return sendPacket(packet);
}

bool ClientNetwork::sendShopPurchase(const std::string& purchaseData) {
	if (!socket) return false;
	sf::Packet packet;
	packet << static_cast<int>(MessageType::ShopPurchase);
	packet << purchaseData;
	return sendPacket(packet);
}

std::optional<sf::Packet> ClientNetwork::receivePacket() {
	if (!socket) return std::nullopt;

	if (!receivedPackets.empty()) {
		sf::Packet packet = receivedPackets.front();
		receivedPackets.pop();
		return packet;
	}
	return std::nullopt;
}

bool ClientNetwork::sendPacket(sf::Packet& packet) {
	sf::Socket::Status status = socket->send(packet);
	return status == sf::Socket::Status::Done;
}
