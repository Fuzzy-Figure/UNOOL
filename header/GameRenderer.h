#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <optional>
#include <string>
#include "TextManager.h"
#include "ImageManager.h"
#include "GameState.h"
#include "utils.h"

//游戏渲染器
class GameRenderer {
public:
	struct Config {
		sf::Vector2u windowSize = { 2000, 1200 };
		sf::Vector2f cardSize = { 180, 270 };
		sf::Vector2f pointerSize = { 90, 135 };
		sf::Vector2f characterSize = { 315, 405 };
		std::string windowName;
		Config(const std::string& _windowName) :windowName(_windowName) {
			const auto& config = unool::getConfig();
			const auto& size = config["size"];
			windowSize = { size["window"]["width"], size["window"]["height"] };
			cardSize = { size["card"]["width"], size["card"]["height"] };
			pointerSize = { size["pointer"]["width"], size["pointer"]["height"] };
			characterSize = { size["character"]["width"], size["character"]["height"] };
		}
	};
	//选项提示相关
	struct Choice {
		std::wstring title;
		std::vector<std::wstring> options;
		bool forced;
		std::wstring errorMsg;
		std::optional<std::size_t> timeoutMs;
	};

private:
	std::unique_ptr<sf::RenderWindow> window;
	TextManager textMgr;
	ImageManager imageMgr;
	GameState currentState;
	Config config;
	std::optional<Choice> choice;
	std::optional<std::size_t> infoBoxPlayerId; // 当前显示信息框的角色id

	//infoBox 缓存：仅当切换角色或 hp 变化时重算
	struct InfoBoxCache {
		std::size_t playerId = static_cast<std::size_t>(-1);
		std::size_t hp = 0;
		std::wstring text;
		float boxHeight = 0.f;
	};
	InfoBoxCache infoBoxCache;
	sf::Clock countdownClock;

	//渲染子模块
	void renderPlayers();
	void renderDiscardPile();
	void renderChoice();
	void renderInfoBox();

public:
	GameRenderer(const Config& _config);
	~GameRenderer();

	void updateState(const GameState& state);
	void display();
	void handleMouseClick(const sf::Vector2f& mousePos);

	//显示文字
	void displayText(const std::wstring& text,
					 const sf::Vector2f& pos,
					 const sf::Vector2f& size = { 20,40 },
					 const sf::Color& color = sf::Color::Black);
	void displayTextInCenter(const std::wstring& text,
							 const sf::Vector2f& size = { 20,40 },
							 const sf::Color& color = sf::Color::Black);
	void displayTextInRight(const std::wstring& text,
							const sf::Vector2f& size = { 20,40 },
							const sf::Color& color = sf::Color::Black);
	void displayTextInUpRight(const std::wstring& text,
							  const sf::Vector2f& size = { 20,40 },
							  const sf::Color& color = sf::Color::Black);
	void displayTextInLeft(const std::wstring& text,
						   const sf::Vector2f& size = { 20,40 },
						   const sf::Color& color = sf::Color::Black);

	//显示图片
	void displayImage(const std::string& path,
					  const sf::Vector2f& pos,
					  const sf::Vector2f& size);
	void displayImageInCenter(const std::string& path,
							  const sf::Vector2f& size);

	bool windowIsOpen() const;
	void closeWindow();
	std::optional<sf::Event> pollEvent();

	//选项提示相关
	void setChoicePrompt(const Choice& prompt);
	void clearChoicePrompt();
};
