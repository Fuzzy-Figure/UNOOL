#include "../header/Socket.h"
#include "../header/utils.h"
#include "../header/AccountProtocol.h"
#include "../header/UserDB.h"
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
	for (auto& socket : clientSockets) {
		socket->disconnect();
	}
	clientSockets.clear();
	if (listener) {
		listener->close();
		listener.reset();
	}
	serverReady = false;
	std::cout << "[ServerNetwork] 已断开所有连接" << std::endl;
}

void ServerNetwork::update() {
	if (selector.wait(sf::milliseconds(10))) {
		if (selector.isReady(*listener)) {
			std::unique_ptr<sf::TcpSocket> newSocket = std::make_unique<sf::TcpSocket>();
			if (listener->accept(*newSocket) == sf::Socket::Status::Done) {
				if (clientSockets.size() < 2) {
					newSocket->setBlocking(false);
					selector.add(*newSocket);
					clientSockets.push_back(std::move(newSocket));

					std::size_t newPlayerId = clientSockets.size() - 1;
					sendConnectionInfo(newPlayerId);

					std::cout << "[ServerNetwork] 客户端" << newPlayerId << "已连接，等待登录..." << std::endl;
				}
				else {
					std::cout << "[ServerNetwork] 客户端连接被拒绝（已达到最大人数）" << std::endl;
				}
			}
		}

		for (std::size_t i = 0; i < clientSockets.size(); ) {
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
				else if (status == sf::Socket::Status::Disconnected) {
					std::cout << "[ServerNetwork] 客户端" << i << " 断开连接" << std::endl;
					selector.remove(socket);
					clientSockets.erase(clientSockets.begin() + i);
					clientSlots_[i].loggedIn = false;
					clientSlots_[i].username.clear();
					continue;
				}
			}
			++i;
		}
	}
}

void ServerNetwork::handleAccountPacket(std::size_t clientIdx, MessageType type, sf::Packet packet) {
	if (clientIdx >= clientSockets.size()) return;

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
		sendPacketToClient(*clientSockets[clientIdx], resp);
	}
	else if (type == MessageType::LoginRequest) {
		auto req = AccountProtocol::parseAccountRequest(packet);
		if (!req) return;
		std::string errMsg;
		auto userInfo = UserDB::instance().login(req->username, req->password, errMsg);

		bool ok = userInfo.has_value();
		// 检查另一端是否已用同一账号登录
		if (ok) {
			std::size_t other = 1 - clientIdx;
			if (other < clientSlots_.size()
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
		sendPacketToClient(*clientSockets[clientIdx], resp);

		if (ok) {
			clientSlots_[clientIdx].loggedIn = true;
			clientSlots_[clientIdx].username = req->username;
			clientSlots_[clientIdx].points = pts;
			clientSlots_[clientIdx].wins = w;
			clientSlots_[clientIdx].losses = l;

			std::cout << "[ServerNetwork] 客户端" << clientIdx << " 登录: " << req->username
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
		sendPacketToClient(*clientSockets[clientIdx], resp);
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
	if (clientIndex >= clientSockets.size()) return false;

	sf::Packet packet;
	packet << static_cast<int>(MessageType::GameState);
	packet << state;
	return sendPacketToClient(*clientSockets[clientIndex], packet);
}

bool ServerNetwork::sendConnectionInfo(std::size_t newPlayerId) {
	sf::Packet packet;
	packet << static_cast<int>(MessageType::ConnectionInfo) << newPlayerId;
	return sendPacketToClient(*clientSockets.back(), packet);
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

bool ServerNetwork::sendPlayerChoice(std::size_t clientIndex, const std::wstring& title, const std::vector<std::wstring>& options, bool forced, const std::wstring& errorMsg, std::optional<std::size_t> timeoutMs, std::size_t currentPage, std::size_t totalPages) {
	if (clientIndex >= clientSockets.size()) return false;

	sf::Packet packet;
	packet << static_cast<int>(MessageType::Choice);
	packet << sf::String(title);
	packet << static_cast<std::size_t>(options.size());
	for (const auto& option : options) {
		packet << sf::String(option);
	}
	packet << forced;
	packet << sf::String(errorMsg);
	bool hasTimeout = timeoutMs.has_value();
	packet << hasTimeout;
	if (hasTimeout) {
		packet << static_cast<std::uint64_t>(timeoutMs.value());
	}
	packet << currentPage;
	packet << totalPages;

	return sendPacketToClient(*clientSockets[clientIndex], packet);
}

bool ServerNetwork::sendCharInfo(const CharInfo& info) {
	sf::Packet packet;
	packet << static_cast<int>(MessageType::CharInfo);
	packet << info;
	return sendPacketToAll(packet);
}

bool ServerNetwork::sendPointerUpdate(std::size_t playerId, std::size_t selectedIndex) {
	sf::Packet packet;
	packet << static_cast<int>(MessageType::PointerUpdate);
	packet << playerId << selectedIndex;
	return sendPacketToAll(packet);
}

bool ServerNetwork::sendPacketToAll(sf::Packet& packet) {
	bool allOk = true;
	for (auto& socket : clientSockets) {
		if (!sendPacketToClient(*socket, packet)) {
			allOk = false;
		}
	}
	return allOk;
}

bool ServerNetwork::sendPacketToClient(sf::TcpSocket& socket, sf::Packet& packet) {
	for (int attempt = 0; attempt < 3; ++attempt) {
		sf::Socket::Status status = socket.send(packet);
		if (status == sf::Socket::Status::Done) return true;
		if (attempt < 2) std::this_thread::sleep_for(5ms);
	}
	return false;
}

ClientNetwork::~ClientNetwork() {
	disconnect();
}

bool ClientNetwork::connect(const std::string& ip, unsigned short port) {
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

void ClientNetwork::disconnect() {
	if (socket) {
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
			else if (status == sf::Socket::Status::Disconnected) {
				std::cout << "[ClientNetwork] 与服务器断开连接" << std::endl;
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
