#include "LoginViewModel.h"

#include <algorithm>
#include <format>
#include <utility>

namespace unool::shell {

	namespace {

		// 输入框字符上限（与原 View 的 32 一致）。
		constexpr std::size_t kMaxInputChars = 32;
		// 房间名上限：比账号短一些，保证大厅列表一行放得下。
		constexpr std::size_t kMaxRoomNameChars = 20;

		// 只接受可打印 ASCII (33..126)：排除空格以对齐 safeReadNoSpace 规则。
		constexpr char32_t kFirstPrintable = 33;
		constexpr char32_t kLastPrintable = 126;

	}

	void LoginViewModel::setNow(const std::int64_t nowMs) {
		nowMs_ = nowMs;
	}

	bool LoginViewModel::acceptsInput() const {
		// 主大厅只有按钮、没有输入框；房间界面允许输入房间名；房间内不接受任何文本输入。
		if (mode_ == Mode::Lobby) return false;
		if (mode_ == Mode::Room) return false;
		return status_ == Status::Idle;
	}

	void LoginViewModel::focusUsername() {
		if (mode_ != Mode::Login && mode_ != Mode::Register) return;
		if (!acceptsInput()) return;
		focus_ = Focus::Username;
	}

	void LoginViewModel::focusPassword() {
		if (mode_ != Mode::Login && mode_ != Mode::Register) return;
		if (!acceptsInput()) return;
		focus_ = Focus::Password;
	}

	void LoginViewModel::focusRoomName() {
		if (mode_ != Mode::RoomBrowser) return;
		if (!acceptsInput()) return;
		focus_ = Focus::RoomName;
	}

	void LoginViewModel::toggleFocus() {
		if (!acceptsInput()) return;
		if (mode_ == Mode::RoomBrowser) {
			focus_ = Focus::RoomName;
			return;
		}
		if (focus_ == Focus::Username) focus_ = Focus::Password;
		else if (focus_ == Focus::Password) focus_ = Focus::Username;
		else focus_ = Focus::Username;
	}

	void LoginViewModel::backspace() {
		if (!acceptsInput()) return;
		if (focus_ == Focus::Username && !username_.empty()) username_.pop_back();
		else if (focus_ == Focus::Password && !password_.empty()) password_.pop_back();
		else if (focus_ == Focus::RoomName && !roomNameInput_.empty()) roomNameInput_.pop_back();
	}

	void LoginViewModel::typeChar(const char32_t ch) {
		if (!acceptsInput()) return;
		if (focus_ == Focus::None) return;
		if (ch < kFirstPrintable || ch > kLastPrintable) return;
		const auto c = static_cast<char>(ch);
		if (focus_ == Focus::Username) {
			if (username_.size() < kMaxInputChars) username_.push_back(c);
		}
		else if (focus_ == Focus::Password) {
			if (password_.size() < kMaxInputChars) password_.push_back(c);
		}
		else if (focus_ == Focus::RoomName) {
			if (roomNameInput_.size() < kMaxRoomNameChars) roomNameInput_.push_back(c);
		}
	}

	void LoginViewModel::submit() {
		if (!acceptsInput()) return;
		if (mode_ == Mode::RoomBrowser) {
			clickCreateRoom();
			return;
		}
		if (focus_ == Focus::Username) {
			focus_ = Focus::Password;
		}
		else if (focus_ == Focus::Password) {
			if (mode_ == Mode::Login) requestLogin();
			else requestCheckUsername();
		}
	}

	void LoginViewModel::escape() {
		// 房间内 Esc = 离开房间（与按钮一致）；房间界面 Esc = 返回大厅。
		if (mode_ == Mode::Room) {
			clickLeaveRoom();
			return;
		}
		if (mode_ == Mode::RoomBrowser) {
			clickBackToLobby();
			return;
		}
		actions_.push_back(Action::CloseWindow);
	}

	void LoginViewModel::clickLogin() {
		if (!acceptsInput()) return;
		if (mode_ != Mode::Login) return;
		focus_ = Focus::None;
		requestLogin();
	}

