#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include <tuple>
#include <cstdint>
#include <cstring>
#include <functional>

class TextManager {
private:
	sf::Font font;
	sf::RenderWindow& window;

	//measureText 缓存：(text, charSize) -> 实际宽高
	struct MeasureKey {
		std::wstring text;
		unsigned int charSize;
		bool operator==(const MeasureKey& o) const noexcept {
			return charSize == o.charSize && text == o.text;
		}
	};
	struct MeasureKeyHash {
		std::size_t operator()(const MeasureKey& k) const noexcept;
	};
	mutable std::unordered_map<MeasureKey, sf::Vector2f, MeasureKeyHash> measureCache;

	//displayText 缓存：(text, charSize, color) -> sf::Text
	struct DisplayKey {
		std::wstring text;
		unsigned int charSize;
		sf::Color color;
		bool operator==(const DisplayKey& o) const noexcept {
			return charSize == o.charSize && color == o.color && text == o.text;
		}
	};
	struct DisplayKeyHash {
		std::size_t operator()(const DisplayKey& k) const noexcept;
	};
	mutable std::unordered_map<DisplayKey, sf::Text, DisplayKeyHash> textCache;

	//从缓存取（或创建）一次文本对象，用作 displayText 内部工具
	//返回 sf::Text 引用，调用方再 setPosition/draw
	sf::Text& acquireText(const std::wstring& text,
						  unsigned int charSize,
						  const sf::Color& color) const;

public:
	//获取字体行间距
	float getLineSpacing(unsigned int charSize) const;
	//直接用字体测量多行文本的实际像素尺寸（使用给定 charSize，带缓存）
	sf::Vector2f measureText(const std::wstring& text, unsigned int charSize) const;
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
