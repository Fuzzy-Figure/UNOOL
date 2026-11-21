// Arch-C P1-4：LoginViewModel 单测。
//
// 这里断言的是「登录场景会怎么变」而不是「怎么画」：状态机搬到 ViewModel 之后，
// 点退出匹配后 3 秒兜底、断线结束会话、注册链自动流转这些分支都能直接喂时间与事件，
// 不必开窗口、连服务端、真等 3 秒。
//
// 输入侧一律走语义方法（typeChar / submit / clickXxx），输出侧看两样：
//   - takeActions()：VM 要 View 去做的事（发包 / 开商城 / 关窗）；
//   - 只读状态：mode / status / message / session / 弹窗与横幅。
// 发包失败用 onActionFailed() 回告，验证状态回滚与原 View 的发包失败分支一致。

#include "test_loginviewmodel.h"

#include "shell/LoginViewModel.h"

#include <string>
#include <vector>

using unool::protocol::MessageType;
using unool::shell::LoginViewModel;
using unool::tests::TestResult;

namespace {

using Mode = LoginViewModel::Mode;
using Status = LoginViewModel::Status;
using Focus = LoginViewModel::Focus;
using Action = LoginViewModel::Action;

void typeText(LoginViewModel& vm, const std::string& text) {
	for (const char c : text) vm.typeChar(static_cast<char32_t>(c));
}

// 取出 Actions 并判断「恰好是这一个」。
bool onlyAction(LoginViewModel& vm, const Action expected) {
	const auto actions = vm.takeActions();
	return actions.size() == 1 && actions.front() == expected;
}

unool::protocol::AccountEvent makeEvent(const MessageType type) {
	unool::protocol::AccountEvent event{};
	event.type = type;
	return event;
}

// 把 VM 送进大厅：登录成功路径的公共前置。
void enterLobby(LoginViewModel& vm) {
	typeText(vm, "alice");
	vm.toggleFocus();
	typeText(vm, "pw");
	vm.submit();
	vm.takeActions();

	unool::protocol::AccountEvent ok = makeEvent(MessageType::LoginResponse);
	ok.ok = true;
	ok.msg = "欢迎回来";
	ok.points = 100;
	ok.wins = 5;
	ok.losses = 2;
	vm.onAccountEvent(ok);
}

// --- 登录与输入 ---

void testLoginSuccess(TestResult& r) {
	LoginViewModel vm;
	r.check(vm.mode() == Mode::Login);
	r.check(vm.focus() == Focus::Username);

	typeText(vm, "alice");
	r.check(vm.username() == "alice");

	vm.toggleFocus();
	r.check(vm.focus() == Focus::Password);
	typeText(vm, "pw");
	vm.submit();

	r.check(vm.status() == Status::WaitingLogin);
	r.check(vm.message() == "登录中...");
	r.check(onlyAction(vm, Action::SendLogin));

	unool::protocol::AccountEvent ok = makeEvent(MessageType::LoginResponse);
	ok.ok = true;
	ok.msg = "欢迎回来";
	ok.points = 120;
	ok.wins = 7;
	ok.losses = 3;
	vm.onAccountEvent(ok);

	r.check(vm.mode() == Mode::Lobby);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "登录成功");
	r.check(vm.session().ok);
	r.check(vm.session().username == "alice");
	r.check(vm.session().password == "pw");
	r.check(vm.session().points == 120);
	r.check(vm.session().wins == 7);
	r.check(vm.session().losses == 3);

	const auto logs = vm.takeLogs();
	r.check(logs.size() == 1);
	r.check(!logs.empty() && logs.front().find("登录成功") != std::string::npos);

	// 登录成功但还没开局：会话未结束。
	vm.onFrame({ 0, false, true });
	r.check(!vm.done());
	// GameStart 出现即结束登录阶段（包本身留给 gamePhase，VM 只判定）。
	vm.onFrame({ 0, true, true });
	r.check(vm.done());
	r.check(vm.session().ok);
}

void testLoginRejectsEmptyInput(TestResult& r) {
	LoginViewModel vm;
	vm.clickLogin();
	r.check(vm.message() == "用户名和密码不能为空");
	r.check(vm.status() == Status::Idle);
	r.check(vm.takeActions().empty());

	// 只有用户名：仍然拒绝。
	typeText(vm, "alice");
	vm.clickLogin();
	r.check(vm.message() == "用户名和密码不能为空");
	r.check(vm.takeActions().empty());
}

