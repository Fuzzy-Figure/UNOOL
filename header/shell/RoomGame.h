#pragma once
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include <SFML/Window/Keyboard.hpp>

#include "utils.h"
#include "MessageType.h"
#include "contract/Keys.h"
#include "core/IDecisionPort.h"
#include "core/IServerView.h"
#include "core/Services.h"

// sf::Packet 只前向声明：本头可供不认识 SFML 网络类型的编译单元包含。
namespace sf { class Packet; }

class GameLogic;

namespace unool::shell {

	class RoomGame;
	class RoomManager;
	class ServerNetwork;

	// 唯一的「SFML 扫描码 -> 领域键码」翻译点（原 LegacyBlockingDecisionAdapter 平移）。
	// 入参是线上 ClientInput.key 反序列化得到的 sf::Keyboard::Scancode，
	// 返回值是领域语义键码；端口与领域层不再出现任何底层扫描码数值。
	// 未列出的键（含 Unknown）一律回落 Key::Unknown。
	unool::contract::Key toDomainKey(sf::Keyboard::Scancode sc);

	// 房间对局的服务端出站视图（IServerView 实现）。
	// 与原 NetworkServerView 的区别只有「座位号 → 连接槽位」的映射：
	// 座位 0/1 是本局玩法身份，槽位是连接编号，两者由房间成员顺序绑定。
	class RoomServerView final : public unool::core::IServerView {
	public:
		explicit RoomServerView(RoomGame& game) : game_(game) {}

		void attach(GameLogic& logic) { logic_ = std::ref(logic); }

		void onState() override;
		void onCharInfo() override;
		std::string usernameOf(unool::core::player_id id) const override;
		bool spendPoints(unool::core::player_id id, int amount) override;

	private:
		RoomGame& game_;
		opt_ref<GameLogic> logic_{};
	};

	// 房间对局的决策通道（IDecisionPort 实现）。
	// poll() 不再驱动网络：socket 由 ServerNetwork 的 IO 线程独占，
	// 房间线程只从自己的输入队列取包，因此多房间真正并行而互不阻塞。
	class RoomDecisionAdapter final : public unool::core::IDecisionPort {
	public:
		explicit RoomDecisionAdapter(RoomGame& game) : game_(game) {}

		void poll() override;
		std::optional<unool::core::DecisionInput> takeInput() override;
		void showChoice(unool::core::player_id id, const unool::core::ChoicePrompt& prompt) override;
		void clearChoice(unool::core::player_id id) override;
		bool isConnected(unool::core::player_id id) const override;
		bool isLoggedIn(unool::core::player_id id) const override;
		int reconnectTimeoutSec() const override;

	private:
		RoomGame& game_;
	};

	// 一个房间的对局：一份 GameLogic + 一个独立线程。
	//
	// 生命周期：RoomManager 用 shared_ptr 持有；start() 里把 shared_ptr 捕获进线程，
	// 线程跑完对局后回调 RoomManager::onGameFinished() 交回房间状态，然后自行退出。
	// 因此房间线程永远持有自己的对象，RoomManager 清指针不会把它拽掉。
	class RoomGame final : public std::enable_shared_from_this<RoomGame> {
	public:
		RoomGame(std::uint64_t roomId, std::string roomName,
				 std::array<std::size_t, 2> slots, std::array<std::string, 2> names,
				 ServerNetwork& net, RoomManager& manager, unool::core::DomainServices& services);
		// 析构必须在 RoomGame.cpp 里定义：成员 unique_ptr<GameLogic> 的删除需要完整的
		// GameLogic，而本头只前向声明它（否则包含本头的其它 .cpp 会报
		// 「can't delete an incomplete type」）。
		~RoomGame();

		// 启动对局线程（立即返回）
		void start();

		// === IO 线程 -> 房间线程 ===
		void pushInput(std::size_t seat, unool::core::DecisionInput input);

		// === 房间线程使用 ===
		void waitInput(std::chrono::milliseconds timeout);
		std::optional<unool::core::DecisionInput> takeInput();
		bool isSeatOnline(std::size_t seat) const;
		std::size_t slotOfSeat(std::size_t seat) const;
		const std::string& nameOfSeat(std::size_t seat) const;
		bool sendToSeat(std::size_t seat, const sf::Packet& packet);
		// 房间内消费积分（技能扣分走这条，与账号用例共用一份存档）
		bool spendPointsOfSeat(std::size_t seat, int amount);
		int reconnectTimeoutSec() const;

		std::uint64_t roomId() const { return roomId_; }
		const std::string& roomName() const { return roomName_; }

	private:
		void run();
		void sessionLoop();
		void gameLoop();
		void handleGameOver();
		bool continueRequested();
		void initCharacters();

		std::uint64_t roomId_;
		std::string roomName_;
		std::array<std::size_t, 2> slots_;
		std::array<std::string, 2> names_;
		ServerNetwork& net_;
		RoomManager& manager_;
		unool::core::DomainServices& services_;

		std::mutex inputMutex_;
		std::condition_variable inputCv_;
		std::deque<unool::core::DecisionInput> inputs_;

		RoomServerView view_;
		RoomDecisionAdapter decision_;
		std::unique_ptr<GameLogic> logic_;
	};

}
