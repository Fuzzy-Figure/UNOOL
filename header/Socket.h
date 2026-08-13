#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>
#include <queue>
#include <optional>

#include "GameState.h"
//网络
enum class MessageType {
	None,
	ClientInput,
	GameState,
	ConnectionInfo,
	ConnectionRefused,
	GameStart,
	GameEnd,
	Choice
};

struct ClientInput {
	sf::Keyboard::Scancode key;
	std::size_t playerId;
};

struct ConnectionInfo {
	std::size_t playerId;
	bool isReady;
};

struct GameEndInfo {
	std::size_t winnerId;
};

class ServerNetwork {
private:
	std::unique_ptr<sf::TcpListener> listener;
	std::vector<std::unique_ptr<sf::TcpSocket>> clientSockets;
	sf::SocketSelector selector;
	bool serverReady = false;
	std::queue<sf::Packet> receivedPackets;

private:
	bool sendPacketToClient(sf::TcpSocket& socket, sf::Packet& packet);
	bool sendPacketToAll(sf::Packet& packet);

public:
	ServerNetwork() = default;
	~ServerNetwork();

	bool start(unsigned short port);
	void disconnect();
	void update();

	std::optional<ClientInput> receiveClientInput();
	bool sendGameState(const GameState& state);
	bool sendGameStateToClient(std::size_t clientIndex, const GameState& state);
	bool sendConnectionInfo(std::size_t playerId);
	bool sendGameStart();
	bool sendGameEnd(std::optional<std::size_t> winnerId);
	bool sendPlayerChoice(std::size_t clientIndex, const std::wstring& title, const std::vector<std::wstring>& options, bool forced, const std::wstring& errorMsg = L"", std::optional<std::size_t> timeoutMs = std::nullopt, std::size_t currentPage = 0, std::size_t totalPages = 1);

	bool isReady() const { return serverReady; }
	std::size_t getClientCount() const { return clientSockets.size(); }
};

class ClientNetwork {
private:
	std::unique_ptr<sf::TcpSocket> socket;
	sf::SocketSelector selector;
	std::size_t playerId = 0;
	std::queue<sf::Packet> receivedPackets;

private:
	bool sendPacket(sf::Packet& packet);

public:
	ClientNetwork() = default;
	~ClientNetwork();

	bool connect(const std::string& ip, unsigned short port);
	void disconnect();
	void update();

	bool sendClientInput(sf::Keyboard::Scancode key);
	std::optional<sf::Packet> receivePacket();

	void setPlayerId(std::size_t id) { playerId = id; }
	std::size_t getPlayerId() const { return playerId; }
};
