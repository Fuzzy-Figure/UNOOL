#include "../header/LoginScene.h"
#include "../header/GameRenderer.h"
#include "../header/AccountProtocol.h"
#include <iostream>

LoginScene::LoginScene(GameRenderer& r, ClientNetwork& n, const std::string& title)
	: renderer(r), net(n), titleBrackets(title) {
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
}

LoginScene::Result LoginScene::run() {
	while (renderer.windowIsOpen()) {
		for (auto ev = renderer.pollEvent(); ev; ev = renderer.pollEvent()) {
			handleEvent(*ev);
		}
		net.update();
		pollAccountPackets();
		render();
		if (status == Status::Done) break;
	}
	return result;
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
	auto sc = key.scancode;
	if (sc == sf::Keyboard::Scancode::Escape) {
		renderer.closeWindow();
		return;
	}
	if (status != Status::Idle) return; // 等待响应时不接受输入
	if (sc == sf::Keyboard::Scancode::Backspace) {
		if (focus == Focus::Username && !username.empty()) username.pop_back();
		else if (focus == Focus::Password && !password.empty()) password.pop_back();
		return;
	}
	if (sc == sf::Keyboard::Scancode::Tab) {
		if (focus == Focus::Username) focus = Focus::Password;
		else if (focus == Focus::Password) focus = Focus::Username;
		else focus = Focus::Username;
		return;
	}
	if (sc == sf::Keyboard::Scancode::Enter) {
		if (focus == Focus::Username) {
			focus = Focus::Password;
		}
		else if (focus == Focus::Password) {
			if (mode == Mode::Login) sendLoginRequest();
			else sendCheckUsername();
		}
		return;
	}
}

void LoginScene::handleTextEntered(const sf::Event::TextEntered& te) {
	if (status != Status::Idle) return;
	if (focus == Focus::None) return;
	char32_t ch = te.unicode;
	// 只接受可打印 ASCII (33..126)，排除空格 (对齐 safeReadNoSpace 规则)
	if (ch < 33 || ch > 126) return;
	char c = static_cast<char>(ch);
	if (focus == Focus::Username) {
		if (username.size() < 32) username.push_back(c);
	}
	else if (focus == Focus::Password) {
		if (password.size() < 32) password.push_back(c);
	}
}

void LoginScene::handleMouseClick(const sf::Vector2f& pos) {
	if (status != Status::Idle) return;
	if (usernameBox.contains(pos)) {
		focus = Focus::Username;
		return;
	}
	if (passwordBox.contains(pos)) {
		focus = Focus::Password;
		return;
	}
	if (loginBtn.contains(pos)) {
		mode = Mode::Login;
		focus = Focus::None;
		sendLoginRequest();
		return;
	}
	if (registerBtn.contains(pos)) {
		mode = Mode::Register;
		focus = Focus::None;
		sendCheckUsername();
		return;
	}
}

void LoginScene::sendLoginRequest() {
	if (username.empty() || password.empty()) {
		message = L"用户名和密码不能为空";
		return;
	}
	sf::Packet req = AccountProtocol::makeLoginRequest(username, password);
	if (!net.send(req)) {
		message = L"发送失败，请重试";
		return;
	}
	status = Status::WaitingLogin;
	message = L"登录中...";
}

void LoginScene::sendRegisterRequest() {
	if (password.empty()) {
		message = L"密码不能为空";
		status = Status::Idle;
		return;
	}
	sf::Packet req = AccountProtocol::makeRegisterRequest(username, password);
	if (!net.send(req)) {
		message = L"发送失败，请重试";
		status = Status::Idle;
		return;
	}
	status = Status::WaitingRegister;
	message = L"注册中...";
}

void LoginScene::sendCheckUsername() {
	if (username.empty()) {
		message = L"用户名不能为空";
		return;
	}
	sf::Packet req = AccountProtocol::makeCheckUsernameRequest(username);
	if (!net.send(req)) {
		message = L"发送失败，请重试";
		return;
	}
	status = Status::WaitingCheck;
	message = L"检查用户名...";
}

