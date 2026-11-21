#include "ShopScene.h"
#include "GameRenderer.h"
#include "Character.h"
#include <algorithm>
#include <format>
#include <string>
#include <utility>
#include "ShopProtocol.h"
#include "ShopData.h"
#include "AccountProtocol.h"

// Arch-C P1-4：展示缓存由组合根持有（进程级 static 已移除），界面状态与
// 导航 / 购买规则在 ShopViewModel 里；本单元只做事件翻译与绘制。

// MessageType 归 protocol 层；本单元类在全局命名空间，这里做 TU 局部的名字转发。
using unool::protocol::MessageType;
using ViewModel = unool::shell::ShopViewModel;

ShopScene::ShopScene(GameRenderer& r, unool::shell::ConnectionState& n, const std::string& uname,
					 unool::shell::MatchNoticeState& notice,
					 unool::shell::ShopPresentationCache& cache)
	: renderer(r), net(n), username(uname), matchNotice(notice), vm(cache) {}

void ShopScene::run() {
	// 进入商城即请求一次商城数据（动作由 VM 排队，这里立即执行）
	vm.requestShopData();
	executeActions();
	while (renderer.windowIsOpen() && !vm.exiting()) {
		for (auto ev = renderer.pollEvent(); ev; ev = renderer.pollEvent()) {
			handleEvent(*ev);
		}
		if (vm.exiting()) break;
		net.pump();
		if (!net.isConnected()) break;
		handleNetwork();
		executeActions();
		vm.setNow(static_cast<std::int64_t>(clock.getElapsedTime().asMilliseconds()));
		render();
	}
}

// ===== 出站动作 =====

void ShopScene::executeActions() {
	for (const auto& action : vm.takeActions()) {
		switch (action.type) {
			case ViewModel::ActionType::RequestShopData: net.sendShopRequest(); break;
			case ViewModel::ActionType::RequestSkillInfo: net.sendSkillInfoRequest(action.names); break;
			case ViewModel::ActionType::BuyCharacter: net.sendBuyCharacter(action.name); break;
			case ViewModel::ActionType::BuySkin: net.sendBuySkin(action.name, action.skin); break;
			case ViewModel::ActionType::BuyItem: net.sendBuyItem(action.name); break;
		}
	}
}

// ===== 网络入站 =====

void ShopScene::handleNetwork() {
	// P1-3：DrainAll 模式，消费队列所有包；坏包丢弃与心跳回复收拢到 ConnectionState。
	while (auto next = net.takeAny()) {
		auto [mt, packet] = std::move(*next);

		// 匹配提示：商城阶段是 DrainAll，若此处不接管这两个事件，大厅将永远
		// 收不到（「非订阅包留队」只对 LoginScene 的 Filtered 模式生效）。
		// 写回的是 LoginScene 持有的同一份状态，退出商城后大厅横幅自动同步。
		// 退出匹配不产生文字提示，只把横幅清掉。
		if (mt == MessageType::MatchmakingStarted) {
			if (auto who = unool::protocol::parseMatchmakingStarted(packet);
				who && !who->empty() && *who != username) {
				matchNotice.opponentMatching = true;
				matchNotice.opponentName = *who;
			}
			continue;
		}
		if (mt == MessageType::MatchmakingCancelled) {
			matchNotice.opponentMatching = false;
			matchNotice.opponentName.clear();
			continue;
		}

		if (mt == MessageType::ShopData) {
			if (auto parsed = unool::protocol::parseShopData(packet)) {
				vm.onShopData(std::move(*parsed));
			}
			else {
				vm.onShopDataParseFailed();
			}
			continue;
		}
		if (mt == MessageType::SkillInfoResponse) {
			if (auto parsed = unool::protocol::parseSkillInfoResponse(packet)) {
				std::vector<std::pair<std::string, std::string>> skills;
				skills.reserve(parsed->skills.size());
				for (auto& [name, text] : parsed->skills) {
					skills.emplace_back(name, std::move(text));
				}
				vm.onSkillInfoResponse(skills);
			}
			// 解析失败：静默吞掉（与原 catch(const std::exception&) {} 一致）
			continue;
		}
		if (mt == MessageType::ShopResult) {
			auto parsed = unool::protocol::parseShopPurchaseResult(packet);
			if (!parsed) {
				vm.onPurchaseResult(false, "购买结果解析失败");
				continue;
			}
			vm.onPurchaseResult(parsed->ok, parsed->msg);
			continue;
		}
		// 心跳包已由 ConnectionState::pump 统一消费并回复
		// 其它包忽略（商城场景不处理游戏/账号包）
	}
}

