#include "ShopScene.h"
#include "GameRenderer.h"
#include <algorithm>
#include <format>
#include <string>

ShopScene::ShopScene(GameRenderer& r, ClientNetwork& n, const std::string& uname)
	: renderer(r), net(n), username(uname) {
	rebuildDisplayedIndices();
}

void ShopScene::run() {
	// 进入商城即请求一次商城数据
	requestShopData();
	while (renderer.windowIsOpen() && !exiting) {
		for (auto ev = renderer.pollEvent(); ev; ev = renderer.pollEvent()) {
			handleEvent(*ev);
		}
		if (exiting) break;
		net.update();
		if (!net.isConnected()) break;
		handleNetwork();
		render();
	}
}

// ===== 网络通信 =====

void ShopScene::requestShopData() {
	ShopPurchaseRequest req;
	req.type = "request";
	nlohmann::json j = req;
	net.sendShopPurchase(j.dump());
}

void ShopScene::handleNetwork() {
	while (auto packetOpt = net.receivePacket()) {
		sf::Packet packet = *packetOpt;
		int msgType;
		if (!(packet >> msgType)) continue;
		auto mt = static_cast<MessageType>(msgType);

		if (mt == MessageType::ShopData) {
			std::string jsonStr;
			if (packet >> jsonStr) {
				try {
					nlohmann::json j = nlohmann::json::parse(jsonStr);
					shopData = j.get<ShopData>();
					dataReady = true;
					// 数据刷新后重建显示并夹紧导航状态
					rebuildDisplayedIndices();
					clampPage();
					rebuildRows();
					clampCursor();
				}
				catch (const std::exception&) {
					setPurchaseMsg("商城数据解析失败");
				}
			}
			continue;
		}
		if (mt == MessageType::ShopResult) {
			std::string jsonStr;
			if (packet >> jsonStr) {
				ShopPurchaseResult result;
				try {
					nlohmann::json j = nlohmann::json::parse(jsonStr);
					result = j.get<ShopPurchaseResult>();
				}
				catch (const std::exception&) {
					setPurchaseMsg("购买结果解析失败");
					continue;
				}
				if (result.ok) {
					setPurchaseMsg("购买成功");
				}
				else {
					// 失败：积分不足或其它，统一提示
					setPurchaseMsg(result.msg.empty() ? "积分不足！" : result.msg);
				}
			}
			continue;
		}
		// 心跳包：立即回复，防止服务器判定掉线
		if (mt == MessageType::Heartbeat) {
			sf::Packet hb;
			hb << static_cast<int>(MessageType::Heartbeat);
			net.send(hb);
			continue;
		}
		// 其它包忽略（商城场景不处理游戏/账号包）
	}
}

void ShopScene::tryBuyCharacter(const std::string& name) {
	ShopPurchaseRequest req;
	req.type = "character";
	req.name = name;
	nlohmann::json j = req;
	net.sendShopPurchase(j.dump());
	setPurchaseMsg("正在购买...");
}

void ShopScene::tryBuySkin(const std::string& charName, const std::string& skinName) {
	ShopPurchaseRequest req;
	req.type = "skin";
	req.charName = charName;
	req.skinName = skinName;
	nlohmann::json j = req;
	net.sendShopPurchase(j.dump());
	setPurchaseMsg("正在购买...");
}

void ShopScene::tryBuyItem(const std::string& itemName) {
	ShopPurchaseRequest req;
	req.type = "item";
	req.name = itemName;
	nlohmann::json j = req;
	net.sendShopPurchase(j.dump());
	setPurchaseMsg("正在购买...");
}

