#pragma once
#include <SFML/Graphics.hpp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "shell/ConnectionState.h"
#include "shell/IShopOpener.h"
#include "shell/LoginViewModel.h"

class GameRenderer;

// 登录场景：图形界面完成注册/登录，登录成功后进入大厅。
//
// 房间功能（协议版本 3）：大厅不再是「开始游戏 = 匹配」，而是
//   大厅（房间列表 + 创建房间 + 加入选中房间 + 刷新 + 商城 + 每日签到）
//   → 房间（成员与准备状态 + 准备/取消准备 + 离开房间）
//   → 双方准备后服务端发 GameStart，本场景返回，ClientApp 进入对局。
//
// P1-3：成员与构造参数收敛到 ConnectionState&，发包经其语义方法、收包经显式入站队列。
// P1-4：Mode / Status / Focus 状态机与全部迁移规则在 unool::shell::LoginViewModel
// （纯标准库、可 headless 驱动）。本类只做三件事：
//   ① sf::Event / 鼠标坐标 → VM 语义方法（几何命中留在 layoutBoxes 算好的矩形上）；
//   ② VM 排出的 Actions → ConnectionState / IShopOpener；
//   ③ VM 状态 → 绘制。
class LoginScene {
public:
	using Result = unool::shell::LoginViewModel::Session;

private:
	GameRenderer& renderer;
	unool::shell::ConnectionState& net;
	unool::shell::IShopOpener& shopOpener;
	const std::string titleBrackets;

	unool::shell::LoginViewModel vm;

	// 发出 LoginRequest 的时刻（毫秒）。收到 LoginResponse 时打印往返耗时。
	std::int64_t loginSentMs_ = 0;

	// === 登录 / 注册界面 ===
	sf::FloatRect usernameBox;
	sf::FloatRect passwordBox;
	sf::FloatRect loginBtn;
	sf::FloatRect registerBtn;

	// === 主大厅（只有按钮：开始游戏 / 商城 / 每日签到） ===
	sf::FloatRect startGameBtn;    // 开始游戏 → 进入房间界面
	sf::FloatRect lobbyShopBtn;    // 商城
	sf::FloatRect checkinBtn;      // 每日签到

	// === 房间界面（点「开始游戏」后进入：创建 / 查看列表 / 加入） ===
	sf::FloatRect roomNameBox;     // 房间名输入框
	sf::FloatRect createRoomBtn;   // 创建房间
	sf::FloatRect joinRoomBtn;     // 加入选中房间
	sf::FloatRect refreshRoomsBtn; // 刷新列表
	sf::FloatRect backToLobbyBtn;  // 返回大厅
	sf::FloatRect roomListArea;    // 房间列表区域
	float roomRowHeight = 56.f;
	std::size_t visibleRoomRows = 8;

	// === 房间内 ===
	sf::FloatRect readyBtn;        // 准备 / 取消准备
	sf::FloatRect leaveRoomBtn;    // 离开房间

	void layoutBoxes(); // 根据 windowSize 计算矩形
	sf::FloatRect roomRowRect(std::size_t visibleIndex) const;
	std::size_t firstVisibleRoom() const;

public:
	// initialMessage：进入登录界面时直接展示的提示（例如被顶下线的原因）。
	// 只写 VM 的提示文案，不改状态机。
	LoginScene(GameRenderer& r, unool::shell::ConnectionState& n,
	           unool::shell::IShopOpener& opener, const std::string& title,
	           std::string initialMessage = {});
	Result run(); // 跑登录循环，成功/失败/关窗后返回

private:
	void handleEvent(const sf::Event& event);
	void handleKeyPressed(const sf::Event::KeyPressed& key);
	void handleTextEntered(const sf::Event::TextEntered& text);
	void handleMouseClick(const sf::Vector2f& pos);
	void handleLobbyClick(const sf::Vector2f& pos);
	void handleRoomBrowserClick(const sf::Vector2f& pos);
	void handleRoomClick(const sf::Vector2f& pos);
	void pollAccountPackets();
	void executeActions(); // VM 的 Actions → 网络 / 场景切换
	void printLogs();      // VM 记录的日志行（带 titleBrackets 前缀）
	void drawCheckinPopup();
	void drawLobby();
	void drawRoomBrowser();
	void drawRoom();
	void render();
};