void LoginScene::pollAccountPackets() {
	while (auto packetOpt = net.receivePacket()) {
		sf::Packet packet = *packetOpt;
		int msgType;
		if (!(packet >> msgType)) continue;
		auto mt = static_cast<MessageType>(msgType);

		if (mt == MessageType::ConnectionInfo) {
			std::size_t pid;
			if (packet >> pid) {
				net.setPlayerId(pid);
				std::cout << titleBrackets << " 分配到玩家ID: " << pid << std::endl;
			}
			continue;
		}

		if (mt == MessageType::CheckUsernameResponse && status == Status::WaitingCheck) {
			auto exists = AccountProtocol::parseCheckUsernameResponse(packet);
			if (!exists) { message = L"响应解析失败"; status = Status::Idle; continue; }
			if (*exists) {
				message = L"该用户名已存在";
				status = Status::Idle;
			}
			else {
				message = L"用户名可用，继续注册";
				sendRegisterRequest(); // 自动进入注册请求
			}
			continue;
		}

		if (mt == MessageType::RegisterResponse && status == Status::WaitingRegister) {
			auto resp = AccountProtocol::parseAccountResponse(packet);
			if (!resp) { message = L"响应解析失败"; status = Status::Idle; continue; }
			if (resp->ok) {
				message = L"注册成功，自动登录中...";
				sendLoginRequest(); // 注册成功，自动登录
			}
			else {
				message = L"注册失败: " + unool::string::to_utf16(resp->msg);
				status = Status::Idle;
			}
			continue;
		}

		if (mt == MessageType::LoginResponse && status == Status::WaitingLogin) {
			auto resp = AccountProtocol::parseAccountResponse(packet);
			if (!resp) { message = L"响应解析失败"; status = Status::Idle; continue; }
			if (resp->ok) {
				result.username = username;
				result.points = resp->points;
				result.wins = resp->wins;
				result.losses = resp->losses;
				result.ok = true;
				status = Status::Done;
				message = L"登录成功";
				std::cout << titleBrackets << " 登录成功: " << resp->msg
					<< " 积分=" << resp->points
					<< " 胜=" << resp->wins
					<< " 负=" << resp->losses << std::endl;
			}
			else {
				message = L"登录失败: " + unool::string::to_utf16(resp->msg);
				status = Status::Idle;
			}
			continue;
		}
	}
}

void LoginScene::render() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	window.clear(sf::Color::White);

	// 标题（靠上居中，避免与输入框重叠）
	textMgr.displayTextInUp(L"UNOOL 账号系统", { 40, 80 }, sf::Color::Black);

	auto drawBox = [&](const sf::FloatRect& r, bool highlighted) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(sf::Color::White);
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(highlighted ? sf::Color::Blue : sf::Color::Black);
		window.draw(shape);
	};

	auto drawButton = [&](const sf::FloatRect& r, const std::wstring& label) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(sf::Color(220, 230, 240));
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(sf::Color::Black);
		window.draw(shape);
		textMgr.displayText(label, { r.position.x + 40.f, r.position.y + 10.f }, { 30, 60 }, sf::Color::Black);
	};

	// 用户名行
	textMgr.displayText(L"用户名:", { usernameBox.position.x - 160.f, usernameBox.position.y + 15.f }, { 25, 50 }, sf::Color::Black);
	drawBox(usernameBox, focus == Focus::Username);
	textMgr.displayText(unool::string::to_utf16(username),
		{ usernameBox.position.x + 15.f, usernameBox.position.y + 15.f }, { 25, 50 }, sf::Color::Black);

	// 密码行 (明文显示)
	textMgr.displayText(L"密码:", { passwordBox.position.x - 160.f, passwordBox.position.y + 15.f }, { 25, 50 }, sf::Color::Black);
	drawBox(passwordBox, focus == Focus::Password);
	textMgr.displayText(unool::string::to_utf16(password),
		{ passwordBox.position.x + 15.f, passwordBox.position.y + 15.f }, { 25, 50 }, sf::Color::Black);

	// 按钮
	drawButton(loginBtn, L"登录");
	drawButton(registerBtn, L"注册");

	// 状态提示
	if (!message.empty()) {
		textMgr.displayTextInCenter(message, { 25, 50 }, sf::Color::Red);
	}

	// 操作提示
	textMgr.displayTextInCenter(L"Tab 切换输入框 | Enter 提交 | Esc 退出",
		{ 18, 36 }, sf::Color(120, 120, 120));

	window.display();
}
