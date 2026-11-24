// Arch-C P1-4：LoginViewModel 单测。
//
// 这里断言的是「登录场景会怎么变」而不是「怎么画」：状态机搬到 ViewModel 之后，
// 注册链自动流转、房间列表刷新时机、创建/加入/离开房间、房内准备与开局判定这些分支
// 都能直接喂时间与事件，不必开窗口、连服务端。
//
// 房间功能（协议版本 3）：登录后不再匹配，而是「大厅房间列表 → 创建/加入 → 房内准备 → 开局」。
//
// 输入侧一律走语义方法（typeChar / submit / clickXxx），输出侧看两样：
//   - takeActions()：VM 要 View 去做的事（发包 / 开商城 / 关窗）；
//   - 只读状态：mode / status / message / session / 房间与弹窗。
// 发包失败用 onActionFailed() 回告，验证状态回滚与原 View 的发包失败分支一致。

#include "test_loginviewmodel.h"

#include "shell/LoginViewModel.h"

#include <string>
#include <utility>
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

// 取出 Actions 并判断「恰好是这两个，且顺序一致」。
bool onlyActions(LoginViewModel& vm, const Action first, const Action second) {
	const auto actions = vm.takeActions();
	return actions.size() == 2 && actions[0] == first && actions[1] == second;
}

unool::protocol::AccountEvent makeEvent(const MessageType type) {
	unool::protocol::AccountEvent event{};
	event.type = type;
	return event;
}

unool::protocol::RoomSummary makeSummary(const std::uint64_t id, std::string name, std::string owner,
										const std::size_t members, const bool inGame = false) {
	unool::protocol::RoomSummary summary{};
	summary.id = id;
	summary.name = std::move(name);
	summary.owner = std::move(owner);
	summary.members = members;
	summary.capacity = unool::protocol::kRoomCapacity;
	summary.inGame = inGame;
	return summary;
}

unool::protocol::RoomSnapshot makeSnapshot(const std::uint64_t id, std::string name, std::string owner,
										   std::vector<unool::protocol::RoomMember> members,
										   const bool inGame = false) {
	unool::protocol::RoomSnapshot snapshot{};
	snapshot.id = id;
	snapshot.name = std::move(name);
	snapshot.owner = std::move(owner);
	snapshot.capacity = unool::protocol::kRoomCapacity;
	snapshot.inGame = inGame;
	snapshot.members = std::move(members);
	return snapshot;
}

unool::protocol::RoomMember makeMember(std::string name, const bool ready, const bool owner) {
	unool::protocol::RoomMember member{};
	member.name = std::move(name);
	member.ready = ready;
	member.owner = owner;
	return member;
}

// 把 VM 送进主大厅：登录成功路径的公共前置（主大厅只有按钮：开始游戏 / 商城 / 签到）。
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
	// 登录成功会写一条日志；公共前置只负责「进大厅」
	(void)vm.takeLogs();
	(void)vm.takeActions();
}

// 主大厅「开始游戏」→ 房间界面（会立刻排一次房间列表请求）
void enterRoomBrowser(LoginViewModel& vm) {
	enterLobby(vm);
	vm.clickStartGame();
	(void)vm.takeActions();
}