void testLoginFailureKeepsIdle(TestResult& r) {
	LoginViewModel vm;
	typeText(vm, "alice");
	vm.toggleFocus();
	typeText(vm, "pw");
	vm.submit();
	vm.takeActions();

	unool::protocol::AccountEvent bad = makeEvent(MessageType::LoginResponse);
	bad.ok = false;
	bad.msg = "密码错误";
	vm.onAccountEvent(bad);

	r.check(vm.status() == Status::Idle);
	r.check(vm.mode() == Mode::Login);
	r.check(vm.message() == "登录失败: 密码错误");
	r.check(!vm.session().ok);
	r.check(vm.takeLogs().empty());
}

void testSendFailureRollsBack(TestResult& r) {
	LoginViewModel vm;
	typeText(vm, "alice");
	vm.toggleFocus();
	typeText(vm, "pw");
	vm.submit();
	r.check(onlyAction(vm, Action::SendLogin));

	// View 发包失败 → 回告 VM，状态与文案回到原 View 的发包失败分支。
	vm.onActionFailed(Action::SendLogin);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "发送失败，请关闭客户端的.exe文件后重试");
}

void testInputRules(TestResult& r) {
	LoginViewModel vm;

	// 空格、非 ASCII、控制字符都不进输入框。
	vm.typeChar(U' ');
	vm.typeChar(U'中');
	vm.typeChar(U'\n');
	r.check(vm.username().empty());

	// 上限 32 字符。
	typeText(vm, std::string(40, 'a'));
	r.check(vm.username().size() == 32);

	// Backspace 删尾字符；删空后再删不抛。
	vm.backspace();
	r.check(vm.username().size() == 31);
	typeText(vm, std::string(40, 'b'));
	vm.backspace();
	vm.backspace();
	vm.backspace();
	r.check(vm.username().size() == 29);

	// 焦点切到密码后，输入不再进用户名。
	vm.toggleFocus();
	typeText(vm, "pw");
	r.check(vm.username().size() == 29);
	r.check(vm.password() == "pw");

	// 等响应期间不接受输入。
	vm.submit();
	r.check(vm.status() == Status::WaitingLogin);
	typeText(vm, "xxx");
	r.check(vm.password() == "pw");

	// 大厅 / 匹配状态同样不接受输入。
	LoginViewModel lobby;
	enterLobby(lobby);
	typeText(lobby, "zzz");
	r.check(lobby.username() == "alice");
}

// --- 注册链：查重 → 注册 → 登录 ---

void testRegisterChain(TestResult& r) {
	LoginViewModel vm;
	typeText(vm, "bob");
	vm.clickRegister();

	r.check(vm.mode() == Mode::Register);
	r.check(vm.focus() == Focus::None);
	r.check(vm.status() == Status::WaitingCheck);
	r.check(vm.message() == "检查用户名...");
	r.check(onlyAction(vm, Action::SendCheckUsername));

	// 已存在：停在注册界面，不发后续请求。
	unool::protocol::AccountEvent exists = makeEvent(MessageType::CheckUsernameResponse);
	exists.exists = true;
	vm.onAccountEvent(exists);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "该用户名已存在");
	r.check(vm.takeActions().empty());

	// 可用但密码为空：链在注册前停住。
	vm.clickRegister();
	unool::protocol::AccountEvent free = makeEvent(MessageType::CheckUsernameResponse);
	free.exists = false;
	vm.onAccountEvent(free);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "密码不能为空");
	r.check(vm.takeActions().empty());

	// 有密码：查重 → 注册 → 自动登录。
	LoginViewModel ok;
	typeText(ok, "bob");
	ok.toggleFocus();
	typeText(ok, "pw");
	ok.clickRegister();
	ok.takeActions();

	unool::protocol::AccountEvent okFree = makeEvent(MessageType::CheckUsernameResponse);
	okFree.exists = false;
	ok.onAccountEvent(okFree);
	r.check(ok.message() == "用户名可用，继续注册");
	r.check(ok.status() == Status::WaitingRegister);
	r.check(onlyAction(ok, Action::SendRegister));

	unool::protocol::AccountEvent registered = makeEvent(MessageType::RegisterResponse);
	registered.ok = true;
	ok.onAccountEvent(registered);
	r.check(ok.message() == "注册成功，自动登录中...");
	r.check(ok.status() == Status::WaitingLogin);
	r.check(onlyAction(ok, Action::SendLogin));

	unool::protocol::AccountEvent login = makeEvent(MessageType::LoginResponse);
	login.ok = true;
	login.msg = "首次登录";
	ok.onAccountEvent(login);
	r.check(ok.mode() == Mode::Lobby);
	r.check(ok.session().ok);
}

