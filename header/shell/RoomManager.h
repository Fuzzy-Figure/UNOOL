#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Window/Keyboard.hpp>

#include "MessageType.h"
#include "protocol/RoomProtocol.h"
#include "core/Services.h"

namespace sf { class Packet; }

namespace unool::shell {

	class ServerNetwork;
	class RoomGame;

	// 房间管理器：登录后的一切组队行为都在这里。
	//
	// 线程模型：
	//   - 除 onGameFinished() 外，所有方法都由 ServerNetwork 的 IO 线程调用；
	//   - onGameFinished() 由房间线程在对局结束时调用，用来把房间交回等待状态；
	//   - 内部所有房间状态由 mutex_ 保护；持有的锁顺序固定为「房间锁 → 网络锁」
	//     （方法内会调 ServerNetwork::sendToSlot / roomIdOfSlot），网络层不会反向回调，
	//     因此不会死锁。
	//
	// 房间规则（用户口径）：
	//   - 房间固定 2 人（= 一桌座位数），房间号由服务端生成；
	//   - 坐满后双方各自「准备」，两人都准备即开局；
	//   - 房主退出/掉线时自动把房主转让给另一名成员；最后一人离开后房间自动解散；
	//   - 对局中不允许加入、离开（断线保留成员身份，同账号重连顶替回原座位）。
	class RoomManager {
	public:
		RoomManager(ServerNetwork& net, unool::core::DomainServices& services);
		~RoomManager();

		// === IO 线程入口 ===
		// packet 已消费开头的 int 消息号
		void handleRoomRequest(std::size_t slot, unool::protocol::MessageType type, sf::Packet& packet);
		void routeInput(std::size_t slot, sf::Keyboard::Scancode key, std::size_t selectedIndex);
		void onClientDisconnected(std::size_t slot);
		void onClientLoggedIn(std::size_t slot);

		// === 房间线程入口 ===
		void onGameFinished(std::uint64_t roomId);

	private:
		struct Member {
			std::size_t slot = 0;
			std::string name;
			bool ready = false;
		};

		struct Room {
			std::uint64_t id = 0;
			std::string name;
			std::size_t ownerSlot = 0;
			std::vector<Member> members;
			bool inGame = false;
			std::shared_ptr<RoomGame> game;
		};

		// 以下 *Locked 版本要求调用方已持有 mutex_
		void sendRoomListLocked(std::size_t slot);
		void broadcastRoomListLocked();
		void broadcastRoomStateLocked(const Room& room);
		void sendError(std::size_t slot, const std::string& msg);
		void sendClosedSnapshotLocked(std::size_t slot, const Room& room, const std::string& reason);
		unool::protocol::RoomSnapshot snapshotOfLocked(const Room& room) const;

		void createRoomLocked(std::size_t slot, const std::string& roomName);
		void joinRoomLocked(std::size_t slot, std::uint64_t roomId);
		void removeMemberLocked(std::size_t slot, const std::string& reason);
		void setReadyLocked(std::size_t slot, bool ready);
		void startGameLocked(Room& room);

		Room* findRoomLocked(std::uint64_t roomId);
		Member* findMemberLocked(Room& room, std::size_t slot);
		std::size_t maxRoomsLocked() const;

		ServerNetwork& net_;
		unool::core::DomainServices& services_;
		std::mutex mutex_;
		std::unordered_map<std::uint64_t, Room> rooms_;
		std::uint64_t nextRoomId_ = 1;
	};

}
