#include "GameRenderer.h"
#include "Card.h"
#include "Character.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <ranges>
#include <unordered_map>

GameRenderer::GameRenderer(const Config& _config)
	:config(_config),
	window(std::make_unique<sf::RenderWindow>(
		sf::VideoMode(_config.windowSize), _config.windowName
	)),
	textMgr(*window),
	imageMgr(*window) {
	window->setFramerateLimit(60);
}

GameRenderer::~GameRenderer() {
	window->close();
}

void GameRenderer::updateState(const GameState& state) {
	currentState = state;
}

void GameRenderer::updatePointer(std::size_t playerId, std::size_t selectedIndex) {
	for (auto& ps : currentState.players) {
		if (ps.id == playerId) {
			ps.hand.setSelectedIndex(selectedIndex);
			break;
		}
	}
}

void GameRenderer::movePointerLeft(std::size_t playerId) {
	for (auto& ps : currentState.players) {
		if (ps.id == playerId) {
			ps.hand.selectLeft();
			break;
		}
	}
}

void GameRenderer::movePointerRight(std::size_t playerId) {
	for (auto& ps : currentState.players) {
		if (ps.id == playerId) {
			ps.hand.selectRight();
			break;
		}
	}
}

std::size_t GameRenderer::getSelectedIndex(std::size_t playerId) const {
	for (const auto& ps : currentState.players) {
		if (ps.id == playerId) return ps.hand.getSelectedIndex();
	}
	return 0;
}

std::string GameRenderer::getCharacterName(std::size_t playerId) const {
	for (const auto& ps : currentState.players) {
		if (ps.id != playerId) continue;
		if (ps.characterNames.empty()) return "";
		std::string name = ps.characterNames.front();
		for (std::size_t i = 1; i < ps.characterNames.size(); ++i) {
			name += "&" + ps.characterNames[i];
		}
		return name;
	}
	return "";
}

bool GameRenderer::isChoiceActive() const {
	return choice.has_value();
}

bool GameRenderer::hasChoiceOptions() const {
	return choice.has_value() && !choice->options.empty();
}

bool GameRenderer::canSelectLocal() const {
	return currentState.operatingPlayerId.has_value()
		&& currentState.operatingPlayerId.value() == localPlayerId
		&& !hasChoiceOptions();
}

void GameRenderer::updateCharInfo(std::size_t playerIndex, const std::string& fullText) {
	if (playerIndex < 2) {
		charInfoCache[playerIndex] = fullText;
		//使infoBox缓存失效，强制重算
		infoBoxCache.playerId = static_cast<std::size_t>(-1);
	}
}

void GameRenderer::display() {
	window->clear(sf::Color::White);
	renderPlayers();
	renderDiscardPile();
	renderChoice();
	renderBpPreview();
	renderInfoBox();
	renderGameInfo();
	window->display();
}

