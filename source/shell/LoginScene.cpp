#include "LoginScene.h"
#include "GameRenderer.h"
#include "AccountProtocol.h"
#include "AccountEvent.h"
#include "RoomProtocol.h"
#include "shell/IShopOpener.h"
#include <algorithm>
#include <chrono>
#include <format>
#include <print>
#include <string>
#include <utility>
#include <vector>

// P1-3/P1-4：收发均经 ConnectionState；协议构造上移到语义方法，本单元只做 View 与事件翻译。
// 房间功能（协议版本 3）：大厅 = 房间列表（创建 / 加入 / 刷新），房间内 = 准备 / 离开。

// MessageType 归 protocol 层；本单元类在全局命名空间，这里做 TU 局部的名字转发。
using unool::protocol::MessageType;

namespace {

	using Vm = unool::shell::LoginViewModel;

	// 注入给 ViewModel 的单调时钟（毫秒）。VM 不读系统时钟，超时逻辑才能 headless 断言。
	std::int64_t steadyNowMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
				   std::chrono::steady_clock::now().time_since_epoch())
			.count();
	}

}

LoginScene::LoginScene(GameRenderer& r, unool::shell::ConnectionState& n,
					   unool::shell::IShopOpener& opener, const std::string& title,
					   std::string initialMessage)
	: renderer(r), net(n), shopOpener(opener), titleBrackets(title) {
	layoutBoxes();
	// 顶下线等原因带到登录界面：只写提示文案，登录状态机从干净的说起
	if (!initialMessage.empty()) vm.setMessage(std::move(initialMessage));
}

void LoginScene::layoutBoxes() {
	const auto& cfg = renderer.getConfig();
	const float w = static_cast<float>(cfg.windowSize.x);
	const float h = static_cast<float>(cfg.windowSize.y);
	const float cx = w / 2.f;

	// === 登录 / 注册 ===
	const float boxW = 600.f;
	const float boxH = 70.f;
	const float btnW = 220.f;
	const float btnH = 70.f;
	const float gap = 40.f;
	const float totalBtnW = btnW * 2 + gap;

	const float unameY = h * 0.38f;
	const float pwdY = h * 0.50f;
	const float btnY = h * 0.62f;

	usernameBox = sf::FloatRect({ cx - boxW / 2, unameY }, { boxW, boxH });
	passwordBox = sf::FloatRect({ cx - boxW / 2, pwdY }, { boxW, boxH });
	loginBtn = sf::FloatRect({ cx - totalBtnW / 2, btnY }, { btnW, btnH });
	registerBtn = sf::FloatRect({ cx - totalBtnW / 2 + btnW + gap, btnY }, { btnW, btnH });

	// === 主大厅：开始游戏 / 商城 / 每日签到 ===
	const float lobbyW = 480.f;
	checkinBtn = sf::FloatRect({ cx - 300.f, h * 0.28f }, { 600.f, 76.f });
	startGameBtn = sf::FloatRect({ cx - lobbyW / 2, h * 0.44f }, { lobbyW, 92.f });
	lobbyShopBtn = sf::FloatRect({ cx - lobbyW / 2, h * 0.44f + 116.f }, { lobbyW, 70.f });

	// === 房间界面：房间名输入 + 创建 + 返回大厅 + 房间列表 ===
	const float rowY = h * 0.24f;
	roomNameBox = sf::FloatRect({ cx - 620.f, rowY }, { 460.f, 60.f });
	createRoomBtn = sf::FloatRect({ cx - 140.f, rowY }, { 220.f, 60.f });
	backToLobbyBtn = sf::FloatRect({ cx + 100.f, rowY }, { 220.f, 60.f });

	// 房间列表：固定可见行数，选中项超出时自动滚动
	roomListArea = sf::FloatRect({ cx - 620.f, h * 0.33f }, { 1200.f, roomRowHeight * visibleRoomRows });

	// 列表下方一排操作按钮
	const float actionY = roomListArea.position.y + roomListArea.size.y + 28.f;
	joinRoomBtn = sf::FloatRect({ cx - 620.f, actionY }, { 300.f, 64.f });
	refreshRoomsBtn = sf::FloatRect({ cx - 300.f, actionY }, { 220.f, 64.f });

	// === 房间内 ===
	readyBtn = sf::FloatRect({ cx - 240.f, h * 0.60f }, { 480.f, 80.f });
	leaveRoomBtn = sf::FloatRect({ cx - 240.f, h * 0.60f + 112.f }, { 480.f, 70.f });
}

