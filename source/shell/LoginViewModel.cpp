#include "LoginViewModel.h"

#include <format>
#include <utility>

namespace unool::shell {

	namespace {

		// 输入框字符上限（与原 View 的 32 一致）。
		constexpr std::size_t kMaxInputChars = 32;

		// 只接受可打印 ASCII (33..126)：排除空格以对齐 safeReadNoSpace 规则。
		constexpr char32_t kFirstPrintable = 33;
		constexpr char32_t kLastPrintable = 126;

	}

	void LoginViewModel::setNow(const std::int64_t nowMs) {
		nowMs_ = nowMs;
	}

	bool LoginViewModel::acceptsInput() const {
		return mode_ != Mode::Lobby && mode_ != Mode::Matching && status_ == Status::Idle;
	}

	void LoginViewModel::focusUsername() {
		if (!acceptsInput()) return;
		focus_ = Focus::Username;
	}

	void LoginViewModel::focusPassword() {
		if (!acceptsInput()) return;
		focus_ = Focus::Password;
	}

	void LoginViewModel::toggleFocus() {
		if (!acceptsInput()) return;
		if (focus_ == Focus::Username) focus_ = Focus::Password;
		else if (focus_ == Focus::Password) focus_ = Focus::Username;
		else focus_ = Focus::Username;
	}

	void LoginViewModel::backspace() {
		if (!acceptsInput()) return;
		if (focus_ == Focus::Username && !username_.empty()) username_.pop_back();
		else if (focus_ == Focus::Password && !password_.empty()) password_.pop_back();
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
	}

	void LoginViewModel::submit() {
		if (!acceptsInput()) return;
		if (focus_ == Focus::Username) {
			focus_ = Focus::Password;
		}
		else if (focus_ == Focus::Password) {
			if (mode_ == Mode::Login) requestLogin();
			else requestCheckUsername();
		}
	}

	void LoginViewModel::escape() {
		// 匹配等待期间 Esc = 退出匹配（与按钮一致），不再直接关掉整个客户端。
		if (mode_ == Mode::Matching) {
			if (status_ == Status::WaitingMatch) requestCancelMatchmaking();
			return;
		}
		actions_.push_back(Action::CloseWindow);
	}

	void LoginViewModel::clickLogin() {
		if (!acceptsInput()) return;
		mode_ = Mode::Login;
		focus_ = Focus::None;
		requestLogin();
	}

	void LoginViewModel::clickRegister() {
		if (!acceptsInput()) return;
		mode_ = Mode::Register;
		focus_ = Focus::None;
		requestCheckUsername();
	}

	void LoginViewModel::clickStartGame() {
		if (mode_ != Mode::Lobby || status_ != Status::Idle) return;
		// 先落状态再排队发包：发包失败由 onActionFailed 回滚，见 .h 的分工说明。
		mode_ = Mode::Matching;
		status_ = Status::WaitingMatch;
		clearMatchNotice(); // 自己已加入匹配，大厅的横幅不再需要
		message_.clear();
		matchStartedMs_ = nowMs_;
		actions_.push_back(Action::SendReadyToPlay);
	}

	void LoginViewModel::clickShop() {
		if (mode_ != Mode::Lobby || status_ != Status::Idle) return;
		actions_.push_back(Action::OpenShop);
	}

	void LoginViewModel::clickCheckin() {
		if (mode_ != Mode::Lobby || status_ != Status::Idle) return;
		requestCheckin();
	}

	void LoginViewModel::clickCancelMatch() {
		if (mode_ != Mode::Matching || status_ != Status::WaitingMatch) return;
		requestCancelMatchmaking();
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

	void LoginViewModel::requestCancelMatchmaking() {
		status_ = Status::WaitingCancel;
		message_.clear();
		cancelRequestedMs_ = nowMs_;
		actions_.push_back(Action::SendCancelMatchmaking);
	}

	void LoginViewModel::backToLobby() {
		mode_ = Mode::Lobby;
		status_ = Status::Idle;
		message_.clear(); // 退出匹配静默返回大厅，不显示一次性文字提示
	}

	void LoginViewModel::clearMatchNotice() {
		matchNotice_.opponentMatching = false;
		matchNotice_.opponentName.clear();
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

	void LoginViewModel::onMatchmakingStarted(std::string opponentName) {
		// 不检查 status：该广播可能先于 LoginResponse 到达（对手已准备、自己才刚登录）。
		if (opponentName.empty() || opponentName == username_) return;
		matchNotice_.opponentMatching = true;
		matchNotice_.opponentName = std::move(opponentName);
		logs_.push_back(
			std::format("对手「{}」已开始匹配，请点击「开始游戏」加入", matchNotice_.opponentName));
	}

	void LoginViewModel::onMatchmakingCancelled() {
		// 服务端广播给双方：清横幅；正处于匹配等待的一方静默回大厅。
		clearMatchNotice();
		if (mode_ == Mode::Matching) backToLobby();
		else if (mode_ == Mode::Lobby) message_.clear();
	}

	void LoginViewModel::onShopClosed(const bool sessionUsable) {
		if (sessionUsable) return;
		status_ = Status::Done;
		session_.ok = false;
	}

	void LoginViewModel::onFrame(const Frame& frame) {
		setNow(frame.nowMs);

		// GameStart 只判定、不消费：跨阶段留包由 ConnectionState 的显式队列保证。
		if (frame.gameStartAvailable && (mode_ == Mode::Lobby || mode_ == Mode::Matching)) {
			status_ = Status::Done;
			return;
		}
		if (mode_ != Mode::Matching) return;

		// 匹配阶段不再有「无限等待」：断线直接结束会话，取消请求超时则本地兜底回大厅。
		if (!frame.connected) {
			status_ = Status::Done;
			session_.ok = false;
			return;
		}
		if (status_ == Status::WaitingCancel && nowMs_ - cancelRequestedMs_ >= cancelTimeoutMs) {
			backToLobby();
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
			case Action::SendReadyToPlay:
				message_ = "发送失败，请重试";
				mode_ = Mode::Lobby;
				status_ = Status::Idle;
				return;
			case Action::SendCheckin:
				message_ = "发送失败，请重试";
				status_ = Status::Idle;
				return;
			case Action::SendCancelMatchmaking:
				// 发包失败也不能把用户困在等待界面，本地直接回大厅（不显示提示）。
				backToLobby();
				return;
			case Action::OpenShop:
			case Action::CloseWindow:
				return;
		}
	}

	std::int64_t LoginViewModel::matchElapsedSeconds() const {
		const std::int64_t elapsed = nowMs_ - matchStartedMs_;
		return elapsed > 0 ? elapsed / 1000 : 0;
	}

}
