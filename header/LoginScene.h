#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "Socket.h"

class GameRenderer;

// 登录场景：图形界面完成注册/登录，成功后返回账号信息
class LoginScene {
public:
	struct Result {
		std::string username;
		int points = 0;
		int wins = 0;
		int losses = 0;
		bool ok = false;
	};

private:
	enum class Mode { Login, Register } mode = Mode::Login;
	enum class Status { Idle, WaitingCheck, WaitingRegister, WaitingLogin, Done } status = Status::Idle;
	enum class Focus { None, Username, Password } focus = Focus::None;

	GameRenderer& renderer;
	ClientNetwork& net;
	const std::string titleBrackets;

	std::string username;
	std::string password;
	std::wstring message; // 状态/错误提示
	Result result;

	// 固定布局矩形
	sf::FloatRect usernameBox;
	sf::FloatRect passwordBox;
	sf::FloatRect loginBtn;
	sf::FloatRect registerBtn;

	void layoutBoxes(); // 根据 windowSize 计算矩形

public:
	LoginScene(GameRenderer& r, ClientNetwork& n, const std::string& title);
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
	void render();
};