// ===== 事件处理 =====

void ShopScene::handleEvent(const sf::Event& event) {
	if (event.is<sf::Event::Closed>()) {
		renderer.closeWindow();
		return;
	}
	if (event.is<sf::Event::KeyPressed>()) {
		if (auto kp = event.getIf<sf::Event::KeyPressed>()) handleKeyPressed(*kp);
		return;
	}
	if (vm.searching() && event.is<sf::Event::TextEntered>()) {
		if (auto te = event.getIf<sf::Event::TextEntered>()) handleTextEntered(*te);
		return;
	}
	if (event.is<sf::Event::MouseButtonPressed>()) {
		if (auto mb = event.getIf<sf::Event::MouseButtonPressed>(); mb && mb->button == sf::Mouse::Button::Left) {
			handleMouseClick(static_cast<sf::Vector2f>(mb->position));
		}
		return;
	}
}

void ShopScene::handleKeyPressed(const sf::Event::KeyPressed& key) {
	const auto sc = key.scancode;

	// 搜索模式下：ESC 退出搜索，其它交给文本输入与导航
	if (vm.searching()) {
		if (sc == sf::Keyboard::Scancode::Escape) { vm.leaveSearch(); return; }
		if (sc == sf::Keyboard::Scancode::Backspace) { vm.backspace(); return; }
		// 搜索模式下仅用方向键导航（W/A/S/D 留给文本输入）
		if (sc == sf::Keyboard::Scancode::Up) { vm.moveUp(); return; }
		if (sc == sf::Keyboard::Scancode::Down) { vm.moveDown(); return; }
		if (sc == sf::Keyboard::Scancode::Enter) { vm.activateCurrentRow(); return; }
		return;
	}

	// 浏览模式
	switch (sc) {
		case sf::Keyboard::Scancode::Escape:
			// 退出商城，返回登录界面（不关闭窗口）
			vm.requestExit();
			return;
		case sf::Keyboard::Scancode::E: vm.showItems(); return;
		case sf::Keyboard::Scancode::H: vm.showHeroes(); return;
		case sf::Keyboard::Scancode::Up:
		case sf::Keyboard::Scancode::W: vm.moveUp(); return;
		case sf::Keyboard::Scancode::Down:
		case sf::Keyboard::Scancode::S: vm.moveDown(); return;
		case sf::Keyboard::Scancode::Left:
		case sf::Keyboard::Scancode::A: vm.previousPage(); return;
		case sf::Keyboard::Scancode::Right:
		case sf::Keyboard::Scancode::D: vm.nextPage(); return;
		case sf::Keyboard::Scancode::Enter: vm.activateCurrentRow(); return;
		default: return;
	}
}

void ShopScene::handleTextEntered(const sf::Event::TextEntered& te) {
	vm.typeChar(te.unicode);
}

void ShopScene::handleMouseClick(const sf::Vector2f& pos) {
	const auto& cfg = renderer.getConfig();
	const float listX = static_cast<float>(cfg.windowSize.x) * 0.06f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	const float rowH = 56.f;
	const float listStartY = 140.f;

	// 英雄视图下：点击搜索框进入搜索状态（保留现有关键词继续编辑）
	if (vm.view() == ViewModel::View::Hero) {
		const float barW = static_cast<float>(cfg.windowSize.x) * 0.5f;
		const float barH = 42.f;
		const float barX = (static_cast<float>(cfg.windowSize.x) - barW) / 2.f;
		const float barY = 72.f;
		if (pos.x >= barX && pos.x <= barX + barW && pos.y >= barY && pos.y <= barY + barH) {
			vm.enterSearch();
			return;
		}
	}

	// 先检查"购买"按钮
	if (buyButtonRect().contains(pos)) {
		vm.buyCurrent();
		return;
	}

	// 鼠标点击行：选中并触发展开/收起（不购买）
	if (pos.x < listX || pos.x > listX + listW) return;
	const auto& rows = vm.rows();
	for (std::size_t i = 0; i < rows.size(); ++i) {
		const float y = listStartY + static_cast<float>(i) * rowH;
		if (pos.y >= y && pos.y <= y + rowH) {
			vm.clickRow(i);
			return;
		}
	}
}

