#pragma once
#include <SFML/Graphics.hpp>

class ImageManager {
private:
	sf::RenderWindow& window;
	std::string UNOOL;
	std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textureCache;

public:
	ImageManager(sf::RenderWindow& _window);

	// 显示图片，返回是否成功
	void displayImage(const std::string& path,
					  const sf::Vector2f& pos,
					  const sf::Vector2f& size);
};