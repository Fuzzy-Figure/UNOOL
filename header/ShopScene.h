#pragma once
#include <SFML/Graphics.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include "Socket.h"
#include "ShopData.h"

class GameRenderer;

// 商城场景：图形界面浏览/购买角色、皮肤、道具。
// 参考 LoginScene 的 SFML 场景模式，run() 返回时表示用户退出商城。
class ShopScene {
public:
	ShopScene(GameRenderer& r, ClientNetwork& n, const std::string& username);
	// 跑商城主循环，ESC 或关窗后返回
	void run();

private:
	GameRenderer& renderer;
	ClientNetwork& net;
	const std::string username;

	// 商城数据
	ShopData shopData;
	bool dataReady = false;
	std::string purchaseMsg;   // 购买结果消息
	sf::Clock msgTimer;        // 消息显示计时器
	bool msgActive = false;
	bool exiting = false;      // 用户请求退出商城（ESC），不关闭窗口

	// 界面状态
	enum class View { Hero, Item } currentView = View::Hero;
	enum class Mode { Browse, Search } mode = Mode::Browse;

	// 分页与导航
	static constexpr std::size_t kPerPage = 10;
	std::size_t pageIdx = 0;             // 当前页码
	std::size_t cursor = 0;              // 当前选中行（在 rows 中）
	std::size_t expandedHeroIdx = static_cast<std::size_t>(-1); // 展开皮肤列表的英雄索引（在 displayedIndices 中），-1 表示无展开
	std::string searchKeyword;           // 搜索关键词（UTF-8）
	std::vector<std::size_t> displayedIndices; // 当前可显示的英雄在 shopData.characters 中的下标

	// 扁平行表（渲染与导航用）
	struct Row {
		enum class Type { HeroCollapsed, HeroEntry, SkinEntry, ItemEntry } type;
		std::size_t heroIdx = 0;   // 在 shopData.characters 中的下标
		std::size_t skinIdx = 0;   // SkinEntry 时为 hero.skins 中的下标
		std::size_t itemIdx = 0;   // ItemEntry 时为 shopData.items 中的下标
	};
	std::vector<Row> rows;

	// 请求商城数据
	void requestShopData();
	// 处理网络包（ShopData / ShopResult）
	void handleNetwork();
	// 渲染
	void render();
	void renderTopBar();
	void renderHeroView();
	void renderItemView();
	void renderSearchBar();
	void renderBuyButton();
	// 事件处理
	void handleEvent(const sf::Event& event);
	void handleKeyPressed(const sf::Event::KeyPressed& key);
	void handleTextEntered(const sf::Event::TextEntered& te);
	void handleMouseClick(const sf::Vector2f& pos);
	// 购买
	void tryBuyCharacter(const std::string& name);
	void tryBuySkin(const std::string& charName, const std::string& skinName);
	void tryBuyItem(const std::string& itemName);
	// 对当前选中行执行 Enter 语义（仅展开/收起英雄，不购买）
	void activateCurrentRow();
	// 通过"购买"按钮/空格触发：购买当前选中项
	void tryBuyCurrentSelection();
	// 当前选中行是否可购买
	bool currentRowBuyable() const;
	// 重建扁平行表
	void rebuildDisplayedIndices();
	void rebuildRows();
	// 辅助
	std::size_t totalPages() const;
	void clampPage();
	void clampCursor();
	void setPurchaseMsg(const std::string& msg);
	bool affordable(int price) const;
	// 渲染工具
	void drawRowBox(const sf::Vector2f& pos, const sf::Vector2f& size, bool selected) const;
};
