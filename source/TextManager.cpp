#include "TextManager.h"
#include "utils.h"
#include <iostream>

TextManager::TextManager(sf::RenderWindow& _window)
	:window(_window) {
	const auto& config = unool::getClientConfig();
	if (!font.openFromFile(config["fonts"])) {
		std::cout << "错误：加载字体失败" << std::endl;
	}
}
std::size_t TextManager::MeasureKeyHash::operator()(const MeasureKey& k) const noexcept {
	std::size_t h = std::hash<unsigned int>{}(k.charSize);
	for (char c : k.text) {
		h ^= std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(c)) + 0x9e3779b9u + (h << 6) + (h >> 2);
	}
	return h;
}

std::size_t TextManager::DisplayKeyHash::operator()(const DisplayKey& k) const noexcept {
	std::size_t h = std::hash<unsigned int>{}(k.charSize);
	h ^= std::hash<std::uint32_t>{}(k.color.toInteger()) + 0x9e3779b9u + (h << 6) + (h >> 2);
	for (char c : k.text) {
		h ^= std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(c)) + 0x9e3779b9u + (h << 6) + (h >> 2);
	}
	return h;
}

// ===== 文本/尺寸缓存：避免每帧重新 sf::Text + glyph 计算 =====
sf::Text& TextManager::acquireText(const std::string& text,
								   unsigned int charSize,
								   const sf::Color& color) const {
	DisplayKey key{ text, charSize, color };
	auto it = textCache.find(key);
	if (it != textCache.end()) return it->second;
	//未命中：构造并插入缓存
	sf::Text t(font, sf::String::fromUtf8(text.begin(), text.end()), charSize);
	t.setFillColor(color);
	auto [ins, ok] = textCache.emplace(std::move(key), std::move(t));
	return ins->second;
}


//获取字体行间距
float TextManager::getLineSpacing(unsigned int charSize) const {
	return font.getLineSpacing(charSize);
}

sf::Vector2f TextManager::measureText(const std::string& text, unsigned int charSize) const {
	MeasureKey key{ text, charSize };
	auto it = measureCache.find(key);
	if (it != measureCache.end()) return it->second;
	//未命中：按真实 sf::Text 测量并存缓存
	sf::Text temp(font, sf::String::fromUtf8(text.begin(), text.end()), charSize);
	const sf::FloatRect bounds = temp.getLocalBounds();
	const sf::Vector2f res{ bounds.position.x + bounds.size.x,
							bounds.position.y + bounds.size.y };
	measureCache.emplace(std::move(key), res);
	return res;
}

// 从 UTF-8 中取出下一个完整字符，返回 {码位, 字节数}
static std::pair<char32_t, std::size_t> decodeUtf8(const std::string& s, std::size_t i) {
	const unsigned char c0 = static_cast<unsigned char>(s[i]);
	std::size_t len = 1; char32_t cp = c0;
	if ((c0 & 0xE0) == 0xC0) { len = 2; cp = c0 & 0x1Fu; }
	else if ((c0 & 0xF0) == 0xE0) { len = 3; cp = c0 & 0x0Fu; }
	else if ((c0 & 0xF8) == 0xF0) { len = 4; cp = c0 & 0x07u; }
	else if ((c0 & 0xC0) == 0x80) return { 0xFFFD, 1 };          // 孤立续字节
	len = std::min(len, s.size() - i);
	for (std::size_t k = 1; k < len; ++k) {
		const unsigned char c = static_cast<unsigned char>(s[i + k]);
		if ((c & 0xC0) != 0x80) return { 0xFFFD, 1 };            // 序列不完整
		cp = (cp << 6) | (c & 0x3Fu);
	}
	return { cp, len };
}

std::string TextManager::wrapText(const std::string& text, float maxWidth,
								  const sf::Vector2f& size) const {
	const unsigned int charSize = static_cast<unsigned int>(size.y);
	std::string result, line;
	float width = 0.f;

	for (std::size_t i = 0; i < text.size(); ) {
		const auto [cp, len] = decodeUtf8(text, i);
		const std::string ch = text.substr(i, len);
		i += len;
		if (cp == U'\n') { result += line + '\n'; line.clear(); width = 0.f; continue; }

		const float adv = font.getGlyph(cp, charSize, false).advance;  // 按码位量，不再是字节
		if (width + adv > maxWidth && !line.empty()) {
			result += line + '\n';
			line = ch; width = adv;
		}
		else { line += ch; width += adv; }
	}
	result += line;
	return result;
}

void TextManager::displayText(const std::string& text,
							  const sf::Vector2f& pos,
							  const sf::Vector2f& size,
							  const sf::Color& color) {
	sf::Text& sfText = acquireText(text, static_cast<unsigned int>(size.y), color);
	sfText.setPosition(pos);
	window.draw(sfText);
}



void TextManager::displayTextInCenter(const std::string& text,
									  const sf::Vector2f& size,
									  const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();

	const sf::Vector2f pos(
		std::max(0.f, (windowSize.x - actualSize.x) / 2.f),
		std::max(0.f, (windowSize.y - actualSize.y) / 2.f)
	);

	displayText(text, pos, size, color);
}

void TextManager::displayTextInUp(const std::string& text,
								  const sf::Vector2f& size,
								  const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();

	displayText(text, { std::max(0.f, (windowSize.x - actualSize.x) / 2.f), 0.f }, size, color);
}

void TextManager::displayTextInRight(const std::string& text,
									 const sf::Vector2f& size,
									 const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();
	const float x = windowSize.x - actualSize.x;
	const float y = (windowSize.y - actualSize.y) / 2.f;

	displayText(text, { std::max(0.f, x), std::max(0.f, y) }, size, color);
}
void TextManager::displayTextInUpRight(const std::string& text,
									   const sf::Vector2f& size,
									   const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();
	const float x = windowSize.x - actualSize.x;

	displayText(text, { std::max(0.f, x), 0 }, size, color);
}

void TextManager::displayTextInLeft(const std::string& text,
									const sf::Vector2f& size,
									const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();
	const float y = (windowSize.y - actualSize.y) / 2.f;

	displayText(text, { 0, std::max(0.f, y) }, size, color);
}
