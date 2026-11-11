#pragma once
#include "core/IDecisionPort.h"
#include "core/IConfigProvider.h"

class ServerNetwork;

namespace unool::shell {

	// 第一阶段的遗留适配器：把 ServerNetwork 的能力原样映射为决策出站端口。
	//
	// 只实现 IDecisionPort；IServerView 已交给 NetworkServerView，
	// 由 ServerMain 作为 Composition Root 分别装配。
	// 所有方法只是转发，不含规则判断，行为与重构前逐字等价。
	class LegacyBlockingDecisionAdapter final : public core::IDecisionPort {
	public:
		explicit LegacyBlockingDecisionAdapter(ServerNetwork& network,
											   core::IConfigProvider& config);

		// IDecisionPort
		void poll() override;
		std::optional<core::DecisionInput> takeInput() override;
		void showChoice(contract::player_id, const core::ChoicePrompt&) override;
		void clearChoice(contract::player_id) override;
		bool isConnected(contract::player_id) const override;
		bool isLoggedIn(contract::player_id) const override;
		int  reconnectTimeoutSec() const override;

	private:
		ServerNetwork& network_;
		// 重连超时属配置读取，统一经 IConfigProvider，避免适配器直读 server_config.json。
		core::IConfigProvider& config_;
	};
}