sf::FloatRect LoginScene::roomRowRect(const std::size_t visibleIndex) const {
	return sf::FloatRect(
		{ roomListArea.position.x, roomListArea.position.y + static_cast<float>(visibleIndex) * roomRowHeight },
		{ roomListArea.size.x, roomRowHeight - 6.f });
}

std::size_t LoginScene::firstVisibleRoom() const {
	const std::size_t selected = vm.selectedRoom();
	if (selected < visibleRoomRows) return 0;
	return selected - visibleRoomRows + 1;
}

LoginScene::Result LoginScene::run() {
	while (renderer.windowIsOpen()) {
		for (auto ev = renderer.pollEvent(); ev; ev = renderer.pollEvent()) {
			handleEvent(*ev);
		}
		// P1-3：连接状态机每帧统一驱动 IO、心跳与入站队列。
		net.pump();
		pollAccountPackets();

		// 每帧判定交给 ViewModel：GameStart 可见即结束登录阶段（包留给 gamePhase），
		// 大厅 / 房间阶段断线结束会话，房间列表定时刷新。
		vm.onFrame({ steadyNowMs(), net.contains(MessageType::GameStart), net.isConnected() });
		executeActions();
		printLogs();

		render();
		if (vm.done()) break;
	}
	return vm.session();
}

void LoginScene::handleEvent(const sf::Event& event) {
	if (event.is<sf::Event::Closed>()) {
		renderer.closeWindow();
		return;
	}
	if (event.is<sf::Event::KeyPressed>()) {
		if (auto kp = event.getIf<sf::Event::KeyPressed>()) handleKeyPressed(*kp);
		return;
	}
	if (event.is<sf::Event::TextEntered>()) {
		if (auto te = event.getIf<sf::Event::TextEntered>()) handleTextEntered(*te);
		return;
	}
	if (event.is<sf::Event::MouseButtonPressed>()) {
		if (auto mb = event.getIf<sf::Event::MouseButtonPressed>(); mb && mb->button == sf::Mouse::Button::Left) {
			handleMouseClick(static_cast<sf::Vector2f>(mb->position));
		}
		return;
	}
}

void LoginScene::handleKeyPressed(const sf::Event::KeyPressed& key) {
	// 所有「这个键在当前状态下有没有意义」的判断都在 ViewModel 里，这里只做键位翻译。
	switch (key.scancode) {
		case sf::Keyboard::Scancode::Escape:
			vm.escape();
			return;
		case sf::Keyboard::Scancode::Backspace:
			vm.backspace();
			return;
		case sf::Keyboard::Scancode::Tab:
			vm.toggleFocus();
			return;
		case sf::Keyboard::Scancode::Enter:
			vm.submit();
			return;
		case sf::Keyboard::Scancode::Up:
			if (vm.mode() == Vm::Mode::RoomBrowser) vm.selectRoom(-1);
			return;
		case sf::Keyboard::Scancode::Down:
			if (vm.mode() == Vm::Mode::RoomBrowser) vm.selectRoom(+1);
			return;
		default:
			return;
	}
}

void LoginScene::handleTextEntered(const sf::Event::TextEntered& te) {
	vm.typeChar(te.unicode);
}

void LoginScene::handleMouseClick(const sf::Vector2f& pos) {
	// 几何命中留在这里（矩形由 layoutBoxes 算好），命中之后一律交给 VM。
	switch (vm.mode()) {
		case Vm::Mode::Room:
			handleRoomClick(pos);
			return;
		case Vm::Mode::RoomBrowser:
			handleRoomBrowserClick(pos);
			return;
		case Vm::Mode::Lobby:
			handleLobbyClick(pos);
			return;
		default:
			break;
	}

	if (usernameBox.contains(pos)) {
		vm.focusUsername();
		return;
	}
	if (passwordBox.contains(pos)) {
		vm.focusPassword();
		return;
	}
	if (loginBtn.contains(pos)) {
		vm.clickLogin();
		return;
	}
	if (registerBtn.contains(pos)) {
		vm.clickRegister();
		return;
	}
}

void LoginScene::handleLobbyClick(const sf::Vector2f& pos) {
	// 主大厅只有三个入口：签到（含结果弹窗）/ 开始游戏 / 商城
	if (vm.checkinPopup()) {
		vm.dismissCheckinPopup();
		return;
	}
	if (checkinBtn.contains(pos)) {
		vm.clickCheckin();
		return;
	}
	if (startGameBtn.contains(pos)) {
		vm.clickStartGame();
		return;
	}
	if (lobbyShopBtn.contains(pos)) {
		vm.clickShop();
		return;
	}
}