sf::Color ShopScene::stateColor(bool unlocked, unool::contract::price_t price, bool canAfford) {
	if (unlocked) return sf::Color::Green;
	if (!price.has_value()) return sf::Color(170, 170, 170);
	return canAfford ? sf::Color::Yellow : sf::Color::Red;
}

// ===== 渲染 =====

void ShopScene::drawRowBox(const sf::Vector2f& pos, const sf::Vector2f& size, bool selected) const {
	auto& window = renderer.getWindow();
	sf::RectangleShape shape(size);
	shape.setPosition(pos);
	shape.setFillColor(selected ? sf::Color(70, 80, 120) : sf::Color(44, 48, 62));
	shape.setOutlineThickness(selected ? 3.f : 1.5f);
	shape.setOutlineColor(selected ? sf::Color(140, 170, 230) : sf::Color(90, 100, 130));
	window.draw(shape);
}

void ShopScene::render() {
	auto& window = renderer.getWindow();
	window.clear(sf::Color(24, 26, 36));

	renderTopBar();

	// 英雄视图下始终显示搜索框（点击进入搜索状态）
	if (vm.view() == ViewModel::View::Hero) renderSearchBar();

	if (vm.view() == ViewModel::View::Hero) renderHeroView();
	else renderItemView();

	if (vm.view() == ViewModel::View::Hero) renderDetailPanel();
	else renderItemDetailPanel();

	renderBuyButton();

	// 匹配提示横幅放在最后绘制：详情面板 / 行表从 y=140 一直铺到屏幕下部，
	// 只有后置绘制才能保证横幅完整可见、不被面板覆盖。
	renderMatchNotice();

	// 消息提示（底部居中）
	if (vm.messageVisible()) {
		auto& textMgr = renderer.getTextManager();
		const auto& cfg = renderer.getConfig();
		sf::Vector2f msgSize = { 22, 44 };
		sf::Vector2f measured = textMgr.measureText(vm.purchaseMessage(), static_cast<unsigned int>(msgSize.y));
		float x = (static_cast<float>(cfg.windowSize.x) - measured.x) / 2.f;
		float y = static_cast<float>(cfg.windowSize.y) - 80.f;
		sf::Color col = (vm.purchaseMessage() == "购买成功") ? sf::Color(120, 230, 140) : sf::Color(250, 120, 120);
		textMgr.displayText(vm.purchaseMessage(), { x, y }, msgSize, col);
	}

	// 底部操作提示
	{
		auto& textMgr = renderer.getTextManager();
		const auto& cfg = renderer.getConfig();
		std::string hint;
		if (vm.searching()) {
			hint = "输入关键词筛选 | Backspace 删除 | ESC 退出搜索";
		}
		else if (vm.view() == ViewModel::View::Hero) {
			hint = "W/S 导航 | Enter 展开 | A/D 翻页 | 点击搜索框搜索 | E 道具 | ESC 退出";
		}
		else {
			hint = "W/S 导航 | H 英雄 | 右侧查看道具说明 | ESC 退出";
		}
		sf::Vector2f hintSize = { 16, 32 };
		sf::Vector2f measured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
		float x = (static_cast<float>(cfg.windowSize.x) - measured.x) / 2.f;
		float y = static_cast<float>(cfg.windowSize.y) - 36.f;
		textMgr.displayText(hint, { x, y }, hintSize, sf::Color(150, 150, 160));
	}

	window.display();
}