// 房间界面「创建房间」→ 拿到自己的房间快照，进入房间
void enterRoom(LoginViewModel& vm) {
	enterRoomBrowser(vm);
	vm.clickCreateRoom();
	(void)vm.takeActions();
	vm.onRoomState(makeSnapshot(1, "alice的房间", "alice", { makeMember("alice", false, true) }));
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
	// 主大厅只有按钮：登录成功不拉房间列表（点「开始游戏」进房间界面时才拉）
	r.check(vm.focus() == Focus::None);
	r.check(vm.takeActions().empty());

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

	// 主大厅没有输入框：打字不改变任何字段；房间界面才接收房间名（上限 20 字符）
	LoginViewModel lobby;
	enterLobby(lobby);
	typeText(lobby, "zzz");
	r.check(lobby.username() == "alice");
	r.check(lobby.roomNameInput().empty());

	LoginViewModel browser;
	enterRoomBrowser(browser);
	typeText(browser, "zzz");
	r.check(browser.username() == "alice");
	r.check(browser.roomNameInput() == "zzz");
	typeText(browser, std::string(40, 'q'));
	r.check(browser.roomNameInput().size() == 20);
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
	(void)vm.takeActions();  // 这一次查重请求已经发出，先排空再看后续
	unool::protocol::AccountEvent free = makeEvent(MessageType::CheckUsernameResponse);
	free.exists = false;
	vm.onAccountEvent(free);
	r.check(vm.status() == Status::Idle);
	r.check(vm.message() == "密码不能为空");
	r.check(vm.takeActions().empty(), "密码为空不再发注册请求");

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
	// 「用户名可用，继续注册」会被紧接着的 requestRegister() 覆盖成「注册中...」，
	// 与原 View 一致：用户实际看到的是注册中，不是可用提示。
	r.check(ok.message() == "注册中...");
	r.check(ok.status() == Status::WaitingRegister);
	r.check(onlyAction(ok, Action::SendRegister));

	unool::protocol::AccountEvent registered = makeEvent(MessageType::RegisterResponse);
	registered.ok = true;
	ok.onAccountEvent(registered);
	// 同理：「注册成功，自动登录中...」被自动登录的「登录中...」覆盖。
	r.check(ok.message() == "登录中...");
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

// --- 房间界面：进入 / 返回 / 房间列表 ---

void testEnterRoomBrowser(TestResult& r) {
	LoginViewModel vm;
	enterLobby(vm);
	r.check(vm.mode() == Mode::Lobby);

	// 「开始游戏」不再匹配：进入房间界面并立刻拉一次列表
	vm.clickStartGame();
	r.check(vm.mode() == Mode::RoomBrowser);
	r.check(vm.focus() == Focus::RoomName);
	r.check(onlyAction(vm, Action::RequestRoomList));

	// 房间界面「返回大厅」：回到主大厅，不关窗、不发包
	vm.clickBackToLobby();
	r.check(vm.mode() == Mode::Lobby);
	r.check(vm.focus() == Focus::None);
	r.check(vm.takeActions().empty());

	// 房间界面 Esc = 返回大厅（主大厅 Esc 才是关窗）
	LoginViewModel escVm;
	enterRoomBrowser(escVm);
	escVm.escape();
	r.check(escVm.mode() == Mode::Lobby);
	r.check(escVm.takeActions().empty());
}

void testRoomListRefresh(TestResult& r) {
	LoginViewModel vm;
	vm.setNow(1000);
	enterRoomBrowser(vm);   // 进界面已经拉过一次（动作在上面的 helper 里排空）

	// 主大厅不刷新列表：这里用大厅验证「不请求」
	LoginViewModel lobby;
	enterLobby(lobby);
	lobby.onFrame({ 5000, false, true });
	r.check(lobby.takeActions().empty(), "主大厅不拉房间列表");

	// 房间界面：1 秒内不重复请求。
	vm.onFrame({ 1900, false, true });
	r.check(vm.takeActions().empty(), "1 秒内不重复拉列表");

	// 满 2 秒自动刷新一次。
	vm.onFrame({ 3000, false, true });
	r.check(onlyAction(vm, Action::RequestRoomList));

	// 列表内容进 VM，选中项越界时自动夹紧。
	vm.onRoomList({ makeSummary(3, "甲的房间", "甲", 1), makeSummary(4, "乙的房间", "乙", 2, true) });
	r.check(vm.rooms().size() == 2);
	r.check(vm.selectedRoom() == 0);
	vm.selectRoom(1);
	r.check(vm.selectedRoom() == 1);
	vm.selectRoom(1);
	r.check(vm.selectedRoom() == 0, "循环选择");
	r.check(vm.selectedRoomInfo() != nullptr && vm.selectedRoomInfo()->id == 3u);

	// 只剩一个房间时，选中项夹到最后一行。
	vm.onRoomList({ makeSummary(9, "丙的房间", "丙", 1) });
	r.check(vm.selectedRoom() == 0);
	r.check(vm.selectedRoomInfo() != nullptr && vm.selectedRoomInfo()->id == 9u);

	// 点行 = 选中，不直接加入。
	vm.clickRoom(0);
	r.check(vm.mode() == Mode::RoomBrowser);
	r.check(vm.takeActions().empty());
}

void testCreateRoom(TestResult& r) {
	LoginViewModel vm;
	enterRoomBrowser(vm);

	// 房间名留空：回退为「账号的房间」
	r.check(vm.effectiveRoomName() == "alice的房间");

	vm.clickCreateRoom();
	r.check(vm.status() == Status::WaitingCreateRoom);
	r.check(onlyAction(vm, Action::CreateRoom));

	// 服务端回快照 → 进入房间态
	vm.onRoomState(makeSnapshot(1, "alice的房间", "alice",
								{ makeMember("alice", false, true) }));
	r.check(vm.mode() == Mode::Room);
	r.check(vm.inRoom());
	r.check(vm.isOwner());
	r.check(!vm.selfReady());
	r.check(!vm.roomInGame());

	// 建房间失败：服务端回错误 → 留在房间界面，显示原因
	LoginViewModel bad;
	enterRoomBrowser(bad);
	bad.clickCreateRoom();
	(void)bad.takeActions();
	bad.onRoomError("房间数量已达上限，请稍后再试");
	r.check(bad.mode() == Mode::RoomBrowser);
	r.check(bad.status() == Status::Idle);
	r.check(bad.message() == "房间数量已达上限，请稍后再试");
}

void testJoinRoom(TestResult& r) {
	LoginViewModel vm;
	enterRoomBrowser(vm);

	// 没有房间时点加入：提示，不发包
	vm.clickJoinRoom();
	r.check(vm.message() == "当前没有可加入的房间");
	r.check(vm.takeActions().empty());

	vm.onRoomList({ makeSummary(5, "满人房", "甲", 2), makeSummary(6, "对局房", "乙", 2, true),
					makeSummary(7, "可加入", "丙", 1) });

	// 已满 / 对局中：提示且不发包
	vm.clickRoom(0);
	vm.clickJoinRoom();
	r.check(vm.message() == "该房间已满");
	r.check(vm.takeActions().empty());

	vm.clickRoom(1);
	vm.clickJoinRoom();
	r.check(vm.message() == "该房间正在对局中，无法加入");
	r.check(vm.takeActions().empty());

	// 可加入：进入等待，发包
	vm.clickRoom(2);
	vm.clickJoinRoom();
	r.check(vm.status() == Status::WaitingJoinRoom);
	r.check(onlyAction(vm, Action::JoinRoom));

	// 服务端回快照：房主是丙，自己是成员但非房主
	vm.onRoomState(makeSnapshot(7, "可加入", "丙",
								{ makeMember("丙", false, true), makeMember("alice", false, false) }));
	r.check(vm.mode() == Mode::Room);
	r.check(!vm.isOwner());
	r.check(vm.room() != nullptr && vm.room()->members.size() == 2u);

	// 加入失败（房间已解散）：留在房间界面并提示
	LoginViewModel gone;
	enterRoomBrowser(gone);
	gone.onRoomList({ makeSummary(8, "已解散", "丁", 1) });
	gone.clickJoinRoom();
	(void)gone.takeActions();
	gone.onRoomError("房间不存在或已解散");
	r.check(gone.mode() == Mode::RoomBrowser);
	r.check(gone.message() == "房间不存在或已解散");
}

// --- 房间内：准备 / 离开 / 开局 ---

void testReadyToggle(TestResult& r) {
	LoginViewModel vm;
	enterRoom(vm);

	vm.clickReady();
	r.check(onlyAction(vm, Action::SetReady));

	// 服务端广播自己已准备 → 再点就是取消准备
	vm.onRoomState(makeSnapshot(1, "alice的房间", "alice",
								{ makeMember("alice", true, true) }));
	r.check(vm.selfReady());
	vm.clickReady();
	r.check(onlyAction(vm, Action::SetUnready));

	// 对局中不能改准备
	vm.onRoomState(makeSnapshot(1, "alice的房间", "alice",
								{ makeMember("alice", true, true), makeMember("bob", true, false) }, true));
	r.check(vm.roomInGame());
	vm.clickReady();
	r.check(vm.takeActions().empty());

	// 发包失败：回滚到可重试，不留在等响应状态
	LoginViewModel failing;
	enterRoom(failing);
	failing.clickReady();
	(void)failing.takeActions();
	failing.onActionFailed(Action::SetReady);
	r.check(failing.message() == "发送失败，请重试");
}

void testLeaveRoom(TestResult& r) {
	LoginViewModel vm;
	enterRoom(vm);

	vm.clickLeaveRoom();
	// 离开房间回到「房间界面」（列表），不是主大厅
	r.check(vm.mode() == Mode::RoomBrowser);
	r.check(!vm.inRoom());
	r.check(onlyActions(vm, Action::LeaveRoom, Action::RequestRoomList));

	// 服务端确认离开（closed 快照）：只作为提示文案
	unool::protocol::RoomSnapshot closed{};
	closed.id = 1;
	closed.name = "alice的房间";
	closed.closed = true;
	closed.reason = "已离开房间";
	vm.onRoomState(closed);
	r.check(vm.mode() == Mode::RoomBrowser);
	r.check(vm.message() == "已离开房间");

	// 房内 Esc = 离开房间（不关窗）
	LoginViewModel escVm;
	enterRoom(escVm);
	escVm.escape();
	r.check(escVm.mode() == Mode::RoomBrowser);
	r.check(onlyActions(escVm, Action::LeaveRoom, Action::RequestRoomList));
}

void testOwnerTransferAndDissolve(TestResult& r) {
	LoginViewModel vm;
	enterRoom(vm);

	// 房主退出后服务端把房主转让给另一名玩家：VM 只跟随快照
	vm.onRoomState(makeSnapshot(1, "alice的房间", "bob",
								{ makeMember("bob", false, true), makeMember("alice", false, false) }));
	r.check(vm.mode() == Mode::Room);
	r.check(!vm.isOwner());

	// 房间解散：回房间界面并提示原因
	unool::protocol::RoomSnapshot closed{};
	closed.id = 1;
	closed.closed = true;
	closed.reason = "房主已解散房间";
	vm.onRoomState(closed);
	r.check(vm.mode() == Mode::RoomBrowser);
	r.check(!vm.inRoom());
	r.check(vm.message() == "房主已解散房间");
	r.check(onlyAction(vm, Action::RequestRoomList));
}

void testGameStartAndDisconnect(TestResult& r) {
	LoginViewModel inRoom;
	enterRoom(inRoom);
	// 房间内收到 GameStart：结束登录阶段（包留给 gamePhase）
	inRoom.onFrame({ 0, true, true });
	r.check(inRoom.done());
	r.check(inRoom.session().ok);

	// 大厅中断线：会话失败结束
	LoginViewModel lobby;
	enterLobby(lobby);
	lobby.onFrame({ 0, false, false });
	r.check(lobby.done());
	r.check(!lobby.session().ok);
}

void testEscapeClosesWindow(TestResult& r) {
	LoginViewModel vm;
	vm.escape();
	r.check(onlyAction(vm, Action::CloseWindow));
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

	// 商城期间没断线、窗口还在：继续留在主大厅（房间列表等进房间界面时再拉）
	vm.onShopClosed(true);
	r.check(vm.mode() == Mode::Lobby);
	r.check(!vm.done());
	r.check(vm.takeActions().empty());

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
	r.section("room.enter_browser", testEnterRoomBrowser);
	r.section("room.list_refresh", testRoomListRefresh);
	r.section("room.create", testCreateRoom);
	r.section("room.join", testJoinRoom);
	r.section("room.ready_toggle", testReadyToggle);
	r.section("room.leave", testLeaveRoom);
	r.section("room.owner_transfer", testOwnerTransferAndDissolve);
	r.section("room.game_start_and_disconnect", testGameStartAndDisconnect);
	r.section("lobby.escape", testEscapeClosesWindow);
	r.section("lobby.checkin", testCheckinPopup);
	r.section("lobby.shop", testShopAndSessionCheck);

	return r;
}
