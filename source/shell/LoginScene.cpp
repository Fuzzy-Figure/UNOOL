#include "LoginScene.h"
#include "GameRenderer.h"
#include "AccountProtocol.h"
#include "AccountEvent.h"
#include <SFML/Network.hpp>
#include "shell/IShopOpener.h"
#include <algorithm>
#include <chrono>
#include <format>
#include <print>
#include <string>
#include <vector>

// P1-3/P1-4：收发均经 ConnectionState；协议构造上移到语义方法，本单元只做 View 与事件翻译。

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
					   unool::shell::IShopOpener& opener, const std::string& title)
	: renderer(r), net(n), shopOpener(opener), titleBrackets(title) {
	layoutBoxes();
}

void LoginScene::layoutBoxes() {
	const auto& cfg = renderer.getConfig();
	const float cx = static_cast<float>(cfg.windowSize.x) / 2.f;
	const float boxW = 600.f;
	const float boxH = 70.f;
	const float btnW = 220.f;
	const float btnH = 70.f;
	const float gap = 40.f;
	const float totalBtnW = btnW * 2 + gap;

	const float unameY = static_cast<float>(cfg.windowSize.y) * 0.38f;
	const float pwdY = static_cast<float>(cfg.windowSize.y) * 0.50f;
	const float btnY = static_cast<float>(cfg.windowSize.y) * 0.62f;

	usernameBox = sf::FloatRect({ cx - boxW / 2, unameY }, { boxW, boxH });
	passwordBox = sf::FloatRect({ cx - boxW / 2, pwdY }, { boxW, boxH });
	loginBtn = sf::FloatRect({ cx - totalBtnW / 2, btnY }, { btnW, btnH });
	registerBtn = sf::FloatRect({ cx - totalBtnW / 2 + btnW + gap, btnY }, { btnW, btnH });

	// 大厅按钮：开始游戏 + 商城
	const float lobbyBtnY = static_cast<float>(cfg.windowSize.y) * 0.55f;
	startGameBtn = sf::FloatRect({ cx - totalBtnW / 2, lobbyBtnY }, { btnW, btnH });
	lobbyShopBtn = sf::FloatRect({ cx - totalBtnW / 2 + btnW + gap, lobbyBtnY }, { btnW, btnH });

	// 签到按钮：位于两个按钮上方，水平居中
	const float checkinW = totalBtnW;
	const float checkinH = 64.f;
	const float checkinY = lobbyBtnY - checkinH - 30.f;
	checkinBtn = sf::FloatRect({ cx - checkinW / 2, checkinY }, { checkinW, checkinH });

	// 匹配等待界面：居中的「退出匹配」按钮
	const float cancelY = static_cast<float>(cfg.windowSize.y) * 0.58f;
	cancelMatchBtn = sf::FloatRect({ cx - totalBtnW / 2, cancelY }, { totalBtnW, btnH });
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
		// 匹配中断线结束会话，取消请求 3 秒无应答本地兜底回大厅。
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
		case Vm::Mode::Matching:
			// 退出中不再重复发请求，等服务端确认或 3 秒超时后由 run() 回到大厅。
			if (cancelMatchBtn.contains(pos)) vm.clickCancelMatch();
			return;

		case Vm::Mode::Lobby:
			// 签到结果弹窗：点击任意位置关闭（优先于其它按钮）。
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

void LoginScene::executeActions() {
	for (const auto action : vm.takeActions()) {
		switch (action) {
			case Vm::Action::SendLogin:
				if (!net.sendLogin(vm.username(), vm.password())) vm.onActionFailed(action);
				break;
			case Vm::Action::SendRegister:
				if (!net.sendRegister(vm.username(), vm.password())) vm.onActionFailed(action);
				break;
			case Vm::Action::SendCheckUsername:
				if (!net.sendCheckUsername(vm.username())) vm.onActionFailed(action);
				break;
			case Vm::Action::SendReadyToPlay:
				if (!net.sendReadyToPlay()) vm.onActionFailed(action);
				break;
			case Vm::Action::SendCheckin:
				if (!net.sendCheckin()) vm.onActionFailed(action);
				break;
			case Vm::Action::SendCancelMatchmaking:
				if (!net.sendCancelMatchmaking()) vm.onActionFailed(action);
				break;
			case Vm::Action::OpenShop:
				// 商城的断线 / 关窗判定仍在这里收口：返回值只表达「会话是否还能继续」。
				shopOpener.openShop(vm.username(), vm.matchNotice());
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
	// 收包动作收拢在 ConnectionState 的显式队列：只消费订阅的 7 类，
	// 其余（尤其 GameStart）留在队列里交给 gamePhase。
	// ConnectionInfo 属于「连接」而非「账号状态」，就地处理（setPlayerId + 日志）；
	// 其余响应解析成值对象后交给 ViewModel，解析失败走 onAccountParseFailure。
	while (auto next = net.take({
			   MessageType::ConnectionInfo,
			   MessageType::CheckUsernameResponse,
			   MessageType::RegisterResponse,
			   MessageType::LoginResponse,
			   MessageType::CheckinResponse,
			   MessageType::MatchmakingCancelled,
			   MessageType::MatchmakingStarted })) {
		auto [mt, packet] = std::move(*next);

		switch (mt) {
			case MessageType::ConnectionInfo: {
				auto event = unool::protocol::parseAccountEvent(mt, packet);
				if (event) {
					net.setPlayerId(event->pid);
					std::println("{} 分配到玩家: {}", titleBrackets, vm.username());
				}
				continue;
			}

			case MessageType::MatchmakingStarted:
				vm.onMatchmakingStarted(unool::protocol::parseMatchmakingStarted(packet).value_or(std::string{}));
				continue;

			case MessageType::MatchmakingCancelled:
				vm.onMatchmakingCancelled();
				continue;

			default: {
				auto event = unool::protocol::parseAccountEvent(mt, packet);
				if (event) vm.onAccountEvent(*event);
				else vm.onAccountParseFailure(mt);
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

void LoginScene::render() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	window.clear(sf::Color::White);

	// 标题（靠上居中，避免与输入框重叠）
	textMgr.displayTextInUp("UNOOL", { 50, 100 });

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

	const sf::Vector2u winSize = window.getSize();
	const std::string& message = vm.message();

	if (vm.mode() == Vm::Mode::Matching) {
		// 顶部标题由 render() 开头统一绘制，此处不重复
		std::string acctInfo = std::format("账号: {}  积分: {}", vm.username(), vm.session().points);
		textMgr.displayTextInUpRight(acctInfo, { 20, 40 }, sf::Color(60, 60, 60));

		const bool cancelling = (vm.status() == Vm::Status::WaitingCancel);
		const std::string title = cancelling ? "正在退出匹配..." : "正在匹配对手...";
		const sf::Vector2f titleSize = { 40, 80 };
		const auto titleMeasured = textMgr.measureText(title, static_cast<unsigned int>(titleSize.y));
		textMgr.displayText(title,
							{ (static_cast<float>(winSize.x) - titleMeasured.x) / 2.f,
							  static_cast<float>(winSize.y) * 0.28f },
							titleSize, sf::Color(40, 40, 40));

		const std::string sub = cancelling
			? "正在等待服务器确认，请稍候"
			: std::format("已准备，等待另一名玩家点击开始游戏（已等待 {} 秒）",
						  vm.matchElapsedSeconds());
		const sf::Vector2f subSize = { 22, 44 };
		const auto subMeasured = textMgr.measureText(sub, static_cast<unsigned int>(subSize.y));
		textMgr.displayText(sub,
							{ (static_cast<float>(winSize.x) - subMeasured.x) / 2.f,
							  static_cast<float>(winSize.y) * 0.42f },
							subSize, sf::Color(90, 90, 90));

		drawButton(cancelMatchBtn, cancelling ? "退出中..." : "退出匹配",
				   cancelling ? sf::Color(210, 210, 210) : sf::Color(245, 205, 205));

		const std::string hint = cancelling
			? "正在等待服务器确认..."
			: "点击「退出匹配」或按 Esc 返回大厅";
		const sf::Vector2f hintSize = { 18, 36 };
		const auto hintMeasured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
		textMgr.displayText(hint,
							{ (static_cast<float>(winSize.x) - hintMeasured.x) / 2.f,
							  static_cast<float>(winSize.y) * 0.92f },
							hintSize, sf::Color(120, 120, 120));

		window.display();
		return;
	}

	if (vm.mode() == Vm::Mode::Lobby) {
		// 大厅模式：右上角显示账号名和积分
		std::string acctInfo = std::format("账号: {}  积分: {}", vm.username(), vm.session().points);
		textMgr.displayTextInUpRight(acctInfo, { 20, 40 }, sf::Color(60, 60, 60));

		// 「对手已开始匹配」常驻横幅：放在标题与签到按钮之间的空白区，
		// 与底部的一次性 message 互不遮挡。
		if (vm.matchNotice().opponentMatching) {
			const std::string notice = std::format("对手「{}」已开始匹配，请点击「开始游戏」加入",
												   vm.matchNotice().opponentName);
			const sf::Vector2f noticeSize = { 26, 52 };
			const auto noticeMeasured = textMgr.measureText(notice, static_cast<unsigned int>(noticeSize.y));
			const float panelW = noticeMeasured.x + 100.f > 880.f ? noticeMeasured.x + 100.f : 880.f;
			const float panelH = 92.f;
			const float panelX = (static_cast<float>(winSize.x) - panelW) / 2.f;
			const float panelY = static_cast<float>(winSize.y) * 0.28f;

			sf::RectangleShape panel({ panelW, panelH });
			panel.setPosition({ panelX, panelY });
			panel.setFillColor(sf::Color(255, 244, 205));
			panel.setOutlineThickness(3.f);
			panel.setOutlineColor(sf::Color(210, 160, 40));
			window.draw(panel);

			textMgr.displayText(notice,
								{ panelX + (panelW - noticeMeasured.x) / 2.f,
								  panelY + (panelH - noticeMeasured.y) / 2.f },
								noticeSize, sf::Color(130, 85, 10));
		}

		// 签到按钮（金色系，突出显示）
		{
			sf::RectangleShape shadow({ checkinBtn.size.x, checkinBtn.size.y });
			shadow.setPosition({ checkinBtn.position.x + 3.f, checkinBtn.position.y + 5.f });
			shadow.setFillColor(sf::Color(0, 0, 0, 55));
			window.draw(shadow);

			sf::RectangleShape shape({ checkinBtn.size.x, checkinBtn.size.y });
			shape.setPosition({ checkinBtn.position.x, checkinBtn.position.y });
			shape.setFillColor(sf::Color(255, 224, 140));
			shape.setOutlineThickness(3.f);
			shape.setOutlineColor(sf::Color(198, 148, 42));
			window.draw(shape);

			// 顶部高光
			sf::RectangleShape gloss({ checkinBtn.size.x, 8.f });
			gloss.setPosition({ checkinBtn.position.x, checkinBtn.position.y });
			gloss.setFillColor(sf::Color(255, 255, 255, 130));
			window.draw(gloss);

			const std::string label = (vm.status() == Vm::Status::WaitingCheckin) ? "签到中..." : "每日签到";
			const sf::Vector2f labelSize = { 28, 56 };
			const sf::Vector2f labelMeasured = textMgr.measureText(label, static_cast<unsigned int>(labelSize.y));
			textMgr.displayText(label,
								{ checkinBtn.position.x + (checkinBtn.size.x - labelMeasured.x) / 2.f,
								  checkinBtn.position.y + (checkinBtn.size.y - labelMeasured.y) / 2.f },
								labelSize, sf::Color(110, 72, 12));
		}

		// 两个按钮
		drawButton(startGameBtn, "开始游戏");
		drawButton(lobbyShopBtn, "商城");

		// 大厅提示：登录 / 注册 / 签到等状态与错误文案（「登录成功」不在大厅展示）；
		// 退出匹配不产生文字提示，静默回到大厅。
		if (!message.empty() && message != "登录成功") {
			const sf::Vector2f msgSize = { 25, 50 };
			const auto msgMeasured = textMgr.measureText(message, static_cast<unsigned int>(msgSize.y));
			textMgr.displayText(message,
								{ (static_cast<float>(winSize.x) - msgMeasured.x) / 2.f,
								  static_cast<float>(winSize.y) * 0.78f },
								msgSize, sf::Color(180, 90, 20));
		}

		// 操作提示
		const std::string hint = "点击按钮选择 | Esc 退出";
		const sf::Vector2f hintSize = { 18, 36 };
		const sf::Vector2f hintMeasured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
		textMgr.displayText(hint,
							{ (static_cast<float>(winSize.x) - hintMeasured.x) / 2.f, static_cast<float>(winSize.y) * 0.92f },
							hintSize, sf::Color(120, 120, 120));

		if (vm.checkinPopup()) drawCheckinPopup();

		window.display();
		return;
	}

	// 登录/注册模式
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
	if (!message.empty()) {
		const sf::Vector2f msgSize = { 25, 50 };
		const auto msgMeasured = textMgr.measureText(message, static_cast<unsigned int>(msgSize.y));
		textMgr.displayText(message,
							{ (static_cast<float>(winSize.x) - msgMeasured.x) / 2.f, static_cast<float>(winSize.y) * 0.78f },
							msgSize, sf::Color::Red);
	}

	// 操作提示（底部居中）
	const std::string hint = "Tab 切换输入框 | Enter 提交 | Esc 退出";
	const sf::Vector2f hintSize = { 18, 36 };
	const sf::Vector2f hintMeasured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
	textMgr.displayText(hint,
						{ (static_cast<float>(winSize.x) - hintMeasured.x) / 2.f, static_cast<float>(winSize.y) * 0.92f },
						hintSize, sf::Color(120, 120, 120));

	window.display();
}
