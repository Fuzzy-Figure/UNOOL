#pragma once
#include <android/asset_manager.h>

#include <format>
#include <json.hpp>
#include <map>
#include <print>
#include <string>
#include <vector>

#include "core/IAssetCatalog.h"

namespace unool::android {

	// 安卓服务端资源目录：APK 内只带 character_skins.json 皮肤名清单（很小），
	// 不带图片本体。图片由客户端 assets 提供，加载失败由客户端上报并在服务端打印。
	class AndroidAssetCatalog final : public core::IAssetCatalog {
	public:
		explicit AndroidAssetCatalog(AAssetManager* manager) : manager_(manager) {
			loadInventory();
		}

		std::vector<std::string> skinsOf(const std::string& group,
										 const std::string& name) const override {
			const auto it = inventory_.find(group + "/" + name);
			if (it == inventory_.end()) return { "默认" };
			return it->second;
		}

		std::string imagePath(const std::string& group, const std::string& name,
							  const std::string& skin) const override {
			return std::format("images/characters/{}/{}/{}.jpg", group, name, skin);
		}

	private:
		void loadInventory() {
			if (manager_ == nullptr) return;
			AAsset* asset = AAssetManager_open(manager_, "character_skins.json", AASSET_MODE_BUFFER);
			if (asset == nullptr) {
				std::println(stderr, "[AndroidAssetCatalog] assets 缺少 character_skins.json");
				return;
			}
			const auto length = static_cast<std::size_t>(AAsset_getLength(asset));
			std::string text(length, '\0');
			if (length > 0) {
				AAsset_read(asset, text.data(), length);
			}
			AAsset_close(asset);
			try {
				const auto json = nlohmann::json::parse(text);
				for (auto it = json.begin(); it != json.end(); ++it) {
					inventory_.emplace(it.key(), it.value().get<std::vector<std::string>>());
				}
			}
			catch (const std::exception& e) {
				std::println(stderr, "[AndroidAssetCatalog] 解析 character_skins.json 失败：{}", e.what());
			}
		}

		AAssetManager* manager_;
		std::map<std::string, std::vector<std::string>> inventory_;
	};

}
