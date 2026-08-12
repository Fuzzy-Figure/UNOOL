#include "../header/TextManager.h"
#include "../header/utils.h"
#include <iostream>

TextManager::TextManager(sf::RenderWindow& _window)
	:window(_window) {
	const auto& config = unool::getConfig();
	if (!font.openFromFile(config["fonts"])) {
		std::cout << "错误：加载字体失败" << std::endl;
	}
}

// 辅助函数：计算文字实际渲染尺寸
sf::Vector2f TextManager::getActualTextSize(const std::wstring& text,
											const sf::Vector2f& scale) const {
	sf::Text temp(font, text, 1);
	temp.setScale(scale);
	sf::FloatRect bounds = temp.getLocalBounds();
	return { bounds.size.x * scale.x, bounds.size.y * scale.y };
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
	sf::Text sfText(font, sf::String::fromUtf32(text.begin(), text.end()), static_cast<unsigned int>(size.y));
	sfText.setPosition(pos);
	sfText.setFillColor(color);
	window.draw(sfText);
}



void TextManager::displayTextInCenter(const std::wstring& text,
									  const sf::Vector2f& size,
									  const sf::Color& color) {
	const sf::Vector2f actualSize = getActualTextSize(text, size);
	const sf::Vector2u windowSize = window.getSize();

	const sf::Vector2f pos(
		(windowSize.x - actualSize.x) / 2.f,
		(windowSize.y - actualSize.y) / 2.f
	);

	displayText(text, pos, size, color);
}

void TextManager::displayTextInRight(const std::wstring& text,
									 const sf::Vector2f& size,
									 const sf::Color& color) {
	const sf::Vector2f actualSize = getActualTextSize(text, size);
	const sf::Vector2u windowSize = window.getSize();

	const sf::Vector2f pos(
		windowSize.x - 2 * actualSize.x,
		(windowSize.y - actualSize.y) / 2.f
	);

	displayText(text, pos, size, color);
}
void TextManager::displayTextInUpRight(const std::wstring& text,
									   const sf::Vector2f& size,
									   const sf::Color& color) {
	const sf::Vector2f actualSize = getActualTextSize(text, size);
	const sf::Vector2u windowSize = window.getSize();

	const sf::Vector2f pos(
		windowSize.x - 2 * actualSize.x,
		0
	);

	displayText(text, pos, size, color);
}

void TextManager::displayTextInLeft(const std::wstring& text,
									const sf::Vector2f& size,
									const sf::Color& color) {
	const sf::Vector2f actualSize = getActualTextSize(text, size);
	const sf::Vector2u windowSize = window.getSize();

	const sf::Vector2f pos(
		0,
		(windowSize.y - actualSize.y) / 2.f
	);

	displayText(text, pos, size, color);
}