void LoginScene::handleRoomBrowserClick(const sf::Vector2f& pos) {
	if (roomNameBox.contains(pos)) {
		vm.focusRoomName();
		return;
	}
	if (createRoomBtn.contains(pos)) {
		vm.clickCreateRoom();
		return;
	}
	if (backToLobbyBtn.contains(pos)) {
		vm.clickBackToLobby();
		return;
	}
	if (joinRoomBtn.contains(pos)) {
		vm.clickJoinRoom();
		return;
	}
	if (refreshRoomsBtn.contains(pos)) {
		vm.clickRefreshRooms();
		return;
	}

	// 房间列表：点未选中的行 = 选中；点已选中的行 = 直接加入
	const auto& rooms = vm.rooms();
	const std::size_t first = firstVisibleRoom();
	for (std::size_t i = 0; i < visibleRoomRows; ++i) {
		const std::size_t index = first + i;
		if (index >= rooms.size()) break;
		if (!roomRowRect(i).contains(pos)) continue;
		if (vm.selectedRoom() == index) {
			vm.clickJoinRoom();
		}
		else {
			vm.clickRoom(index);
		}
		return;
	}
}

void LoginScene::handleRoomClick(const sf::Vector2f& pos) {
	if (readyBtn.contains(pos)) {
		vm.clickReady();
		return;
	}
	if (leaveRoomBtn.contains(pos)) {
		vm.clickLeaveRoom();
		return;
	}
}

void LoginScene::executeActions() {
	for (const auto action : vm.takeActions()) {
		switch (action) {
			case Vm::Action::SendLogin:
				loginSentMs_ = steadyNowMs();
				if (!net.sendLogin(vm.username(), vm.password())) {
					loginSentMs_ = 0;
					vm.onActionFailed(action);
				}
				break;
			case Vm::Action::SendRegister:
				if (!net.sendRegister(vm.username(), vm.password())) vm.onActionFailed(action);
				break;
			case Vm::Action::SendCheckUsername:
				if (!net.sendCheckUsername(vm.username())) vm.onActionFailed(action);
				break;
			case Vm::Action::SendCheckin:
				if (!net.sendCheckin()) vm.onActionFailed(action);
				break;
			case Vm::Action::RequestRoomList:
				if (!net.requestRoomList()) vm.onActionFailed(action);
				break;
			case Vm::Action::CreateRoom:
				if (!net.createRoom(vm.effectiveRoomName())) vm.onActionFailed(action);
				break;
			case Vm::Action::JoinRoom: {
				const auto* info = vm.selectedRoomInfo();
				if (!info || !net.joinRoom(info->id)) vm.onActionFailed(action);
				break;
			}
			case Vm::Action::LeaveRoom:
				if (!net.leaveRoom()) vm.onActionFailed(action);
				break;
			case Vm::Action::SetReady:
				if (!net.sendRoomReady(true)) vm.onActionFailed(action);
				break;
			case Vm::Action::SetUnready:
				if (!net.sendRoomReady(false)) vm.onActionFailed(action);
				break;
			case Vm::Action::OpenShop:
				// 商城的断线 / 关窗判定仍在这里收口：返回值只表达「会话是否还能继续」。
				shopOpener.openShop(vm.username());
				vm.onShopClosed(net.isConnected() && renderer.windowIsOpen());
				break;
			case Vm::Action::CloseWindow:
				renderer.closeWindow();
				break;
		}
	}
}

void LoginScene::printLogs() {
	for (const auto& line : vm.takeLogs()) {
		std::println("{} {}", titleBrackets, line);
	}
}

