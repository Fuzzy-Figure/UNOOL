#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <SFML/Network.hpp>

#include "MessageType.h"
#include "core/IAccountSession.h"
#include "utils.h"   // opt_ref（端口引用别名）

namespace unool::core {
	class AccountService;
	class ShopService;
}

namespace unool::shell {

	class RoomManager;

	// 服务端多连接网络层（协议版本 3）。
	//
	// 职责边界：
	//   - 本类是**唯一的 socket 持有者与唯一的 IO 线程**（主线程每帧 update()）：
	//     接受连接、收包、心跳、超时判定、出站队列冲刷。
	//   - 账号 / 商城 / 房间命令在本线程内同步交给对应服务处理。
	//   - 对局输入（ClientInput）不在本类处理，路由给 RoomManager 投递到对应房间的输入队列；
	//     对局发包由房间线程调用 sendToSlot()，只入出站队列，绝不直接碰 socket。
	//
	// 这样「多房间真并发」不需要给 socket 加锁：房间线程只与队列/状态快照打交道，
	// SFML socket 始终单线程使用。
	//
	// 槽位（slot）是连接的稳定编号，等于 IAccountSession 的 clientIdx；
	// 断线后槽位保留 username / roomId 一段时间，供同账号重连顶替（原逻辑平移）。
	class ServerNetwork final : public unool::core::IAccountSession {
	public:
		// 一个连接槽位。除 socket 外的字段都受 mutex_ 保护；
		// socket 只在 IO 线程读写（其它线程通过 outbox 间接发包）。
		struct Slot {
			std::unique_ptr<sf::TcpSocket> socket;
			bool loggedIn = false;
			bool disconnected = false;
			std::string username;
			int points = 0;
			int wins = 0;
			int losses = 0;
			std::uint64_t roomId = 0;   // 0 = 不在房间
			std::chrono::steady_clock::time_point lastRecv{};
			std::chrono::steady_clock::time_point disconnectedAt{};
			std::deque<sf::Packet> outbox;  // 任意线程入队，IO 线程冲刷
		};

	private:
		std::unique_ptr<sf::TcpListener> listener;
		sf::SocketSelector selector;
		std::vector<Slot> slots_;
		std::size_t capacity_ = 0;

		// 槽位已满或有掉线槽位时，先收下的待登录 socket（与旧实现同构）
		std::unique_ptr<sf::TcpSocket> pendingSocket;

		sf::Clock heartbeatClock;

		// 状态与出站队列的唯一互斥量。持锁期间不做 socket IO，也不回调上层服务，
		// 避免「网络锁 → 房间锁」的反向嵌套。
		mutable std::mutex mutex_;

		opt_ref<unool::core::AccountService> accountService_{};
		opt_ref<unool::core::ShopService> shopService_{};
		opt_ref<RoomManager> roomManager_{};

	private:
		bool sendPacketLocked(std::size_t slot, const sf::Packet& packet);
		// 只在 IO 线程内使用：直发（用于「发完就断开」的包）
		bool sendNow(std::size_t slot, const sf::Packet& packet);
		void flushOutboxes();
		void handleNewConnections();
		void handleClientPackets();
		void handlePendingSocket();
		void sendHeartbeat();
		void checkTimeouts();

		void handleAccountPacket(std::size_t clientIdx, unool::protocol::MessageType type, sf::Packet packet);
		void handleRoomPacket(std::size_t clientIdx, unool::protocol::MessageType type, sf::Packet packet);
		void handleClientInput(std::size_t clientIdx, sf::Packet packet);

		// 同账号重连顶替：返回最终承载这次登录的槽位（可能把 socket 挪到旧槽位）
		std::size_t reconcileReconnect(std::size_t newSlot, const std::string& username);
		void removeClient(std::size_t clientIdx);
		void clearSlot(std::size_t clientIdx);

	public:
		ServerNetwork() = default;
		~ServerNetwork() override;

		bool start(unsigned short port, std::size_t capacity);
		void update();
		void disconnect();

		// 组合根装配：首次 update() 之前完成
		void installAccountService(unool::core::AccountService& service) {
			accountService_ = std::ref(service);
		}
		void installShopService(unool::core::ShopService& service) {
			shopService_ = std::ref(service);
		}
		void installRoomManager(RoomManager& manager) {
			roomManager_ = std::ref(manager);
		}

		// === 房间线程可调用的线程安全接口 ===
		// 只入出站队列，不碰 socket；返回 false 表示该槽位当前离线
		bool sendToSlot(std::size_t slot, const sf::Packet& packet);
		bool isSlotOnline(std::size_t slot) const;
		std::string usernameOfSlot(std::size_t slot) const;
		int pointsOfSlot(std::size_t slot) const;
		std::uint64_t roomIdOfSlot(std::size_t slot) const;
		void setRoomId(std::size_t slot, std::uint64_t roomId);
		// 房间内消费积分（薄转发到 AccountService，读的是进程级共享存档）
		bool trySpendPoints(std::size_t clientIdx, int amount);

		// === IAccountSession ===
		std::size_t capacity() const override;
		bool isConnected(std::size_t clientIdx) const override;
		bool isLoggedIn(std::size_t clientIdx) const override;
		bool isDisconnected(std::size_t clientIdx) const override;
		std::string usernameOf(std::size_t clientIdx) const override;
		bool isUsernameLoggedInElsewhere(std::size_t excludeIdx,
										 const std::string& username) const override;
		void dropConnection(std::size_t clientIdx) override;
		void onLoginSucceeded(std::size_t clientIdx, const std::string& username,
							  int points, int wins, int losses) override;
		void onPointsSpent(std::size_t clientIdx, int amount) override;
		void sendAccountResponse(std::size_t clientIdx, unool::protocol::MessageType type, bool ok,
								 const std::string& msg, int points, int wins, int losses) override;
		void sendCheckUsernameResponse(std::size_t clientIdx, bool exists) override;
		void onPointsChanged(std::size_t clientIdx, int points) override;
		bool sendPayload(std::size_t clientIdx, unool::protocol::MessageType type,
						 const std::string& jsonPayload) override;
		void sendCheckinResponse(std::size_t clientIdx, const std::string& msg, bool ok, int points) override;
		bool sendShopResult(std::size_t clientIdx, const std::string& resultJson) override;
	};

}
