#pragma once
#include <string>

namespace unool::adapters {

	// 阶段 4 · S3a（用户 49.10 口径③A）：场景切换的「意图」端口。
	// LoginScene 不再自行构造 ShopScene，只把「用户要进商城」这一意图交给组合根；
	// ClientApp 提供的实现负责构造并同步运行 ShopScene，退出后回到大厅。
	// 只换调用者、行为等价：断线 / 关窗判定仍留在 LoginScene 原有位置，
	// ClientApp::run() 的线性装配顺序（47.3）不变。
	//
	// 本段只建意图通道；结果回传通道要等 S3b / S3c 把响应解析上移后才需要，顺延。
	// 纯标准库，不含 SFML / nlohmann。
	class IShopOpener {
	public:
		virtual ~IShopOpener() = default;

		virtual void openShop(const std::string& username) = 0;
	};

}
