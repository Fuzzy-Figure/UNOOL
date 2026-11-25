#include "RoomManager.h"

#include "RoomGame.h"
#include "ServerNetwork.h"
#include "MessageCodec.h"

#include <SFML/Network.hpp>

#include <algorithm>
#include <format>
#include <print>
#include <utility>

namespace unool::shell {

	namespace {
		// 房间名长度上限（字节）。超出时按 UTF-8 边界截断，避免把多字节字符切一半。
		constexpr std::size_t kMaxRoomNameBytes = 64;

		std::string trimRoomName(const std::string& raw) {
			const auto isSpace = [](const unsigned char c) {
				return c == ' ' || c == '\t' || c == '\r' || c == '\n';
			};
			std::size_t begin = 0;
			std::size_t end = raw.size();
			while (begin < end && isSpace(static_cast<unsigned char>(raw[begin]))) ++begin;
			while (end > begin && isSpace(static_cast<unsigned char>(raw[end - 1]))) --end;
			std::string name = raw.substr(begin, end - begin);
			if (name.size() > kMaxRoomNameBytes) {
				std::size_t cut = kMaxRoomNameBytes;
				// 回退到 UTF-8 起始字节（0b10xxxxxx 是续字节）
				while (cut > 0 && (static_cast<unsigned char>(name[cut]) & 0xC0) == 0x80) --cut;
				name.resize(cut);
			}
			return name;
		}
	}

	RoomManager::RoomManager(ServerNetwork& net, unool::core::DomainServices& services)
		: net_(net), services_(services) {}

	RoomManager::~RoomManager() = default;

	// === IO 线程入口 ===

	void RoomManager::handleRoomRequest(const std::size_t slot, const unool::protocol::MessageType type, sf::Packet& packet) {
		switch (type) {
			case unool::protocol::MessageType::RoomListRequest:
			{
				std::lock_guard lock(mutex_);
				sendRoomListLocked(slot);
				return;
			}
			case unool::protocol::MessageType::RoomCreateRequest:
			{
				auto roomName = unool::protocol::parseRoomNameRequest(packet);
				if (!roomName) return;
				std::lock_guard lock(mutex_);
				createRoomLocked(slot, *roomName);
				return;
			}
			case unool::protocol::MessageType::RoomJoinRequest:
			{
				auto roomId = unool::protocol::parseRoomIdRequest(packet);
				if (!roomId) return;
				std::lock_guard lock(mutex_);
				joinRoomLocked(slot, *roomId);
				return;
			}
			case unool::protocol::MessageType::RoomLeaveRequest:
			{
				std::lock_guard lock(mutex_);
				removeMemberLocked(slot, "已离开房间");
				return;
			}
			case unool::protocol::MessageType::RoomReadyRequest:
			{
				auto ready = unool::protocol::parseRoomReadyRequest(packet);
				if (!ready) return;
				std::lock_guard lock(mutex_);
				setReadyLocked(slot, *ready);
				return;
			}
			default:
				return;
		}
	}

	void RoomManager::routeInput(const std::size_t slot, const sf::Keyboard::Scancode key,
								 const std::size_t selectedIndex) {
		std::lock_guard lock(mutex_);
		Room* room = findRoomLocked(net_.roomIdOfSlot(slot));
		if (!room || !room->game) return;
		for (std::size_t seat = 0; seat < room->members.size(); ++seat) {
			if (room->members[seat].slot != slot) continue;
			room->game->pushInput(
				seat, unool::core::DecisionInput{ seat, toDomainKey(key), selectedIndex, std::nullopt });
			return;
		}
	}

	void RoomManager::routeChoiceSelect(const std::size_t slot, const std::size_t choiceIndex,
										const std::size_t selectedIndex) {
		std::lock_guard lock(mutex_);
		Room* room = findRoomLocked(net_.roomIdOfSlot(slot));
		if (!room || !room->game) return;
		for (std::size_t seat = 0; seat < room->members.size(); ++seat) {
			if (room->members[seat].slot != slot) continue;
			room->game->pushInput(
				seat, unool::core::DecisionInput{ seat, unool::contract::Key::Unknown, selectedIndex, choiceIndex });
			return;
		}
	}

	void RoomManager::onClientDisconnected(const std::size_t slot) {
		std::lock_guard lock(mutex_);
		Room* room = findRoomLocked(net_.roomIdOfSlot(slot));
		if (!room) {
			net_.setRoomId(slot, 0);
			return;
		}
		// 对局中掉线：保留成员身份，等待同账号重连顶替回原座位
		if (room->inGame) {
			std::println("[Room {}] 成员 {} 掉线，保留座位等待重连", room->id, net_.usernameOfSlot(slot));
			return;
		}
		removeMemberLocked(slot, "成员掉线，已离开房间");
	}