void ShopScene::renderMatchNotice() {
	if (!matchNotice.opponentMatching) return;
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	const std::string text = std::format("对手「{}」已开始匹配，请退出商城并点击「开始游戏」加入",
										 matchNotice.opponentName);
	const sf::Vector2f textSize = { 20, 40 };
	const auto measured = textMgr.measureText(text, static_cast<unsigned int>(textSize.y));
	const float panelW = measured.x + 80.f > 900.f ? measured.x + 80.f : 900.f;
	const float panelH = 62.f;
	const float panelX = (static_cast<float>(cfg.windowSize.x) - panelW) / 2.f;
	const float panelY = (static_cast<float>(cfg.windowSize.y)) - 190.f;

	sf::RectangleShape panel({ panelW, panelH });
	panel.setPosition({ panelX, panelY });
	panel.setFillColor(sf::Color(78, 66, 32));
	panel.setOutlineThickness(2.f);
	panel.setOutlineColor(sf::Color(255, 205, 90));
	window.draw(panel);

	textMgr.displayText(text,
						{ panelX + (panelW - measured.x) / 2.f,
						  panelY + (panelH - measured.y) / 2.f },
						textSize, sf::Color(255, 228, 150));
}

void ShopScene::renderTopBar() {
	auto& textMgr = renderer.getTextManager();
	// 左上角操作提示
	std::string leftHint = "E/H切换界面";
	if (vm.searching()) leftHint = "搜索模式（ESC 退出）";
	textMgr.displayText(leftHint, { 20.f, 20.f }, { 18, 36 }, sf::Color(200, 200, 210));
	// 右上角积分
	std::string pts = std::format("积分: {}", vm.shopData().points);
	textMgr.displayTextInUpRight(pts, { 22, 44 }, sf::Color(255, 215, 80));
	// 视图标识（顶部居中）
	std::string title = (vm.view() == ViewModel::View::Hero) ? "英雄商城" : "道具商城";
	if (vm.searching()) title = "英雄商城 - 搜索";
	textMgr.displayTextInUp(title, { 24, 48 }, sf::Color(230, 230, 240));
}

void ShopScene::renderSearchBar() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();
	const bool active = vm.searching();
	const float barW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	const float barH = 42.f;
	const float barX = (static_cast<float>(cfg.windowSize.x) - barW) / 2.f;
	const float barY = 72.f;
	sf::RectangleShape bar({ barW, barH });
	bar.setPosition({ barX, barY });
	bar.setFillColor(sf::Color(30, 34, 48));
	bar.setOutlineThickness(2.f);
	// 激活时高亮边框，非激活灰暗
	bar.setOutlineColor(active ? sf::Color(140, 170, 230) : sf::Color(80, 90, 120));
	window.draw(bar);
	if (vm.keyword().empty() && !active) {
		// 占位提示
		textMgr.displayText("点击搜索...", { barX + 12.f, barY + 4.f }, { 20, 40 }, sf::Color(120, 124, 140));
		return;
	}
	std::string label = "搜索: " + vm.keyword();
	// 光标闪烁（仅激活时）
	static sf::Clock blink;
	if (active && blink.getElapsedTime().asMilliseconds() % 1000 < 500) label += "_";
	textMgr.displayText(label, { barX + 12.f, barY + 4.f }, { 20, 40 }, sf::Color(235, 235, 245));
}

