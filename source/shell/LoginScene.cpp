#include "LoginScene.h"
#include "GameRenderer.h"
#include "AccountProtocol.h"
#include "AccountEvent.h"
#include <SFML/Network.hpp>
#include "shell/IShopOpener.h"
#include "PacketDispatcher.h"
#include <utility>


LoginScene::LoginScene(GameRenderer& r, unool::shell::IClientGateway& n,
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
		net.update();
		pollAccountPackets();

		// 服务端裁定开局后只 peek、不消费 GameStart：跨阶段留包约定要求把它交给
		// gamePhase 打印并处理。大厅 / 匹配等待都生效，覆盖「刚点退出匹配、对手
		// 同时点开始」的竞态——以服务端的 GameStart 为准直接进游戏。
		if (auto peek = net.peekMessageType();
			peek && *peek == static_cast<int>(MessageType::GameStart)
			&& (mode == Mode::Lobby || mode == Mode::Matching)) {
			status = Status::Done;
		}
		// 匹配阶段不再有「无限等待」：断线直接结束会话，取消请求 3 秒无应答则本地兜底回大厅。
		else if (mode == Mode::Matching) {
			if (!net.isConnected()) {
				status = Status::Done;
				result.ok = false;
			}
			else if (status == Status::WaitingCancel
				&& std::chrono::steady_clock::now() - cancelRequestedAt_ >= std::chrono::seconds(3)) {
				backToLobby(); // 超时兜底：静默回大厅，不显示文字提示
			}
		}
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
		// 匹配等待期间 Esc = 退出匹配（与按钮一致），不再直接关掉整个客户端
		if (mode == Mode::Matching) {
			if (status == Status::WaitingMatch) sendCancelMatchmaking();
			return;
		}
		renderer.closeWindow();
		return;
	}
	if (mode == Mode::Lobby || mode == Mode::Matching) return; // 大厅 / 匹配等待仅鼠标操作
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
	if (mode == Mode::Lobby || mode == Mode::Matching) return;
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
	if (mode == Mode::Matching) {
		// 退出中不再重复发请求，等服务端确认或 3 秒超时后由 run() 回到大厅
		if (status == Status::WaitingMatch && cancelMatchBtn.contains(pos)) {
			sendCancelMatchmaking();
		}
		return;
	}
	if (mode == Mode::Lobby) {
		// 签到结果弹窗：点击任意位置关闭
		if (checkinPopup) {
			checkinPopup = false;
			return;
		}
		if (status != Status::Idle) return;
		if (checkinBtn.contains(pos)) {
			sendCheckinRequest();
			return;
		}
		if (startGameBtn.contains(pos)) {
			// 退出匹配功能：不再点完就 Done 进入 gamePhase 空等，而是留在本场景的
			// 匹配等待界面；收到 GameStart 后 run() 才结束（包留给 gamePhase）。
			sf::Packet readyPkt = unool::protocol::makeReadyToPlay();
			if (!net.send(readyPkt)) {
				message = "发送失败，请重试";
				return;
			}
			mode = Mode::Matching;
			status = Status::WaitingMatch;
			clearMatchNotice(); // 自己已加入匹配，大厅的「对手已开始匹配」横幅不再需要
			message.clear();
			matchStartedAt_ = std::chrono::steady_clock::now();
			return;
		}
		if (lobbyShopBtn.contains(pos)) {
			// 进入商城，退出后回到大厅（S3a：只把构造 / 运行上移到组合根，
			// 此处仅发意图；断线与关窗判定仍在原位置，行为等价）。
		// 匹配提示状态随意图一起交给商城：商城期间对手开始 / 退出匹配时
		// ShopScene 就地写回，退出商城后大厅横幅自动同步（无需回传文案）。
		shopOpener.openShop(username, matchNotice_);
			if (!net.isConnected() || !renderer.windowIsOpen()) {
				status = Status::Done;
				result.ok = false;
			}
			return;
		}
		return;
	}
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
	sf::Packet req = unool::protocol::makeLoginRequest(username, password);
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
	sf::Packet req = unool::protocol::makeRegisterRequest(username, password);
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
	sf::Packet req = unool::protocol::makeCheckUsernameRequest(username);
	if (!net.send(req)) {
		message = "发送失败，请重试";
		return;
	}
	status = Status::WaitingCheck;
	message = "检查用户名...";
}

void LoginScene::sendCheckinRequest() {
	sf::Packet req = unool::protocol::makeCheckinRequest();
	if (!net.send(req)) {
		message = "发送失败，请重试";
		return;
	}
	status = Status::WaitingCheckin;
}

void LoginScene::sendCancelMatchmaking() {
	sf::Packet req = unool::protocol::makeCancelMatchmaking();
	if (!net.send(req)) {
		// 发包失败也不能把用户困在等待界面，本地直接回大厅（不显示文字提示）
		backToLobby();
		return;
	}
	status = Status::WaitingCancel;
	message.clear();
	cancelRequestedAt_ = std::chrono::steady_clock::now();
}

void LoginScene::backToLobby() {
	mode = Mode::Lobby;
	status = Status::Idle;
	message.clear(); // 退出匹配静默返回大厅，不显示一次性文字提示
}

void LoginScene::clearMatchNotice() {
	matchNotice_.opponentMatching = false;
	matchNotice_.opponentName.clear();
}

