#pragma once
// json.hpp 必须早于 utils.h：后者在返回类型里直接用了 nlohmann::json。
#include <json.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <print>
#include <string>
#include <vector>

#include "application/IAssetCatalog.h"
#include "utils.h"

namespace unool::adapters {

	// 角色图片资产出站端口：把文件系统访问与路径拼装从领域侧移出。
	// 领域侧仍负责 infos 查表与异常校验，这里只做 I/O 与字符串拼接。
	// 目录起点用 exe 自身路径推导的 resourceRoot()，与进程工作目录无关。
	class FilesystemAssetCatalog final : public application::IAssetCatalog {
	public:
		std::vector<std::string> skinsOf(const std::string& group,
										const std::string& name) const override {
			namespace fs = std::filesystem;
			const fs::path dir = unool::paths::resourceRoot() / "images" / "characters"
				/ unool::string::to_utf16(group) / unool::string::to_utf16(name);
			if (!fs::exists(dir) || !fs::is_directory(dir)) {
				std::println(stderr, "角色 <{}> 的皮肤目录不存在", name);
				return {};
			}
			std::vector<std::string> skins;
			for (const auto& entry : fs::directory_iterator(dir)) {
				if (entry.is_regular_file() && entry.path().extension() == ".jpg") {
					skins.push_back(unool::string::to_utf8(entry.path().stem().wstring()));
				}
			}
			if (skins.empty()) {
				std::println(stderr, "角色 <{}> 的皮肤目录下无 .jpg 文件", name);
				return {};
			}
			//排序，"默认"置首
			std::ranges::sort(skins,
							  [](const std::string& name1, const std::string& name2) {
				if (name1 == "默认") return true;
				if (name2 == "默认") return false;
				return name1 < name2;
			});
			return skins;
		}
		std::string imagePath(const std::string& group, const std::string& name,
							  const std::string& skin) const override {
			return std::format("images/characters/{}/{}/{}.jpg", group, name, skin);
		}
	};
}
