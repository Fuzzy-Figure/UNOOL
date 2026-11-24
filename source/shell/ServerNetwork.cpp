#include "ServerNetwork.h"

#include "RoomManager.h"
#include "AccountProtocol.h"
#include "RoomProtocol.h"
#include "MessageCodec.h"
#include "core/AccountService.h"
#include "core/ShopService.h"

#include <SFML/Window/Keyboard.hpp>

#include <algorithm>
#include <chrono>
#include <print>
#include <utility>

namespace unool::shell {

	namespace {
		// 心跳：每秒一次；连续 3 秒没有任何回包判定掉线（原值不变）。
		constexpr float kHeartbeatSeconds = 1.0f;
		constexpr float kTimeoutSeconds = 3.0f;
		// 掉线槽位的保留时长：与 server_config.json 的 network.reconnectTimeoutSec 默认值一致。
		// 保留期内同账号重连会顶替回原槽位（房间成员身份随之保留）。
		constexpr int kDisconnectedSlotKeepSeconds = 600;
		// IO 线程每帧最多等多久（毫秒）
		constexpr int kSelectorTimeoutMs = 10;
	}

	ServerNetwork::~ServerNetwork() {
		disconnect();
	}

	bool ServerNetwork::start(const unsigned short port, const std::size_t capacity) {
		capacity_ = std::max<std::size_t>(capacity, 2);

		listener = std::make_unique<sf::TcpListener>();
		if (listener->listen(port) != sf::Socket::Status::Done) {
			std::println(stderr, "[ServerNetwork] 启动失败（端口 {}）", port);
			listener.reset();
			return false;
		}
		listener->setBlocking(false);
		selector.add(*listener);

		std::lock_guard lock(mutex_);
		slots_.clear();
		slots_.resize(capacity_);
		std::println("[ServerNetwork] 已启动，监听端口：{}，连接槽位上限：{}（最多 {} 个房间）",
					 port, capacity_, capacity_ / unool::protocol::kRoomCapacity);
		return true;
	}

	void ServerNetwork::disconnect() {
		std::lock_guard lock(mutex_);
		for (auto& slot : slots_) {
			if (slot.socket) {
				selector.remove(*slot.socket);
				slot.socket->disconnect();
				slot.socket.reset();
			}
			slot.loggedIn = false;
			slot.disconnected = true;
			slot.outbox.clear();
		}
		if (pendingSocket) {
			selector.remove(*pendingSocket);
			pendingSocket->disconnect();
			pendingSocket.reset();
		}
		if (listener) {
			listener->close();
			listener.reset();
		}
	}

	void ServerNetwork::update() {
		if (listener && selector.wait(sf::milliseconds(kSelectorTimeoutMs))) {
			handleNewConnections();
			handleClientPackets();
			handlePendingSocket();
		}
		sendHeartbeat();
		checkTimeouts();
		flushOutboxes();
	}

	// === 出站：任意线程只入队，IO 线程统一冲刷 ===

	bool ServerNetwork::sendPacketLocked(const std::size_t slot, const sf::Packet& packet) {
		if (slot >= slots_.size()) return false;
		if (!slots_[slot].socket) return false;
		slots_[slot].outbox.push_back(packet);
		return true;
	}

	bool ServerNetwork::sendToSlot(const std::size_t slot, const sf::Packet& packet) {
		std::lock_guard lock(mutex_);
		return sendPacketLocked(slot, packet);
	}

	// 只在 IO 线程内使用：某些「发完就断开」的包必须直发，不能走会在 disconnect 时被清空的出站队列
	bool ServerNetwork::sendNow(const std::size_t slot, const sf::Packet& packet) {
		sf::TcpSocket* socket = nullptr;
		{
			std::lock_guard lock(mutex_);
			if (slot >= slots_.size() || !slots_[slot].socket) return false;
			socket = slots_[slot].socket.get();
		}
		// sf::TcpSocket::send 只接受非 const 引用（发送会消费包内读指针），这里复制一份再发
		sf::Packet outbound = packet;
		return socket->send(outbound) == sf::Socket::Status::Done;
	}

