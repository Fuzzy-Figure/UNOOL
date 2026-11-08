#pragma once
#include "application/IDecisionPort.h"
#include "application/IConfigProvider.h"

class ServerNetwork;

namespace unool::adapters {

	// 第一阶段的遗留适配器：把 ServerNetwork 的能力原样映射为决策出站端口。
	//
	// 只实现 IDecisionPort；IServerView 已交给 NetworkServerView，
	// 由 ServerMain 作为 Composition Root 分别装配。
	// 所有方法只是转发，不含规则判断，行为与重构前逐字等价。
	class LegacyBlockingDecisionAdapter final : public application::IDecisionPort {
	public:
		explicit LegacyBlockingDecisionAdapter(ServerNetwork& network,
											   application::IConfigProvider& config);

		// IDecisionPort
		void poll() override;
		std::optional<application::DecisionInput> takeInput() override;
		void showChoice(domain::player_id, const application::ChoicePrompt&) override;
		void clearChoice(domain::player_id) override;
		bool isConnected(domain::player_id) const override;
		bool isLoggedIn(domain::player_id) const override;
		int  reconnectTimeoutSec() const override;

	private:
		ServerNetwork& network_;
		// 重连超时属配置读取，统一经 IConfigProvider，避免适配器直读 server_config.json。
		application::IConfigProvider& config_;
	};
}