void LoginScene::pollAccountPackets() {
	// 收包动作收拢在 ConnectionState 的显式队列：只消费订阅的 8 类，
	// 其余（尤其 GameStart）留在队列里交给 gamePhase。
	// ConnectionInfo 属于「连接」而非「账号状态」，就地处理（setPlayerId + 日志）；
	// 房间类事件解析成值对象后交给 ViewModel。
	while (auto next = net.take({
			   MessageType::ConnectionInfo,
			   MessageType::CheckUsernameResponse,
			   MessageType::RegisterResponse,
			   MessageType::LoginResponse,
			   MessageType::CheckinResponse,
			   MessageType::RoomListResponse,
			   MessageType::RoomStateResponse,
			   MessageType::RoomErrorResponse })) {
		auto [mt, packet] = std::move(*next);

		switch (mt) {
			case MessageType::ConnectionInfo: {
				auto event = unool::protocol::parseAccountEvent(mt, packet);
				if (event) {
					net.setPlayerId(event->pid);
					std::println("{} 连接槽位: {}", titleBrackets, event->pid);
				}
				continue;
			}

			case MessageType::RoomListResponse:
				if (auto rooms = unool::protocol::parseRoomListResponse(packet)) {
					vm.onRoomList(std::move(*rooms));
				}
				else {
					vm.onRoomError("房间列表解析失败");
				}
				continue;

			case MessageType::RoomStateResponse:
				if (auto room = unool::protocol::parseRoomStateResponse(packet)) {
					vm.onRoomState(std::move(*room));
				}
				else {
					vm.onRoomError("房间状态解析失败");
				}
				continue;

			case MessageType::RoomErrorResponse:
				if (auto msg = unool::protocol::parseRoomErrorResponse(packet)) {
					vm.onRoomError(std::move(*msg));
				}
				else {
					vm.onRoomError("房间操作失败");
				}
				continue;

			default: {
				auto event = unool::protocol::parseAccountEvent(mt, packet);
				if (!event) {
					vm.onAccountParseFailure(mt);
					continue;
				}
				// 登录往返耗时：把「服务端口令派生」与「网络 / 帧循环」分开看。
				if (mt == MessageType::LoginResponse && loginSentMs_ != 0) {
					std::println("{} 登录往返 {} ms", titleBrackets, steadyNowMs() - loginSentMs_);
					loginSentMs_ = 0;
				}
				vm.onAccountEvent(*event);
				continue;
			}
		}
	}
}

void LoginScene::drawCheckinPopup() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const sf::Vector2u winSize = window.getSize();
	const std::string& checkinMsg = vm.checkinMsg();

	// 先把文案拆成行，便于按行数动态计算弹窗高度
	std::vector<std::string> lines;
	{
		std::string line;
		for (std::size_t i = 0; i <= checkinMsg.size(); ++i) {
			if (i == checkinMsg.size() || checkinMsg[i] == '\n') {
				if (!line.empty()) lines.push_back(line);
				line.clear();
				if (i == checkinMsg.size()) break;
				continue;
			}
			line.push_back(checkinMsg[i]);
		}
	}
	if (lines.empty()) lines.emplace_back(checkinMsg);

	// 行数多时压缩字号与行距，保证不溢出屏幕
	float fontH = 44.f;
	float lineStep = 50.f;
	if (lines.size() > 14) {
		fontH = 24.f;
		lineStep = 28.f;
	}
	else if (lines.size() > 8) {
		fontH = 32.f;
		lineStep = 36.f;
	}

	// 半透明遮罩
	sf::RectangleShape mask({ static_cast<float>(winSize.x), static_cast<float>(winSize.y) });
	mask.setFillColor(sf::Color(0, 0, 0, 130));
	window.draw(mask);

	// 弹窗面板
	const float w = 760.f;
	const float maxH = static_cast<float>(winSize.y) - 80.f;
	const float contentH = static_cast<float>(lines.size()) * lineStep;
	const float h = std::min(maxH, 120.f + contentH + 70.f);
	const float x = (static_cast<float>(winSize.x) - w) / 2.f;
	const float y = (static_cast<float>(winSize.y) - h) / 2.f;

	sf::RectangleShape shadow({ w, h });
	shadow.setPosition({ x + 6.f, y + 8.f });
	shadow.setFillColor(sf::Color(0, 0, 0, 90));
	window.draw(shadow);

	sf::RectangleShape panel({ w, h });
	panel.setPosition({ x, y });
	panel.setFillColor(sf::Color(252, 250, 242));
	panel.setOutlineThickness(4.f);
	panel.setOutlineColor(sf::Color(198, 148, 42));
	window.draw(panel);

	// 顶部装饰条
	sf::RectangleShape bar({ w, 10.f });
	bar.setPosition({ x, y });
	bar.setFillColor(sf::Color(255, 200, 90));
	window.draw(bar);

	// 标题
	const std::string title = "签到结果";
	const sf::Vector2f titleSize = { 30, 60 };
	const sf::Vector2f titleMeasured = textMgr.measureText(title, static_cast<unsigned int>(titleSize.y));
	textMgr.displayText(title,
						{ x + (w - titleMeasured.x) / 2.f, y + 26.f },
						titleSize, sf::Color(180, 120, 20));

	// 正文（按行居中显示，超出面板时按行截断）
	const sf::Vector2f lineSize = { 22, fontH };
	float lineY = y + 96.f;
	const float textBottom = y + h - 60.f;
	for (const std::string& line : lines) {
		if (lineY + fontH > textBottom) break;
		const sf::Vector2f measured = textMgr.measureText(line, static_cast<unsigned int>(lineSize.y));
		textMgr.displayText(line,
							{ x + (w - measured.x) / 2.f, lineY },
							lineSize, sf::Color(60, 60, 60));
		lineY += lineStep;
	}

	// 关闭提示
	const std::string hint = "点击任意位置关闭";
	const sf::Vector2f hintSize = { 18, 36 };
	const sf::Vector2f hintMeasured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
	textMgr.displayText(hint,
						{ x + (w - hintMeasured.x) / 2.f, y + h - 52.f },
						hintSize, sf::Color(140, 140, 140));
}