void LoginScene::pollAccountPackets() {
	// 阶段 4 · S2：收包动作收拢到 PacketDispatcher。订阅集在原有 5 类账号响应上
	// 增加 MatchmakingCancelled / MatchmakingStarted（共 7 类；Heartbeat 由
	// dispatcher 内部统一消费并回复），「非订阅包不消费、留给 gamePhase」的
	// 跨阶段留包约定由 next() 逐字保留。
	// 阶段 4 · S3b2（53.9 ②A）：响应解析上移到 unool::protocol::parseAccountEvent()，
	// 结果以同步值对象返回；订阅集、每帧顺序、continue 与链式请求
	// （查重→注册→登录）的同帧触发点逐字保留，message / status / mode /
	// result / checkinPopup 仍由 View 写。
	while (auto next = unool::shell::PacketDispatcher::next(net, {
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
					std::println("{} 分配到玩家: {}", titleBrackets, username);
				}
				continue;
			}

			case MessageType::CheckUsernameResponse:
				if (status != Status::WaitingCheck) continue;
				{
					auto event = unool::protocol::parseAccountEvent(mt, packet);
					if (!event) { message = "响应解析失败"; status = Status::Idle; continue; }
					if (event->exists) {
						message = "该用户名已存在";
						status = Status::Idle;
					}
					else {
						message = "用户名可用，继续注册";
						sendRegisterRequest(); // 自动进入注册请求
					}
				}
				continue;

			case MessageType::RegisterResponse:
				if (status != Status::WaitingRegister) continue;
				{
					auto event = unool::protocol::parseAccountEvent(mt, packet);
					if (!event) { message = "响应解析失败"; status = Status::Idle; continue; }
					if (event->ok) {
						message = "注册成功，自动登录中...";
						sendLoginRequest(); // 注册成功，自动登录
					}
					else {
						message = std::format("注册失败: {}", event->msg);
						status = Status::Idle;
					}
				}
				continue;

			case MessageType::LoginResponse:
				if (status != Status::WaitingLogin) continue;
				{
					auto event = unool::protocol::parseAccountEvent(mt, packet);
					if (!event) { message = "响应解析失败"; status = Status::Idle; continue; }
					if (event->ok) {
						result.username = username;
						result.password = password;
						result.points = event->points;
						result.wins = event->wins;
						result.losses = event->losses;
						result.ok = true;
						// 登录成功后切换到大厅模式
						mode = Mode::Lobby;
						status = Status::Idle;
						message = "登录成功";
						std::println("{} 登录成功: {} 积分={} 胜={} 负={}", titleBrackets, event->msg, event->points, event->wins, event->losses);
					}
					else {
						message = std::format("登录失败: {}", event->msg);
						status = Status::Idle;
					}
				}
				continue;

			// 签到结果：弹出结果框，同步最新积分
			case MessageType::CheckinResponse:
				if (status != Status::WaitingCheckin) continue;
				{
					auto event = unool::protocol::parseAccountEvent(mt, packet);
					// parseAccountEvent 对该类型恒返回值对象（不检查 >> 结果），
					// 与原 View 的 msg / points 默认值行为逐字一致。
					checkinMsg = event ? event->msg : std::string{};
					checkinPopup = true;
					result.points = event ? event->points : 0;
					status = Status::Idle;
				}
				continue;

			// 对手开始匹配：大厅显示常驻横幅，直到对手退出 / 掉线或自己点开始游戏。
			// 不检查 status —— 该广播可能「先于 LoginResponse」到达（对手已准备、
			// 自己才刚登录），服务端也会在 onLoginSucceeded 里补发一次。
			case MessageType::MatchmakingStarted: {
				auto who = unool::protocol::parseMatchmakingStarted(packet);
				const std::string whoName = who.value_or(std::string{});
				if (whoName.empty() || whoName == username) continue; // 坏包 / 自己发起
				matchNotice_.opponentMatching = true;
				matchNotice_.opponentName = whoName;
				std::println("{} 对手「{}」已开始匹配，请点击「开始游戏」加入", titleBrackets, whoName);
				continue;
			}

			// 匹配解散事件由服务端广播给双方：清掉「对手已开始匹配」横幅，
			// 处于匹配等待的一方静默回到大厅（不再显示退出匹配的文字提示）；
			// 若此时 GameStart 已在队列中，run() 会优先让游戏开始。
			case MessageType::MatchmakingCancelled: {
				clearMatchNotice();
				if (mode == Mode::Matching) backToLobby();
				else if (mode == Mode::Lobby) message.clear();
				continue;
			}

			default:
				continue;
		}
	}
}

void LoginScene::drawCheckinPopup() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const sf::Vector2u winSize = window.getSize();

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

	if (mode == Mode::Matching) {
		// 顶部标题由 render() 开头统一绘制，此处不重复
		std::string acctInfo = std::format("账号: {}  积分: {}", username, result.points);
		textMgr.displayTextInUpRight(acctInfo, { 20, 40 }, sf::Color(60, 60, 60));

		const bool cancelling = (status == Status::WaitingCancel);
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
			              std::chrono::duration_cast<std::chrono::seconds>(
			                  std::chrono::steady_clock::now() - matchStartedAt_).count());
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

	if (mode == Mode::Lobby) {
		// 大厅模式：右上角显示账号名和积分
		std::string acctInfo = std::format("账号: {}  积分: {}", username, result.points);
		textMgr.displayTextInUpRight(acctInfo, { 20, 40 }, sf::Color(60, 60, 60));

		// 「对手已开始匹配」常驻横幅：放在标题与签到按钮之间的空白区，
		// 与底部的一次性 message 互不遮挡。
		if (matchNotice_.opponentMatching) {
			const std::string notice = std::format("对手「{}」已开始匹配，请点击「开始游戏」加入",
			                                      matchNotice_.opponentName);
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

			const std::string label = (status == Status::WaitingCheckin) ? "签到中..." : "每日签到";
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

		if (checkinPopup) drawCheckinPopup();

		window.display();
		return;
	}

	// 登录/注册模式
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
