#pragma once
#include "IDecisionPort.h"

namespace unool::application {

	// 入站用例端口（骨架）。
	//
	// 第一阶段仍由 ServerMain 主循环直接驱动 GameLogic，这里只固化端口边界，
	// 不接入、不改变调用链；阶段 5 换非阻塞决策适配器时再由 Driving Adapter 实现。
	// 按确认口径去掉 tick()：当前为单线程阻塞模型，没有可驱动的 tick。
	class IGameCommands {
	public:
		virtual ~IGameCommands() = default;

		virtual void submit(const DecisionInput&) = 0;
		virtual void onLoggedIn(player_id) = 0;
		virtual void onDisconnected(player_id) = 0;
		virtual void onReconnected(player_id) = 0;
	};

	// IAccountCommands / IShopCommands / IClientCommands 在阶段 2 落实
	// AccountService / ShopService / ClientApp 时按各自用例定义；
	// 本阶段不预先声明尚无调用方的方法签名。
}