void GameRenderer::renderPlayers() {
	for (const auto& playerState : currentState.players) {
		//角色图横向并排原尺寸渲染（组合角色多张）
		sf::Vector2f charPos = { 0,0 };
		if (playerState.id == 0) charPos = { 0,0 };
		else if (playerState.id == 1) charPos = { 0,config.windowSize.y - config.characterSize.y };
		const std::size_t imgCount = playerState.characterNames.size();
		for (std::size_t i = 0; i < imgCount; ++i) {
			sf::Vector2f imgPos = { charPos.x + i * config.characterSize.x, charPos.y };
			//立绘路径由服务端随 GameState 下发，客户端不查本地角色表
			if (i < playerState.characterImagePaths.size() && !playerState.characterImagePaths[i].empty()) {
				displayImage(playerState.characterImagePaths[i], imgPos, config.characterSize);
			}
		}
		const float charTotalWidth = static_cast<float>(imgCount) * config.characterSize.x;

		//标记（覆盖在最后一张角色图右上角，等比缩放至角色图面积的1/12）
		if (playerState.marks.contains("幽灵")) {
			const std::string markPath = "images/marks/幽灵.jpg";
			sf::Vector2u texSize = imageMgr.getTextureSize(markPath);
			float targetArea = config.characterSize.x * config.characterSize.y / 12.0f;
			float scale = std::sqrt(targetArea / (static_cast<float>(texSize.x) * texSize.y));
			sf::Vector2f markSize = { texSize.x * scale, texSize.y * scale };
			sf::Vector2f markPos = { charPos.x + charTotalWidth - markSize.x, charPos.y };
			displayImage(markPath, markPos, markSize);
		}

		//座次、体力
		if (!currentState.seatOrder.empty()) {
			//座位号
			std::size_t seat = currentState.seatOrder[playerState.id];
			std::string seatText = (seat == 0) ? "一号位" : "二号位";
			//体力（有护盾时显示为 体力：200(+30)/200）
			std::string hpText = (playerState.shield > 0)
				? std::format("体力：{}(+{})/{}", playerState.hp, playerState.shield, playerState.maxHp)
				: std::format("体力：{}/{}", playerState.hp, playerState.maxHp);
			//显示
			sf::Vector2f pos;
			if (playerState.id == 0) {
				pos = { charPos.x, charPos.y + config.characterSize.y };
			}
			else {
				pos = { charPos.x, charPos.y - 50 };
			}
			displayText(std::format("{}，{}", seatText, hpText), pos);
		}

		const bool isLocalPlayer = playerState.id == localPlayerId;
		//手牌信息
		std::string cardsInfoText = std::format("手牌数：{}", playerState.hand.count());
		if (isLocalPlayer) {
			cardsInfoText += std::format("；总价值：{}", playerState.hand.value());
		}
		sf::Vector2f cardsInfoPos;
		if (playerState.id == 0) {
			cardsInfoPos = { charPos.x, charPos.y + config.characterSize.y + 50 };
		}
		else {
			cardsInfoPos = { charPos.x, charPos.y - 100 };
		}
		displayText(cardsInfoText, cardsInfoPos);

		//手牌起始位置：组合角色按图片总宽，单角色按单图宽
		const sf::Vector2f& handDisplayPos =
			playerState.id == 0 ?
			sf::Vector2f{ charTotalWidth, 0 } :
			sf::Vector2f{ charTotalWidth, config.windowSize.y - config.characterSize.y };

		const bool canSelect = isLocalPlayer && canSelectLocal();

		displayHand(playerState.hand, handDisplayPos, config.cardSize, isLocalPlayer ? config.pointerSize : sf::Vector2f{ 0, 0 }, canSelect);
	}
}