	void RoomManager::onClientLoggedIn(const std::size_t slot) {
		std::lock_guard lock(mutex_);
		Room* room = findRoomLocked(net_.roomIdOfSlot(slot));
		if (!room) return;

		// 重连回原槽位：把房间快照补发一次，客户端才能回到房间界面
		sf::Packet snapshot = unool::protocol::makeRoomStateResponse(snapshotOfLocked(*room));
		net_.sendToSlot(slot, snapshot);

		// 对局中还要补发开局座位号（客户端据此从 GameState 认出自己的手牌）
		if (room->inGame) {
			for (std::size_t seat = 0; seat < room->members.size(); ++seat) {
				if (room->members[seat].slot != slot) continue;
				sf::Packet start = unool::protocol::makeGameStart(seat);
				net_.sendToSlot(slot, start);
				break;
			}
		}
		std::println("[Room {}] 成员 {} 已回到房间", room->id, net_.usernameOfSlot(slot));
	}

	// === 房间线程入口 ===

	void RoomManager::onGameFinished(const std::uint64_t roomId) {
		std::lock_guard lock(mutex_);
		Room* room = findRoomLocked(roomId);
		if (!room) return;

		room->game.reset();
		room->inGame = false;
		for (auto& member : room->members) member.ready = false;

		// 对局期间掉线且没回来的成员直接移出房间
		for (auto it = room->members.begin(); it != room->members.end();) {
			if (!net_.isSlotOnline(it->slot)) {
				net_.setRoomId(it->slot, 0);
				it = room->members.erase(it);
			}
			else {
				++it;
			}
		}

		if (room->members.empty()) {
			std::println("[Room {}] 对局结束后已无在线成员，房间解散", roomId);
			rooms_.erase(roomId);
			broadcastRoomListLocked();
			return;
		}

		bool ownerPresent = false;
		for (const auto& member : room->members) {
			if (member.slot == room->ownerSlot) ownerPresent = true;
		}
		if (!ownerPresent) room->ownerSlot = room->members.front().slot;

		broadcastRoomStateLocked(*room);
		broadcastRoomListLocked();
	}

	// === 房间操作（持锁） ===

	std::size_t RoomManager::maxRoomsLocked() const {
		return std::max<std::size_t>(1, net_.capacity() / unool::protocol::kRoomCapacity);
	}

	RoomManager::Room* RoomManager::findRoomLocked(const std::uint64_t roomId) {
		if (roomId == unool::protocol::kNoRoom) return nullptr;
		const auto it = rooms_.find(roomId);
		return it == rooms_.end() ? nullptr : &it->second;
	}

	RoomManager::Member* RoomManager::findMemberLocked(Room& room, const std::size_t slot) {
		for (auto& member : room.members) {
			if (member.slot == slot) return &member;
		}
		return nullptr;
	}

	void RoomManager::createRoomLocked(const std::size_t slot, const std::string& roomName) {
		if (!net_.isSlotOnline(slot)) return;
		if (net_.roomIdOfSlot(slot) != unool::protocol::kNoRoom) {
			sendError(slot, "你已在房间中");
			return;
		}
		if (rooms_.size() >= maxRoomsLocked()) {
			sendError(slot, "房间数量已达上限，请稍后再试");
			return;
		}

		const std::string username = net_.usernameOfSlot(slot);
		std::string name = trimRoomName(roomName);
		if (name.empty()) name = std::format("{}的房间", username);

		Room room;
		room.id = nextRoomId_++;
		room.name = name;
		room.ownerSlot = slot;
		room.members.push_back(Member{ slot, username, false });

		const std::uint64_t roomId = room.id;
		rooms_.emplace(roomId, std::move(room));
		net_.setRoomId(slot, roomId);

		if (Room* created = findRoomLocked(roomId)) {
			sf::Packet snapshot = unool::protocol::makeRoomStateResponse(snapshotOfLocked(*created));
			net_.sendToSlot(slot, snapshot);
		}
		broadcastRoomListLocked();
		std::println("[Room {}] 由 {} 创建，房间名：{}", roomId, username, name);
	}

