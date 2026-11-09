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
#include "utils.h"
#include "MessageType.h"
#include "application/IAccountSession.h"
#include "application/AccountService.h"


struct GameState;
struct CharInfo;

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

class ServerNetwork : public unool::application::IAccountSession {
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
	std::array<std::unique_ptr<sf::TcpSocket>, unool::MAX_PLAYERS> clientSockets;
	std::array<ClientSlot, unool::MAX_PLAYERS> clientSlots_;
	sf::SocketSelector selector;
	bool serverReady = false;
	std::array<bool, unool::MAX_PLAYERS> playerReady{};  //玩家点"开始游戏"后置 true
	std::queue<sf::Packet> receivedPackets;
	//槽位已满时收下的待登录 socket，用于识别重连意图后顶替旧连接
	std::unique_ptr<sf::TcpSocket> pendingSocket;
	//心跳机制：每秒向所有 loggedIn 客户端发 Heartbeat，超过 3 秒未收到任何回包则视为掉线
	sf::Clock heartbeatClock;
	std::array<sf::Clock, unool::MAX_PLAYERS> lastRecvClocks;
	// W5 第一段：账号用例服务由组合根注入，本类只保留会话状态与薄转发
	unool::opt_ref<unool::application::AccountService> accountService_{};

private:
	bool sendPacketToClient(std::size_t clientIndex, sf::Packet& packet);
	bool sendPacketToAll(sf::Packet& packet);
	void handleAccountPacket(std::size_t clientIdx, MessageType type, sf::Packet packet);
	// 发送购买结果给客户端（JSON 字符串载荷）
	bool sendShopResult(std::size_t clientIndex, const std::string& resultJson);
	void removeClient(std::size_t clientIndex);
	//update() 的拆分子任务
	void handleNewConnections();
	void handleClientPackets();
	void handlePendingSocket();
	void sendHeartbeat();
	void checkTimeouts();

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
	bool clearPlayerChoice(std::size_t clientIndex);
	bool sendCharInfo(const CharInfo& info);

	// 发送商城数据给指定客户端
	bool sendShopData(std::size_t clientIndex, const std::string& username);
	// 处理购买请求
	void handleShopPurchase(std::size_t clientIndex, const std::string& purchaseData);
	// 处理技能描述请求：按角色名生成技能文本并回送 SkillInfoResponse
	void handleSkillInfoRequest(std::size_t clientIndex, const std::string& requestData);
	// 处理每日签到请求：按奖池抽取奖品并回送 CheckinResponse
	void handleCheckin(std::size_t clientIndex);

	bool isReady() const { return serverReady; }
	std::size_t getClientCount() const;
	bool isClientConnected(std::size_t clientIndex) const;
	bool isClientLoggedIn(std::size_t clientIndex) const;
	const std::array<ClientSlot, unool::MAX_PLAYERS>& getClientSlots() const { return clientSlots_; }
	// 尝试消耗指定客户端 amount 积分，成功则同步更新 ClientSlot.points
	// （W5 第一段起为薄转发，业务判断在 AccountService）
	bool trySpendPoints(std::size_t clientIndex, int amount);

	// 组合根装配账号用例服务；ServerNetwork 自身实现 IAccountSession 端口
	void installAccountService(unool::application::AccountService& service) {
		accountService_ = std::ref(service);
	}

	// === IAccountSession ===
	bool isConnected(std::size_t clientIdx) const override;
	bool isLoggedIn(std::size_t clientIdx) const override;
	bool isDisconnected(std::size_t clientIdx) const override;
	std::string usernameOf(std::size_t clientIdx) const override;
	void dropConnection(std::size_t clientIdx) override;
	void onLoginSucceeded(std::size_t clientIdx, const std::string& username,
						  int points, int wins, int losses) override;
	void onPointsSpent(std::size_t clientIdx, int amount) override;
	void onReadyToPlay(std::size_t clientIdx) override;
	void sendResponse(std::size_t clientIdx, sf::Packet& packet) override;
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
	// 发送商城购买请求（purchaseData 为 JSON 字符串）
	bool sendShopPurchase(const std::string& purchaseData);
	std::optional<sf::Packet> receivePacket();
	std::optional<int> peekMessageType() const;

	void setPlayerId(std::size_t id) { playerId = id; }
	std::size_t getPlayerId() const { return playerId; }
	const std::string& getUsername() const { return username; }
	bool isConnected() const { return socket != nullptr; }

	void setCredentials(const std::string& u, const std::string& p) { username = u; password = p; }
	bool reconnect();
};
