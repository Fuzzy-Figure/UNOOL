#include "shell/LegacyBlockingDecisionAdapter.h"
#include "Socket.h"
#include <SFML/Window/Keyboard.hpp>

namespace unool::shell {

	namespace {
		// 唯一的「SFML 扫描码 -> 领域键码」翻译点。
		// 入参是线上 ClientInput.key 反序列化得到的 sf::Keyboard::Scancode，
		// 返回值是领域语义键码；端口与领域层不再出现任何底层扫描码数值。
		// 未列出的键（含 Unknown）一律回落 Key::Unknown，与重构前
		// 「数值不匹配任何按键判定」的行为一致。
		// 升级 SFML 导致 Scan 编号变化时，只需维护本函数。
		contract::Key toDomainKey(sf::Keyboard::Scancode sc) {
			using Sc = sf::Keyboard::Scancode;
			using K = contract::Key;
			switch (sc) {
				case Sc::A:       return K::A;
				case Sc::B:       return K::B;
				case Sc::D:       return K::D;
				case Sc::S:       return K::S;
				case Sc::W:       return K::W;
				case Sc::Num1:    return K::Num1;
				case Sc::Num2:    return K::Num2;
				case Sc::Num3:    return K::Num3;
				case Sc::Num4:    return K::Num4;
				case Sc::Num5:    return K::Num5;
				case Sc::Num6:    return K::Num6;
				case Sc::Num7:    return K::Num7;
				case Sc::Num8:    return K::Num8;
				case Sc::Num9:    return K::Num9;
				case Sc::Num0:    return K::Num0;
				case Sc::Space:   return K::Space;
				case Sc::Right:   return K::Right;
				case Sc::Left:    return K::Left;
				case Sc::Down:    return K::Down;
				case Sc::Up:      return K::Up;
				case Sc::Numpad1: return K::Numpad1;
				case Sc::Numpad2: return K::Numpad2;
				case Sc::Numpad3: return K::Numpad3;
				case Sc::Numpad4: return K::Numpad4;
				case Sc::Numpad5: return K::Numpad5;
				case Sc::Numpad6: return K::Numpad6;
				case Sc::Numpad7: return K::Numpad7;
				case Sc::Numpad8: return K::Numpad8;
				case Sc::Numpad9: return K::Numpad9;
				case Sc::Numpad0: return K::Numpad0;
				default:          return K::Unknown;
			}
		}
	}

	LegacyBlockingDecisionAdapter::LegacyBlockingDecisionAdapter(ServerNetwork& network,
																 core::IConfigProvider& config)
		: network_(network), config_(config) {}

	// === IDecisionPort ===

	void LegacyBlockingDecisionAdapter::poll() {
		network_.update();
	}

	std::optional<core::DecisionInput> LegacyBlockingDecisionAdapter::takeInput() {
		auto input = network_.receiveClientInput();
		if (!input.has_value()) return std::nullopt;
		return core::DecisionInput{
			input->playerId,
			toDomainKey(input->key),
			input->selectedIndex
		};
	}

	void LegacyBlockingDecisionAdapter::showChoice(contract::player_id id,
												   const core::ChoicePrompt& prompt) {
		network_.sendPlayerChoice(id, prompt.title, prompt.options, prompt.forced,
								  prompt.errorMsg, prompt.timeoutMs,
								  prompt.currentPage, prompt.totalPages);
	}

	void LegacyBlockingDecisionAdapter::clearChoice(contract::player_id id) {
		network_.clearPlayerChoice(id);
	}

	bool LegacyBlockingDecisionAdapter::isConnected(contract::player_id id) const {
		return network_.isClientConnected(id);
	}

	bool LegacyBlockingDecisionAdapter::isLoggedIn(contract::player_id id) const {
		return network_.isClientLoggedIn(id);
	}

	int LegacyBlockingDecisionAdapter::reconnectTimeoutSec() const {
		return config_.reconnectTimeoutSec();
	}
}