void testRegisterFailure(TestResult& r) {
	LoginViewModel vm;
	typeText(vm, "bob");
	vm.toggleFocus();
	typeText(vm, "pw");
	vm.clickRegister();
	vm.takeActions();

	unool::protocol::AccountEvent free = makeEvent(MessageType::CheckUsernameResponse);
	vm.onAccountEvent(free);
	vm.takeActions();

	unool::protocol::AccountEvent bad = makeEvent(MessageType::RegisterResponse);
	bad.ok = false;
	bad.msg = "服务端拒绝";
	vm.onAccountEvent(bad);

	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "注册失败: 服务端拒绝");
	r.check(vm.takeActions().empty());
}

void testParseFailure(TestResult& r) {
	LoginViewModel vm;
	typeText(vm, "alice");
	vm.toggleFocus();
	typeText(vm, "pw");
	vm.submit();
	vm.takeActions();

	// 不在等注册响应：整条丢弃，状态不动。
	vm.onAccountParseFailure(MessageType::RegisterResponse);
	r.check(vm.status() == Status::WaitingLogin);

	// 正在等登录响应：回到可重试状态。
	vm.onAccountParseFailure(MessageType::LoginResponse);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "响应解析失败");
}

// --- 匹配等待与 3 秒兜底 ---

void testMatchingCancelTimeout(TestResult& r) {
	LoginViewModel vm;
	vm.setNow(1000);
	enterLobby(vm);

	vm.clickStartGame();
	r.check(vm.mode() == Mode::Matching);
	r.check(vm.status() == Status::WaitingMatch);
	r.check(vm.message().empty());
	r.check(onlyAction(vm, Action::SendReadyToPlay));

	// 等待秒数按注入的时间算：3.5 秒后显示 3。
	vm.onFrame({ 4500, false, true });
	r.check(vm.matchElapsedSeconds() == 3);
	r.check(vm.status() == Status::WaitingMatch);

	// 点退出匹配：立刻进入 WaitingCancel，并补一次静默。
	vm.setNow(5000);
	vm.clickCancelMatch();
	r.check(vm.status() == Status::WaitingCancel);
	r.check(vm.message().empty());
	r.check(onlyAction(vm, Action::SendCancelMatchmaking));

	// 退出中再点/再按 Esc 不重复发包。
	vm.clickCancelMatch();
	vm.escape();
	r.check(vm.takeActions().empty());

	// 2999ms 仍在等，3000ms 本地兜底回大厅。
	vm.onFrame({ 7999, false, true });
	r.check(vm.status() == Status::WaitingCancel);
	vm.onFrame({ 8000, false, true });
	r.check(vm.mode() == Mode::Lobby);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message().empty());
}

void testMatchingSendFailureReturnsLobby(TestResult& r) {
	LoginViewModel vm;
	enterLobby(vm);
	vm.clickStartGame();
	r.check(onlyAction(vm, Action::SendReadyToPlay));

	// 开始游戏发包失败：回大厅，文案提示，不把人困在匹配界面。
	vm.onActionFailed(Action::SendReadyToPlay);
	r.check(vm.mode() == Mode::Lobby);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "发送失败，请重试");

	// 退出匹配发包失败同样直接回大厅（静默）。
	vm.clickStartGame();
	vm.takeActions();
	vm.clickCancelMatch();
	r.check(onlyAction(vm, Action::SendCancelMatchmaking));
	vm.onActionFailed(Action::SendCancelMatchmaking);
	r.check(vm.mode() == Mode::Lobby);
	r.check(vm.message().empty());
}

void testMatchingDisconnectAndGameStart(TestResult& r) {
	LoginViewModel matching;
	enterLobby(matching);
	matching.clickStartGame();
	matching.takeActions();

	// 匹配中断线：会话失败结束。
	matching.onFrame({ 0, false, false });
	r.check(matching.done());
	r.check(!matching.session().ok);

	LoginViewModel started;
	enterLobby(started);
	started.clickStartGame();
	started.takeActions();
	started.onFrame({ 0, true, false });
	r.check(started.done());
	r.check(started.session().ok);
}

