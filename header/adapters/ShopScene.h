#pragma once
#include <SFML/Graphics.hpp>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "adapters/IClientGateway.h"
#include "adapters/IShopOpener.h"
#include "ShopData.h"

namespace unool::adapters {
	// 道具展示元数据（服务端随 ShopData 下发）：客户端缓存的只是这两项展示信息，
	// 数量 / 售价 / 可购买状态始终以每次下发的 ShopData 为准，不进缓存。
	struct ShopItemPresentation {
		std::string description;
		std::optional<int> maxCount;
	};
}

class GameRenderer;

// 商城场景：图形界面浏览/购买角色、皮肤、道具。
// 参考 LoginScene 的 SFML 场景模式，run() 返回时表示用户退出商城。
class ShopScene {
public:
	ShopScene(GameRenderer& r, unool::adapters::IClientGateway& n, const std::string& username,
	          unool::adapters::MatchNoticeState& notice);
	// 跑商城主循环，ESC 或关窗后返回
	void run();

private:
	GameRenderer& renderer;
	unool::adapters::IClientGateway& net;
	const std::string username;
	// 「对手已开始匹配」提示：引用 LoginScene 持有的同一份状态，商城期间收到的
	// 匹配事件就地写回，退出商城后大厅无需额外同步即可显示。
	unool::adapters::MatchNoticeState& matchNotice;

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
		enum class Type {
			HeroCollapsed, HeroEntry, SkinEntry, ItemEntry
		} type = Type::HeroCollapsed;
		std::size_t heroIdx = 0;   // 在 shopData.characters 中的下标
		std::size_t skinIdx = 0;   // SkinEntry 时为 hero.skins 中的下标
		std::size_t itemIdx = 0;   // ItemEntry 时为 shopData.items 中的下标
	};
	std::vector<Row> rows;

	// 请求商城数据
	void requestShopData();
	// 请求角色技能描述（缓存缺失时调用；pendingSkills 防止重复请求）
	void requestSkillInfo(const std::vector<std::string>& names);
	// 技能描述缓存：进程级静态数据，商城退出后仍保留
	static std::unordered_map<std::string, std::string> skillsCache;
	std::unordered_set<std::string> pendingSkills;
	// 道具展示元数据缓存：服务端随 ShopData 下发的效果说明 / 购买上限，进程级保留，
	// 退出商城后仍在；每次收到新版 ShopData 会刷新，不会像技能文案那样永不失效。
	static std::unordered_map<std::string, unool::adapters::ShopItemPresentation> itemPresentationCache;
	// 处理网络包（ShopData / ShopResult / SkillInfoResponse / 匹配提示事件）
	void handleNetwork();
	// 渲染
	void render();
	void renderTopBar();
	// 商城内的「对手已开始匹配」横幅（深色界面用浅色文字）
	void renderMatchNotice();
	void renderHeroView();
	void renderItemView();
	void renderSearchBar();
	void renderBuyButton();
	void renderDetailPanel();
	// 道具视图：右侧显示当前选中道具的名称、数量/售价与效果说明
	void renderItemDetailPanel();
	// 购买按钮矩形（渲染与点击检测共用；英雄视图在详情面板右上角，道具视图在列表右侧）
	sf::FloatRect buyButtonRect() const;
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
	bool affordable(Character::price_t price) const;
	// 状态→文字颜色：已解锁绿；未解锁可购买黄；未解锁积分不足红；不可购买(nullopt)灰
	static sf::Color stateColor(bool unlocked, Character::price_t price, bool canAfford);
	// 渲染工具
	void drawRowBox(const sf::Vector2f& pos, const sf::Vector2f& size, bool selected) const;
};