void GameRenderer::renderDiscardPile() {
	if (!currentState.discardPile.empty()) {
		//当前牌（顶牌）
		const Card& lastCard = currentState.discardPile.front();
		sf::Vector2f currentCardPos{ config.windowSize.x / 2.0f - config.cardSize.x / 2,
									 config.windowSize.y / 2.0f - config.cardSize.y / 2 };
		displayCard(lastCard, currentCardPos, config.cardSize);
		//下方写 DiscardReason（按真实字宽水平居中于卡牌下方）
		const std::string reasonText = std::format("{}", lastCard.getDiscardReason());
		if (!reasonText.empty()) {
			const sf::Vector2f textSize{ 20, 40 };
			const float textPad = 8.0f;
			const sf::Vector2f actualTextSize = textMgr.measureText(reasonText, static_cast<unsigned int>(textSize.y));
			const float cardCenterX = currentCardPos.x + config.cardSize.x / 2.0f;
			const sf::Vector2f textPos{
				cardCenterX - actualTextSize.x / 2.0f,
				currentCardPos.y + config.cardSize.y + textPad
			};
			displayText(reasonText, textPos, textSize);
		}

		// 历史牌：缩小至 60%，紧邻当前牌左侧从新到旧排列（向左展开），垂直居中
		sf::Vector2f historySize{ config.cardSize.x * 0.6f, config.cardSize.y * 0.6f };
		sf::Vector2f historyPos{ config.windowSize.x / 2.0f - config.cardSize.x / 2 - historySize.x,
								 config.windowSize.y / 2.0f - historySize.y / 2 };
		size_t historyCount = currentState.discardPile.size() - 1;
		if (historyCount > 3) historyCount = 3;
		const sf::Vector2f historyTextSize{ 20, 40 };
		const float historyTextPad = 6.0f;
		for (size_t i = 1; i <= historyCount; ++i) {
			const Card& histCard = currentState.discardPile[i];
			displayCard(histCard, historyPos, historySize);
			const std::string histReason = std::format("{}", histCard.getDiscardReason());
			if (!histReason.empty()) {
				const sf::Vector2f actualTextSize = textMgr.measureText(histReason, static_cast<unsigned int>(historyTextSize.y));
				const float cardCenterX = historyPos.x + historySize.x / 2.0f;
				const sf::Vector2f textPos{
					cardCenterX - actualTextSize.x / 2.0f,
					historyPos.y + historySize.y + historyTextPad
				};
				displayText(histReason, textPos, historyTextSize);
			}
			historyPos.x -= historySize.x;
		}
	}
	displayTextInLeft(std::format("当前颜色：{}\n当前牌名：{}", currentState.currentColor, currentState.currentName));
}

void GameRenderer::renderChoice() {
	choiceRows.clear();
	if (!choice.has_value()) return;

	std::string choiceText;
	//每行对应的选项下标；nullopt 表示标题 / 提示 / 错误 / 倒计时等非选项行。
	//同时保留原始文本，用于按"实际绘制行号"映射命中矩形（标题含 \n 时也占多行）。
	std::vector<std::pair<std::string, std::optional<std::size_t>>> lines;
	auto addLine = [&](const std::string& line, std::optional<std::size_t> optionIndex = std::nullopt) {
		choiceText += line;
		choiceText += "\n";
		lines.push_back({ line, optionIndex });
	};
	if (!choice->title.empty()) {
		addLine(choice->title);
	}
	for (const auto& [i, option] : choice->options | std::views::enumerate) {
		addLine(std::format("[{}] {}", i + 1, option), static_cast<std::size_t>(i));
	}
	if (!choice->options.empty()) {
		if (choice->forced) {
			if (choice->totalPages > 1) {
				addLine(std::format("第 {}/{}页，<-->翻页", choice->currentPage + 1, choice->totalPages));
				addLine(std::format("输入数字1-{}选择（必须选择）", choice->options.size()));
			}
			else {
				addLine(std::format("输入数字1-{}选择（必须选择）", choice->options.size()));
			}
		}
		else {
			if (choice->totalPages > 1) {
				addLine(std::format("第 {}/{}页，<-->翻页", choice->currentPage + 1, choice->totalPages));
				addLine(std::format("输入数字0-{}选择（0表示不选择）", choice->options.size()));
			}
			else {
				addLine(std::format("输入数字0-{}选择（0表示不选择）", choice->options.size()));
			}
		}
	}
	if (!choice->errorMsg.empty()) {
		addLine(choice->errorMsg);
	}
	if (choice->timeoutMs.has_value()) {
		float elapsedMs = static_cast<float>(countdownClock.getElapsedTime().asMilliseconds());
		float remainingMs = static_cast<float>(choice->timeoutMs.value()) - elapsedMs;
		if (remainingMs < 0.f) remainingMs = 0.f;
		float remainingSec = remainingMs / 1000.f;
		int intPart = static_cast<int>(remainingSec);
		int decPart = static_cast<int>((remainingSec - intPart) * 10.f);
		addLine(std::format("剩余时间：{}.{} 秒", intPart, decPart));
	}

	//只有 BP 选项（带元数据）才需要命中矩形与悬停高亮；普通 ask 保持原样绘制
	if (!choice->optionMeta.empty()) {
		const unsigned int charSize = 40u;
		const sf::Vector2f measured = textMgr.measureText(choiceText, charSize);
		const float lineHeight = textMgr.getLineSpacing(charSize);
		const float rowX = std::max(0.f, static_cast<float>(config.windowSize.x) - measured.x);
		const float startY = std::max(0.f, (static_cast<float>(config.windowSize.y) - measured.y) / 2.f);
		//按"实际绘制行号"映射：标题内部的 \n 也会占行，若用逻辑行下标会导致整体错位
		std::size_t drawIndex = 0;
		for (const auto& [text, optIdx] : lines) {
			const std::size_t internalNL = static_cast<std::size_t>(
				std::count(text.begin(), text.end(), '\n'));
			const std::size_t drawRows = internalNL + 1;
			if (optIdx.has_value()) {
				choiceRows.push_back({ *optIdx,
					sf::FloatRect({ rowX, startY + static_cast<float>(drawIndex) * lineHeight },
						{ measured.x, static_cast<float>(drawRows) * lineHeight }) });
			}
			drawIndex += drawRows;
		}
		if (bpHoverIndex.has_value()) {
			for (const auto& row : choiceRows) {
				if (row.optionIndex != *bpHoverIndex) continue;
				sf::RectangleShape highlight(row.bounds.size);
				highlight.setPosition(row.bounds.position);
				highlight.setFillColor(sf::Color(190, 220, 255, 150));
				window->draw(highlight);
				break;
			}
		}
	}

	displayTextInRight(choiceText);
}