void ShopScene::renderHeroView() {
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	if (!vm.dataReady()) {
		textMgr.displayTextInCenter("正在加载商城数据...", { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}
	if (vm.displayedIndices().empty()) {
		std::string msg = vm.searching() ? "未找到匹配的角色" : "暂无可浏览的英雄";
		textMgr.displayTextInCenter(msg, { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}

	const float listX = static_cast<float>(cfg.windowSize.x) * 0.06f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	const float rowH = 56.f;
	const float startY = 140.f;

	const auto& rows = vm.rows();
	const auto& data = vm.shopData();
	for (std::size_t i = 0; i < rows.size(); ++i) {
		const auto& row = rows[i];
		const float y = startY + static_cast<float>(i) * rowH;
		const bool selected = (i == vm.cursor());
		sf::Vector2f boxPos = { listX, y };
		sf::Vector2f boxSize = { listW, rowH - 6.f };

		if (row.type == ViewModel::Row::Type::HeroCollapsed) {
			const auto& character = data.characters[row.heroIdx];
			drawRowBox(boxPos, boxSize, selected);
			// 角色名 + 等级 + 解锁状态 + 售价（整行按状态着色）
			const sf::Color stateCol = stateColor(character.unlocked, character.price, vm.affordable(character.price));
			const std::string line = std::format("{}（{}）", character.name, character.level);
			std::string status;
			if (character.unlocked) {
				status = "已解锁";
			}
			else if (!character.price.has_value()) {
				status = "不可购买";
			}
			else {
				status = std::format("售价 {}", *character.price);
			}
			textMgr.displayText(line, { boxPos.x + 16.f, y + 6.f }, { 22, 44 }, stateCol);
			textMgr.displayTextTopRightCorner(status, { boxPos.x + listW - 50.f, y + 6.f }, { 22, 44 }, stateCol);
		}
		else if (row.type == ViewModel::Row::Type::SkinEntry) {
			const auto& hero = data.characters[row.heroIdx];
			const auto& skin = hero.skins[row.skinIdx];
			const sf::Vector2f subPos = { listX + 80.f, y };
			const sf::Vector2f subSize = { listW - 80.f, rowH - 6.f };
			drawRowBox(subPos, subSize, selected);
			// 皮肤名 + 品质 + 解锁状态/售价（整行按状态着色）
			const sf::Color stateCol = stateColor(skin.unlocked, skin.price, vm.affordable(skin.price));
			std::string status;
			if (skin.unlocked) {
				status = "已解锁";
			}
			else if (!skin.price.has_value()) {
				status = "不售卖";
			}
			else {
				status = std::format("售价 {}", *skin.price);
			}
			textMgr.displayText(skin.name, { subPos.x + 16.f, y + 6.f }, { 20, 40 }, stateCol);
			textMgr.displayText(status, { subPos.x + subSize.x - 160.f, y + 6.f }, { 18, 36 }, stateCol);
		}
	}

	// 翻页指示（屏幕底部居中，避免被角色介绍面板遮挡）
	const std::size_t pages = vm.totalPages();
	if (pages > 0) {
		std::string pageStr = std::format("第 {}/{} 页", vm.page() + 1, pages);
		const sf::Vector2f measured = textMgr.measureText(pageStr, 36);
		textMgr.displayText(
			pageStr,
			{ (static_cast<float>(cfg.windowSize.x) - measured.x) / 2.f,
			  static_cast<float>(cfg.windowSize.y) - 100.f },
			{ 18, 36 }, sf::Color(180, 180, 200)
		);
	}
}

void ShopScene::renderItemView() {
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	if (!vm.dataReady()) {
		textMgr.displayTextInCenter("正在加载商城数据...", { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}

	const float listX = static_cast<float>(cfg.windowSize.x) * 0.06f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	const float rowH = 56.f;
	const float startY = 140.f;

	const auto& rows = vm.rows();
	const auto& data = vm.shopData();
	if (rows.empty()) {
		textMgr.displayTextInCenter("暂无可购买的道具", { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}

	for (std::size_t i = 0; i < rows.size(); ++i) {
		const auto& row = rows[i];
		const auto& item = data.items[row.itemIdx];
		const float y = startY + static_cast<float>(i) * rowH;
		const bool selected = (i == vm.cursor());
		sf::Vector2f boxPos = { listX, y };
		sf::Vector2f boxSize = { listW, rowH - 6.f };
		drawRowBox(boxPos, boxSize, selected);

		// 道具名 + 数量 + 售价（整行按是否买得起着色）
		const sf::Color stateCol = stateColor(false, item.price, vm.affordable(item.price));
		const std::string line = std::format("{}  (当前数量：{})", item.name, item.count);
		textMgr.displayText(line, { boxPos.x + 16.f, y + 6.f }, { 22, 44 }, stateCol);

		const std::string priceStr = std::format("售价 {}", item.price.value_or(0));
		textMgr.displayText(priceStr, { boxPos.x + listW - 170.f, y + 6.f }, { 20, 40 }, stateCol);
	}
}

sf::FloatRect ShopScene::buyButtonRect() const {
	const auto& cfg = renderer.getConfig();
	constexpr float btnW = 140.f;
	constexpr float btnH = 44.f;
	const float listX = static_cast<float>(cfg.windowSize.x) * 0.06f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	// 两个视图的详情面板尺寸相同，购买按钮统一放在面板右上角：道具视图原先压在
	// 面板左上角，新增标题 / 说明后会互相遮挡，故与英雄视图对齐。
	const float panelX = listX + listW + 20.f;
	const float panelW = static_cast<float>(cfg.windowSize.x) - panelX - 20.f;
	return { { panelX + panelW - btnW - 12.f, 140.f + 12.f }, { btnW, btnH } };
}

void ShopScene::renderBuyButton() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();

	const sf::FloatRect rect = buyButtonRect();

	const bool buyable = vm.currentRowBuyable();
	sf::Color btnColor = buyable ? sf::Color(60, 120, 80) : sf::Color(50, 50, 58);
	sf::Color btnOutline = buyable ? sf::Color(120, 230, 140) : sf::Color(80, 80, 90);
	sf::Color textColor = buyable ? sf::Color(235, 245, 235) : sf::Color(120, 120, 130);

	sf::RectangleShape btn(rect.size);
	btn.setPosition(rect.position);
	btn.setFillColor(btnColor);
	btn.setOutlineThickness(2.f);
	btn.setOutlineColor(btnOutline);
	window.draw(btn);

	const sf::Vector2f textSize = textMgr.measureText("购买", 44);
	textMgr.displayText("购买",
						{ rect.position.x + (rect.size.x - textSize.x) / 2.f,
						  rect.position.y + (rect.size.y - textSize.y) / 2.f },
						{ 22, 44 }, textColor);
}

void ShopScene::renderItemDetailPanel() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	const float listX = static_cast<float>(cfg.windowSize.x) * 0.06f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	const float panelX = listX + listW + 20.f;
	const float panelW = static_cast<float>(cfg.windowSize.x) - panelX - 20.f;
	const float panelY = 140.f;
	const float panelH = static_cast<float>(cfg.windowSize.y) - panelY - 80.f;

	// 与英雄视图详情面板同款背景，保证两个视图右侧观感一致
	sf::RectangleShape bg({ panelW, panelH });
	bg.setPosition({ panelX, panelY });
	bg.setFillColor(sf::Color(28, 30, 42));
	bg.setOutlineThickness(2.f);
	bg.setOutlineColor(sf::Color(80, 90, 120));
	window.draw(bg);

	const float textX = panelX + 24.f;
	const auto& rows = vm.rows();
	const std::size_t cursor = vm.cursor();
	if (!vm.dataReady() || rows.empty() || cursor >= rows.size()
		|| rows[cursor].type != ViewModel::Row::Type::ItemEntry) {
		textMgr.displayText(vm.dataReady() ? "暂无可购买的道具" : "正在加载商城数据...",
							{ textX, panelY + 24.f }, { 20, 40 }, sf::Color(160, 160, 180));
		return;
	}
	const auto& item = vm.shopData().items[rows[cursor].itemIdx];

	// 展示元数据读跨会话缓存（每次 ShopData 到达时全量刷新）；缓存里没有该道具
	// 时得到空值，由下方显示占位文案。
	static const unool::shell::ShopItemPresentation kNoPresentation{};
	const auto& presentations = vm.cache().itemPresentation;
	const auto it = presentations.find(item.name);
	const auto& presentation = (it != presentations.end()) ? it->second : kNoPresentation;

	// 道具名
	float textY = panelY + 24.f;
	textMgr.displayText(item.name, { textX, textY }, { 28, 56 }, sf::Color(255, 215, 90));
	textY += 68.f;

	// 数量 / 售价分两行，避免长文案超出面板宽度；第二行只有服务端下发了
	// 购买上限的道具才有内容，但位置固定占位，保证各道具「效果说明」起始高度一致。
	textMgr.displayText(std::format("当前数量：{}    售价 {}", item.count, item.price.value_or(0)),
						{ textX, textY }, { 20, 40 }, sf::Color(190, 200, 220));
	textY += 44.f;
	if (presentation.maxCount.has_value()) {
		textMgr.displayText(std::format("购买上限 {} 次，已购 {} 次", *presentation.maxCount, item.count),
							{ textX, textY }, { 18, 36 }, sf::Color(160, 175, 205));
	}
	textY += 40.f;
	textY += 14.f;

	sf::RectangleShape divider({ panelW - 48.f, 2.f });
	divider.setPosition({ textX, textY });
	divider.setFillColor(sf::Color(80, 90, 120));
	window.draw(divider);
	textY += 24.f;

	textMgr.displayText("效果说明", { textX, textY }, { 18, 36 }, sf::Color(140, 170, 230));
	textY += 48.f;

	// 服务端下发的效果说明按面板宽度自动折行（长文案不会溢出）
	const std::string text = presentation.description.empty()
		? std::string("该道具暂无说明") : presentation.description;
	const std::string wrapped = textMgr.wrapText(text, panelW - 48.f, { 22, 44 });
	textMgr.displayText(wrapped, { textX, textY }, { 22, 44 }, sf::Color(225, 225, 235));
}

void ShopScene::renderDetailPanel() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	const float listX = static_cast<float>(cfg.windowSize.x) * 0.06f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	const float panelX = listX + listW + 20.f;
	const float panelW = static_cast<float>(cfg.windowSize.x) - panelX - 20.f;
	const float panelY = 140.f;
	const float panelH = static_cast<float>(cfg.windowSize.y) - panelY - 80.f;

	// 背景框
	sf::RectangleShape bg({ panelW, panelH });
	bg.setPosition({ panelX, panelY });
	bg.setFillColor(sf::Color(28, 30, 42));
	bg.setOutlineThickness(2.f);
	bg.setOutlineColor(sf::Color(80, 90, 120));
	window.draw(bg);

	const auto& rows = vm.rows();
	if (rows.empty()) return;
	const std::size_t cursor = vm.cursor();
	if (cursor >= rows.size()) return;
	const auto& row = rows[cursor];
	const auto& data = vm.shopData();

	// 确定角色名、皮肤名与立绘路径（全部来自服务端 ShopData，客户端不查角色表）
	std::string charName, skinName, imgPath;
	if (row.type == ViewModel::Row::Type::HeroCollapsed) {
		charName = data.characters[row.heroIdx].name;
		skinName = "默认";
		imgPath = data.characters[row.heroIdx].defaultImagePath;
	}
	else if (row.type == ViewModel::Row::Type::SkinEntry) {
		charName = data.characters[row.heroIdx].name;
		skinName = data.characters[row.heroIdx].skins[row.skinIdx].name;
		imgPath = data.characters[row.heroIdx].skins[row.skinIdx].imagePath;
	}
	else {
		return;
	}

	// 显示皮肤图片
	const float imgW = std::min(panelW - 24.f, 200.f);
	const float imgH = imgW * 1.4f;  // 竖向比例
	if (!imgPath.empty()) {
		renderer.displayImage(imgPath, { panelX + 12.f, panelY + 12.f }, { imgW, imgH });
	}

	// 显示角色名
	float textX = panelX + imgW + 24.f;
	float textY = panelY + 12.f;
	textMgr.displayText(charName, { textX, textY }, { 20, 40 }, sf::Color(230, 230, 240));
	textY += 44.f;
	textMgr.displayText("皮肤：" + skinName, { textX, textY }, { 16, 32 }, sf::Color(200, 200, 210));
	textY += 36.f;

	// 技能描述：从服务器下发的缓存取，未命中则补发请求并显示加载中
	std::string skillsText;
	if (const auto* cached = vm.skillText(charName)) {
		skillsText = *cached;
	}
	else {
		skillsText = "技能描述加载中...";
		vm.ensureSkillRequested(charName);
	}

	// 技能描述（自动折行）
	if (!skillsText.empty()) {
		float descY = panelY + imgH + 24.f;
		const float descW = panelW - 32.f;
		// 按 \n 分段，每段单独 wrapText
		std::string segment;
		for (char c : skillsText) {
			if (c == '\n') {
				if (!segment.empty()) {
					std::string wrapped = textMgr.wrapText(segment, descW, { 16, 32 });
					textMgr.displayText(wrapped, { panelX + 16.f, descY }, { 16, 32 }, sf::Color(220, 220, 230));
					descY += textMgr.getLineSpacing(32) * (1 + std::ranges::count(wrapped, '\n'));
					segment.clear();
				}
				else {
					descY += textMgr.getLineSpacing(32) / 2;  // 空行间距小一些
				}
			}
			else {
				segment += c;
			}
		}
		if (!segment.empty()) {
			std::string wrapped = textMgr.wrapText(segment, descW, { 16, 32 });
			textMgr.displayText(wrapped, { panelX + 16.f, descY }, { 16, 32 }, sf::Color(220, 220, 230));
		}
	}
}