	void LoginViewModel::clickRegister() {
		if (!acceptsInput()) return;
		mode_ = Mode::Register;
		focus_ = Focus::None;
		requestCheckUsername();
	}

	// 「开始游戏」不再匹配：进入房间界面（列表 + 创建 / 加入），并立刻拉一次列表。
	void LoginViewModel::clickStartGame() {
		if (mode_ != Mode::Lobby || status_ != Status::Idle) return;
		mode_ = Mode::RoomBrowser;
		status_ = Status::Idle;
		focus_ = Focus::RoomName;
		message_.clear();
		requestRoomList();
	}

	void LoginViewModel::clickBackToLobby() {
		if (mode_ != Mode::RoomBrowser) return;
		backToLobby();
	}

	void LoginViewModel::clickCreateRoom() {
		if (mode_ != Mode::RoomBrowser || status_ != Status::Idle) return;
		// 先落状态再排队发包：发包失败由 onActionFailed 回滚
		status_ = Status::WaitingCreateRoom;
		message_.clear();
		actions_.push_back(Action::CreateRoom);
	}

	void LoginViewModel::clickJoinRoom() {
		if (mode_ != Mode::RoomBrowser || status_ != Status::Idle) return;
		const auto* info = selectedRoomInfo();
		if (!info) {
			message_ = "当前没有可加入的房间";
			return;
		}
		if (info->inGame) {
			message_ = "该房间正在对局中，无法加入";
			return;
		}
		if (info->members >= info->capacity) {
			message_ = "该房间已满";
			return;
		}
		status_ = Status::WaitingJoinRoom;
		message_.clear();
		actions_.push_back(Action::JoinRoom);
	}

	void LoginViewModel::clickLeaveRoom() {
		if (mode_ != Mode::Room) return;
		// 乐观回房间界面：服务端还会回一个 closed 快照，届时只当作提示文案
		room_.reset();
		mode_ = Mode::RoomBrowser;
		status_ = Status::Idle;
		focus_ = Focus::RoomName;
		message_.clear();
		actions_.push_back(Action::LeaveRoom);
		actions_.push_back(Action::RequestRoomList);
	}

	void LoginViewModel::clickReady() {
		if (mode_ != Mode::Room) return;
		if (!room_ || room_->inGame) return;
		actions_.push_back(selfReady() ? Action::SetUnready : Action::SetReady);
	}

	void LoginViewModel::clickRefreshRooms() {
		if (mode_ != Mode::RoomBrowser || status_ != Status::Idle) return;
		requestRoomList();
	}

	void LoginViewModel::clickShop() {
		if (mode_ != Mode::Lobby || status_ != Status::Idle) return;
		actions_.push_back(Action::OpenShop);
	}

	void LoginViewModel::clickCheckin() {
		if (mode_ != Mode::Lobby || status_ != Status::Idle) return;
		requestCheckin();
	}

	void LoginViewModel::selectRoom(const int delta) {
		if (mode_ != Mode::RoomBrowser || rooms_.empty()) return;
		const auto count = static_cast<int>(rooms_.size());
		int current = static_cast<int>(selectedRoom_);
		if (delta < 0) current = (current - 1 + count) % count;
		else if (delta > 0) current = (current + 1) % count;
		selectedRoom_ = static_cast<std::size_t>(current);
	}

	void LoginViewModel::clickRoom(const std::size_t index) {
		if (mode_ != Mode::RoomBrowser) return;
		if (index >= rooms_.size()) return;
		selectedRoom_ = index;
	}

	void LoginViewModel::dismissCheckinPopup() {
		checkinPopup_ = false;
	}

	void LoginViewModel::requestLogin() {
		if (username_.empty() || password_.empty()) {
			message_ = "用户名和密码不能为空";
			return;
		}
		status_ = Status::WaitingLogin;
		message_ = "登录中...";
		actions_.push_back(Action::SendLogin);
	}

	void LoginViewModel::requestRegister() {
		if (password_.empty()) {
			message_ = "密码不能为空";
			status_ = Status::Idle;
			return;
		}
		status_ = Status::WaitingRegister;
		message_ = "注册中...";
		actions_.push_back(Action::SendRegister);
	}