void GameRenderer::renderInfoBox() {
	if (!infoBoxPlayerId.has_value()) return;

	//从缓存获取角色信息全文（服务器预格式化：角色名（等级）\n技能：\n...）
	const std::string& ciCache = charInfoCache[infoBoxPlayerId.value()];
	if (ciCache.empty()) return;

	//边框与文本参数
	const float boxWidth = 700.f;
	const float sidePad = 30.f;
	const float topPad = 20.f;
	const float bottomPad = 20.f;
	const float maxWidth = boxWidth - 2 * sidePad;
	const sf::Vector2f textSize = { 24, 28 };
	const unsigned int charSize = static_cast<unsigned int>(textSize.y);

	//缓存判断：仅当切换角色时重算
	if (infoBoxCache.playerId != infoBoxPlayerId.value()) {
		std::string infoText = textMgr.wrapText(
			ciCache, maxWidth, textSize);

		const sf::Vector2f measured = textMgr.measureText(infoText, charSize);

		infoBoxCache.playerId = infoBoxPlayerId.value();
		infoBoxCache.text = std::move(infoText);
		infoBoxCache.boxHeight = measured.y + topPad + bottomPad;
	}

	//绘制（使用缓存）
	const sf::Vector2f boxPos = {
		(config.windowSize.x - boxWidth) / 2.f,
		(config.windowSize.y - infoBoxCache.boxHeight) / 2.f
	};
	sf::RectangleShape box({ boxWidth, infoBoxCache.boxHeight });
	box.setPosition(boxPos);
	box.setFillColor(sf::Color(255, 255, 255, 230));
	box.setOutlineColor(sf::Color::Black);
	box.setOutlineThickness(3);
	window->draw(box);

	displayText(infoBoxCache.text, { boxPos.x + sidePad, boxPos.y + topPad }, textSize);
}

void GameRenderer::renderGameInfo() {
	//一场未开始（未摸初始手牌）时不显示
	if (currentState.players.empty() || currentState.gameCount == 0) return;
	//本地玩家的局胜负
	std::size_t wins = 0, losses = 0;
	for (const auto& ps : currentState.players) {
		if (ps.id == localPlayerId) { wins = ps.wins; losses = ps.losses; break; }
	}
	displayTextInUpRight(std::format(
		"第{}局\n胜{}局，负{}局",
		currentState.gameCount, wins, losses
	));
}