void LoginScene::drawLobby() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const sf::Vector2u winSize = window.getSize();
	const float w = static_cast<float>(winSize.x);
	const float h = static_cast<float>(winSize.y);

	auto drawButton = [&](const sf::FloatRect& r, const std::string& label,
						  const sf::Color& fill = sf::Color(220, 230, 240),
						  const sf::Color& textColor = sf::Color::Black,
						  const sf::Vector2f& labelSize = { 26, 52 }) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(fill);
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(sf::Color::Black);
		window.draw(shape);
		const sf::Vector2f measured = textMgr.measureText(label, static_cast<unsigned int>(labelSize.y));
		textMgr.displayText(label,
							{ r.position.x + (r.size.x - measured.x) / 2.f,
							  r.position.y + (r.size.y - measured.y) / 2.f },
							labelSize, textColor);
	};

	// 右上角账号信息
	const std::string acctInfo = std::format("账号: {}  积分: {}", vm.username(), vm.session().points);
	textMgr.displayTextInUpRight(acctInfo, { 20, 40 }, sf::Color(60, 60, 60));

	// 每日签到（金色高亮）
	const bool checkingIn = (vm.status() == Vm::Status::WaitingCheckin);
	drawButton(checkinBtn, checkingIn ? "签到中..." : "每日签到",
			   checkingIn ? sf::Color(235, 235, 235) : sf::Color(255, 224, 140),
			   sf::Color(110, 72, 12), { 30, 60 });

	// 开始游戏：进入房间界面（创建 / 加入房间）
	drawButton(startGameBtn, "开始游戏", sf::Color(190, 220, 255), sf::Color(20, 40, 70), { 40, 80 });
	drawButton(lobbyShopBtn, "商城", sf::Color(240, 232, 210), sf::Color::Black, { 30, 60 });

	// 提示文案
	const std::string& message = vm.message();
	if (!message.empty() && message != "登录成功") {
		const sf::Vector2f msgSize = { 24, 48 };
		const auto measured = textMgr.measureText(message, static_cast<unsigned int>(msgSize.y));
		textMgr.displayText(message, { (w - measured.x) / 2.f, h * 0.82f },
							msgSize, sf::Color(180, 90, 20));
	}

	// 操作提示
	const std::string hint = "点击「开始游戏」进入房间界面 | Esc 退出";
	const sf::Vector2f hintSize = { 18, 36 };
	const auto hintMeasured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
	textMgr.displayText(hint, { (w - hintMeasured.x) / 2.f, h * 0.95f },
						hintSize, sf::Color(120, 120, 120));

	if (vm.checkinPopup()) drawCheckinPopup();
}

