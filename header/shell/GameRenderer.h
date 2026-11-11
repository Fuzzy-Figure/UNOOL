#pragma once
#include <SFML/Graphics.hpp>
#include <array>
#include <vector>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "TextManager.h"
#include "ImageManager.h"
#include "GameState.h"
#include "utils.h"
#include "shell/JsonConfig.h"
#include "core/IDecisionPort.h"

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
			const auto& config = unool::shell::getClientConfig();
			const auto& size = config["size"];
			windowSize = { size["window"]["width"], size["window"]["height"] };
			cardSize = { size["card"]["width"], size["card"]["height"] };
			pointerSize = { size["pointer"]["width"], size["pointer"]["height"] };
			characterSize = { size["character"]["width"], size["character"]["height"] };
		}
	};
	//选项提示相关
	struct Choice {
		std::string title;
		std::vector<std::string> options;
		bool forced{};
		std::string errorMsg;
		std::optional<std::size_t> timeoutMs;
		std::size_t currentPage = 0;
		std::size_t totalPages = 1;
		//BP 阶段每个选项对应的角色展示信息（服务端下发）；空 = 普通选项
		std::vector<unool::core::ChoiceOptionMeta> optionMeta;
	};

private:
	std::unique_ptr<sf::RenderWindow> window;
	TextManager textMgr;
	ImageManager imageMgr;
	GameState currentState;
	Config config;
	std::optional<Choice> choice;
	std::optional<std::size_t> infoBoxPlayerId; // 当前显示信息框的角色id
	std::size_t localPlayerId = 0;

	//infoBox 缓存：仅当切换角色时重算
	struct InfoBoxCache {
		std::size_t playerId = static_cast<std::size_t>(-1);
		std::string text;
		float boxHeight = 0.f;
	};
	InfoBoxCache infoBoxCache;
	//角色信息缓存：仅在收到CharInfo包时更新
	std::array<std::string, 2> charInfoCache;
	sf::Clock countdownClock;

	//BP 预览：悬停的选项行、技能描述缓存与按需请求队列
	std::optional<std::size_t> bpHoverIndex;
	std::unordered_map<std::string, std::string> bpSkillsCache;
	std::unordered_set<std::string> bpPendingSkills;
	std::vector<std::string> bpSkillRequestQueue;
	struct ChoiceRow {
		std::size_t optionIndex = 0;
		sf::FloatRect bounds;
	};
	std::vector<ChoiceRow> choiceRows;
	void queueSkillRequests();

	//渲染子模块
	void renderPlayers();
	void renderDiscardPile();
	void renderChoice();
	void renderBpPreview();
	void renderInfoBox();
	//右上角：第X局 + 本地玩家局胜负
	void renderGameInfo();

public:
	GameRenderer(const Config& _config);
	~GameRenderer();

	void updateState(const GameState& state);
	void updatePointer(std::size_t playerId, std::size_t selectedIndex);
	void movePointerLeft(std::size_t playerId);
	void movePointerRight(std::size_t playerId);
	std::size_t getSelectedIndex(std::size_t playerId) const;
	//获取指定玩家的角色名（双将为 A&B）；无该玩家时返回空串
	std::string getCharacterName(std::size_t playerId) const;
	bool isChoiceActive() const;
	bool hasChoiceOptions() const;
	bool canSelectLocal() const; //本地玩家是否能选牌（自己是当前操作者且无选项拦截）
	void setLocalPlayerId(std::size_t id) { localPlayerId = id; }
	void updateCharInfo(std::size_t playerIndex, const std::string& fullText);
	void display();
	void handleMouseClick(const sf::Vector2f& mousePos);
	//BP 预览：鼠标悬停切换候选详情；技能描述走服务端 SkillInfoResponse
	void handleMouseMove(const sf::Vector2f& mousePos);
	void setSkillInfo(const std::string& name, const std::string& text);
	//取出本次需要向服务端请求的技能描述角色名（已去重、已标记 pending）
	std::vector<std::string> takePendingSkillNames();

	//显示文字
	void displayText(const std::string& text,
					 const sf::Vector2f& pos,
					 const sf::Vector2f& size = { 20,40 },
					 const sf::Color& color = sf::Color::Black);
	void displayTextInCenter(const std::string& text,
							 const sf::Vector2f& size = { 20,40 },
							 const sf::Color& color = sf::Color::Black);
	void displayTextInRight(const std::string& text,
							const sf::Vector2f& size = { 20,40 },
							const sf::Color& color = sf::Color::Black);
	void displayTextInUpRight(const std::string& text,
							  const sf::Vector2f& size = { 20,40 },
							  const sf::Color& color = sf::Color::Black);
	//以 pos.x 为右边缘锚点、pos.y 为顶部显示文字（可指定 y 实现多行右对齐）
	void displayTextTopRightCorner(const std::string& text,
								   const sf::Vector2f& pos,
								   const sf::Vector2f& size = { 20,40 },
								   const sf::Color& color = sf::Color::Black);
	void displayTextInLeft(const std::string& text,
						   const sf::Vector2f& size = { 20,40 },
						   const sf::Color& color = sf::Color::Black);

	//显示图片
	void displayImage(const std::string& path,
					  const sf::Vector2f& pos,
					  const sf::Vector2f& size);
	void displayImageInCenter(const std::string& path,
							  const sf::Vector2f& size);

	//显示卡牌
	void displayCard(const Card& card, const sf::Vector2f& pos, const sf::Vector2f& cardSize);
	void displayCardInCenter(const Card& card, const sf::Vector2f& cardSize);
	void displayHand(const Hand& hand, const sf::Vector2f& pos, const sf::Vector2f& cardSize,
					 const sf::Vector2f& pointerSize = { 0,0 }, const bool canSelect = false);

	bool windowIsOpen() const;
	void closeWindow();
	std::optional<sf::Event> pollEvent();

	//供登录等前置场景访问渲染资源
	sf::RenderWindow& getWindow() { return *window; }
	TextManager& getTextManager() { return textMgr; }
	const Config& getConfig() const { return config; }

	//选项提示相关
	void setChoicePrompt(const Choice& prompt);
	void clearChoicePrompt();
};
