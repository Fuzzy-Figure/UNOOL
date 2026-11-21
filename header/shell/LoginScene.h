#pragma once
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <string>
#include "shell/ConnectionState.h"
#include "shell/IShopOpener.h"
#include "shell/LoginViewModel.h"

class GameRenderer;

// 登录场景：图形界面完成注册/登录，成功后返回账号信息。
//
// P1-3：成员与构造参数收敛到 ConnectionState&，发包经其语义方法、收包经显式入站队列，
// 本 View 不含网络客户端 / SFML Network / json 符号。
// P1-4：Mode / Status / Focus 三台状态机与全部迁移规则搬到 unool::shell::LoginViewModel
// （纯标准库、可 headless 驱动）。本类退化成三件事：
//   ① sf::Event / 鼠标坐标 → VM 语义方法（几何命中留在 layoutBoxes 算好的矩形上）；
//   ② VM 排出的 Actions → ConnectionState / IShopOpener；
//   ③ VM 状态 → 绘制。
class LoginScene {
public:
	using Result = unool::shell::LoginViewModel::Session;

private:
	GameRenderer& renderer;
	// P1-3：由单一连接状态机（显式入站队列）取代直接的 IClientGateway 收包，
	// 发包/会话方法由 ConnectionState 转发，签名与调用点保持一致。
	unool::shell::ConnectionState& net;
	// S3a：商城由组合根经意图端口打开，本 Scene 不再自行构造 ShopScene。
	unool::shell::IShopOpener& shopOpener;
	const std::string titleBrackets;

	// P1-4：状态机与规则在这里；本 View 不持有任何状态迁移知识。
	unool::shell::LoginViewModel vm;

	// 发出 LoginRequest 的时刻（毫秒）。收到 LoginResponse 时打印往返耗时，
	// 用于区分「服务端口令派生慢」与「网络 / 帧循环慢」，0 表示没有在途请求。
	std::int64_t loginSentMs_ = 0;

	// 固定布局矩形
	sf::FloatRect usernameBox;
	sf::FloatRect passwordBox;
	sf::FloatRect loginBtn;
	sf::FloatRect registerBtn;
	sf::FloatRect startGameBtn; // 大厅：开始游戏按钮
	sf::FloatRect lobbyShopBtn; // 大厅：商城按钮
	sf::FloatRect checkinBtn; // 大厅：签到按钮（位于上述两按钮上方居中）
	sf::FloatRect cancelMatchBtn; // 匹配等待：退出匹配按钮

	void layoutBoxes(); // 根据 windowSize 计算矩形

public:
	LoginScene(GameRenderer& r, unool::shell::ConnectionState& n,
	           unool::shell::IShopOpener& opener, const std::string& title);
	Result run(); // 跑登录循环，成功/失败/关窗后返回

private:
	void handleEvent(const sf::Event& event);
	void handleKeyPressed(const sf::Event::KeyPressed& key);
	void handleTextEntered(const sf::Event::TextEntered& text);
	void handleMouseClick(const sf::Vector2f& pos);
	void pollAccountPackets();
	void executeActions(); // VM 的 Actions → 网络 / 场景切换
	void printLogs();      // VM 记录的日志行（带 titleBrackets 前缀）
	void drawCheckinPopup();
	void render();
};
