#pragma once
#include <SFML/Graphics.hpp>
#include <chrono>
#include <string>
#include "adapters/IClientGateway.h"
#include "adapters/IShopOpener.h"

class GameRenderer;

// 登录场景：图形界面完成注册/登录，成功后返回账号信息
// 阶段 4 · S3b1（用户 51.11 口径①A）：成员与构造参数由具体 ClientNetwork& 切到
// adapters 层的 IClientGateway&，并删掉 #include "Socket.h" 的粗粒度传递依赖。
// 阶段 4 · S3b2（53.9 口径①A + ②A）：5 个请求构造与 4 处响应解析上移到 Protocol
// 层、结果经同步值对象返回；状态机 / render() / run() 主循环仍逐字留在 View。
// ③A：Result::enterShop 全项目零读取点，声明与赋值一并删除。
class LoginScene {
public:
	struct Result {
		std::string username;
		std::string password;
		int points = 0;
		int wins = 0;
		int losses = 0;
		bool ok = false;
	};

private:
	// Matching：点「开始游戏」后停留的匹配等待界面，可退出匹配返回大厅。
	enum class Mode { Login, Register, Lobby, Matching } mode = Mode::Login;
	// WaitingMatch：已发 ReadyToPlay、等对手；WaitingCancel：已发退出请求、等服务端仲裁。
	enum class Status { Idle, WaitingCheck, WaitingRegister, WaitingLogin, WaitingCheckin,
	                    WaitingMatch, WaitingCancel, Done } status = Status::Idle;
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
	sf::FloatRect cancelMatchBtn; // 匹配等待：退出匹配按钮

	// 匹配计时：等待秒数展示 + 退出请求 3 秒无响应时本地兜底返回大厅
	std::chrono::steady_clock::time_point matchStartedAt_{};
	std::chrono::steady_clock::time_point cancelRequestedAt_{};

	// 签到结果弹窗
	bool checkinPopup = false;
	std::string checkinMsg;

	// 「对手已开始匹配」提示：大厅显示常驻横幅；商城打开期间经 IShopOpener
	// 把同一状态的引用交给 ShopScene，退出商城后自动同步。
	unool::adapters::MatchNoticeState matchNotice_;

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
	void sendCancelMatchmaking();
	void backToLobby(); // 静默返回大厅，不显示退出匹配提示
	void clearMatchNotice();
	void drawCheckinPopup();
	void render();
};