void LoginScene::drawRoomBrowser() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const sf::Vector2u winSize = window.getSize();
	const float w = static_cast<float>(winSize.x);
	const float h = static_cast<float>(winSize.y);

	auto drawButton = [&](const sf::FloatRect& r, const std::string& label,
						  const sf::Color& fill = sf::Color(220, 230, 240),
						  const sf::Color& textColor = sf::Color::Black) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(fill);
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(sf::Color::Black);
		window.draw(shape);
		const sf::Vector2f labelSize = { 26, 52 };
		const sf::Vector2f measured = textMgr.measureText(label, static_cast<unsigned int>(labelSize.y));
		textMgr.displayText(label,
							{ r.position.x + (r.size.x - measured.x) / 2.f,
							  r.position.y + (r.size.y - measured.y) / 2.f },
							labelSize, textColor);
	};

	// 右上角账号信息
	const std::string acctInfo = std::format("账号: {}  积分: {}", vm.username(), vm.session().points);
	textMgr.displayTextInUpRight(acctInfo, { 20, 40 }, sf::Color(60, 60, 60));

	// 房间名输入框 + 创建房间 + 返回大厅
	textMgr.displayText("房间名:", { roomNameBox.position.x - 150.f, roomNameBox.position.y + 10.f },
						{ 22, 44 }, sf::Color(70, 70, 70));
	{
		sf::RectangleShape box({ roomNameBox.size.x, roomNameBox.size.y });
		box.setPosition({ roomNameBox.position.x, roomNameBox.position.y });
		box.setFillColor(sf::Color::White);
		box.setOutlineThickness(3.f);
		box.setOutlineColor(vm.focus() == Vm::Focus::RoomName ? sf::Color::Blue : sf::Color::Black);
		window.draw(box);
		textMgr.displayText(vm.roomNameInput(),
							{ roomNameBox.position.x + 14.f, roomNameBox.position.y + 10.f },
							{ 22, 44 });
		const std::string hint = "留空则以「账号的房间」命名";
		textMgr.displayText(hint, { roomNameBox.position.x, roomNameBox.position.y + 68.f },
							{ 15, 30 }, sf::Color(140, 140, 140));
	}
	drawButton(createRoomBtn, "创建房间", sf::Color(210, 240, 210));
	drawButton(backToLobbyBtn, "返回大厅", sf::Color(238, 238, 238));

	// 房间列表
	const auto& rooms = vm.rooms();
	{
		sf::RectangleShape panel({ roomListArea.size.x, roomListArea.size.y });
		panel.setPosition({ roomListArea.position.x, roomListArea.position.y });
		panel.setFillColor(sf::Color(246, 248, 252));
		panel.setOutlineThickness(2.f);
		panel.setOutlineColor(sf::Color(180, 190, 205));
		window.draw(panel);

		const std::string header = std::format("房间列表（{} 个，↑↓ 选择，点两次或按「加入选中房间」加入）",
											   rooms.size());
		textMgr.displayText(header, { roomListArea.position.x, roomListArea.position.y - 38.f },
							{ 18, 36 }, sf::Color(90, 90, 100));

		if (rooms.empty()) {
			const std::string empty = "当前没有房间，先创建一个吧";
			textMgr.displayText(empty,
								{ roomListArea.position.x + 30.f, roomListArea.position.y + 40.f },
								{ 22, 44 }, sf::Color(150, 150, 160));
		}

		const std::size_t first = firstVisibleRoom();
		for (std::size_t i = 0; i < visibleRoomRows; ++i) {
			const std::size_t index = first + i;
			if (index >= rooms.size()) break;
			const auto& room = rooms[index];
			const sf::FloatRect row = roomRowRect(i);
			const bool selected = (index == vm.selectedRoom());

			sf::RectangleShape shape({ row.size.x, row.size.y });
			shape.setPosition({ row.position.x, row.position.y });
			shape.setFillColor(selected ? sf::Color(214, 233, 255) : sf::Color::White);
			shape.setOutlineThickness(selected ? 3.f : 1.f);
			shape.setOutlineColor(selected ? sf::Color(60, 130, 220) : sf::Color(210, 215, 225));
			window.draw(shape);

			textMgr.displayText(room.name,
								{ row.position.x + 20.f, row.position.y + 8.f },
								{ 22, 44 },
								room.inGame ? sf::Color(150, 150, 150) : sf::Color(40, 40, 40));
			textMgr.displayText(std::format("房主：{}", room.owner),
								{ row.position.x + 520.f, row.position.y + 8.f },
								{ 20, 40 }, sf::Color(80, 80, 90));
			textMgr.displayText(std::format("{}/{} 人", room.members, room.capacity),
								{ row.position.x + 900.f, row.position.y + 8.f },
								{ 20, 40 }, sf::Color(80, 80, 90));
			textMgr.displayText(room.inGame ? "对局中" : "等待中",
								{ row.position.x + 1060.f, row.position.y + 8.f },
								{ 20, 40 },
								room.inGame ? sf::Color(190, 110, 40) : sf::Color(60, 140, 80));
		}
	}

	drawButton(joinRoomBtn, "加入选中房间", sf::Color(225, 235, 250));
	drawButton(refreshRoomsBtn, "刷新列表", sf::Color(238, 238, 238));

	// 提示文案
	const std::string& message = vm.message();
	if (!message.empty() && message != "登录成功") {
		const sf::Vector2f msgSize = { 24, 48 };
		const auto measured = textMgr.measureText(message, static_cast<unsigned int>(msgSize.y));
		textMgr.displayText(message, { (w - measured.x) / 2.f, h * 0.90f },
							msgSize, sf::Color(180, 90, 20));
	}

	// 操作提示
	const std::string hint = "↑↓ 选择房间 | Enter 创建房间 | Esc 返回大厅";
	const sf::Vector2f hintSize = { 18, 36 };
	const auto hintMeasured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
	textMgr.displayText(hint, { (w - hintMeasured.x) / 2.f, h * 0.95f },
						hintSize, sf::Color(120, 120, 120));
}

