#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include "protocol/AccountEvent.h"
#include "shell/IShopOpener.h"

namespace unool::shell {

	// Arch-C P1-4：登录场景的三台状态机（Mode / Status / Focus）与它们的全部迁移规则。
	//
	// 之前这些规则和 SFML 事件、窗口几何、ConnectionState 调用缠在 LoginScene 里，
	// 想验证「点退出匹配 → 3 秒兜底回大厅」必须开窗口、连服务端、真等 3 秒。
	// 现在把它们搬到这里：只依赖标准库与协议值对象（AccountEvent），
	// 无 SFML、无网络、无窗口，时间由外部按毫秒注入。
	//
	// 与 View 的分工：
	//   View 负责「几何命中 + 事件翻译」，把 sf::Event 变成 clickXxx()/typeChar()；
	//   本类负责「这个操作意味着什么」，并把对外动作排进 Actions 队列；
	//   View 逐条执行 Actions（sendLogin → ConnectionState、openShop → 组合根）。
	// 因此发包失败不走返回值上抛，而是 View 调 onActionFailed(action) 让本类回滚状态，
	// 语义与原 View「send 失败就地改 message/status」逐条对应。
	class LoginViewModel {
	public:
		enum class Mode { Login, Register, Lobby, Matching };
		// WaitingMatch：已发 ReadyToPlay、等对手；WaitingCancel：已发退出请求、等服务端仲裁。
		enum class Status { Idle, WaitingCheck, WaitingRegister, WaitingLogin, WaitingCheckin,
		                    WaitingMatch, WaitingCancel, Done };
		enum class Focus { None, Username, Password };

		// 对外意图：由 View 翻译成 ConnectionState 调用 / 场景动作。
		enum class Action {
			SendCheckUsername,
			SendRegister,
			SendLogin,
			SendReadyToPlay,
			SendCheckin,
			SendCancelMatchmaking,
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

		// 退出匹配的本地兜底：等待服务端仲裁超过此时长就直接回大厅。
		static constexpr std::int64_t cancelTimeoutMs = 3000;

		// === 时间注入（毫秒） ===
		// View 每帧喂真实时钟；测试直接喂常数，不必真等 3 秒。
		void setNow(std::int64_t nowMs);
		std::int64_t nowMs() const { return nowMs_; }

		// === 键盘语义 ===
		void focusUsername();
		void focusPassword();
		void toggleFocus();                    // Tab
		void backspace();                      // Backspace
		void typeChar(char32_t ch);            // 只接受可打印 ASCII (33..126)，上限 32 字符
		void submit();                         // Enter
		void escape();                         // 匹配中 = 退出匹配，其余 = 关窗

		// === 鼠标语义（View 已做几何命中） ===
		void clickLogin();
		void clickRegister();
		void clickStartGame();
		void clickShop();
		void clickCheckin();
		void clickCancelMatch();
		void dismissCheckinPopup();

		// === 网络事件（View 已把包解析成值对象） ===
		void onAccountEvent(const protocol::AccountEvent& event);
		// 原 View 的「响应解析失败」分支；守卫条件（是否在等该响应）与成功路径一致。
		void onAccountParseFailure(protocol::MessageType type);
		void onMatchmakingStarted(std::string opponentName);
		void onMatchmakingCancelled();
		// 商城同步运行结束后回告：连接已断 / 窗口已关时本次登录会话判定失败。
		void onShopClosed(bool sessionUsable);

		// === 每帧 ===
		void onFrame(const Frame& frame);

		// === 出站 ===
		std::vector<Action> takeActions();
		// Actions 执行失败时的状态回滚（文案与原 View 的发包失败分支逐条一致）。
		void onActionFailed(Action action);
		// 需要打到控制台的日志（已格式化，不含 titleBrackets 前缀）。
		std::vector<std::string> takeLogs();

		// === 只读状态 ===
		Mode mode() const { return mode_; }
		Status status() const { return status_; }
		Focus focus() const { return focus_; }
		const std::string& username() const { return username_; }
		const std::string& password() const { return password_; }
		const std::string& message() const { return message_; }
		const Session& session() const { return session_; }
		bool checkinPopup() const { return checkinPopup_; }
		const std::string& checkinMsg() const { return checkinMsg_; }
		MatchNoticeState& matchNotice() { return matchNotice_; }
		const MatchNoticeState& matchNotice() const { return matchNotice_; }
		bool done() const { return status_ == Status::Done; }
		// 匹配等待界面的「已等待 N 秒」；用注入的时间算，不读系统时钟。
		std::int64_t matchElapsedSeconds() const;

	private:
		// 登录 / 注册界面且不在等响应时才接受键盘输入。
		bool acceptsInput() const;
		void clearMatchNotice();
		void backToLobby();
		void requestLogin();
		void requestRegister();
		void requestCheckUsername();
		void requestCheckin();
		void requestCancelMatchmaking();

		Mode mode_{ Mode::Login };
		Status status_{ Status::Idle };
		Focus focus_{ Focus::Username };

		std::string username_{};
		std::string password_{};
		std::string message_{};
		Session session_{};

		bool checkinPopup_{};
		std::string checkinMsg_{};

		MatchNoticeState matchNotice_{};

		std::int64_t nowMs_{};
		std::int64_t matchStartedMs_{};
		std::int64_t cancelRequestedMs_{};

		std::vector<Action> actions_{};
		std::vector<std::string> logs_{};
	};

}
