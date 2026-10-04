#pragma once
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>
#include <SFML/Network.hpp>
#include <SFML/Window/Keyboard.hpp>

constexpr std::size_t MAX_PLAYERS = 2;

struct GameState;
struct CharInfo;

//网络
enum class MessageType {
	None,
	ClientInput,
	GameState,
	ConnectionInfo,
	ConnectionRefused,
	GameStart,
	GameEnd,
	Choice,
	PointerUpdate,
	CharInfo,
	RegisterRequest,
	RegisterResponse,
	LoginRequest,
	LoginResponse,
	CheckUsernameRequest,
	CheckUsernameResponse
};

struct ClientInput {
	sf::Keyboard::Scancode key;
	std::size_t playerId;
	std::size_t selectedIndex;
};

struct ConnectionInfo {
	std::size_t playerId;
	bool isReady;
};

struct GameEndInfo {
	std::size_t winnerId;
};

class ServerNetwork {
public:
	struct ClientSlot {
		bool loggedIn = false;
		bool disconnected = false;
		std::string username;
		int points = 0;
		int wins = 0;
		int losses = 0;
	};

private:
	std::unique_ptr<sf::TcpListener> listener;
	std::array<std::unique_ptr<sf::TcpSocket>, MAX_PLAYERS> clientSockets;
	std::array<ClientSlot, MAX_PLAYERS> clientSlots_;
	sf::SocketSelector selector;
	bool serverReady = false;
	std::queue<sf::Packet> receivedPackets;

private:
	bool sendPacketToClient(std::size_t clientIndex, sf::Packet& packet);
	bool sendPacketToAll(sf::Packet& packet);
	void handleAccountPacket(std::size_t clientIdx, MessageType type, sf::Packet packet);
	void removeClient(std::size_t clientIndex);

public:
	ServerNetwork() = default;
	~ServerNetwork();

	bool start(unsigned short port);
	void disconnect();
	void update();

	std::optional<ClientInput> receiveClientInput();
	bool sendGameState(const GameState& state);
	bool sendGameStateToClient(std::size_t clientIndex, const GameState& state);
	bool sendConnectionInfo(std::size_t newPlayerId);
	bool sendGameStart();
	bool sendGameEnd(std::optional<std::size_t> winnerId);
	bool sendPlayerChoice(std::size_t clientIndex,
						  const std::string& title,
						  const std::vector<std::string>& options,
						  bool forced,
						  const std::string& errorMsg = "",
						  std::optional<std::size_t> timeoutMs = std::nullopt,
						  std::size_t currentPage = 0,
						  std::size_t totalPages = 1);
	bool sendCharInfo(const CharInfo& info);

	bool isReady() const { return serverReady; }
	std::size_t getClientCount() const;
	bool isClientConnected(std::size_t clientIndex) const;
	bool isClientLoggedIn(std::size_t clientIndex) const;
	const std::array<ClientSlot, MAX_PLAYERS>& getClientSlots() const { return clientSlots_; }
};

class ClientNetwork {
private:
	std::unique_ptr<sf::TcpSocket> socket;
	sf::SocketSelector selector;
	std::size_t playerId = 0;
	std::queue<sf::Packet> receivedPackets;

	std::string serverIp;
	unsigned short serverPort = 0;
	std::string username;
	std::string password;

private:
	bool sendPacket(sf::Packet& packet);

public:
	ClientNetwork() = default;
	~ClientNetwork();

	bool connect(const std::string& ip, unsigned short port);
	void disconnect();
	void update();

	bool sendClientInput(sf::Keyboard::Scancode key, std::size_t selectedIndex);
	bool send(sf::Packet& packet);
	std::optional<sf::Packet> receivePacket();

	void setPlayerId(std::size_t id) { playerId = id; }
	std::size_t getPlayerId() const { return playerId; }
	bool isConnected() const { return socket != nullptr; }

	void setCredentials(const std::string& u, const std::string& p) { username = u; password = p; }
	bool reconnect();
};
