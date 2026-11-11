#pragma once
#include "core/IServerView.h"
#include "utils.h"

class ServerNetwork;
class GameLogic;

namespace unool::shell {

	// 服务端出站端口 IServerView 的网络实现。
	//
	// onState / onCharInfo 是原 GameLogic::broadcastState() / flushCharInfo()
	// 的平移；usernameOf / spendPoints 只是 ServerNetwork 会话查询与积分消费
	// 的转发。所有方法不含规则判断，包内容与重构前逐字等价。
	//
	// GameLogic 在本对象之后才构造，只能延迟绑定；用可空引用包装器表达观察语义。
	class NetworkServerView final : public core::IServerView {
	public:
		explicit NetworkServerView(ServerNetwork& network);

		void attach(GameLogic& game) { game_ = std::ref(game); }

		void onState() override;
		void onCharInfo() override;
		std::string usernameOf(contract::player_id) const override;
		bool spendPoints(contract::player_id, int amount) override;

	private:
		ServerNetwork& network_;
		opt_ref<GameLogic> game_{};
	};
}
