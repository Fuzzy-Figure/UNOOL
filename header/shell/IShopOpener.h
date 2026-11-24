#pragma once
#include <string>

namespace unool::shell {

	// 阶段 4 · S3a（用户 49.10 口径③A）：场景切换的「意图」端口。
	// LoginScene 不再自行构造 ShopScene，只把「用户要进商城」这一意图交给组合根；
	// ClientApp 提供的实现负责构造并同步运行 ShopScene，退出后回到大厅。
	// 只换调用者、行为等价：断线 / 关窗判定仍留在 LoginScene 原有位置，
	// ClientApp::run() 的线性装配顺序（47.3）不变。
	//
	// 房间功能（协议版本 3）：原「对手已开始匹配」的跨场景横幅随匹配一并下线，
	// 商城不再需要共享匹配提示状态，签名只剩账号名。
	// 纯标准库，不含 SFML / nlohmann。
	class IShopOpener {
	public:
		virtual ~IShopOpener() = default;

		virtual void openShop(const std::string& username) = 0;
	};

}
