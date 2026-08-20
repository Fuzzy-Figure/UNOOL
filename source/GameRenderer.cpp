#include "../header/GameRenderer.h"
#include "../header/Card.h"
#include "../header/Character.h"
#include "../header/Skill.h"
#include <algorithm>
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

bool GameRenderer::isChoiceActive() const {
	return choice.has_value();
}

bool GameRenderer::hasChoiceOptions() const {
	return choice.has_value() && !choice->options.empty();
}

bool GameRenderer::isLocalTurn() const {
	return !currentState.players.empty()
		&& currentState.players[currentState.currentPlayerIndex].id == localPlayerId;
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
	renderInfoBox();
	window->display();
}

void GameRenderer::renderPlayers() {
	for (const auto& playerState : currentState.players) {
		//角色
		sf::Vector2f charPos = { 0,0 };
		if (playerState.id == 0) charPos = { 0,0 };
		else if (playerState.id == 1) charPos = { 0,config.windowSize.y - config.characterSize.y };
		displayImage(Character::getImagePath(playerState.characterName, playerState.skin), charPos, config.characterSize);

		//座次编号
		if (!currentState.seatOrder.empty()) {
			std::size_t seat = currentState.seatOrder[playerState.id];
			std::wstring seatText = (seat == 0) ? L"一号位" : L"二号位";
			sf::Vector2f seatPos;
			if (playerState.id == 0) {
				seatPos = { charPos.x, charPos.y + config.characterSize.y };
			}
			else {
				seatPos = { charPos.x, charPos.y - 50 };
			}
			displayText(seatText, seatPos);
		}

		//体力值
		std::wstring hpText = L"体力：" + std::to_wstring(playerState.hp) + L"/" + std::to_wstring(playerState.maxHp);
		sf::Vector2f hpPos;
		if (playerState.id == 0) {
			hpPos = { charPos.x, charPos.y + config.characterSize.y + 50 };
		}
		else {
			hpPos = { charPos.x, charPos.y - 100 };
		}
		displayText(hpText, hpPos);

		//手牌
		const sf::Vector2f& handDisplayPos =
			playerState.id == 0 ?
			sf::Vector2f{ config.characterSize.x, 0 } :
			sf::Vector2f{ config.characterSize.x, config.windowSize.y - config.characterSize.y };

		bool isCurrentPlayer = currentState.players[currentState.currentPlayerIndex].id == playerState.id;
		bool isLocalPlayer = playerState.id == localPlayerId;
		bool canSelect = isLocalPlayer && !hasChoiceOptions() && (isLocalTurn() || isChoiceActive());

		playerState.hand.display(*this, handDisplayPos, config.cardSize, isLocalPlayer ? config.pointerSize : sf::Vector2f{ 0, 0 }, canSelect);
	}
}

void GameRenderer::renderDiscardPile() {
	if (!currentState.discardPile.empty()) {
		const Card& lastCard = currentState.discardPile.front();
		sf::Vector2f currentCardPos{ config.windowSize.x / 2.0f - config.cardSize.x / 2,
									 config.windowSize.y / 2.0f - config.cardSize.y / 2 };
		lastCard.display(*this, currentCardPos, config.cardSize);

		// 历史牌：缩小至 60%，紧邻当前牌左侧从新到旧排列（向左展开），垂直居中
		sf::Vector2f historySize{ config.cardSize.x * 0.6f, config.cardSize.y * 0.6f };
		sf::Vector2f historyPos{ config.windowSize.x / 2.0f - config.cardSize.x / 2 - historySize.x,
								 config.windowSize.y / 2.0f - historySize.y / 2 };
		size_t historyCount = currentState.discardPile.size() - 1;
		if (historyCount > 3) historyCount = 3;
		for (size_t i = 1; i <= historyCount; ++i) {
			currentState.discardPile[i].display(*this, historyPos, historySize);
			historyPos.x -= historySize.x;
		}
	}
	displayTextInLeft(L"当前颜色：" + Card::to_wstring(currentState.currentColor) + L'\n' +
					  L"当前牌名：" + Card::to_wstring(currentState.currentName));
}

