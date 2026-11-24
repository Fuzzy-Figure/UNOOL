#include "ConnectionState.h"

#include "IClientGateway.h"
#include "MessageCodec.h"
#include "AccountProtocol.h"
#include "RoomProtocol.h"
#include "ShopProtocol.h"
#include <utility>

// Arch-C P1-3：连接状态机。收包 / 心跳 / 断线的唯一时序点。
// 原三处 net.update() + 已删除的 PacketDispatcher::next() + 散落心跳分支全部收敛到这里；
// 消息号与包体字节、心跳回复包均与重构前逐字一致。

namespace unool::shell {

	ConnectionState::ConnectionState(IClientGateway& gateway)
		: gateway_(gateway) {}

	void ConnectionState::pump() {
		gateway_.update();

		// Drain 本帧 socket 上所有包：心跳就地回复，其余打 tag 入显式队列。
		while (auto packetOpt = gateway_.receivePacket()) {
			sf::Packet packet = std::move(*packetOpt);
			int msgType{};
			if (!(packet >> msgType)) continue;  // 坏包丢弃，与原 Dispatcher 一致
			const auto type = static_cast<MessageType>(msgType);
			if (type == MessageType::Heartbeat) {
				sf::Packet hb = unool::protocol::makeHeartbeat();
				gateway_.send(hb);
				continue;
			}
			// 顶下线通知：服务端随后就会关连接。这里先记下原因再主动断开，
			// 让所有阶段立刻按「断线」退出，组合根再据此把用户带回登录界面。
			if (type == MessageType::SessionReplaced) {
				kickedReason_ = unool::protocol::parseSessionReplaced(packet)
									.value_or(std::string{ "你已在别处登录，本端被顶下线" });
				kicked_ = true;
				queue_.clear();
				gateway_.disconnect();
				wasConnected_ = false;
				return;
			}
			queue_.push_back(Inbound{ type, std::move(packet) });
		}

		// 断线可枚举：连接态 → 断线态时清空积压，避免旧包跨会话串味。
		const bool nowConnected = gateway_.isConnected();
		if (wasConnected_ && !nowConnected) queue_.clear();
		wasConnected_ = nowConnected;
	}

	std::optional<Inbound> ConnectionState::take(
		std::initializer_list<MessageType> subscription) {
		if (queue_.empty()) return std::nullopt;
		const MessageType front = queue_.front().type;
		for (auto type : subscription) {
			if (type == front) {
				Inbound out = std::move(queue_.front());
				queue_.pop_front();
				return out;
			}
		}
		// 队首未命中：不消费、留在队列给下一阶段（跨阶段留包语义）。
		return std::nullopt;
	}

	std::optional<Inbound> ConnectionState::takeAny() {
		if (queue_.empty()) return std::nullopt;
		Inbound out = std::move(queue_.front());
		queue_.pop_front();
		return out;
	}

	bool ConnectionState::contains(MessageType type) const {
		for (const Inbound& in : queue_) {
			if (in.type == type) return true;
		}
		return false;
	}

	void ConnectionState::clear() {
		queue_.clear();
	}

	bool ConnectionState::isConnected() const {
		return gateway_.isConnected();
	}

	bool ConnectionState::send(sf::Packet& packet) {
		return gateway_.send(packet);
	}

	bool ConnectionState::sendShopPurchase(const std::string& purchaseData) {
		return gateway_.sendShopPurchase(purchaseData);
	}

	void ConnectionState::setPlayerId(std::size_t id) {
		gateway_.setPlayerId(id);
	}

	std::size_t ConnectionState::getPlayerId() const {
		return gateway_.getPlayerId();
	}

	const std::string& ConnectionState::getUsername() const {
		return gateway_.getUsername();
	}

	// === P1-4 语义化出站 ===
	bool ConnectionState::sendLogin(const std::string& username, const std::string& password) {
		sf::Packet p = unool::protocol::makeLoginRequest(username, password);
		return gateway_.send(p);
	}

	bool ConnectionState::sendRegister(const std::string& username, const std::string& password) {
		sf::Packet p = unool::protocol::makeRegisterRequest(username, password);
		return gateway_.send(p);
	}

	bool ConnectionState::sendCheckUsername(const std::string& username) {
		sf::Packet p = unool::protocol::makeCheckUsernameRequest(username);
		return gateway_.send(p);
	}

	bool ConnectionState::sendCheckin() {
		sf::Packet p = unool::protocol::makeCheckinRequest();
		return gateway_.send(p);
	}

	bool ConnectionState::requestRoomList() {
		sf::Packet p = unool::protocol::makeRoomListRequest();
		return gateway_.send(p);
	}

	bool ConnectionState::createRoom(const std::string& roomName) {
		sf::Packet p = unool::protocol::makeRoomCreateRequest(roomName);
		return gateway_.send(p);
	}

	bool ConnectionState::joinRoom(const std::uint64_t roomId) {
		sf::Packet p = unool::protocol::makeRoomJoinRequest(roomId);
		return gateway_.send(p);
	}

	bool ConnectionState::leaveRoom() {
		sf::Packet p = unool::protocol::makeRoomLeaveRequest();
		return gateway_.send(p);
	}

	bool ConnectionState::sendRoomReady(const bool ready) {
		sf::Packet p = unool::protocol::makeRoomReadyRequest(ready);
		return gateway_.send(p);
	}

	bool ConnectionState::sendSkillInfoRequest(const std::vector<std::string>& names) {
		sf::Packet p = unool::protocol::makeSkillInfoRequestPacket(names);
		return gateway_.send(p);
	}

	bool ConnectionState::sendShopRequest() {
		return gateway_.sendShopPurchase(
			unool::protocol::makeShopPurchaseJson(ShopRequestType::Request, "", "", ""));
	}

	bool ConnectionState::sendBuyCharacter(const std::string& name) {
		return gateway_.sendShopPurchase(
			unool::protocol::makeShopPurchaseJson(ShopRequestType::Character, name, "", ""));
	}

	bool ConnectionState::sendBuySkin(const std::string& charName, const std::string& skinName) {
		return gateway_.sendShopPurchase(
			unool::protocol::makeShopPurchaseJson(ShopRequestType::Skin, "", charName, skinName));
	}

	bool ConnectionState::sendBuyItem(const std::string& itemName) {
		return gateway_.sendShopPurchase(
			unool::protocol::makeShopPurchaseJson(ShopRequestType::Item, itemName, "", ""));
	}

}
