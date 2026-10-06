#include "ImageManager.h"
#include "utils.h"

#include <fstream>


ImageManager::ImageManager(sf::RenderWindow& _window) :window(_window) {
	UNOOL = std::format("{}/", std::filesystem::current_path().parent_path().string());
	std::println("UNOOL路径：{}", UNOOL);
}

// 显示图片
void ImageManager::displayImage(const std::string& path, const sf::Vector2f& pos, const sf::Vector2f& size) {
	if (!window.isOpen()) {
		throw std::runtime_error("[ImageManager] 窗口无效");
	}

	const std::string absolutePath = UNOOL + path;
	//缓存未命中
	if (auto it = textureCache.find(absolutePath); it == textureCache.end()) {
		auto texture = std::make_unique<sf::Texture>();
		if (!texture->loadFromFile(unool::string::to_utf16(absolutePath))) {
			//回退到默认图片
			const std::string defaultPath = std::format("{}images/default.jpg", UNOOL);
			if (!texture->loadFromFile(unool::string::to_utf16(defaultPath))) {
				throw std::runtime_error(std::format("[ImageManager] 纹理加载失败，路径：{}", absolutePath));
			}
			std::println("[ImageManager] 纹理加载失败，路径：{}，使用默认图片", absolutePath);
		}
		else {
			std::println("[ImageManager] 纹理加载成功，路径：{}，尺寸：{}*{}", absolutePath, texture->getSize().x, texture->getSize().y);
		}
		textureCache.emplace(absolutePath, std::move(texture));
	}

	// 直接从缓存取，此时一定存在
	sf::Texture& texture = *textureCache[absolutePath];

	sf::Sprite sprite(texture);
	sprite.setPosition(pos);

	sf::Vector2u textureSize = texture.getSize();
	float scaleX = size.x / static_cast<float>(textureSize.x);
	float scaleY = size.y / static_cast<float>(textureSize.y);
	sprite.setScale({ scaleX, scaleY });

	window.draw(sprite);
}

// 获取纹理原始尺寸
sf::Vector2u ImageManager::getTextureSize(const std::string& path) {
	const std::string absolutePath = UNOOL + path;
	if (auto it = textureCache.find(absolutePath); it == textureCache.end()) {
		auto texture = std::make_unique<sf::Texture>();
		if (!texture->loadFromFile(unool::string::to_utf16(absolutePath))) {
			//回退到默认图片
			const std::string defaultPath = std::format("{}images/default.jpg", UNOOL);
			if (!texture->loadFromFile(unool::string::to_utf16(defaultPath))) {
				throw std::runtime_error(std::format("[ImageManager] 纹理加载失败，路径：{}", absolutePath));
			}
		}
		textureCache.emplace(absolutePath, std::move(texture));
	}
	return textureCache[absolutePath]->getSize();
}