	void RoomManager::joinRoomLocked(const std::size_t slot, const std::uint64_t roomId) {
		if (!net_.isSlotOnline(slot)) return;
		if (net_.roomIdOfSlot(slot) != unool::protocol::kNoRoom) {
			sendError(slot, "你已在房间中");
			return;
		}
		Room* room = findRoomLocked(roomId);
		if (!room) {
			sendError(slot, "房间不存在或已解散");
			return;
		}
		if (room->inGame) {
			sendError(slot, "房间正在对局中，无法加入");
			return;
		}
		if (room->members.size() >= unool::protocol::kRoomCapacity) {
			sendError(slot, "房间已满");
			return;
		}

		const std::string username = net_.usernameOfSlot(slot);
		room->members.push_back(Member{ slot, username, false });
		net_.setRoomId(slot, roomId);

		broadcastRoomStateLocked(*room);
		broadcastRoomListLocked();
		std::println("[Room {}] {} 加入房间（{}/{}）", roomId, username, room->members.size(),
					 unool::protocol::kRoomCapacity);
	}

	void RoomManager::removeMemberLocked(const std::size_t slot, const std::string& reason) {
		const std::uint64_t roomId = net_.roomIdOfSlot(slot);
		if (roomId == unool::protocol::kNoRoom) {
			// 不在房间：回一个 closed 快照，客户端据此稳定回到大厅
			unool::protocol::RoomSnapshot empty{};
			empty.closed = true;
			empty.reason = reason;
			sf::Packet packet = unool::protocol::makeRoomStateResponse(empty);
			net_.sendToSlot(slot, packet);
			return;
		}

		Room* room = findRoomLocked(roomId);
		if (!room) {
			net_.setRoomId(slot, unool::protocol::kNoRoom);
			unool::protocol::RoomSnapshot empty{};
			empty.closed = true;
			empty.reason = reason;
			sf::Packet packet = unool::protocol::makeRoomStateResponse(empty);
			net_.sendToSlot(slot, packet);
			return;
		}
		if (room->inGame) {
			// 对局中不允许主动离开：客户端也不提供该按钮，这里只做兜底
			sendError(slot, "对局进行中，无法离开房间");
			return;
		}

		sendClosedSnapshotLocked(slot, *room, reason);
		net_.setRoomId(slot, unool::protocol::kNoRoom);

		const std::string username = net_.usernameOfSlot(slot);
		room->members.erase(
			std::remove_if(room->members.begin(), room->members.end(),
						   [slot](const Member& member) { return member.slot == slot; }),
			room->members.end());
		// 有人离开 → 其余人准备状态清零，避免「二手准备」直接开局
		for (auto& member : room->members) member.ready = false;

		std::println("[Room {}] {} 离开房间（原因：{}），剩余 {} 人", roomId, username, reason,
					 room->members.size());

		if (room->members.empty()) {
			rooms_.erase(roomId);
			std::println("[Room {}] 已无成员，房间解散", roomId);
		}
		else {
			// 房主退出：房主身份转让给仍在房间里的另一名成员
			bool ownerPresent = false;
			for (const auto& member : room->members) {
				if (member.slot == room->ownerSlot) ownerPresent = true;
			}
			if (!ownerPresent) {
				room->ownerSlot = room->members.front().slot;
				std::println("[Room {}] 房主转让给 {}", roomId, room->members.front().name);
			}
			broadcastRoomStateLocked(*room);
		}
		broadcastRoomListLocked();
	}

	void RoomManager::setReadyLocked(const std::size_t slot, const bool ready) {
		Room* room = findRoomLocked(net_.roomIdOfSlot(slot));
		if (!room || room->inGame) return;
		Member* member = findMemberLocked(*room, slot);
		if (!member) return;
		if (member->ready == ready) return;

		member->ready = ready;
		std::println("[Room {}] {} {}", room->id, member->name, (ready ? "已准备" : "取消准备"));
		broadcastRoomStateLocked(*room);

		if (room->members.size() < unool::protocol::kRoomCapacity) return;
		const bool allReady = std::all_of(room->members.begin(), room->members.end(),
										  [](const Member& m) { return m.ready; });
		if (allReady) startGameLocked(*room);
	}