void ShopScene::activateCurrentRow() {
	if (rows.empty()) return;
	const auto& row = rows[cursor];
	if (currentView == View::Hero) {
		if (row.type == Row::Type::HeroCollapsed) {
			// 展开或收起皮肤列表
			if (expandedHeroIdx == row.heroIdx) {
				expandedHeroIdx = static_cast<std::size_t>(-1);
				rebuildRows();
				clampCursor();
			}
			else {
				expandedHeroIdx = row.heroIdx;
				rebuildRows();
				// 展开后直接跳到 HeroEntry 行（下一个）
				if (cursor + 1 < rows.size() && rows[cursor + 1].type == Row::Type::HeroEntry)
					++cursor;
			}
		}
		// HeroEntry/SkinEntry 仅选中，不直接购买
	}
	// 道具视图：ItemEntry 仅选中，不直接购买
}

bool ShopScene::currentRowBuyable() const {
	if (rows.empty()) return false;
	const auto& row = rows[cursor];
	if (currentView == View::Hero) {
		if (row.type == Row::Type::HeroEntry) {
			const auto& hero = shopData.characters[row.heroIdx];
			return !hero.unlocked && hero.price >= 0;
		}
		if (row.type == Row::Type::SkinEntry) {
			const auto& hero = shopData.characters[row.heroIdx];
			const auto& skin = hero.skins[row.skinIdx];
			return !skin.unlocked && skin.price >= 0;
		}
	}
	else {
		if (row.type == Row::Type::ItemEntry) {
			const auto& item = shopData.items[row.itemIdx];
			return item.available && item.price >= 0;
		}
	}
	return false;
}

