#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "adapters/IClientGateway.h"
#include "adapters/IShopOpener.h"

class GameRenderer;

// 登录场景：图形界面完成注册/登录，成功后返回账号信息
// 阶段 4 · S3b1（用户 51.11 口径①A）：成员与构造参数由具体 ClientNetwork& 切到
// adapters 层的 IClientGateway&，并删掉 #include "Socket.h" 的粗粒度传递依赖。
// 5 个请求构造 / 4 个响应解析 / 状态机 / render() / run() 主循环逐字留在 .cpp，
// 协议上移按 51.5 ①A 归 S3b2；本批零行为变化。
class LoginScene {
public:
	struct Result {
		std::string username;
		std::string password;
		int points = 0;
		int wins = 0;
		int losses = 0;
		bool ok = false;
		bool enterShop = false; // 登录成功后进入商城（而非直接开始游戏）
	};

private:
	enum class Mode { Login, Register, Lobby } mode = Mode::Login;
	enum class Status { Idle, WaitingCheck, WaitingRegister, WaitingLogin, WaitingCheckin, Done } status = Status::Idle;
	enum class Focus { None, Username, Password } focus = Focus::Username;

	GameRenderer& renderer;
	unool::adapters::IClientGateway& net;
	// S3a：商城由组合根经意图端口打开，本 Scene 不再自行构造 ShopScene。
	unool::adapters::IShopOpener& shopOpener;
	const std::string titleBrackets;

	std::string username;
	std::string password;
	std::string message; // 状态/错误提示
	Result result;

	// 固定布局矩形
	sf::FloatRect usernameBox;
	sf::FloatRect passwordBox;
	sf::FloatRect loginBtn;
	sf::FloatRect registerBtn;
	sf::FloatRect startGameBtn; // 大厅：开始游戏按钮
	sf::FloatRect lobbyShopBtn; // 大厅：商城按钮
	sf::FloatRect checkinBtn; // 大厅：签到按钮（位于上述两按钮上方居中）

	// 签到结果弹窗
	bool checkinPopup = false;
	std::string checkinMsg;

	void layoutBoxes(); // 根据 windowSize 计算矩形

public:
	LoginScene(GameRenderer& r, unool::adapters::IClientGateway& n,
	           unool::adapters::IShopOpener& opener, const std::string& title);
	Result run(); // 跑登录循环，成功/失败/关窗后返回

private:
	void handleEvent(const sf::Event& event);
	void handleKeyPressed(const sf::Event::KeyPressed& key);
	void handleTextEntered(const sf::Event::TextEntered& text);
	void handleMouseClick(const sf::Vector2f& pos);
	void pollAccountPackets();
	void sendLoginRequest();
	void sendRegisterRequest();
	void sendCheckUsername();
	void sendCheckinRequest();
	void drawCheckinPopup();
	void render();
};
