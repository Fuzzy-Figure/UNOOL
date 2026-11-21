#pragma once
#include <SFML/Graphics.hpp>
#include <cstddef>
#include <string>
#include <vector>
#include "shell/ConnectionState.h"
#include "shell/IShopOpener.h"
#include "shell/ShopPresentationCache.h"
#include "shell/ShopViewModel.h"
#include "ShopData.h"
#include "contract/CharacterTypes.h"

class GameRenderer;

// 商城场景：图形界面浏览/购买角色、皮肤、道具。
// Arch-C P1-4：全部界面状态与导航/购买规则在 unool::shell::ShopViewModel 里，
// 本类只剩「事件翻译 + 命中判定 + 绘制」，因此 ViewModel 可以在 Tests 里
// headless 驱动。run() 返回时表示用户退出商城。
class ShopScene {
public:
	ShopScene(GameRenderer& r, unool::shell::ConnectionState& n, const std::string& username,
	          unool::shell::MatchNoticeState& notice,
	          unool::shell::ShopPresentationCache& cache);
	// 跑商城主循环，ESC 或关窗后返回
	void run();

private:
	GameRenderer& renderer;
	unool::shell::ConnectionState& net;
	const std::string username;
	// 「对手已开始匹配」提示：引用 LoginScene 持有的同一份状态，商城期间收到的
	// 匹配事件就地写回，退出商城后大厅无需额外同步即可显示。
	unool::shell::MatchNoticeState& matchNotice;

	// 状态机、导航与购买规则（无 SFML / 无网络）
	unool::shell::ShopViewModel vm;
	// 提示消息 2.5 秒淡出的时间源（时间由外部注入 ViewModel，VM 不自持时钟）
	sf::Clock clock;

	// 把 VM 排队的动作翻译成协议发送
	void executeActions();
	// 处理网络包（解析协议后喂给 VM）
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
	// 状态→文字颜色：已解锁绿；未解锁可购买黄；未解锁积分不足红；不可购买(nullopt)灰
	static sf::Color stateColor(bool unlocked, unool::contract::price_t price, bool canAfford);
	// 渲染工具
	void drawRowBox(const sf::Vector2f& pos, const sf::Vector2f& size, bool selected) const;
};