	void RoomManager::startGameLocked(Room& room) {
		room.inGame = true;

		std::array<std::size_t, 2> slots{};
		std::array<std::string, 2> names{};
		for (std::size_t seat = 0; seat < unool::protocol::kRoomCapacity; ++seat) {
			slots[seat] = room.members[seat].slot;
			names[seat] = room.members[seat].name;
		}

		// 开局只发 GameStart，不再补发「inGame=true」的房间快照：
		// 客户端见到 GameStart 就切到对局界面，而任何后到的 RoomState 都会把界面按回房间页
		// （旧客户端更是无条件回房间页，表现为「双方准备后停在房间页，要重连才进对局」）。
		// 房间快照在 setReadyLocked 里已经随准备状态发过，对局结束才算需要再发。
		// 大厅房间列表仍要刷新，「对局中」标记靠它广播。

		// 开局包携带座位号：客户端据此认出自己的手牌
		for (std::size_t seat = 0; seat < unool::protocol::kRoomCapacity; ++seat) {
			sf::Packet start = unool::protocol::makeGameStart(seat);
			net_.sendToSlot(slots[seat], start);
		}

		auto game = std::make_shared<RoomGame>(room.id, room.name, slots, names, net_, *this, services_);
		room.game = game;
		game->start();

		broadcastRoomListLocked();
		std::println("[Room {}] 双方已准备，对局开始：{} vs {}", room.id, names[0], names[1]);
	}

	// === 出站 ===

	void RoomManager::sendError(const std::size_t slot, const std::string& msg) {
		sf::Packet packet = unool::protocol::makeRoomErrorResponse(msg);
		net_.sendToSlot(slot, packet);
	}

	unool::protocol::RoomSnapshot RoomManager::snapshotOfLocked(const Room& room) const {
		unool::protocol::RoomSnapshot snapshot;
		snapshot.id = room.id;
		snapshot.name = room.name;
		snapshot.capacity = unool::protocol::kRoomCapacity;
		snapshot.inGame = room.inGame;
		for (const auto& member : room.members) {
			unool::protocol::RoomMember item;
			item.name = member.name;
			item.ready = member.ready;
			item.owner = (member.slot == room.ownerSlot);
			if (item.owner) snapshot.owner = member.name;
			snapshot.members.push_back(std::move(item));
		}
		return snapshot;
	}

	void RoomManager::sendClosedSnapshotLocked(const std::size_t slot, const Room& room,
											   const std::string& reason) {
		unool::protocol::RoomSnapshot snapshot = snapshotOfLocked(room);
		snapshot.closed = true;
		snapshot.reason = reason;
		snapshot.members.clear();
		sf::Packet packet = unool::protocol::makeRoomStateResponse(snapshot);
		net_.sendToSlot(slot, packet);
	}

	void RoomManager::broadcastRoomStateLocked(const Room& room) {
		sf::Packet packet = unool::protocol::makeRoomStateResponse(snapshotOfLocked(room));
		for (const auto& member : room.members) {
			net_.sendToSlot(member.slot, packet);
		}
	}

	void RoomManager::sendRoomListLocked(const std::size_t slot) {
		std::vector<unool::protocol::RoomSummary> rooms;
		rooms.reserve(rooms_.size());
		for (const auto& [id, room] : rooms_) {
			unool::protocol::RoomSummary summary;
			summary.id = id;
			summary.name = room.name;
			summary.members = room.members.size();
			summary.capacity = unool::protocol::kRoomCapacity;
			summary.inGame = room.inGame;
			for (const auto& member : room.members) {
				if (member.slot == room.ownerSlot) summary.owner = member.name;
			}
			rooms.push_back(std::move(summary));
		}
		sf::Packet packet = unool::protocol::makeRoomListResponse(rooms);
		net_.sendToSlot(slot, packet);
	}

	void RoomManager::broadcastRoomListLocked() {
		std::vector<unool::protocol::RoomSummary> rooms;
		rooms.reserve(rooms_.size());
		for (const auto& [id, room] : rooms_) {
			unool::protocol::RoomSummary summary;
			summary.id = id;
			summary.name = room.name;
			summary.members = room.members.size();
			summary.capacity = unool::protocol::kRoomCapacity;
			summary.inGame = room.inGame;
			for (const auto& member : room.members) {
				if (member.slot == room.ownerSlot) summary.owner = member.name;
			}
			rooms.push_back(std::move(summary));
		}

		// 只推给大厅里的人：房间内成员看自己的房间快照，不需要列表
		const sf::Packet packet = unool::protocol::makeRoomListResponse(rooms);
		for (std::size_t slot = 0; slot < net_.capacity(); ++slot) {
			if (!net_.isSlotOnline(slot)) continue;
			if (net_.roomIdOfSlot(slot) != unool::protocol::kNoRoom) continue;
			net_.sendToSlot(slot, packet);
		}
	}

}
