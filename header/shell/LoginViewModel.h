#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "protocol/AccountEvent.h"
#include "protocol/RoomProtocol.h"

namespace unool::shell {

	// Arch-C P1-4：登录场景的状态机（Mode / Status / Focus）与它们的全部迁移规则。
	//
	// 之前这些规则和 SFML 事件、窗口几何、ConnectionState 调用缠在 LoginScene 里，
	// 想验证「点退出匹配 → 3 秒兜底回大厅」必须开窗口、连服务端、真等 3 秒。
	// 现在把它们搬到这里：只依赖标准库与协议值对象，无 SFML / 无网络 / 无窗口，
	// 时间由外部按毫秒注入。
	//
	// 房间功能（协议版本 3）：登录后不再匹配，而是：
	//   Login/Register → Lobby（主大厅：开始游戏 / 商城 / 每日签到）
	//   → 点「开始游戏」进入 RoomBrowser（房间界面：房间列表 + 创建 / 加入 / 刷新 / 返回大厅）
	//   → 建好或加入某个房间后进入 Room（房内准备）→ 双方准备后开局。
	//
	// 与 View 的分工：
	//   View 负责「几何命中 + 事件翻译」，把 sf::Event 变成 clickXxx()/typeChar()；
	//   本类负责「这个操作意味着什么」，并把对外动作排进 Actions 队列；
	//   View 逐条执行 Actions（发包 → ConnectionState、openShop → 组合根）。
	// 发包失败不走返回值上抛，而是 View 调 onActionFailed(action) 让本类回滚状态。
	class LoginViewModel {
	public:
		enum class Mode { Login, Register, Lobby, RoomBrowser, Room };
		enum class Status {
			Idle,
			WaitingCheck,
			WaitingRegister,
			WaitingLogin,
			WaitingCheckin,
			WaitingCreateRoom,
			WaitingJoinRoom,
			Done
		};
		enum class Focus { None, Username, Password, RoomName };

		// 对外意图：由 View 翻译成 ConnectionState 调用 / 场景动作。
		enum class Action {
			SendCheckUsername,
			SendRegister,
			SendLogin,
			SendCheckin,
			RequestRoomList,
			CreateRoom,
			JoinRoom,
			LeaveRoom,
			SetReady,
			SetUnready,
			OpenShop,     // 交给组合根的 IShopOpener
			CloseWindow,  // Esc 退出
		};

		// 每帧输入：时间、GameStart 是否已在入站队列、连接是否存活。
		struct Frame {
			std::int64_t nowMs{};
			bool gameStartAvailable{};
			bool connected{ true };
		};

		struct Session {
			std::string username;
			std::string password;
			int points{};
			int wins{};
			int losses{};
			bool ok{};
		};

		// 大厅房间列表的自动刷新间隔：不点刷新也能看到别人新开的房间。
		static constexpr std::int64_t roomListRefreshMs = 2000;

		// === 时间注入（毫秒） ===
		void setNow(std::int64_t nowMs);
		std::int64_t nowMs() const { return nowMs_; }

		// === 键盘语义 ===
		void focusUsername();
		void focusPassword();
		void focusRoomName();
		void toggleFocus();                    // Tab
		void backspace();                      // Backspace
		void typeChar(char32_t ch);            // 只接受可打印 ASCII (33..126)，上限 32 字符
		void submit();                         // Enter（房间界面 = 创建房间）
		void escape();                         // 房间内 = 离开房间，房间界面 = 返回大厅，其余 = 关窗

		// === 鼠标语义（View 已做几何命中） ===
		void clickLogin();
		void clickRegister();
		void clickStartGame();                 // 主大厅「开始游戏」→ 进入房间界面
		void clickBackToLobby();               // 房间界面「返回大厅」
		void clickCreateRoom();
		void clickJoinRoom();
		void clickLeaveRoom();                 // 房间内「离开房间」→ 回房间界面
		void clickReady();                     // 按当前状态自动切换「准备 / 取消准备」
		void clickRefreshRooms();
		void clickShop();
		void clickCheckin();
		void selectRoom(int delta);            // 房间列表上下选择（↑↓ / W S）
		void clickRoom(std::size_t index);     // 直接选中列表第 index 行（View 已做命中）
		void dismissCheckinPopup();

		// === 网络事件（View 已把包解析成值对象） ===
		void onAccountEvent(const protocol::AccountEvent& event);
		// 原 View 的「响应解析失败」分支；守卫条件（是否在等该响应）与成功路径一致。
		void onAccountParseFailure(protocol::MessageType type);
		void onRoomList(std::vector<protocol::RoomSummary> rooms);
		void onRoomState(protocol::RoomSnapshot room);
		void onRoomError(std::string message);
		// 商城同步运行结束后回告：连接已断 / 窗口已关时本次登录会话判定失败。
		void onShopClosed(bool sessionUsable);

		// === 每帧 ===
		void onFrame(const Frame& frame);

		// === 出站 ===
		std::vector<Action> takeActions();
		// Actions 执行失败时的状态回滚
		void onActionFailed(Action action);
		// 需要打到控制台的日志（已格式化，不含 titleBrackets 前缀）。
		std::vector<std::string> takeLogs();

		// === 只读状态 ===
		Mode mode() const { return mode_; }
		Status status() const { return status_; }
		Focus focus() const { return focus_; }
		const std::string& username() const { return username_; }
		const std::string& password() const { return password_; }
		const std::string& roomNameInput() const { return roomNameInput_; }
		// 实际发送的房间名：输入框为空时回退为「<账号>的房间」（服务端同样兜底）
		std::string effectiveRoomName() const;
		const std::string& message() const { return message_; }
		const Session& session() const { return session_; }
		bool checkinPopup() const { return checkinPopup_; }
		const std::string& checkinMsg() const { return checkinMsg_; }
		bool done() const { return status_ == Status::Done; }

		const std::vector<protocol::RoomSummary>& rooms() const { return rooms_; }
		std::size_t selectedRoom() const { return selectedRoom_; }
		const protocol::RoomSummary* selectedRoomInfo() const;
		bool hasRooms() const { return !rooms_.empty(); }

		// 房间内状态（未在房间时 room() 返回 nullptr）
		const protocol::RoomSnapshot* room() const { return room_ ? &*room_ : nullptr; }
		bool inRoom() const { return room_.has_value(); }
		bool isOwner() const;
		bool selfReady() const;
		bool roomInGame() const { return room_ && room_->inGame; }

	private:
		// 登录 / 注册界面且不在等响应时才接受键盘输入。
		bool acceptsInput() const;
		void backToLobby();
		void requestLogin();
		void requestRegister();
		void requestCheckUsername();
		void requestCheckin();
		void requestRoomList();

		Mode mode_{ Mode::Login };
		Status status_{ Status::Idle };
		Focus focus_{ Focus::Username };

		std::string username_{};
		std::string password_{};
		std::string roomNameInput_{};
		std::string message_{};
		Session session_{};

		bool checkinPopup_{};
		std::string checkinMsg_{};

		std::vector<protocol::RoomSummary> rooms_{};
		std::size_t selectedRoom_{};
		std::optional<protocol::RoomSnapshot> room_{};

		std::int64_t nowMs_{};
		std::int64_t lastRoomListRequestMs_{};
		bool roomListRequested_{};

		std::vector<Action> actions_{};
		std::vector<std::string> logs_{};
	};

}