	void LoginViewModel::requestCheckUsername() {
		if (username_.empty()) {
			message_ = "用户名不能为空";
			return;
		}
		status_ = Status::WaitingCheck;
		message_ = "检查用户名...";
		actions_.push_back(Action::SendCheckUsername);
	}

	void LoginViewModel::requestCheckin() {
		status_ = Status::WaitingCheckin;
		actions_.push_back(Action::SendCheckin);
	}

	void LoginViewModel::requestRoomList() {
		lastRoomListRequestMs_ = nowMs_;
		roomListRequested_ = true;
		actions_.push_back(Action::RequestRoomList);
	}

	void LoginViewModel::backToLobby() {
		mode_ = Mode::Lobby;
		status_ = Status::Idle;
		focus_ = Focus::None;
		message_.clear();
	}

	std::string LoginViewModel::effectiveRoomName() const {
		if (!roomNameInput_.empty()) return roomNameInput_;
		return std::format("{}的房间", username_);
	}

	const protocol::RoomSummary* LoginViewModel::selectedRoomInfo() const {
		if (rooms_.empty()) return nullptr;
		const std::size_t index = std::min(selectedRoom_, rooms_.size() - 1);
		return &rooms_[index];
	}

	bool LoginViewModel::isOwner() const {
		return room_ && room_->owner == username_;
	}

	bool LoginViewModel::selfReady() const {
		if (!room_) return false;
		for (const auto& member : room_->members) {
			if (member.name == username_) return member.ready;
		}
		return false;
	}

	void LoginViewModel::onAccountEvent(const protocol::AccountEvent& event) {
		switch (event.type) {
			case protocol::MessageType::CheckUsernameResponse:
				if (status_ != Status::WaitingCheck) return;
				if (event.exists) {
					message_ = "该用户名已存在";
					status_ = Status::Idle;
				}
				else {
					message_ = "用户名可用，继续注册";
					requestRegister(); // 自动进入注册请求
				}
				return;

			case protocol::MessageType::RegisterResponse:
				if (status_ != Status::WaitingRegister) return;
				if (event.ok) {
					message_ = "注册成功，自动登录中...";
					requestLogin(); // 注册成功，自动登录
				}
				else {
					message_ = std::format("注册失败: {}", event.msg);
					status_ = Status::Idle;
				}
				return;

			case protocol::MessageType::LoginResponse:
				if (status_ != Status::WaitingLogin) return;
				if (event.ok) {
					session_.username = username_;
					session_.password = password_;
					session_.points = event.points;
					session_.wins = event.wins;
					session_.losses = event.losses;
					session_.ok = true;
					mode_ = Mode::Lobby;
					status_ = Status::Idle;
					// 主大厅只有按钮：登录成功不拉房间列表，等点「开始游戏」进房间界面时再拉
					focus_ = Focus::None;
					message_ = "登录成功";
					logs_.push_back(std::format("登录成功: {} 积分={} 胜={} 负={}",
												event.msg, event.points, event.wins, event.losses));
				}
				else {
					message_ = std::format("登录失败: {}", event.msg);
					status_ = Status::Idle;
				}
				return;

			case protocol::MessageType::CheckinResponse:
				// parseAccountEvent 对该类型恒返回值对象，故成功路径不区分解析失败。
				if (status_ != Status::WaitingCheckin) return;
				checkinMsg_ = event.msg;
				checkinPopup_ = true;
				session_.points = event.points;
				status_ = Status::Idle;
				return;

			default:
				return;
		}
	}

	void LoginViewModel::onAccountParseFailure(const protocol::MessageType type) {
		// 守卫条件与成功路径一致：不在等该响应就整条丢弃。
		switch (type) {
			case protocol::MessageType::CheckUsernameResponse:
				if (status_ != Status::WaitingCheck) return;
				break;
			case protocol::MessageType::RegisterResponse:
				if (status_ != Status::WaitingRegister) return;
				break;
			case protocol::MessageType::LoginResponse:
				if (status_ != Status::WaitingLogin) return;
				break;
			default:
				// CheckinResponse 恒有值；其余类型不进本分支。
				return;
		}
		message_ = "响应解析失败";
		status_ = Status::Idle;
	}