void LoginScene::drawRoom() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const sf::Vector2u winSize = window.getSize();
	const float w = static_cast<float>(winSize.x);
	const float h = static_cast<float>(winSize.y);
	const auto* room = vm.room();

	auto drawButton = [&](const sf::FloatRect& r, const std::string& label, const sf::Color& fill) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(fill);
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(sf::Color::Black);
		window.draw(shape);
		const sf::Vector2f labelSize = { 28, 56 };
		const sf::Vector2f measured = textMgr.measureText(label, static_cast<unsigned int>(labelSize.y));
		textMgr.displayText(label,
							{ r.position.x + (r.size.x - measured.x) / 2.f,
							  r.position.y + (r.size.y - measured.y) / 2.f },
							labelSize);
	};

	const std::string acctInfo = std::format("账号: {}  积分: {}", vm.username(), vm.session().points);
	textMgr.displayTextInUpRight(acctInfo, { 20, 40 }, sf::Color(60, 60, 60));

	if (!room) {
		// 房间快照还没到（刚加入 / 刚重连）：本帧只画标题，下一帧补上
		return;
	}

	// 房间标题
	const sf::Vector2f titleSize = { 40, 80 };
	const auto titleMeasured = textMgr.measureText(room->name, static_cast<unsigned int>(titleSize.y));
	textMgr.displayText(room->name, { (w - titleMeasured.x) / 2.f, h * 0.16f },
						titleSize, sf::Color(40, 40, 40));

	const std::string sub = std::format("房主：{}  |  {}/{} 人{}",
										room->owner, room->members.size(), room->capacity,
										room->inGame ? "  |  对局中" : "");
	const sf::Vector2f subSize = { 22, 44 };
	const auto subMeasured = textMgr.measureText(sub, static_cast<unsigned int>(subSize.y));
	textMgr.displayText(sub, { (w - subMeasured.x) / 2.f, h * 0.25f },
						subSize, sf::Color(90, 90, 100));

	// 成员列表
	float memberY = h * 0.36f;
	for (const auto& member : room->members) {
		const std::string line = std::format("{}{}    {}", member.name,
											 (member.owner ? "（房主）" : ""),
											 (member.ready ? "已准备" : "未准备"));
		const sf::Vector2f lineSize = { 26, 52 };
		const auto measured = textMgr.measureText(line, static_cast<unsigned int>(lineSize.y));
		textMgr.displayText(line, { (w - measured.x) / 2.f, memberY }, lineSize,
							member.ready ? sf::Color(50, 140, 80) : sf::Color(80, 80, 90));
		memberY += 64.f;
	}
	// 空位
	if (room->members.size() < room->capacity) {
		const std::string waiting = "等待其他玩家加入...";
		const sf::Vector2f lineSize = { 24, 48 };
		const auto measured = textMgr.measureText(waiting, static_cast<unsigned int>(lineSize.y));
		textMgr.displayText(waiting, { (w - measured.x) / 2.f, memberY }, lineSize,
							sf::Color(150, 150, 160));
	}

	// 准备 / 取消准备
	const bool ready = vm.selfReady();
	const bool canAct = !room->inGame;
	const std::string readyLabel = room->inGame ? "对局已开始" : (ready ? "取消准备" : "准备");
	drawButton(readyBtn, readyLabel,
			   !canAct ? sf::Color(225, 225, 225) : (ready ? sf::Color(215, 240, 215) : sf::Color(230, 240, 255)));
	drawButton(leaveRoomBtn, canAct ? "离开房间" : "对局进行中", sf::Color(245, 215, 215));

	// 提示
	const std::string hintText = room->members.size() < room->capacity
		? "等待第二名玩家加入房间"
		: (ready ? "已准备，等待对手准备后开局" : "双方都准备后自动开局");
	const sf::Vector2f hintSize = { 20, 40 };
	const auto hintMeasured = textMgr.measureText(hintText, static_cast<unsigned int>(hintSize.y));
	textMgr.displayText(hintText, { (w - hintMeasured.x) / 2.f, h * 0.80f },
						hintSize, sf::Color(90, 110, 140));

	const std::string& message = vm.message();
	if (!message.empty()) {
		const sf::Vector2f msgSize = { 22, 44 };
		const auto measured = textMgr.measureText(message, static_cast<unsigned int>(msgSize.y));
		textMgr.displayText(message, { (w - measured.x) / 2.f, h * 0.88f },
							msgSize, sf::Color(180, 90, 20));
	}

	const std::string hint = "点击「准备」后等待开局 | Esc 离开房间";
	const sf::Vector2f tailSize = { 18, 36 };
	const auto tailMeasured = textMgr.measureText(hint, static_cast<unsigned int>(tailSize.y));
	textMgr.displayText(hint, { (w - tailMeasured.x) / 2.f, h * 0.95f },
						tailSize, sf::Color(120, 120, 120));
}