void GameRenderer::displayText(const std::string& text,
							   const sf::Vector2f& pos,
							   const sf::Vector2f& size,
							   const sf::Color& color) {
	textMgr.displayText(text, pos, size, color);
}

void GameRenderer::displayTextInCenter(const std::string& text,
									   const sf::Vector2f& size,
									   const sf::Color& color) {
	textMgr.displayTextInCenter(text, size, color);
}

void GameRenderer::displayTextInRight(const std::string& text,
									  const sf::Vector2f& size,
									  const sf::Color& color) {
	textMgr.displayTextInRight(text, size, color);
}

void GameRenderer::displayTextInUpRight(const std::string& text,
										const sf::Vector2f& size,
										const sf::Color& color) {
	textMgr.displayTextInUpRight(text, size, color);
}

void GameRenderer::displayTextTopRightCorner(const std::string& text,
											 const sf::Vector2f& pos,
											 const sf::Vector2f& size,
											 const sf::Color& color) {
	textMgr.displayTextTopRightCorner(text, pos, size, color);
}

void GameRenderer::displayTextInLeft(const std::string& text,
									 const sf::Vector2f& size,
									 const sf::Color& color) {
	textMgr.displayTextInLeft(text, size, color);
}

void GameRenderer::displayImage(const std::string& path, const sf::Vector2f& pos, const sf::Vector2f& size) {
	imageMgr.displayImage(path, pos, size);
}

void GameRenderer::displayImageInCenter(const std::string& path, const sf::Vector2f& size) {
	const sf::Vector2f pos = {
		config.windowSize.x / 2 - size.x / 2,
		config.windowSize.y / 2 - size.y / 2,
	};
	displayImage(path, pos, size);
}

void GameRenderer::displayCard(const Card& card, const sf::Vector2f& pos, const sf::Vector2f& cardSize) {
	displayImage(card.getImagePath(), pos, cardSize);
}

void GameRenderer::displayCardInCenter(const Card& card, const sf::Vector2f& cardSize) {
	displayImageInCenter(card.getImagePath(), cardSize);
}

void GameRenderer::displayHand(const Hand& hand, const sf::Vector2f& pos, const sf::Vector2f& cardSize,
							   const sf::Vector2f& pointerSize, const bool canSelect) {
	bool displayPointer = pointerSize != sf::Vector2f{ 0, 0 };
	const std::size_t foldCardWidth = static_cast<std::size_t>(cardSize.x / 3);
	std::size_t dx = 0;
	std::size_t selectedPos = 0;
	for (std::size_t i = 0; i < hand.count(); ++i) {
		displayCard(hand[i], { pos.x + dx, pos.y }, cardSize);
		if (displayPointer && hand.getSelectedIndex() == i) {
			selectedPos = dx;
			dx += static_cast<std::size_t>(cardSize.x);
		}
		else {
			dx += foldCardWidth;
		}
	}
	if (displayPointer && !hand.empty() && canSelect) {
		displayImage(
			"images/cards/pointer/默认.jpg",
			{ pos.x + selectedPos + cardSize.x / 2 - pointerSize.x / 2,
			pos.y + cardSize.y },
			pointerSize
		);
	}
}

bool GameRenderer::windowIsOpen() const {
	return window->isOpen();
}

void GameRenderer::closeWindow() {
	window->close();
}

std::optional<sf::Event> GameRenderer::pollEvent() {
	return window->pollEvent();
}

