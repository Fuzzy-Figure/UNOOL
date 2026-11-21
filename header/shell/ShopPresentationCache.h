#pragma once

// 商城展示缓存：服务端随 ShopData / SkillInfoResponse 下发、需要跨商城会话保留的
// 纯展示数据。抽成独立头（不依赖 SFML）是为了让 ShopViewModel 可以在 Tests 里
// headless 构造——缓存本身由组合根（ClientApp）持有并注入，生命周期与进程一致。
#include <optional>
#include <string>
#include <unordered_map>

namespace unool::shell {
	// 道具展示元数据（服务端随 ShopData 下发）：客户端缓存的只是这两项展示信息，
	// 数量 / 售价 / 可购买状态始终以每次下发的 ShopData 为准，不进缓存。
	struct ShopItemPresentation {
		std::string description;
		std::optional<int> maxCount;
	};

	struct ShopPresentationCache {
		std::unordered_map<std::string, std::string> skills;              // 角色名 → 技能全文
		std::unordered_map<std::string, ShopItemPresentation> itemPresentation; // 道具名 → 展示元数据
	};
}