void testEscapeClosesWindow(TestResult& r) {
	LoginViewModel vm;
	vm.escape();
	r.check(onlyAction(vm, Action::CloseWindow));

	// 匹配界面 Esc = 退出匹配，不关窗。
	LoginViewModel matching;
	enterLobby(matching);
	matching.clickStartGame();
	matching.takeActions();
	matching.escape();
	r.check(matching.status() == Status::WaitingCancel);
	r.check(onlyAction(matching, Action::SendCancelMatchmaking));
}

// --- 匹配横幅 ---

void testMatchNotice(TestResult& r) {
	LoginViewModel vm;
	enterLobby(vm);

	// 自己发起 / 空名字：忽略。
	vm.onMatchmakingStarted("alice");
	vm.onMatchmakingStarted("");
	r.check(!vm.matchNotice().opponentMatching);
	r.check(vm.takeLogs().empty());

	vm.onMatchmakingStarted("bob");
	r.check(vm.matchNotice().opponentMatching);
	r.check(vm.matchNotice().opponentName == "bob");
	const auto logs = vm.takeLogs();
	r.check(logs.size() == 1);

	// 自己点开始游戏：横幅清掉，随后收到解散广播也无副作用。
	vm.clickStartGame();
	vm.takeActions();
	r.check(!vm.matchNotice().opponentMatching);

	// 大厅里收到解散：清横幅 + 清一次性文案（登录成功文案也一并清掉）。
	LoginViewModel lobby;
	enterLobby(lobby);
	lobby.onMatchmakingStarted("bob");
	lobby.onMatchmakingCancelled();
	r.check(!lobby.matchNotice().opponentMatching);
	r.check(lobby.mode() == Mode::Lobby);
	r.check(lobby.message().empty());

	// 匹配中收到解散：静默回大厅。
	LoginViewModel matching;
	enterLobby(matching);
	matching.clickStartGame();
	matching.takeActions();
	matching.onMatchmakingCancelled();
	r.check(matching.mode() == Mode::Lobby);
	r.check(matching.status() == Status::Idle);
}

// --- 签到与商城 ---

void testCheckinPopup(TestResult& r) {
	LoginViewModel vm;
	enterLobby(vm);

	vm.clickCheckin();
	r.check(vm.status() == Status::WaitingCheckin);
	r.check(onlyAction(vm, Action::SendCheckin));

	unool::protocol::AccountEvent done = makeEvent(MessageType::CheckinResponse);
	done.msg = "签到成功\n今日 +10 积分";
	done.points = 110;
	vm.onAccountEvent(done);

	r.check(vm.status() == Status::Idle);
	r.check(vm.checkinPopup());
	r.check(vm.checkinMsg() == "签到成功\n今日 +10 积分");
	r.check(vm.session().points == 110);

	vm.dismissCheckinPopup();
	r.check(!vm.checkinPopup());

	// 签到发包失败：提示后回到可重试状态。
	vm.clickCheckin();
	vm.takeActions();
	vm.onActionFailed(Action::SendCheckin);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "发送失败，请重试");
}

void testShopAndSessionCheck(TestResult& r) {
	LoginViewModel vm;
	enterLobby(vm);

	vm.clickShop();
	r.check(onlyAction(vm, Action::OpenShop));

	// 商城期间没断线、窗口还在：继续留在大厅。
	vm.onShopClosed(true);
	r.check(vm.mode() == Mode::Lobby);
	r.check(!vm.done());

	// 商城之后发现断线 / 关窗：本次登录会话判定失败。
	vm.onShopClosed(false);
	r.check(vm.done());
	r.check(!vm.session().ok);
}

}

TestResult runLoginViewModelTests() {
	TestResult r;

	r.section("login.success", testLoginSuccess);
	r.section("login.empty_input", testLoginRejectsEmptyInput);
	r.section("login.failure", testLoginFailureKeepsIdle);
	r.section("login.send_failure", testSendFailureRollsBack);
	r.section("login.input_rules", testInputRules);
	r.section("register.chain", testRegisterChain);
	r.section("register.failure", testRegisterFailure);
	r.section("account.parse_failure", testParseFailure);
	r.section("matching.cancel_timeout", testMatchingCancelTimeout);
	r.section("matching.send_failure", testMatchingSendFailureReturnsLobby);
	r.section("matching.disconnect_and_gamestart", testMatchingDisconnectAndGameStart);
	r.section("matching.escape", testEscapeClosesWindow);
	r.section("match_notice", testMatchNotice);
	r.section("lobby.checkin", testCheckinPopup);
	r.section("lobby.shop", testShopAndSessionCheck);

	return r;
}