	void LoginViewModel::onRoomList(std::vector<protocol::RoomSummary> rooms) {
		rooms_ = std::move(rooms);
		if (rooms_.empty()) {
			selectedRoom_ = 0;
			return;
		}
		if (selectedRoom_ >= rooms_.size()) selectedRoom_ = rooms_.size() - 1;
	}

	void LoginViewModel::onRoomState(protocol::RoomSnapshot room) {
		if (room.closed) {
			// 房间解散 / 已离开：回到房间界面（列表）并提示原因
			const std::string reason = room.reason;
			room_.reset();
			mode_ = Mode::RoomBrowser;
			status_ = Status::Idle;
			focus_ = Focus::RoomName;
			if (!reason.empty()) message_ = reason;
			requestRoomList();
			return;
		}

		room_ = std::move(room);
		mode_ = Mode::Room;
		status_ = Status::Idle;
		focus_ = Focus::None;
		message_.clear();
		logs_.push_back(std::format("进入房间「{}」（房主：{}，{}/{} 人）",
									room_->name, room_->owner, room_->members.size(), room_->capacity));
	}

	void LoginViewModel::onRoomError(std::string message) {
		status_ = Status::Idle;
		message_ = std::move(message);
	}

	void LoginViewModel::onShopClosed(const bool sessionUsable) {
		// 商城只能从主大厅进入，房间列表由「开始游戏」进入房间界面时重新拉取，这里无需补拉
		if (sessionUsable) return;
		status_ = Status::Done;
		session_.ok = false;
	}

	void LoginViewModel::onFrame(const Frame& frame) {
		setNow(frame.nowMs);

		// GameStart 只判定、不消费：跨阶段留包由 ConnectionState 的显式队列保证。
		if (frame.gameStartAvailable
			&& (mode_ == Mode::Lobby || mode_ == Mode::RoomBrowser || mode_ == Mode::Room)) {
			status_ = Status::Done;
			return;
		}
		if (mode_ != Mode::Lobby && mode_ != Mode::RoomBrowser && mode_ != Mode::Room) return;

		// 大厅 / 房间界面 / 房间内断线：本次登录会话结束
		// （否则界面会停在一个永远等不到响应的状态）
		if (!frame.connected) {
			status_ = Status::Done;
			session_.ok = false;
			return;
		}

		// 房间界面：定时刷新房间列表（进入界面时已立即拉过一次）
		if (mode_ == Mode::RoomBrowser) {
			if (!roomListRequested_ || nowMs_ - lastRoomListRequestMs_ >= roomListRefreshMs) {
				requestRoomList();
			}
		}
	}

	// 返回类型写在 `LoginViewModel::` 之前，此时尚未进入类作用域，Action 必须限定。
	std::vector<LoginViewModel::Action> LoginViewModel::takeActions() {
		std::vector<Action> out{};
		out.swap(actions_);
		return out;
	}

	std::vector<std::string> LoginViewModel::takeLogs() {
		std::vector<std::string> out{};
		out.swap(logs_);
		return out;
	}

	void LoginViewModel::onActionFailed(const Action action) {
		switch (action) {
			case Action::SendLogin:
				message_ = "发送失败，请关闭客户端的.exe文件后重试";
				status_ = Status::Idle;
				return;
			case Action::SendRegister:
			case Action::SendCheckUsername:
				message_ = "发送失败，请重试";
				status_ = Status::Idle;
				return;
			case Action::SendCheckin:
				message_ = "发送失败，请重试";
				status_ = Status::Idle;
				return;
			case Action::CreateRoom:
			case Action::JoinRoom:
			case Action::SetReady:
			case Action::SetUnready:
				message_ = "发送失败，请重试";
				status_ = Status::Idle;
				return;
			case Action::LeaveRoom:
				// 已经乐观回到大厅，发包失败不再额外提示
				return;
			case Action::RequestRoomList:
				return;
			case Action::OpenShop:
			case Action::CloseWindow:
				return;
		}
	}

}
