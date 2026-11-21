// Arch-C P1-3：连接状态机单测。
// 用 fake gateway 驱动 ConnectionState，验证三项验收：
//   1) 跨阶段留包顺序 —— 非订阅包留在队列，交给下一阶段消费
//   2) 心跳自动回复 —— pump 内部消费 Heartbeat 并回包
//   3) 断线行为可枚举 —— 连接态转断线态时清空积压
#include "test_connection.h"

#include "shell/ConnectionState.h"
#include "shell/IClientGateway.h"
#include <SFML/Network.hpp>

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <utility>

// MessageType 归 protocol 层；本文件在全局命名空间，这里做 TU 局部的名字转发。
using unool::protocol::MessageType;
using unool::tests::TestResult;

namespace {

	// 纯内存网关：实现 IClientGateway，记录心跳回复次数，注入预设入站包。
	class FakeGateway final : public unool::shell::IClientGateway {
	public:
		bool connected = true;
		std::size_t playerId = 0;
		std::string username = "tester";
		std::deque<sf::Packet> inbound;
		int heartbeatReplies = 0;

		// --- IClientSession ---
		bool connect(const std::string&, unsigned short) override { connected = true; return true; }
		void disconnect() override { connected = false; }
		void update() override {}  // fake 无 socket，包由 enqueue 直接注入
		bool isConnected() const override { return connected; }
		std::size_t getPlayerId() const override { return playerId; }
		const std::string& getUsername() const override { return username; }

		// --- IClientGateway 出站 ---
		bool send(sf::Packet&) override { ++heartbeatReplies; return true; }
		bool sendShopPurchase(const std::string&) override { return true; }
		void setPlayerId(std::size_t id) override { playerId = id; }

		// --- IClientGateway 入站 ---
		std::optional<sf::Packet> receivePacket() override {
			if (inbound.empty()) return std::nullopt;
			sf::Packet p = std::move(inbound.front());
			inbound.pop_front();
			return p;
		}
	};

	sf::Packet packetOf(MessageType type) {
		sf::Packet p;
		p << static_cast<int>(type);
		return p;
	}

	// 跨阶段留包：队首非订阅包不被消费，GameStart 留给下一阶段 takeAny 取出。
	void testCrossStageLeaving(TestResult& r) {
		FakeGateway gw;
		gw.inbound.push_back(packetOf(MessageType::GameStart));
		gw.inbound.push_back(packetOf(MessageType::LoginResponse));
		unool::shell::ConnectionState conn(gw);

		conn.pump();
		// 订阅只含 LoginResponse：队首是 GameStart，不消费；但 LoginResponse 在其后，
		// 按 take 的「队首命中才取」语义也不会被跳过取出 —— 这正是原 Filtered 约定。
		auto noTake = conn.take({ MessageType::LoginResponse });
		r.check(!noTake.has_value());
		r.check(conn.contains(MessageType::GameStart));
		r.check(conn.contains(MessageType::LoginResponse));

		// 下一阶段 DrainAll：按入队顺序取出（FIFO），顺序不依赖调用次序。
		auto first = conn.takeAny();
		r.check(first && first->type == MessageType::GameStart);
		auto second = conn.takeAny();
		r.check(second && second->type == MessageType::LoginResponse);
		r.check(!conn.takeAny().has_value());
	}

	// 心跳：pump 消费 Heartbeat 并回复一次，其余包正常入队。
	void testHeartbeatAutoReply(TestResult& r) {
		FakeGateway gw;
		gw.inbound.push_back(packetOf(MessageType::Heartbeat));
		gw.inbound.push_back(packetOf(MessageType::GameState));
		unool::shell::ConnectionState conn(gw);

		conn.pump();
		r.check(gw.heartbeatReplies == 1);
		r.check(conn.contains(MessageType::GameState));
		r.check(!conn.contains(MessageType::Heartbeat));
	}

	// 断线：连接态→断线态时清空积压；isConnected 反映网关状态。
	void testDisconnectClearsQueue(TestResult& r) {
		FakeGateway gw;
		gw.inbound.push_back(packetOf(MessageType::GameState));
		unool::shell::ConnectionState conn(gw);

		conn.pump();
		r.check(conn.isConnected());
		r.check(conn.contains(MessageType::GameState));

		gw.connected = false;
		conn.pump();
		r.check(!conn.isConnected());
		r.check(!conn.contains(MessageType::GameState));
	}

} // namespace

unool::tests::TestResult runConnectionStateTests() {
	unool::tests::TestResult r;
	testCrossStageLeaving(r);
	testHeartbeatAutoReply(r);
	testDisconnectClearsQueue(r);
	return r;
}