void GameRenderer::setChoicePrompt(const Choice& prompt) {
	const bool isSameAsk = choice.has_value()
		&& choice->title == prompt.title
		&& choice->forced == prompt.forced
		&& choice->totalPages == prompt.totalPages;
	if (isSameAsk) {
		// 翻页 / 错误提示更新：保留原有倒计时，仅刷新可变字段
		choice->options = prompt.options;
		choice->currentPage = prompt.currentPage;
		choice->totalPages = prompt.totalPages;
		choice->errorMsg = prompt.errorMsg;
		choice->optionMeta = prompt.optionMeta;
		// timeoutMs / title / forced 保持不变
	}
	else {
		// 新的 ask：完整赋值并重启倒计时
		choice = prompt;
		countdownClock.restart();
	}
	//换页 / 换一批后旧悬停位置失效，重新排队技能描述请求
	bpHoverIndex = std::nullopt;
	queueSkillRequests();
}

void GameRenderer::clearChoicePrompt() {
	choice = std::nullopt;
	bpHoverIndex = std::nullopt;
	choiceRows.clear();
}

void GameRenderer::handleMouseClick(const sf::Vector2f& mousePos) {
	for (const auto& playerState : currentState.players) {
		sf::Vector2f charPos = { 0,0 };
		if (playerState.id == 0) charPos = { 0, 0 };
		else if (playerState.id == 1) charPos = { 0, config.windowSize.y - config.characterSize.y };
		else continue;

		//组合角色多张图横向并排，点击区域覆盖全部
		const float charTotalWidth = static_cast<float>(playerState.characterNames.size()) * config.characterSize.x;
		sf::FloatRect charBounds(charPos, { charTotalWidth, config.characterSize.y });
		if (charBounds.contains(mousePos)) {
			//再次点击同一角色则关闭
			if (infoBoxPlayerId.has_value() && infoBoxPlayerId.value() == playerState.id) {
				infoBoxPlayerId = std::nullopt;
			}
			else {
				infoBoxPlayerId = playerState.id;
			}
			return;
		}
	}
	//点击角色图片外区域则关闭
	infoBoxPlayerId = std::nullopt;
}

// ===== BP 阶段候选角色预览 =====

