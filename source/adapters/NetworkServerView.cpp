#include "adapters/NetworkServerView.h"
#include "Socket.h"
#include "GameLogic.h"
#include "GameState.h"
#include "Player.h"
#include "utils.h"
#include "contract/TableConfig.h"

namespace unool::adapters {

	NetworkServerView::NetworkServerView(ServerNetwork& network)
		: network_(network) {}

	// === IServerView ===
	// 以下两段是原 GameLogic::broadcastState() / flushCharInfo() 的平移，
	// 仅把 network 成员换成适配器持有的 network_、把 players 换成 getPlayers()。

	void NetworkServerView::onState() {
		for (std::size_t i = 0; i < unool::contract::TableConfig::seatCount; ++i) {
			if (!network_.isClientConnected(i)) continue;
			GameState state = game_->get().packStateForPlayer(i);
			bool ok = network_.sendGameStateToClient(i, state);
			if (!ok) {
				std::println("[Warning] broadcastState发给{}失败！", game_->get().getPlayerById(i).displayName());
			}
		}
		onCharInfo();
	}

	void NetworkServerView::onCharInfo() {
		const auto players = game_->get().getPlayers();
		for (std::size_t i = 0; i < players.size(); ++i) {
			Player& player = players[i].get();
			if (i < 2 && player.isCharInfoDirty()) {
				CharInfo info;
				info.playerIndex = i;

				//levelPart
				std::string levelPart;
				if (player.isCombined()) {
					auto levels = player.getLevels();
					levelPart = std::format("{}+{}", levels[0], levels[1]);
				}
				else {
					levelPart = std::format("{}", player.characterLevel());
				}

				//marksPart
				std::string marksPart = "标记：";
				for (const auto& [name, count] : player.getMarks()) {
					marksPart += std::format("{}*{}，", name, count);
				}

				//skillsPart
				std::string skillsPart;
				if (!player.isSealed()) skillsPart = "技能：\n";
				else skillsPart = std::format("技能（已被封印，{}回合后解除）：\n", player.getSealed());
				skillsPart += player.getSkillsText();

				//合并
				info.fullText = std::format(
					"{}（{}）\n{}\n{}",
					player.characterName(), levelPart, marksPart, skillsPart
				);

				network_.sendCharInfo(info);
				player.clearCharInfoDirty();
			}
		}
	}

	std::string NetworkServerView::usernameOf(domain::player_id id) const {
		return network_.getClientSlots()[id].username;
	}

	bool NetworkServerView::spendPoints(domain::player_id id, int amount) {
		return network_.trySpendPoints(id, amount);
	}
}
