#include "Socket.h"
#include "AccountProtocol.h"
#include "UserDB.h"
#include "GameState.h"
#include <iostream>
#include <thread>

ServerNetwork::~ServerNetwork() {
	disconnect();
}

bool ServerNetwork::start(unsigned short port) {
	listener = std::make_unique<sf::TcpListener>();
	sf::Socket::Status listenStatus = listener->listen(port);

	if (listenStatus != sf::Socket::Status::Done) {
		std::cerr << "[ServerNetwork] 启动失败" << std::endl;
		listener.reset();
		return false;
	}

	listener->setBlocking(false);
	selector.add(*listener);

	std::cout << "[ServerNetwork] 已启动，监听端口：" << port << std::endl;
	return true;
}

void ServerNetwork::disconnect() {
	for (std::size_t i = 0; i < MAX_PLAYERS; ++i) {
		removeClient(i);
	}
	if (listener) {
		listener->close();
		listener.reset();
	}
	serverReady = false;
	std::cout << "[ServerNetwork] 已断开所有连接" << std::endl;
}

void ServerNetwork::removeClient(std::size_t clientIndex) {
	if (clientIndex >= MAX_PLAYERS) return;
	if (clientSockets[clientIndex]) {
		selector.remove(*clientSockets[clientIndex]);
		clientSockets[clientIndex]->disconnect();
		clientSockets[clientIndex].reset();
	}
	clientSlots_[clientIndex].loggedIn = false;
	clientSlots_[clientIndex].disconnected = true;
	// 保留 username，供重连时识别身份
}

std::size_t ServerNetwork::getClientCount() const {
	std::size_t count = 0;
	for (const auto& s : clientSockets) {
		if (s) ++count;
	}
	return count;
}

bool ServerNetwork::isClientConnected(std::size_t clientIndex) const {
	return clientIndex < MAX_PLAYERS && clientSockets[clientIndex] != nullptr;
}

bool ServerNetwork::isClientLoggedIn(std::size_t clientIndex) const {
	return isClientConnected(clientIndex) && clientSlots_[clientIndex].loggedIn;
}

void ServerNetwork::update() {
	if (selector.wait(sf::milliseconds(10))) {
		if (selector.isReady(*listener)) {
			std::unique_ptr<sf::TcpSocket> newSocket = std::make_unique<sf::TcpSocket>();
			if (listener->accept(*newSocket) == sf::Socket::Status::Done) {
				// 找一个空槽位
				std::size_t newPlayerId = MAX_PLAYERS;
				for (std::size_t i = 0; i < MAX_PLAYERS; ++i) {
					if (!clientSockets[i]) {
						newPlayerId = i;
						break;
					}
				}

				if (newPlayerId < MAX_PLAYERS) {
					newSocket->setBlocking(false);
					selector.add(*newSocket);
					clientSockets[newPlayerId] = std::move(newSocket);

					sendConnectionInfo(newPlayerId);

					std::cout << "[ServerNetwork] 客户端" << newPlayerId << "已连接，等待登录..." << std::endl;
				}
				else {
					std::cout << "[ServerNetwork] 客户端连接被拒绝（已达到最大人数）" << std::endl;
				}
			}
		}

		for (std::size_t i = 0; i < MAX_PLAYERS; ++i) {
			if (!clientSockets[i]) continue;

			sf::TcpSocket& socket = *clientSockets[i];
			if (selector.isReady(socket)) {
				sf::Packet packet;
				sf::Socket::Status status = socket.receive(packet);
				if (status == sf::Socket::Status::Done) {
					// 先 peek 类型，账号包内部处理，其他包入队列
					sf::Packet peeked = packet;
					int msgType;
					if (peeked >> msgType) {
						if (msgType == static_cast<int>(MessageType::RegisterRequest) ||
							msgType == static_cast<int>(MessageType::LoginRequest) ||
							msgType == static_cast<int>(MessageType::CheckUsernameRequest)) {
							handleAccountPacket(i, static_cast<MessageType>(msgType), std::move(packet));
						}
						else {
							receivedPackets.push(packet);
						}
					}
				}
				else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
					std::cout << "[ServerNetwork] 客户端" << i << " 断开连接（status=" << static_cast<int>(status) << "）" << std::endl;
					removeClient(i);
				}
			}
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
		}

		// 检查另一端是否已用同一账号登录
		if (ok) {
			std::size_t other = 1 - clientIdx;
			if (other < MAX_PLAYERS
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

			std::cout << "[ServerNetwork] 客户端" << clientIdx << " "
				<< (isReconnect ? "重连" : "登录") << ": " << req->username
				<< "（积分 " << pts << "）" << std::endl;

			// 两玩家都登录后开局
			if (clientSlots_[0].loggedIn && clientSlots_[1].loggedIn) {
				serverReady = true;
				sendGameStart();
				std::cout << "[ServerNetwork] 两个客户端都已登录，游戏开始" << std::endl;
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

bool ServerNetwork::sendPacketToAll(sf::Packet& packet) {
	bool allOk = true;
	for (std::size_t i = 0; i < MAX_PLAYERS; ++i) {
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
			std::cout << "[ServerNetwork] 发送时检测到客户端" << clientIndex << " 断开" << std::endl;
			removeClient(clientIndex);
			return false;
		}
		if (attempt < 2) std::this_thread::sleep_for(5ms);
	}
	return false;
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
		std::cerr << "[ClientNetwork] 连接服务器失败" << std::endl;
		socket.reset();
		return false;
	}

	socket->setBlocking(false);
	selector.add(*socket);

	std::cout << "[ClientNetwork] 已连接到服务器：" << ip << ":" << port << std::endl;
	return true;
}

bool ClientNetwork::reconnect() {
	if (serverIp.empty() || username.empty()) {
		std::cerr << "[ClientNetwork] 无法重连：缺少服务器地址或凭证" << std::endl;
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
		std::cerr << "[ClientNetwork] 重连：发送登录请求失败" << std::endl;
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
					std::cout << "[ClientNetwork] 重连成功：" << resp->msg << std::endl;
					// 把暂存的包放回队列
					while (!savedPackets.empty()) {
						receivedPackets.push(savedPackets.front());
						savedPackets.pop();
					}
					return true;
				}
				std::cerr << "[ClientNetwork] 重连登录失败：" << (resp ? resp->msg : "解析失败") << std::endl;
				return false;
			}
			// 其他包暂存（原始 packet 读指针未移动）
			savedPackets.push(packet);
		}
		std::this_thread::sleep_for(50ms);
	}

	std::cerr << "[ClientNetwork] 重连：等待登录响应超时" << std::endl;
	return false;
}

void ClientNetwork::disconnect() {
	if (socket) {
		selector.remove(*socket);
		socket->disconnect();
		socket.reset();
	}
	playerId = 0;
	std::cout << "[ClientNetwork] 已断开连接" << std::endl;
}

void ClientNetwork::update() {
	using namespace std::chrono_literals;
	if (socket && selector.wait(sf::milliseconds(10))) {
		if (selector.isReady(*socket)) {
			sf::Packet packet;
			sf::Socket::Status status = socket->receive(packet);
			if (status == sf::Socket::Status::Done) {
				receivedPackets.push(packet);
			}
			else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
				std::cout << "[ClientNetwork] 与服务器断开连接（status=" << static_cast<int>(status) << "）" << std::endl;
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
