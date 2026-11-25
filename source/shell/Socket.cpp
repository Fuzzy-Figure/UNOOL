#include "Socket.h"
#include "MessageCodec.h"

#include <SFML/Network.hpp>

#include <print>

// 客户端传输层实现。服务端多连接网络层见 source/shell/ServerNetwork.cpp。

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

	sf::Packet packet = unool::protocol::makeClientInput(
		static_cast<int>(key), playerId, selectedIndex);
	return sendPacket(packet);
}

bool ClientNetwork::sendChoiceSelect(std::size_t choiceIndex, std::size_t selectedIndex) {
	if (!socket) return false;
	sf::Packet packet = unool::protocol::makeChoiceSelect(choiceIndex, selectedIndex);
	return sendPacket(packet);
}

bool ClientNetwork::send(sf::Packet& packet) {
	if (!socket) return false;
	return sendPacket(packet);
}

bool ClientNetwork::sendShopPurchase(const std::string& purchaseData) {
	if (!socket) return false;
	sf::Packet packet = unool::protocol::makeStringMessage(
		unool::protocol::MessageType::ShopPurchase, purchaseData);
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
