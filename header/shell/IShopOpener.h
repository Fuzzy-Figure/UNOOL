#pragma once
#include <string>

namespace unool::shell {

	// 阶段 4 · S3a（用户 49.10 口径③A）：场景切换的「意图」端口。
	// LoginScene 不再自行构造 ShopScene，只把「用户要进商城」这一意图交给组合根；
	// ClientApp 提供的实现负责构造并同步运行 ShopScene，退出后回到大厅。
	// 只换调用者、行为等价：断线 / 关窗判定仍留在 LoginScene 原有位置，
	// ClientApp::run() 的线性装配顺序（47.3）不变。
	//
	// 「对手已开始匹配」的提示需要跨场景可见：LoginScene（大厅）与 ShopScene（商城）
	// 共用同一份状态，openShop 把该状态的引用交给商城，商城期间收到的匹配事件
	// 就地写回，退出商城后大厅立即显示最新提示，无需结果回传通道。
	// 纯标准库，不含 SFML / nlohmann。
	struct MatchNoticeState {
		bool opponentMatching = false; // 对手是否已开始匹配（横幅显示条件）
		std::string opponentName;      // 对手账号名
	};

	class IShopOpener {
	public:
		virtual ~IShopOpener() = default;

		virtual void openShop(const std::string& username, MatchNoticeState& notice) = 0;
	};

}
