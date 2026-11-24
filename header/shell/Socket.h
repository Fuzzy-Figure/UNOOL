#pragma once
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
#include "shell/IClientGateway.h"

// 客户端传输层。服务端的多连接网络层在 shell/ServerNetwork.h：
// 房间功能（协议版本 3）后服务端不再是「两个固定座位」，两者已无共享状态，
// 因此拆成两个单元，客户端仍只依赖本头。

struct ClientInput {
	sf::Keyboard::Scancode key;
	std::size_t playerId;
	std::size_t selectedIndex;
};

// 实现 shell 层 IClientGateway 端口：该网关间接继承 core 层 IClientSession，
// 本类提供 6 个会话方法 + 1 个入站方法（receivePacket）的具体实现。
// 零调用的凭据写入/重连接口与恒为空的用户名/密码字段已删除。
// ClientNetwork 是当前唯一的具体实现，IClientSession& 端口视图继续有效。
class ClientNetwork : public unool::shell::IClientGateway {
private:
	std::unique_ptr<sf::TcpSocket> socket;
	sf::SocketSelector selector;
	std::size_t playerId = 0;
	std::queue<sf::Packet> receivedPackets;

	// 由 connect() 记录，供连接状态与诊断使用
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

	void setPlayerId(std::size_t id) override { playerId = id; }
	std::size_t getPlayerId() const override { return playerId; }
	// 客户端登录名由 LoginRequest 包承载，本端口不再持有字段，返回静态空串。
	const std::string& getUsername() const override {
		static const std::string empty{};
		return empty;
	}
	bool isConnected() const override { return socket != nullptr; }
};