void GameRenderer::renderBpPreview() {
	if (!choice.has_value() || !bpHoverIndex.has_value()) return;
	const auto& metas = choice->optionMeta;
	if (*bpHoverIndex >= metas.size()) return;
	const auto& meta = metas[*bpHoverIndex];
	if (meta.characterName.empty()) return;

	// ===== 布局参数 =====
	const float pad = 20.f;          // 面板内边距
	const float gap = 8.f;           // 文本行间距
	const float imgW = 210.f;
	const float imgH = 290.f;
	const float infoX = pad + imgW + 26.f;   // 基础信息相对面板的起始 x
	const float infoW = 360.f;               // 基础信息/技能文本宽度
	const float contentW = imgW + 26.f + infoW;
	const float panelW = pad * 2.f + contentW;

	// 字号（相比初版调大）
	const float baseChar = 22.f, baseCharH = 40.f;   // 基础信息
	const unsigned int descChar = 18, descCharH = 32;   // 技能全文

	const float infoLineH = baseCharH + gap;
	const float infoBlockH = 4.f * infoLineH;        // 角色/等级/体力/皮肤 四行

	const float topH = pad + std::max(imgH, infoBlockH);   // 立绘区 + 信息区高度
	const float descTop = topH + 16.f;                     // 技能区顶部（相对面板）
	const float descTitleH = baseCharH + gap;

	// 技能全文：服务端 SkillInfoResponse 生成，按内容宽度整段折行后逐行绘制
	const auto cached = bpSkillsCache.find(meta.characterName);
	const std::string rawSkills = (cached != bpSkillsCache.end())
		? cached->second : std::string("技能描述加载中...");
	const sf::Vector2f descSize = { descChar, descCharH };
	const std::string wrapped = textMgr.wrapText(rawSkills, contentW, descSize);
	const std::size_t descLines = 1u + static_cast<std::size_t>(
		std::count(wrapped.begin(), wrapped.end(), '\n'));

	// 技能行距：在字体默认行距基础上额外增大，便于阅读
	const float descLineH = textMgr.getLineSpacing(descChar) + 10.f;

	// 面板高度与信息量匹配：内容所需高度，封顶到窗口高度的一部分，避免空旷或溢出
	const float contentH = descTop + descTitleH
		+ static_cast<float>(descLines) * descLineH + pad;
	const float maxH = static_cast<float>(config.windowSize.y) * 0.82f;
	const float panelH = std::min(contentH, maxH);
	// 居中
	const float panelX = (static_cast<float>(config.windowSize.x) - panelW) / 2.f;
	const float panelY = (static_cast<float>(config.windowSize.y) - panelH) / 2.f;

	// 面板背景
	sf::RectangleShape panel({ panelW, panelH });
	panel.setPosition({ panelX, panelY });
	panel.setFillColor(sf::Color(255, 255, 255, 248));
	panel.setOutlineColor(sf::Color::Black);
	panel.setOutlineThickness(2.f);
	window->draw(panel);

	// 立绘：路径由服务端下发，客户端只读 images/ 图片文件
	if (!meta.imagePath.empty()) {
		displayImage(meta.imagePath, { panelX + pad, panelY + pad }, { imgW, imgH });
	}

	// 基础信息：全部来自服务端元数据，客户端不查本地角色表
	const sf::Vector2f baseSize = { baseChar, baseCharH };
	float ty = panelY + pad;
	const float tx = panelX + infoX;
	displayText(std::format("{}（{}）", meta.characterName, meta.level), { tx, ty }, baseSize);
	ty += infoLineH;
	displayText(std::format("体力：{}", meta.hp), { tx, ty }, baseSize);

	// 技能全文：逐行绘制，超出面板高度则截断（行距 descLineH 已增大）
	const float maxY = panelY + panelH - pad;
	float sy = panelY + descTop;
	displayText("技能：", { panelX + pad, sy }, { baseChar, baseCharH });
	sy += descTitleH;

	bool truncated = false;
	std::size_t start = 0;
	while (start < wrapped.size()) {
		if (sy + descLineH > maxY) { truncated = true; break; }
		const std::size_t end = wrapped.find('\n', start);
		const std::string line = wrapped.substr(start,
			end == std::string::npos ? std::string::npos : end - start);
		displayText(line, { panelX + pad, sy }, descSize);
		sy += descLineH;
		if (end == std::string::npos) break;
		start = end + 1;
	}
	if (truncated) {
		const float noteY = (sy + descLineH > maxY) ? maxY - descLineH : sy;
		displayText("…（技能描述过长，已截断）", { panelX + pad, noteY }, descSize);
	}
}

void GameRenderer::handleMouseMove(const sf::Vector2f& mousePos) {
	std::optional<std::size_t> hit;
	for (const auto& row : choiceRows) {
		if (row.bounds.contains(mousePos)) {
			hit = row.optionIndex;
			break;
		}
	}
	bpHoverIndex = hit;
}

void GameRenderer::setSkillInfo(const std::string& name, const std::string& text) {
	bpSkillsCache[name] = text;
	bpPendingSkills.erase(name);
}

std::vector<std::string> GameRenderer::takePendingSkillNames() {
	std::vector<std::string> names;
	for (const auto& name : bpSkillRequestQueue) {
		if (bpSkillsCache.contains(name) || bpPendingSkills.contains(name)) continue;
		bpPendingSkills.insert(name);
		names.push_back(name);
	}
	bpSkillRequestQueue.clear();
	return names;
}

void GameRenderer::queueSkillRequests() {
	if (!choice.has_value()) return;
	for (const auto& meta : choice->optionMeta) {
		if (meta.characterName.empty()) continue;
		if (bpSkillsCache.contains(meta.characterName) || bpPendingSkills.contains(meta.characterName)) continue;
		bpSkillRequestQueue.push_back(meta.characterName);
	}
}

