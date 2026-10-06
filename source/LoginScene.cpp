#include "LoginScene.h"
#include "GameRenderer.h"
#include "AccountProtocol.h"


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
		message = "用户名和密码不能为空";
		return;
	}
	sf::Packet req = AccountProtocol::makeLoginRequest(username, password);
	if (!net.send(req)) {
		message = "发送失败，请关闭客户端的.exe文件后重试";
		return;
	}
	status = Status::WaitingLogin;
	message = "登录中...";
}

void LoginScene::sendRegisterRequest() {
	if (password.empty()) {
		message = "密码不能为空";
		status = Status::Idle;
		return;
	}
	sf::Packet req = AccountProtocol::makeRegisterRequest(username, password);
	if (!net.send(req)) {
		message = "发送失败，请重试";
		status = Status::Idle;
		return;
	}
	status = Status::WaitingRegister;
	message = "注册中...";
}

void LoginScene::sendCheckUsername() {
	if (username.empty()) {
		message = "用户名不能为空";
		return;
	}
	sf::Packet req = AccountProtocol::makeCheckUsernameRequest(username);
	if (!net.send(req)) {
		message = "发送失败，请重试";
		return;
	}
	status = Status::WaitingCheck;
	message = "检查用户名...";
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
				std::println("{} 分配到玩家ID: {}", titleBrackets, pid);
			}
			continue;
		}

		if (mt == MessageType::CheckUsernameResponse && status == Status::WaitingCheck) {
			auto exists = AccountProtocol::parseCheckUsernameResponse(packet);
			if (!exists) { message = "响应解析失败"; status = Status::Idle; continue; }
			if (*exists) {
				message = "该用户名已存在";
				status = Status::Idle;
			}
			else {
				message = "用户名可用，继续注册";
				sendRegisterRequest(); // 自动进入注册请求
			}
			continue;
		}

		if (mt == MessageType::RegisterResponse && status == Status::WaitingRegister) {
			auto resp = AccountProtocol::parseAccountResponse(packet);
			if (!resp) { message = "响应解析失败"; status = Status::Idle; continue; }
			if (resp->ok) {
				message = "注册成功，自动登录中...";
				sendLoginRequest(); // 注册成功，自动登录
			}
			else {
				message = std::format("注册失败: {}", resp->msg);
				status = Status::Idle;
			}
			continue;
		}

		if (mt == MessageType::LoginResponse && status == Status::WaitingLogin) {
			auto resp = AccountProtocol::parseAccountResponse(packet);
			if (!resp) { message = "响应解析失败"; status = Status::Idle; continue; }
			if (resp->ok) {
				result.username = username;
				result.password = password;
				result.points = resp->points;
				result.wins = resp->wins;
				result.losses = resp->losses;
				result.ok = true;
				status = Status::Done;
				message = "登录成功";
				std::println("{} 登录成功: {} 积分={} 胜={} 负={}", titleBrackets, resp->msg, resp->points, resp->wins, resp->losses);
			}
			else {
				message = std::format("登录失败: {}", resp->msg);
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
	textMgr.displayTextInUp("UNOOL", { 50, 100 });

	auto drawBox = [&](const sf::FloatRect& r, bool highlighted) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(sf::Color::White);
		shape.setOutlineThickness(3.f);
		shape.setOutlineColor(highlighted ? sf::Color::Blue : sf::Color::Black);
		window.draw(shape);
	};

	auto drawButton = [&](const sf::FloatRect& r, const std::string& label) {
		sf::RectangleShape shape({ r.size.x, r.size.y });
		shape.setPosition({ r.position.x, r.position.y });
		shape.setFillColor(sf::Color(220, 230, 240));
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

	// 用户名行
	textMgr.displayText("用户名:", { usernameBox.position.x - 160.f, usernameBox.position.y + 15.f }, { 25, 50 });
	drawBox(usernameBox, focus == Focus::Username);
	textMgr.displayText(
		username,
		{ usernameBox.position.x + 15.f, usernameBox.position.y + 15.f },
		{ 25, 50 }
	);

	// 密码行 (明文显示)
	textMgr.displayText("密码:", { passwordBox.position.x - 160.f, passwordBox.position.y + 15.f }, { 25, 50 });
	drawBox(passwordBox, focus == Focus::Password);
	textMgr.displayText(
		password,
		{ passwordBox.position.x + 15.f, passwordBox.position.y + 15.f },
		{ 25, 50 }
	);

	// 按钮
	drawButton(loginBtn, "登录");
	drawButton(registerBtn, "注册");

	// 状态提示（按钮下方居中，避免与输入框重叠）
	const sf::Vector2u winSize = window.getSize();
	if (!message.empty()) {
		const sf::Vector2f msgSize = { 25, 50 };
		const sf::Vector2f msgMeasured = textMgr.measureText(message, static_cast<unsigned int>(msgSize.y));
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
