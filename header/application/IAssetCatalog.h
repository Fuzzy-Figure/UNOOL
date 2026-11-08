#pragma once
#include <string>
#include <vector>

namespace unool::application {

	// 角色图片资产出站端口：把文件系统访问从 Character 静态工具中移出。
	class IAssetCatalog {
	public:
		virtual ~IAssetCatalog() = default;

		// 扫描角色皮肤目录，返回排序后的皮肤名（"默认"置首）
		virtual std::vector<std::string> skinsOf(const std::string& group, const std::string& name) const = 0;
		// images/characters/{group}/{name}/{skin}.jpg
		virtual std::string imagePath(const std::string& group, const std::string& name, const std::string& skin) const = 0;
	};
}