void ShopScene::tryBuyCurrentSelection() {
	if (rows.empty()) return;
	const auto& row = rows[cursor];
	if (currentView == View::Hero) {
		if (row.type == Row::Type::HeroEntry) {
			const auto& hero = shopData.characters[row.heroIdx];
			if (!hero.unlocked && hero.price >= 0) tryBuyCharacter(hero.name);
			else setPurchaseMsg(hero.unlocked ? "已解锁" : "不可购买");
		}
		else if (row.type == Row::Type::SkinEntry) {
			const auto& hero = shopData.characters[row.heroIdx];
			const auto& skin = hero.skins[row.skinIdx];
			if (!skin.unlocked && skin.price >= 0) tryBuySkin(hero.name, skin.name);
			else setPurchaseMsg(skin.unlocked ? "已解锁" : "不售卖");
		}
		else {
			setPurchaseMsg("请选择英雄或皮肤");
		}
	}
	else {
		if (row.type == Row::Type::ItemEntry) {
			const auto& item = shopData.items[row.itemIdx];
			if (item.available && item.price >= 0) tryBuyItem(item.name);
			else setPurchaseMsg("不可购买");
		}
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
	if (mode == Mode::Search && event.is<sf::Event::TextEntered>()) {
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
	auto sc = key.scancode;

	// 搜索模式下：ESC 退出搜索，其它交给文本输入与导航
	if (mode == Mode::Search) {
		if (sc == sf::Keyboard::Scancode::Escape) {
			mode = Mode::Browse;
			searchKeyword.clear();
			rebuildDisplayedIndices();
			pageIdx = 0;
			rebuildRows();
			clampCursor();
			return;
		}
		if (sc == sf::Keyboard::Scancode::Backspace) {
			if (!searchKeyword.empty()) {
				// 删除最后一个 UTF-8 字符
				std::size_t n = searchKeyword.size();
				while (n > 0 && (static_cast<unsigned char>(searchKeyword[n - 1]) & 0xC0) == 0x80) --n;
				if (n > 0) --n;
				searchKeyword.erase(n);
				rebuildDisplayedIndices();
				pageIdx = 0;
				rebuildRows();
				clampCursor();
			}
			return;
		}
		// 搜索模式下仅用方向键导航（W/A/S/D 留给文本输入）
		if (sc == sf::Keyboard::Scancode::Up) {
			if (cursor > 0) --cursor;
			return;
		}
		if (sc == sf::Keyboard::Scancode::Down) {
			if (cursor + 1 < rows.size()) ++cursor;
			return;
		}
		if (sc == sf::Keyboard::Scancode::Enter) {
			activateCurrentRow();
			return;
		}
		if (sc == sf::Keyboard::Scancode::Space) {
			tryBuyCurrentSelection();
			return;
		}
		return; // 搜索模式下其它按键忽略（由 TextEntered 处理）
	}

	// 浏览模式
	if (sc == sf::Keyboard::Scancode::Escape) {
		// 退出商城，返回登录界面（不关闭窗口）
		exiting = true;
		return;
	}
	if (sc == sf::Keyboard::Scancode::F) {
		// 进入搜索模式（英雄视图下）
		if (currentView == View::Hero) {
			mode = Mode::Search;
			searchKeyword.clear();
			rebuildDisplayedIndices();
			pageIdx = 0;
			rebuildRows();
			clampCursor();
		}
		return;
	}
	if (sc == sf::Keyboard::Scancode::E) {
		currentView = View::Item;
		pageIdx = 0;
		cursor = 0;
		expandedHeroIdx = static_cast<std::size_t>(-1);
		rebuildRows();
		return;
	}
	if (sc == sf::Keyboard::Scancode::H) {
		currentView = View::Hero;
		pageIdx = 0;
		cursor = 0;
		expandedHeroIdx = static_cast<std::size_t>(-1);
		rebuildRows();
		return;
	}
	if (sc == sf::Keyboard::Scancode::Up || sc == sf::Keyboard::Scancode::W) {
		if (cursor > 0) --cursor;
		return;
	}
	if (sc == sf::Keyboard::Scancode::Down || sc == sf::Keyboard::Scancode::S) {
		if (cursor + 1 < rows.size()) ++cursor;
		return;
	}
	if (sc == sf::Keyboard::Scancode::Left || sc == sf::Keyboard::Scancode::A) {
		if (pageIdx > 0) {
			--pageIdx;
			expandedHeroIdx = static_cast<std::size_t>(-1);
			rebuildRows();
			clampCursor();
		}
		return;
	}
	if (sc == sf::Keyboard::Scancode::Right || sc == sf::Keyboard::Scancode::D) {
		if (pageIdx + 1 < totalPages()) {
			++pageIdx;
			expandedHeroIdx = static_cast<std::size_t>(-1);
			rebuildRows();
			clampCursor();
		}
		return;
	}
	if (sc == sf::Keyboard::Scancode::Enter) {
		activateCurrentRow();
		return;
	}
	if (sc == sf::Keyboard::Scancode::Space) {
		tryBuyCurrentSelection();
		return;
	}
}

void ShopScene::handleTextEntered(const sf::Event::TextEntered& te) {
	char32_t ch = te.unicode;
	if (ch < 33) return; // 排除控制字符与空格
	// 限制长度
	if (searchKeyword.size() >= 96) return;
	// 将码位编码为 UTF-8 追加
	if (ch < 0x80) {
		searchKeyword.push_back(static_cast<char>(ch));
	}
	else if (ch < 0x800) {
		searchKeyword.push_back(static_cast<char>(0xC0 | (ch >> 6)));
		searchKeyword.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
	}
	else if (ch < 0x10000) {
		searchKeyword.push_back(static_cast<char>(0xE0 | (ch >> 12)));
		searchKeyword.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
		searchKeyword.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
	}
	else {
		searchKeyword.push_back(static_cast<char>(0xF0 | (ch >> 18)));
		searchKeyword.push_back(static_cast<char>(0x80 | ((ch >> 12) & 0x3F)));
		searchKeyword.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
		searchKeyword.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
	}
	rebuildDisplayedIndices();
	pageIdx = 0;
	rebuildRows();
	clampCursor();
}

void ShopScene::handleMouseClick(const sf::Vector2f& pos) {
	const auto& cfg = renderer.getConfig();
	const float listX = static_cast<float>(cfg.windowSize.x) * 0.08f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.6f;
	const float rowH = 56.f;
	const float listStartY = 140.f;

	// 先检查"购买"按钮
	const float btnW = 160.f;
	const float btnH = 50.f;
	const float btnX = listX + listW + 20.f;
	const float btnY = listStartY;
	if (pos.x >= btnX && pos.x <= btnX + btnW && pos.y >= btnY && pos.y <= btnY + btnH) {
		tryBuyCurrentSelection();
		return;
	}

	// 鼠标点击行：选中并触发展开/收起（不购买）
	if (pos.x < listX || pos.x > listX + listW) return;
	for (std::size_t i = 0; i < rows.size(); ++i) {
		float y = listStartY + static_cast<float>(i) * rowH;
		if (pos.y >= y && pos.y <= y + rowH) {
			cursor = i;
			activateCurrentRow();
			return;
		}
	}
}

// ===== 行表构建 =====

void ShopScene::rebuildDisplayedIndices() {
	displayedIndices.clear();
	if (mode == Mode::Search && !searchKeyword.empty()) {
		for (std::size_t i = 0; i < shopData.characters.size(); ++i) {
			if (shopData.characters[i].name.find(searchKeyword) != std::string::npos) {
				displayedIndices.push_back(i);
			}
		}
	}
	else {
		for (std::size_t i = 0; i < shopData.characters.size(); ++i) {
			displayedIndices.push_back(i);
		}
	}
}

void ShopScene::rebuildRows() {
	rows.clear();
	if (currentView == View::Item) {
		for (std::size_t i = 0; i < shopData.items.size(); ++i) {
			// 选将扩充卡购买5次后（available=false）从列表消失
			if (!shopData.items[i].available) continue;
			Row r;
			r.type = Row::Type::ItemEntry;
			r.itemIdx = i;
			rows.push_back(r);
		}
		return;
	}
	// 英雄视图：对 displayedIndices 分页
	std::size_t start = pageIdx * kPerPage;
	std::size_t end = std::min(start + kPerPage, displayedIndices.size());
	for (std::size_t i = start; i < end; ++i) {
		std::size_t hi = displayedIndices[i];
		Row r;
		r.type = Row::Type::HeroCollapsed;
		r.heroIdx = hi;
		rows.push_back(r);
		// 展开皮肤列表：首位英雄项 + 后续皮肤项
		if (hi == expandedHeroIdx) {
			Row hr;
			hr.type = Row::Type::HeroEntry;
			hr.heroIdx = hi;
			rows.push_back(hr);
			for (std::size_t s = 0; s < shopData.characters[hi].skins.size(); ++s) {
				Row sr;
				sr.type = Row::Type::SkinEntry;
				sr.heroIdx = hi;
				sr.skinIdx = s;
				rows.push_back(sr);
			}
		}
	}
}

std::size_t ShopScene::totalPages() const {
	if (currentView == View::Item) return 1;
	std::size_t n = displayedIndices.size();
	return (n + kPerPage - 1) / kPerPage;
}

void ShopScene::clampPage() {
	if (pageIdx >= totalPages()) {
		pageIdx = (totalPages() == 0) ? 0 : totalPages() - 1;
	}
}

void ShopScene::clampCursor() {
	if (rows.empty()) {
		cursor = 0;
		return;
	}
	if (cursor >= rows.size()) cursor = rows.size() - 1;
}

void ShopScene::setPurchaseMsg(const std::string& msg) {
	purchaseMsg = msg;
	msgTimer.restart();
	msgActive = true;
}

bool ShopScene::affordable(int price) const {
	return price >= 0 && shopData.points >= price;
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

	if (mode == Mode::Search) renderSearchBar();

	if (currentView == View::Hero) renderHeroView();
	else renderItemView();

	renderBuyButton();

	// 消息提示（底部居中）
	if (msgActive && !purchaseMsg.empty()) {
		if (msgTimer.getElapsedTime().asSeconds() < 2.5f) {
			auto& textMgr = renderer.getTextManager();
			const auto& cfg = renderer.getConfig();
			sf::Vector2f msgSize = { 22, 44 };
			sf::Vector2f measured = textMgr.measureText(purchaseMsg, static_cast<unsigned int>(msgSize.y));
			float x = (static_cast<float>(cfg.windowSize.x) - measured.x) / 2.f;
			float y = static_cast<float>(cfg.windowSize.y) - 80.f;
			sf::Color col = (purchaseMsg == "购买成功") ? sf::Color(120, 230, 140) : sf::Color(250, 120, 120);
			textMgr.displayText(purchaseMsg, { x, y }, msgSize, col);
		}
		else {
			msgActive = false;
		}
	}

	// 底部操作提示
	{
		auto& textMgr = renderer.getTextManager();
		const auto& cfg = renderer.getConfig();
		std::string hint;
		if (mode == Mode::Search) {
			hint = "输入关键词筛选 | Backspace 删除 | ESC 退出搜索 | W/S 导航 | 空格 购买";
		}
		else if (currentView == View::Hero) {
			hint = "W/S 导航 | Enter 展开 | A/D 翻页 | 空格 购买 | F 搜索 | E 道具 | ESC 退出";
		}
		else {
			hint = "W/S 导航 | 空格 购买 | H 英雄 | ESC 退出";
		}
		sf::Vector2f hintSize = { 16, 32 };
		sf::Vector2f measured = textMgr.measureText(hint, static_cast<unsigned int>(hintSize.y));
		float x = (static_cast<float>(cfg.windowSize.x) - measured.x) / 2.f;
		float y = static_cast<float>(cfg.windowSize.y) - 36.f;
		textMgr.displayText(hint, { x, y }, hintSize, sf::Color(150, 150, 160));
	}

	window.display();
}

void ShopScene::renderTopBar() {
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();
	// 左上角操作提示
	std::string leftHint = "F搜索，E/H切换界面";
	if (mode == Mode::Search) leftHint = "搜索模式（ESC 退出）";
	textMgr.displayText(leftHint, { 20.f, 20.f }, { 18, 36 }, sf::Color(200, 200, 210));
	// 右上角积分
	std::string pts = std::format("积分: {}", shopData.points);
	textMgr.displayTextInUpRight(pts, { 22, 44 }, sf::Color(255, 215, 80));
	// 视图标识（顶部居中）
	std::string title = (currentView == View::Hero) ? "英雄商城" : "道具商城";
	if (mode == Mode::Search) title = "英雄商城 - 搜索";
	textMgr.displayTextInUp(title, { 24, 48 }, sf::Color(230, 230, 240));
}

void ShopScene::renderSearchBar() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();
	const float barW = static_cast<float>(cfg.windowSize.x) * 0.5f;
	const float barH = 42.f;
	const float barX = (static_cast<float>(cfg.windowSize.x) - barW) / 2.f;
	const float barY = 72.f;
	sf::RectangleShape bar({ barW, barH });
	bar.setPosition({ barX, barY });
	bar.setFillColor(sf::Color(30, 34, 48));
	bar.setOutlineThickness(2.f);
	bar.setOutlineColor(sf::Color(140, 170, 230));
	window.draw(bar);
	std::string label = "搜索: " + searchKeyword;
	// 光标闪烁
	static sf::Clock blink;
	if (blink.getElapsedTime().asMilliseconds() % 1000 < 500) label += "_";
	textMgr.displayText(label, { barX + 12.f, barY + 4.f }, { 20, 40 }, sf::Color(235, 235, 245));
}

void ShopScene::renderHeroView() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	if (!dataReady) {
		textMgr.displayTextInCenter("正在加载商城数据...", { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}
	if (displayedIndices.empty()) {
		std::string msg = mode == Mode::Search ? "未找到匹配的角色" : "暂无可浏览的英雄";
		textMgr.displayTextInCenter(msg, { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}

	const float listX = static_cast<float>(cfg.windowSize.x) * 0.08f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.6f;
	const float rowH = 56.f;
	const float startY = 140.f;

	for (std::size_t i = 0; i < rows.size(); ++i) {
		const auto& row = rows[i];
		float y = startY + static_cast<float>(i) * rowH;
		bool selected = (i == cursor);
		sf::Vector2f boxPos = { listX, y };
		sf::Vector2f boxSize = { listW, rowH - 6.f };

		if (row.type == Row::Type::HeroCollapsed) {
			const auto& hero = shopData.characters[row.heroIdx];
			drawRowBox(boxPos, boxSize, selected);
			// 角色名 + 等级 + 解锁状态 + 售价
			std::string levelStr = Character::to_string(hero.level);
			std::string line = std::format("{}  {}档", hero.name, levelStr);
			if (hero.unlocked) {
				line += "  (已解锁)";
				textMgr.displayText(line, { boxPos.x + 16.f, y + 6.f }, { 22, 44 }, sf::Color(235, 235, 245));
				textMgr.displayText("√", { boxPos.x + listW - 50.f, y + 6.f }, { 22, 44 }, sf::Color(120, 230, 140));
			}
			else if (hero.price < 0) {
				line += "  (不可购买)";
				textMgr.displayText(line, { boxPos.x + 16.f, y + 6.f }, { 22, 44 }, sf::Color(180, 180, 190));
				textMgr.displayText("×", { boxPos.x + listW - 50.f, y + 6.f }, { 22, 44 }, sf::Color(180, 180, 190));
			}
			else {
				line += "  (未解锁)";
				textMgr.displayText(line, { boxPos.x + 16.f, y + 6.f }, { 22, 44 }, sf::Color(235, 235, 245));
				std::string priceStr = std::format("售价 {}", hero.price);
				sf::Color pc = affordable(hero.price) ? sf::Color(120, 230, 140) : sf::Color(250, 120, 120);
				textMgr.displayText(priceStr, { boxPos.x + listW - 170.f, y + 6.f }, { 20, 40 }, pc);
			}
		}
		else if (row.type == Row::Type::HeroEntry) {
			// 展开列表首项：英雄本身（购买入口）
			const auto& hero = shopData.characters[row.heroIdx];
			sf::Vector2f subPos = { listX + 40.f, y };
			sf::Vector2f subSize = { listW - 40.f, rowH - 6.f };
			drawRowBox(subPos, subSize, selected);
			std::string line;
			if (hero.unlocked) {
				line = std::format("英雄 {} (已解锁)", hero.name);
				textMgr.displayText(line, { subPos.x + 16.f, y + 6.f }, { 20, 40 }, sf::Color(120, 230, 140));
			}
			else if (hero.price < 0) {
				line = std::format("英雄 {} (不可购买)", hero.name);
				textMgr.displayText(line, { subPos.x + 16.f, y + 6.f }, { 20, 40 }, sf::Color(180, 180, 190));
			}
			else {
				line = std::format("英雄 {} (未解锁)", hero.name);
				textMgr.displayText(line, { subPos.x + 16.f, y + 6.f }, { 20, 40 }, sf::Color(235, 235, 245));
				std::string priceStr = std::format("售价 {}", hero.price);
				sf::Color pc = affordable(hero.price) ? sf::Color(120, 230, 140) : sf::Color(250, 120, 120);
				textMgr.displayText(priceStr, { subPos.x + subSize.x - 160.f, y + 6.f }, { 18, 36 }, pc);
			}
		}
		else if (row.type == Row::Type::SkinEntry) {
			const auto& hero = shopData.characters[row.heroIdx];
			const auto& skin = hero.skins[row.skinIdx];
			sf::Vector2f subPos = { listX + 80.f, y };
			sf::Vector2f subSize = { listW - 80.f, rowH - 6.f };
			drawRowBox(subPos, subSize, selected);
			std::string line;
			if (skin.unlocked) {
				line = std::format("{} [{}] (已解锁)", skin.name, skin.quality);
				textMgr.displayText(line, { subPos.x + 16.f, y + 6.f }, { 20, 40 }, sf::Color(120, 230, 140));
			}
			else if (skin.price < 0) {
				line = std::format("{} [{}] (不售卖)", skin.name, skin.quality);
				textMgr.displayText(line, { subPos.x + 16.f, y + 6.f }, { 20, 40 }, sf::Color(180, 180, 190));
			}
			else {
				line = std::format("{} [{}] (未解锁)", skin.name, skin.quality);
				textMgr.displayText(line, { subPos.x + 16.f, y + 6.f }, { 20, 40 }, sf::Color(235, 235, 245));
				std::string priceStr = std::format("售价 {}", skin.price);
				sf::Color pc = affordable(skin.price) ? sf::Color(120, 230, 140) : sf::Color(250, 120, 120);
				textMgr.displayText(priceStr, { subPos.x + subSize.x - 160.f, y + 6.f }, { 18, 36 }, pc);
			}
		}
	}

	// 翻页指示
	std::size_t pages = totalPages();
	if (pages > 0) {
		std::string pageStr = std::format("第 {}/{} 页", pageIdx + 1, pages);
		textMgr.displayTextInRight(pageStr, { 18, 36 }, sf::Color(180, 180, 200));
	}
}

void ShopScene::renderItemView() {
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	if (!dataReady) {
		textMgr.displayTextInCenter("正在加载商城数据...", { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}

	const float listX = static_cast<float>(cfg.windowSize.x) * 0.08f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.6f;
	const float rowH = 56.f;
	const float startY = 140.f;

	if (rows.empty()) {
		textMgr.displayTextInCenter("暂无可购买的道具", { 22, 44 }, sf::Color(200, 200, 210));
		return;
	}

	for (std::size_t i = 0; i < rows.size(); ++i) {
		const auto& row = rows[i];
		const auto& item = shopData.items[row.itemIdx];
		float y = startY + static_cast<float>(i) * rowH;
		bool selected = (i == cursor);
		sf::Vector2f boxPos = { listX, y };
		sf::Vector2f boxSize = { listW, rowH - 6.f };
		drawRowBox(boxPos, boxSize, selected);

		std::string line = std::format("{}  (当前数量：{})", item.name, item.count);
		textMgr.displayText(line, { boxPos.x + 16.f, y + 6.f }, { 22, 44 }, sf::Color(235, 235, 245));

		std::string priceStr = std::format("售价 {}", item.price);
		sf::Color pc = affordable(item.price) ? sf::Color(120, 230, 140) : sf::Color(250, 120, 120);
		textMgr.displayText(priceStr, { boxPos.x + listW - 170.f, y + 6.f }, { 20, 40 }, pc);
	}
}

void ShopScene::renderBuyButton() {
	auto& window = renderer.getWindow();
	auto& textMgr = renderer.getTextManager();
	const auto& cfg = renderer.getConfig();

	const float listX = static_cast<float>(cfg.windowSize.x) * 0.08f;
	const float listW = static_cast<float>(cfg.windowSize.x) * 0.6f;
	const float btnW = 140.f;
	const float btnH = 44.f;
	const float btnX = listX + listW + 20.f;
	const float btnY = 140.f;

	bool buyable = currentRowBuyable();
	sf::Color btnColor = buyable ? sf::Color(60, 120, 80) : sf::Color(50, 50, 58);
	sf::Color btnOutline = buyable ? sf::Color(120, 230, 140) : sf::Color(80, 80, 90);
	sf::Color textColor = buyable ? sf::Color(235, 245, 235) : sf::Color(120, 120, 130);

	sf::RectangleShape btn({ btnW, btnH });
	btn.setPosition({ btnX, btnY });
	btn.setFillColor(btnColor);
	btn.setOutlineThickness(2.f);
	btn.setOutlineColor(btnOutline);
	window.draw(btn);

	textMgr.displayText("购买", { btnX + btnW / 2.f - 20.f, btnY + 4.f }, { 22, 44 }, textColor);
}
