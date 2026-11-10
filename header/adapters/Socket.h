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
#include "adapters/IClientGateway.h"
#include "application/AccountService.h"
#include "application/ShopService.h"


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
	// 注意：ref / opt_ref 是 utils.h 定义在全局命名空间的别名，不能加 unool:: 限定
	opt_ref<unool::application::AccountService> accountService_{};
	// W5 第二段（分段第一段）：商城展示 / 技能描述 / 签到用例服务
	opt_ref<unool::application::ShopService> shopService_{};

private:
	bool sendPacketToClient(std::size_t clientIndex, sf::Packet& packet);
	bool sendPacketToAll(sf::Packet& packet);
	void handleAccountPacket(std::size_t clientIdx, MessageType type, sf::Packet packet);
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

	// 发送商城数据给指定客户端（薄转发到 ShopService）
	bool sendShopData(std::size_t clientIndex, const std::string& username);
	// 处理购买请求（W5 第二段第二段起为薄转发，业务判断在 ShopService）
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

	// 组合根装配商城展示 / 技能描述 / 签到用例服务（首次 update() 之前）
	void installShopService(unool::application::ShopService& service) {
		shopService_ = std::ref(service);
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
	void sendAccountResponse(std::size_t clientIdx, MessageType type, bool ok,
							const std::string& msg, int points, int wins, int losses) override;
	void sendCheckUsernameResponse(std::size_t clientIdx, bool exists) override;
	// W5 第二段：绝对值积分同步与可判成败的响应包发送
	void onPointsChanged(std::size_t clientIdx, int points) override;
	bool sendPayload(std::size_t clientIdx, MessageType type, const std::string& jsonPayload) override;
	void sendCheckinResponse(std::size_t clientIdx, const std::string& msg, bool ok, int points) override;
	// W5 第二段第二段：购买结果回送（IAccountSession 端口实现）。
	// 实现体保留原 7 行纯传输逻辑（连接早退 + MessageType::ShopResult + JSON 字符串）。
	bool sendShopResult(std::size_t clientIndex, const std::string& resultJson) override;
};

// 阶段 4 · S1 实现 IClientSession 端口；S3a（用户 49.10 口径②A）改由
// adapters 层的 IClientGateway 继承实现——该网关端口间接继承 IClientSession，
// 会话 6 方法的签名与函数体仍逐字未改。
// S3b1（用户 51.11 口径②A + ③C）：网关端口新增两个入站纯虚，本类的
// receivePacket / peekMessageType 加 override；同时删除零调用的 setCredentials /
// reconnect 与恒为空的 username / password（不可达死代码链，对外行为零变化）。
// ClientNetwork 仍是唯一的具体实现，S1 的 IClientSession& 端口视图继续有效。
class ClientNetwork : public unool::adapters::IClientGateway {
private:
	std::unique_ptr<sf::TcpSocket> socket;
	sf::SocketSelector selector;
	std::size_t playerId = 0;
	std::queue<sf::Packet> receivedPackets;

	// S3b1：仍由 connect() 赋值，删 reconnect() 后只写不读，顺延到后续批次再判
	std::string serverIp;
	unsigned short serverPort = 0;

private:
	bool sendPacket(sf::Packet& packet);

public:
	ClientNetwork() = default;
	~ClientNetwork();

	bool connect(const std::string& ip, unsigned short port) override;
	void disconnect() override;
	void update() override;

	bool sendClientInput(sf::Keyboard::Scancode key, std::size_t selectedIndex);
	bool send(sf::Packet& packet) override;
	// 发送商城购买请求（purchaseData 为 JSON 字符串）
	bool sendShopPurchase(const std::string& purchaseData) override;
	std::optional<sf::Packet> receivePacket() override;
	std::optional<int> peekMessageType() const override;

	void setPlayerId(std::size_t id) override { playerId = id; }
	std::size_t getPlayerId() const override { return playerId; }
	// S3b1：原实现返回的 username 因 setCredentials() 零调用而恒为空串，删除该字段后
	// 改为返回函数内静态空串，返回值与 ClientApp.cpp 的打印内容逐字不变。
	const std::string& getUsername() const override {
		static const std::string empty{};
		return empty;
	}
	bool isConnected() const override { return socket != nullptr; }
};