void GameRenderer::renderChoice() {
	if (!choice.has_value()) return;

	std::wstring choiceText;
	if (!choice->title.empty()) {
		choiceText += choice->title + L'\n';
	}
	for (const auto& [i, option] : choice->options | std::views::enumerate) {
		choiceText += (L"[" + std::to_wstring(i + 1) + L"] " + option + L'\n');
	}
	if (!choice->options.empty()) {
		if (choice->forced) {
			if (choice->totalPages > 1) {
				choiceText += (L"第 " + std::to_wstring(choice->currentPage + 1) + L"/" + std::to_wstring(choice->totalPages) + L"页，<-->翻页\n");
				choiceText += (L"输入数字1-" + std::to_wstring(choice->options.size()) + L"选择（必须选择）" + L'\n');
			}
			else {
				choiceText += (L"输入数字1-" + std::to_wstring(choice->options.size()) + L"选择（必须选择）" + L'\n');
			}
		}
		else {
			if (choice->totalPages > 1) {
				choiceText += (L"第 " + std::to_wstring(choice->currentPage + 1) + L"/" + std::to_wstring(choice->totalPages) + L"页，<-->翻页\n");
				choiceText += (L"输入数字0-" + std::to_wstring(choice->options.size()) + L"选择（0表示不选择）" + L'\n');
			}
			else {
				choiceText += (L"输入数字0-" + std::to_wstring(choice->options.size()) + L"选择（0表示不选择）" + L'\n');
			}
		}
	}
	if (!choice->errorMsg.empty()) {
		choiceText += choice->errorMsg + L'\n';
	}
	if (choice->timeoutMs.has_value()) {
		float elapsedMs = static_cast<float>(countdownClock.getElapsedTime().asMilliseconds());
		float remainingMs = static_cast<float>(choice->timeoutMs.value()) - elapsedMs;
		if (remainingMs < 0.f) remainingMs = 0.f;
		float remainingSec = remainingMs / 1000.f;
		int intPart = static_cast<int>(remainingSec);
		int decPart = static_cast<int>((remainingSec - intPart) * 10.f);
		choiceText += L"剩余时间：" + std::to_wstring(intPart) + L"." + std::to_wstring(decPart) + L" 秒" + L'\n';
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
		std::wstring infoText = textMgr.wrapText(
			unool::string::to_utf16(ciCache), maxWidth, textSize);

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

void GameRenderer::displayText(const std::wstring& text,
							   const sf::Vector2f& pos,
							   const sf::Vector2f& size,
							   const sf::Color& color) {
	textMgr.displayText(text, pos, size, color);
}

void GameRenderer::displayTextInCenter(const std::wstring& text,
									   const sf::Vector2f& size,
									   const sf::Color& color) {
	textMgr.displayTextInCenter(text, size, color);
}

void GameRenderer::displayTextInRight(const std::wstring& text,
									  const sf::Vector2f& size,
									  const sf::Color& color) {
	textMgr.displayTextInRight(text, size, color);
}

void GameRenderer::displayTextInUpRight(const std::wstring& text,
										const sf::Vector2f& size,
										const sf::Color& color) {
	textMgr.displayTextInUpRight(text, size, color);
}

void GameRenderer::displayTextInLeft(const std::wstring& text,
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
	imageMgr.displayImage(path, pos, size);
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
	choice = prompt;
	countdownClock.restart();
}

void GameRenderer::clearChoicePrompt() {
	choice = std::nullopt;
}

void GameRenderer::handleMouseClick(const sf::Vector2f& mousePos) {
	for (const auto& playerState : currentState.players) {
		sf::Vector2f charPos = { 0,0 };
		if (playerState.id == 0) charPos = { 0, 0 };
		else if (playerState.id == 1) charPos = { 0, config.windowSize.y - config.characterSize.y };
		else continue;

		sf::FloatRect charBounds(charPos, config.characterSize);
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

