#include "../header/TextManager.h"
#include "../header/utils.h"
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
	for (wchar_t c : k.text) {
		h ^= std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(c)) + 0x9e3779b9u + (h << 6) + (h >> 2);
	}
	return h;
}

std::size_t TextManager::DisplayKeyHash::operator()(const DisplayKey& k) const noexcept {
	std::size_t h = std::hash<unsigned int>{}(k.charSize);
	h ^= std::hash<std::uint32_t>{}(k.color.toInteger()) + 0x9e3779b9u + (h << 6) + (h >> 2);
	for (wchar_t c : k.text) {
		h ^= std::hash<std::uint32_t>{}(static_cast<std::uint32_t>(c)) + 0x9e3779b9u + (h << 6) + (h >> 2);
	}
	return h;
}

// ===== 文本/尺寸缓存：避免每帧重新 sf::Text + glyph 计算 =====
sf::Text& TextManager::acquireText(const std::wstring& text,
								   unsigned int charSize,
								   const sf::Color& color) const {
	DisplayKey key{ text, charSize, color };
	auto it = textCache.find(key);
	if (it != textCache.end()) return it->second;
	//未命中：构造并插入缓存
	sf::Text t(font, sf::String::fromUtf32(text.begin(), text.end()), charSize);
	t.setFillColor(color);
	auto [ins, ok] = textCache.emplace(std::move(key), std::move(t));
	return ins->second;
}


//获取字体行间距
float TextManager::getLineSpacing(unsigned int charSize) const {
	return font.getLineSpacing(charSize);
}

sf::Vector2f TextManager::measureText(const std::wstring& text, unsigned int charSize) const {
	MeasureKey key{ text, charSize };
	auto it = measureCache.find(key);
	if (it != measureCache.end()) return it->second;
	//未命中：按真实 sf::Text 测量并存缓存
	sf::Text temp(font, sf::String::fromUtf32(text.begin(), text.end()), charSize);
	const sf::FloatRect bounds = temp.getLocalBounds();
	const sf::Vector2f res{ bounds.position.x + bounds.size.x,
							bounds.position.y + bounds.size.y };
	measureCache.emplace(std::move(key), res);
	return res;
}

// 按最大宽度自动折行（用 font.getGlyph 累加 advance，O(n)）
std::wstring TextManager::wrapText(const std::wstring& text, float maxWidth,
								   const sf::Vector2f& size) const {
	const unsigned int charSize = static_cast<unsigned int>(size.y);
	std::wstring result;
	std::wstring line;
	float width = 0.f;

	for (wchar_t ch : text) {
		if (ch == L'\n') {
			result += line + L'\n';
			line.clear();
			width = 0.f;
			continue;
		}
		const float adv = font.getGlyph(static_cast<char32_t>(ch), charSize, false).advance;
		if (width + adv > maxWidth && !line.empty()) {
			//当前字符导致超宽：换行，当前字符作为新行开头
			result += line + L'\n';
			line = ch;
			width = adv;
		}
		else {
			line += ch;
			width += adv;
		}
	}
	result += line;
	return result;
}


void TextManager::displayText(const std::wstring& text,
							  const sf::Vector2f& pos,
							  const sf::Vector2f& size,
							  const sf::Color& color) {
	sf::Text& sfText = acquireText(text, static_cast<unsigned int>(size.y), color);
	sfText.setPosition(pos);
	window.draw(sfText);
}



void TextManager::displayTextInCenter(const std::wstring& text,
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

void TextManager::displayTextInRight(const std::wstring& text,
									 const sf::Vector2f& size,
									 const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();
	constexpr float margin = 20.0f;
	const float x = windowSize.x - actualSize.x - margin;
	const float y = (windowSize.y - actualSize.y) / 2.f;

	displayText(text, { std::max(0.f, x), std::max(0.f, y) }, size, color);
}
void TextManager::displayTextInUpRight(const std::wstring& text,
									   const sf::Vector2f& size,
									   const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();
	constexpr float margin = 20.0f;
	const float x = windowSize.x - actualSize.x - margin;

	displayText(text, { std::max(0.f, x), 0 }, size, color);
}

void TextManager::displayTextInLeft(const std::wstring& text,
									const sf::Vector2f& size,
									const sf::Color& color) {
	const sf::Vector2f actualSize = measureText(text, static_cast<unsigned int>(size.y));
	const sf::Vector2u windowSize = window.getSize();
	constexpr float margin = 20.0f;
	const float y = (windowSize.y - actualSize.y) / 2.f;

	displayText(text, { margin, std::max(0.f, y) }, size, color);
}
