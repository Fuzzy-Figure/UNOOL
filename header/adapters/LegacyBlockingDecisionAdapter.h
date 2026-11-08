#pragma once
#include "application/IDecisionPort.h"
#include "application/IServerView.h"
#include "application/IConfigProvider.h"
#include "utils.h"

class ServerNetwork;
class GameLogic;

namespace unool::adapters {

	// 第一阶段的遗留适配器：把 ServerNetwork 的能力原样映射为出站端口。
	//
	// 同一个对象同时实现 IDecisionPort 与 IServerView，由 ServerMain 作为
	// Composition Root 装配；attach() 让 onState/onCharInfo 能读回棋局模型。
	// 所有方法只是转发，不含规则判断，行为与重构前逐字等价。
	class LegacyBlockingDecisionAdapter final
		: public application::IDecisionPort
		, public application::IServerView {
	public:
		explicit LegacyBlockingDecisionAdapter(ServerNetwork& network,
											   application::IConfigProvider& config);

		void attach(GameLogic& game) { game_ = std::ref(game); }

		// IDecisionPort
		void poll() override;
		std::optional<application::DecisionInput> takeInput() override;
		void showChoice(domain::player_id, const application::ChoicePrompt&) override;
		void clearChoice(domain::player_id) override;
		bool isConnected(domain::player_id) const override;
		bool isLoggedIn(domain::player_id) const override;
		int  reconnectTimeoutSec() const override;

		// IServerView
		void onState() override;
		void onCharInfo() override;
		std::string usernameOf(domain::player_id) const override;
		bool spendPoints(domain::player_id, int amount) override;

	private:
		ServerNetwork& network_;
		// 重连超时属配置读取，统一经 IConfigProvider，避免适配器直读 server_config.json。
		application::IConfigProvider& config_;
		// GameLogic 在 adapter 之后才构造，只能延迟绑定；用可空引用包装器表达观察语义。
		opt_ref<GameLogic> game_{};
	};
}