void LoginScene::render() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	window.clear(sf::Color::White);

	// 标题（靠上居中，避免与输入框重叠）
	textMgr.displayTextInUp("UNOOL", { 50, 100 });

	if (vm.mode() == Vm::Mode::Lobby) {
		drawLobby();
		window.display();
		return;
	}
	if (vm.mode() == Vm::Mode::RoomBrowser) {
		drawRoomBrowser();
		window.display();
		return;
	}
	if (vm.mode() == Vm::Mode::Room) {
		drawRoom();
		window.display();
		return;
	}

	// 登录/注册模式
	const sf::Vector2u winSize = window.getSize();
	auto drawBox = [&](const sf::FloatRect& r, bool highlighted) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(sf::Color::White);
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(highlighted ? sf::Color::Blue : sf::Color::Black);
		window.draw(shape);
	};
	auto drawButton = [&](const sf::FloatRect& r, const std::string& label,
						  const sf::Color& fill = sf::Color(220, 230, 240)) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(fill);
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(sf::Color::Black);
		window.draw(shape);
		const sf::Vector2f labelSize = { 30, 60 };
		const sf::Vector2f labelMeasured = textMgr.measureText(label, static_cast<unsigned int>(labelSize.y));
		textMgr.displayText(label,
							{ r.position.x + (r.size.x - labelMeasured.x) / 2.f,
							  r.position.y + (r.size.y - labelMeasured.y) / 2.f },
							labelSize);
	};

	const std::string& username = vm.username();
	const std::string& password = vm.password();

	// 用户名行
	textMgr.displayText("用户名:", { usernameBox.position.x - 160.f, usernameBox.position.y + 15.f }, { 25, 50 });
	drawBox(usernameBox, vm.focus() == Vm::Focus::Username);
	textMgr.displayText(
		username,
		{ usernameBox.position.x + 15.f, usernameBox.position.y + 15.f },
		{ 25, 50 }
	);

	// 密码行 (明文显示)
	textMgr.displayText("密码:", { passwordBox.position.x - 160.f, passwordBox.position.y + 15.f }, { 25, 50 });
	drawBox(passwordBox, vm.focus() == Vm::Focus::Password);
	textMgr.displayText(
		password,
		{ passwordBox.position.x + 15.f, passwordBox.position.y + 15.f },
		{ 25, 50 }
	);

	// 按钮
	drawButton(loginBtn, "登录");
	drawButton(registerBtn, "注册");

	// 状态提示（按钮下方居中，避免与输入框重叠）
	const std::string& message = vm.message();
	if (!message.empty()) {
		const sf::Vector2f msgSize = { 25, 50 };
		const auto msgMeasured = textMgr.measureText(message, static_cast<unsigned int>(msgSize.y));
		textMgr.displayText(message,
							{ (static_cast<float>(winSize.x) - msgMeasured.x) / 2.f,
							  static_cast<float>(winSize.y) * 0.78f },
							msgSize, sf::Color::Red);
	}

	// 操作提示（底部居中）
	const std::string hint = "Tab 切换输入框 | Enter 提交 | Esc 退出";
	const sf::Vector2f hintSize = { 18, 36 };
	const auto hintMeasured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
	textMgr.displayText(hint,
						{ (static_cast<float>(winSize.x) - hintMeasured.x) / 2.f,
						  static_cast<float>(winSize.y) * 0.92f },
						hintSize, sf::Color(120, 120, 120));

	window.display();
}
