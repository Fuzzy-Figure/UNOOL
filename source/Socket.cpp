#include "../header/Socket.h"
#include "../header/utils.h"
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

					std::cout << "[ServerNetwork] 客户端" << newPlayerId << "已连接" << std::endl;

					if (clientSockets.size() == 2) {
						serverReady = true;
						sendGameStart();
						std::cout << "[ServerNetwork] 两个客户端已连接，游戏开始" << std::endl;
					}
				}
				else {
					std::cout << "[ServerNetwork] 客户端连接被拒绝（已达到最大人数）" << std::endl;
				}
			}
		}

		for (auto it = clientSockets.begin(); it != clientSockets.end();) {
			sf::TcpSocket& socket = **it;
			if (selector.isReady(socket)) {
				sf::Packet packet;
				sf::Socket::Status status = socket.receive(packet);
				if (status == sf::Socket::Status::Done) {
					receivedPackets.push(packet);
				}
				else if (status == sf::Socket::Status::Disconnected) {
					std::cout << "[ServerNetwork] 客户端断开连接" << std::endl;
					selector.remove(socket);
					it = clientSockets.erase(it);
					continue;
				}
			}
			++it;
		}
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
			if (packet >> keyCode >> input.playerId) {
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

bool ClientNetwork::sendClientInput(sf::Keyboard::Scancode key) {
	if (!socket) return false;

	sf::Packet packet;
	packet << static_cast<int>(MessageType::ClientInput)
		<< static_cast<int>(key)
		<< playerId;

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
