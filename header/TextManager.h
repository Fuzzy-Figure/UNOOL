#pragma once
#include <SFML/Graphics.hpp>

class TextManager {
private:
	sf::Font font;
	sf::RenderWindow& window;

public:

	sf::Vector2f getActualTextSize(const std::wstring& text,
								   const sf::Vector2f& scale) const;
	//获取字体行间距
	float getLineSpacing(unsigned int charSize) const { return font.getLineSpacing(charSize); }
	//直接用字体测量多行文本的实际像素尺寸（使用给定 charSize）
	sf::Vector2f measureText(const std::wstring& text, unsigned int charSize) const {
		sf::Text temp(font, sf::String::fromUtf32(text.begin(), text.end()), charSize);
		const sf::FloatRect bounds = temp.getLocalBounds();
		return { bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y };
	}
	//按最大宽度自动折行，返回含 \n 的文本
	std::wstring wrapText(const std::wstring& text, float maxWidth,
						  const sf::Vector2f& size) const;
	//构造函数
	TextManager(sf::RenderWindow& _window);
	//显示文本
	void displayText(const std::wstring& text,
					 const sf::Vector2f& pos,
					 const sf::Vector2f& size = { 20,40 },
					 const sf::Color& color = sf::Color::Black);
	void displayTextInCenter(const std::wstring& text,
							 const sf::Vector2f& size = { 20,40 },
							 const sf::Color& color = sf::Color::Black);
	void displayTextInRight(const std::wstring& text,
							const sf::Vector2f& size = { 20,40 },
							const sf::Color& color = sf::Color::Black);
	void displayTextInUpRight(const std::wstring& text,
							  const sf::Vector2f& size = { 20,40 },
							  const sf::Color& color = sf::Color::Black);
	void displayTextInLeft(const std::wstring& text,
						   const sf::Vector2f& size = { 20,40 },
						   const sf::Color& color = sf::Color::Black);
};