	void ServerNetwork::flushOutboxes() {
		struct Pending {
			std::size_t slot;
			sf::Packet packet;
		};
		std::vector<Pending> pending;
		{
			std::lock_guard lock(mutex_);
			for (std::size_t i = 0; i < slots_.size(); ++i) {
				auto& outbox = slots_[i].outbox;
				while (!outbox.empty()) {
					pending.push_back(Pending{ i, std::move(outbox.front()) });
					outbox.pop_front();
				}
			}
		}
		if (pending.empty()) return;

		for (auto& item : pending) {
			sf::TcpSocket* socket = nullptr;
			{
				std::lock_guard lock(mutex_);
				if (item.slot < slots_.size() && slots_[item.slot].socket) {
					socket = slots_[item.slot].socket.get();
				}
			}
			if (!socket) continue;
			const auto status = socket->send(item.packet);
			if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
				std::println("[ServerNetwork] 发送时检测到槽位{} 断开", item.slot);
				removeClient(item.slot);
			}
		}
	}

	// === 连接生命周期 ===

	void ServerNetwork::handleNewConnections() {
		if (!selector.isReady(*listener)) return;

		auto newSocket = std::make_unique<sf::TcpSocket>();
		if (listener->accept(*newSocket) != sf::Socket::Status::Done) return;
		newSocket->setBlocking(false);

		std::size_t freeSlot = capacity_;
		bool hasDisconnectedSlot = false;
		{
			std::lock_guard lock(mutex_);
			for (std::size_t i = 0; i < slots_.size(); ++i) {
				// 掉线槽位保留着账号身份，不能被新连接直接占用（否则重连会拿错房间）
				if (!slots_[i].socket && !slots_[i].disconnected) {
					freeSlot = i;
					break;
				}
			}
			if (freeSlot == capacity_) {
				for (const auto& slot : slots_) {
					if (slot.disconnected) {
						hasDisconnectedSlot = true;
						break;
					}
				}
			}
		}

		if (freeSlot < capacity_) {
			selector.add(*newSocket);
			{
				std::lock_guard lock(mutex_);
				slots_[freeSlot].socket = std::move(newSocket);
				slots_[freeSlot].lastRecv = std::chrono::steady_clock::now();
			}
			sf::Packet info = unool::protocol::makeConnectionInfo(freeSlot);
			sendToSlot(freeSlot, info);
			std::println("[ServerNetwork] 槽位{}已连接，等待登录...", freeSlot);
			return;
		}

		if (!hasDisconnectedSlot && !pendingSocket) {
			// 既无空槽位也无掉线身份：可能是同账号重连来顶替，先收下等 LoginRequest 再判定
			selector.add(*newSocket);
			pendingSocket = std::move(newSocket);
			std::println("[ServerNetwork] 槽位已满，新连接进入待登录队列，等待重连识别...");
			return;
		}

		// 有掉线槽位时，新连接一律先挂起：确认账号后顶替回原槽位，避免重连分叉出第二个槽位
		if (hasDisconnectedSlot && !pendingSocket) {
			selector.add(*newSocket);
			pendingSocket = std::move(newSocket);
			std::println("[ServerNetwork] 检测到掉线槽位，新连接进入待登录队列，等待 LoginRequest 匹配账号...");
			return;
		}

		sf::Packet refuse = unool::protocol::makeStringMessage(unool::protocol::MessageType::ConnectionRefused, "服务器已满");
		(void)newSocket->send(refuse);
		newSocket->disconnect();
		std::println("[ServerNetwork] 新连接被拒（待登录队列已占用）");
	}

	void ServerNetwork::handleClientPackets() {
		for (std::size_t i = 0; i < capacity_; ++i) {
			sf::TcpSocket* socket = nullptr;
			{
				std::lock_guard lock(mutex_);
				if (i < slots_.size() && slots_[i].socket) socket = slots_[i].socket.get();
			}
			if (!socket) continue;
			if (!selector.isReady(*socket)) continue;

			sf::Packet packet;
			const auto status = socket->receive(packet);
			if (status == sf::Socket::Status::Done) {
				{
					std::lock_guard lock(mutex_);
					slots_[i].lastRecv = std::chrono::steady_clock::now();
				}
				sf::Packet peek = packet;
				int msgType{};
				if (!(peek >> msgType)) continue;
				const auto type = static_cast<unool::protocol::MessageType>(msgType);

				switch (type) {
					case unool::protocol::MessageType::Heartbeat:
						break;  // 心跳回包：lastRecv 已重置，不入队
					case unool::protocol::MessageType::RegisterRequest:
					case unool::protocol::MessageType::LoginRequest:
					case unool::protocol::MessageType::CheckUsernameRequest:
						handleAccountPacket(i, type, std::move(packet));
						break;
					case unool::protocol::MessageType::ShopPurchase: {
						// 商城购买请求：内联处理，不入队
						int discardedType{};
						std::string purchaseData;
						if (shopService_ && (packet >> discardedType >> purchaseData)) {
							shopService_->get().handleShopPurchase(i, purchaseData);
						}
						break;
					}
					case unool::protocol::MessageType::SkillInfoRequest: {
						// 技能描述请求：内联处理，不入队
						int discardedType{};
						std::string requestData;
						if (shopService_ && (packet >> discardedType >> requestData)) {
							shopService_->get().handleSkillInfoRequest(i, requestData);
						}
						break;
					}
					case unool::protocol::MessageType::CheckinRequest:
						// 每日签到请求：内联处理，不入队
						if (shopService_) shopService_->get().handleCheckin(i);
						break;
					case unool::protocol::MessageType::RoomListRequest:
					case unool::protocol::MessageType::RoomCreateRequest:
					case unool::protocol::MessageType::RoomJoinRequest:
					case unool::protocol::MessageType::RoomLeaveRequest:
					case unool::protocol::MessageType::RoomReadyRequest:
						handleRoomPacket(i, type, std::move(packet));
						break;
					case unool::protocol::MessageType::ClientInput:
						handleClientInput(i, std::move(packet));
						break;
					default:
						break;
				}
			}
			else if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
				std::println("[ServerNetwork] 槽位{} 断开连接（status={}）", i, static_cast<int>(status));
				removeClient(i);
			}
		}
	}

	void ServerNetwork::handleAccountPacket(const std::size_t clientIdx, const unool::protocol::MessageType type, sf::Packet packet) {
		if (!accountService_) return;
		int discarded{};
		packet >> discarded;

		// Arch-C P1-1：登录/注册包首字段带协议版本，不匹配时明确拒绝（不进业务分支）。
		const bool withVersion = (type == unool::protocol::MessageType::LoginRequest || type == unool::protocol::MessageType::RegisterRequest);
		if (withVersion) {
			sf::Packet peek = packet;
			std::uint32_t version{};
			if (!(peek >> version)) return;
			if (!unool::protocol::isProtocolVersionCompatible(version)) {
				sf::Packet resp = unool::protocol::makeVersionMismatchResponse();
				(void)sendNow(clientIdx, resp);
				std::println("[ServerNetwork] 槽位{} 协议版本不兼容（{}，期望 {}），断开",
							 clientIdx, version, unool::protocol::kProtocolVersion);
				removeClient(clientIdx);
				return;
			}
		}

		// 同账号重连顶替：把 socket 挪回原槽位，房间成员身份与账号信息随之保留
		std::size_t slot = clientIdx;
		if (type == unool::protocol::MessageType::LoginRequest) {
			sf::Packet peek = packet;
			std::uint32_t version{};
			std::string username, password;
			if (peek >> version >> username >> password) {
				slot = reconcileReconnect(clientIdx, username);
			}
		}

		auto req = unool::protocol::parseAccountRequest(packet, withVersion);
		if (!req) return;
		accountService_->get().handleAccountRequest(slot, type, *req);
	}

	std::size_t ServerNetwork::reconcileReconnect(const std::size_t newSlot, const std::string& username) {
		if (username.empty()) return newSlot;

		std::size_t target = capacity_;
		{
			std::lock_guard lock(mutex_);
			for (std::size_t i = 0; i < slots_.size(); ++i) {
				if (i == newSlot) continue;
				const auto& slot = slots_[i];
				if ((slot.loggedIn || slot.disconnected) && slot.username == username) {
					target = i;
					break;
				}
			}
			if (target >= slots_.size()) return newSlot;

			auto& oldSlot = slots_[target];
			// 旧连接还活着就踢掉；socket 对象地址不变，selector 里的注册仍然有效
			if (oldSlot.socket) {
				selector.remove(*oldSlot.socket);
				oldSlot.socket->disconnect();
				oldSlot.socket.reset();
			}
			oldSlot.socket = std::move(slots_[newSlot].socket);
			oldSlot.disconnected = false;
			oldSlot.lastRecv = std::chrono::steady_clock::now();

			// 新槽位只借出 socket，其余状态一律清空
			auto& temp = slots_[newSlot];
			temp.loggedIn = false;
			temp.disconnected = false;
			temp.username.clear();
			temp.points = 0;
			temp.wins = 0;
			temp.losses = 0;
			temp.roomId = 0;
			temp.outbox.clear();
		}

		std::println("[ServerNetwork] 检测到重连：账号 {} 顶替回槽位{}", username, target);
		sf::Packet info = unool::protocol::makeConnectionInfo(target);
		sendToSlot(target, info);
		return target;
	}

	void ServerNetwork::handlePendingSocket() {
		if (!pendingSocket || !selector.isReady(*pendingSocket)) return;

		sf::Packet packet;
		const auto status = pendingSocket->receive(packet);
		if (status == sf::Socket::Status::Disconnected || status == sf::Socket::Status::Error) {
			std::println("[ServerNetwork] 待登录队列的 socket 断开（status={}）", static_cast<int>(status));
			selector.remove(*pendingSocket);
			pendingSocket->disconnect();
			pendingSocket.reset();
			return;
		}
		if (status != sf::Socket::Status::Done) return;

		sf::Packet peek = packet;
		int msgType{};
		std::uint32_t version{};
		std::string username, password;
		if (!(peek >> msgType >> version >> username >> password)
			|| msgType != static_cast<int>(unool::protocol::MessageType::LoginRequest)) {
			selector.remove(*pendingSocket);
			pendingSocket->disconnect();
			pendingSocket.reset();
			std::println("[ServerNetwork] 待登录队列的连接被拒绝（首包不是登录请求）");
			return;
		}

		if (!unool::protocol::isProtocolVersionCompatible(version)) {
			sf::Packet resp = unool::protocol::makeVersionMismatchResponse();
			(void)pendingSocket->send(resp);
			selector.remove(*pendingSocket);
			pendingSocket->disconnect();
			pendingSocket.reset();
			std::println("[ServerNetwork] 拒绝旧版本客户端（版本 {}，期望 {}）",
						 version, unool::protocol::kProtocolVersion);
			return;
		}

		// 先找同账号的在线 / 掉线槽位（重连顶替），再找真正的空槽位
		std::size_t target = capacity_;
		{
			std::lock_guard lock(mutex_);
			for (std::size_t i = 0; i < slots_.size(); ++i) {
				if ((slots_[i].loggedIn || slots_[i].disconnected) && slots_[i].username == username) {
					target = i;
					break;
				}
			}
			if (target == capacity_) {
				for (std::size_t i = 0; i < slots_.size(); ++i) {
					if (!slots_[i].socket && !slots_[i].disconnected) {
						target = i;
						break;
					}
				}
			}
			if (target < slots_.size()) {
				auto& slot = slots_[target];
				if (slot.socket) {
					selector.remove(*slot.socket);
					slot.socket->disconnect();
					slot.socket.reset();
				}
				slot.socket = std::move(pendingSocket);
				slot.disconnected = false;
				slot.lastRecv = std::chrono::steady_clock::now();
			}
		}

		if (target >= capacity_) {
			sf::Packet resp = unool::protocol::makeAccountResponse(
				unool::protocol::MessageType::LoginResponse, false, "服务器已满，且无匹配的重连槽位");
			(void)pendingSocket->send(resp);
			selector.remove(*pendingSocket);
			pendingSocket->disconnect();
			pendingSocket.reset();
			std::println("[ServerNetwork] 待登录队列的连接被拒绝（无可用槽位，账号 {}）", username);
			return;
		}

		sf::Packet info = unool::protocol::makeConnectionInfo(target);
		sendToSlot(target, info);
		handleAccountPacket(target, unool::protocol::MessageType::LoginRequest, std::move(packet));
	}

	void ServerNetwork::sendHeartbeat() {
		if (heartbeatClock.getElapsedTime().asSeconds() < kHeartbeatSeconds) return;
		heartbeatClock.restart();

		const sf::Packet heartbeat = unool::protocol::makeHeartbeat();
		std::lock_guard lock(mutex_);
		for (auto& slot : slots_) {
			if (slot.socket && slot.loggedIn) slot.outbox.push_back(heartbeat);
		}
	}

	void ServerNetwork::checkTimeouts() {
		const auto now = std::chrono::steady_clock::now();
		std::vector<std::size_t> toRemove;
		std::vector<std::size_t> toFree;
		{
			std::lock_guard lock(mutex_);
			for (std::size_t i = 0; i < slots_.size(); ++i) {
				const auto& slot = slots_[i];
				if (slot.socket && slot.loggedIn
					&& std::chrono::duration<float>(now - slot.lastRecv).count() >= kTimeoutSeconds) {
					toRemove.push_back(i);
				}
				else if (!slot.socket && slot.disconnected
						 && std::chrono::duration<double>(now - slot.disconnectedAt).count()
								>= kDisconnectedSlotKeepSeconds) {
					toFree.push_back(i);
				}
			}
		}

		for (const std::size_t i : toRemove) {
			std::println("[ServerNetwork] 槽位{}（账号 {}）超过 {} 秒未响应，判定掉线", i, usernameOfSlot(i),
						 static_cast<int>(kTimeoutSeconds));
			removeClient(i);
		}
		for (const std::size_t i : toFree) {
			std::lock_guard lock(mutex_);
			std::println("[ServerNetwork] 槽位{}（账号 {}）重连超时，释放槽位", i, slots_[i].username);
			clearSlot(i);
		}
	}

	void ServerNetwork::removeClient(const std::size_t clientIdx) {
		if (clientIdx >= slots_.size()) return;
		std::string username;
		{
			std::lock_guard lock(mutex_);
			auto& slot = slots_[clientIdx];
			username = slot.username;
			if (slot.socket) {
				selector.remove(*slot.socket);
				slot.socket->disconnect();
				slot.socket.reset();
			}
			slot.loggedIn = false;
			slot.disconnected = true;
			slot.outbox.clear();
			slot.disconnectedAt = std::chrono::steady_clock::now();
			slot.lastRecv = slot.disconnectedAt;
		}
		// 房间通知在锁外发出，避免「网络锁 → 房间锁」的反向嵌套
		if (roomManager_) roomManager_->get().onClientDisconnected(clientIdx);
	}

	void ServerNetwork::clearSlot(const std::size_t clientIdx) {
		if (clientIdx >= slots_.size()) return;
		auto& slot = slots_[clientIdx];
		slot.loggedIn = false;
		slot.disconnected = false;
		slot.username.clear();
		slot.points = 0;
		slot.wins = 0;
		slot.losses = 0;
		slot.roomId = 0;
		slot.outbox.clear();
	}

	void ServerNetwork::handleRoomPacket(const std::size_t clientIdx, const unool::protocol::MessageType type, sf::Packet packet) {
		if (!roomManager_) return;
		int discarded{};
		packet >> discarded;
		roomManager_->get().handleRoomRequest(clientIdx, type, packet);
	}

	void ServerNetwork::handleClientInput(const std::size_t clientIdx, sf::Packet packet) {
		if (!roomManager_) return;
		// parseClientInput 的约定是「消息号已消费」（见 MessageCodec.h）：先吃掉消息号，
		// 否则第一个字段会把 101（ClientInput 的消息号）当成扫描码读进来 —— 而 101 恰好
		// 等于 Scancode::Numpad4，于是任何按键都被解析成「按了数字 4」。
		int discarded{};
		if (!(packet >> discarded)) return;
		auto data = unool::protocol::parseClientInput(packet);
		if (!data) {
			std::println(stderr, "[ServerNetwork] 槽位{} 的输入包解析失败（字段缺失或格式不符）", clientIdx);
			return;
		}
		roomManager_->get().routeInput(clientIdx, static_cast<sf::Keyboard::Scancode>(data->key),
									   data->selectedIndex);
	}

	// === 线程安全的状态查询 ===

	bool ServerNetwork::isSlotOnline(const std::size_t slot) const {
		std::lock_guard lock(mutex_);
		return slot < slots_.size() && slots_[slot].socket != nullptr;
	}

	std::string ServerNetwork::usernameOfSlot(const std::size_t slot) const {
		std::lock_guard lock(mutex_);
		if (slot >= slots_.size()) return {};
		return slots_[slot].username;
	}

	int ServerNetwork::pointsOfSlot(const std::size_t slot) const {
		std::lock_guard lock(mutex_);
		if (slot >= slots_.size()) return 0;
		return slots_[slot].points;
	}

	std::uint64_t ServerNetwork::roomIdOfSlot(const std::size_t slot) const {
		std::lock_guard lock(mutex_);
		if (slot >= slots_.size()) return 0;
		return slots_[slot].roomId;
	}

	void ServerNetwork::setRoomId(const std::size_t slot, const std::uint64_t roomId) {
		std::lock_guard lock(mutex_);
		if (slot >= slots_.size()) return;
		slots_[slot].roomId = roomId;
	}

	bool ServerNetwork::trySpendPoints(const std::size_t clientIdx, const int amount) {
		if (!accountService_) return false;
		return accountService_->get().spendPoints(clientIdx, amount);
	}

	// === IAccountSession ===

	std::size_t ServerNetwork::capacity() const {
		return capacity_;
	}

	bool ServerNetwork::isConnected(const std::size_t clientIdx) const {
		return isSlotOnline(clientIdx);
	}

	bool ServerNetwork::isLoggedIn(const std::size_t clientIdx) const {
		std::lock_guard lock(mutex_);
		return clientIdx < slots_.size() && slots_[clientIdx].socket && slots_[clientIdx].loggedIn;
	}

	bool ServerNetwork::isDisconnected(const std::size_t clientIdx) const {
		std::lock_guard lock(mutex_);
		return clientIdx < slots_.size() && slots_[clientIdx].disconnected;
	}

	std::string ServerNetwork::usernameOf(const std::size_t clientIdx) const {
		return usernameOfSlot(clientIdx);
	}

	bool ServerNetwork::isUsernameLoggedInElsewhere(const std::size_t excludeIdx,
													const std::string& username) const {
		if (username.empty()) return false;
		std::lock_guard lock(mutex_);
		for (std::size_t i = 0; i < slots_.size(); ++i) {
			if (i == excludeIdx) continue;
			if (slots_[i].loggedIn && slots_[i].socket && slots_[i].username == username) return true;
		}
		return false;
	}

	void ServerNetwork::dropConnection(const std::size_t clientIdx) {
		std::lock_guard lock(mutex_);
		if (clientIdx >= slots_.size()) return;
		if (slots_[clientIdx].socket) {
			selector.remove(*slots_[clientIdx].socket);
			slots_[clientIdx].socket->disconnect();
			slots_[clientIdx].socket.reset();
		}
	}

	void ServerNetwork::onLoginSucceeded(const std::size_t clientIdx, const std::string& username,
										 const int points, const int wins, const int losses) {
		const bool isReconnect = isDisconnected(clientIdx);
		{
			std::lock_guard lock(mutex_);
			if (clientIdx >= slots_.size()) return;
			auto& slot = slots_[clientIdx];
			slot.loggedIn = true;
			slot.disconnected = false;
			slot.username = username;
			slot.points = points;
			slot.wins = wins;
			slot.losses = losses;
			slot.lastRecv = std::chrono::steady_clock::now();
		}
		std::println("[ServerNetwork] 槽位{} {}: {}（积分 {}）", clientIdx,
					 (isReconnect ? "重连" : "登录"), username, points);
		// 锁外通知房间：重连回原槽位时，把房间快照（或开局座位）补发给客户端
		if (roomManager_) roomManager_->get().onClientLoggedIn(clientIdx);
	}

	void ServerNetwork::onPointsSpent(const std::size_t clientIdx, const int amount) {
		std::lock_guard lock(mutex_);
		if (clientIdx >= slots_.size()) return;
		slots_[clientIdx].points -= amount;
	}

	void ServerNetwork::onPointsChanged(const std::size_t clientIdx, const int points) {
		std::lock_guard lock(mutex_);
		if (clientIdx >= slots_.size()) return;
		slots_[clientIdx].points = points;
	}

	void ServerNetwork::sendAccountResponse(const std::size_t clientIdx, const unool::protocol::MessageType type,
											const bool ok, const std::string& msg,
											const int points, const int wins, const int losses) {
		sf::Packet packet = unool::protocol::makeAccountResponse(type, ok, msg, points, wins, losses);
		sendToSlot(clientIdx, packet);
	}

	void ServerNetwork::sendCheckUsernameResponse(const std::size_t clientIdx, const bool exists) {
		sf::Packet packet = unool::protocol::makeCheckUsernameResponse(exists);
		sendToSlot(clientIdx, packet);
	}

	bool ServerNetwork::sendPayload(const std::size_t clientIdx, const unool::protocol::MessageType type,
									const std::string& jsonPayload) {
		sf::Packet packet = unool::protocol::makeStringMessage(type, jsonPayload);
		return sendToSlot(clientIdx, packet);
	}

	void ServerNetwork::sendCheckinResponse(const std::size_t clientIdx, const std::string& msg,
											const bool ok, const int points) {
		sf::Packet packet = unool::protocol::makeCheckinResponse(msg, ok, points);
		sendToSlot(clientIdx, packet);
	}

	bool ServerNetwork::sendShopResult(const std::size_t clientIdx, const std::string& resultJson) {
		sf::Packet packet = unool::protocol::makeStringMessage(unool::protocol::MessageType::ShopResult, resultJson);
		return sendToSlot(clientIdx, packet);
	}

}
