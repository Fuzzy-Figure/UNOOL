#include "shell/RoomGame.h"

#include <SFML/Window/Keyboard.hpp>

// 唯一的「SFML 扫描码 -> 领域键码」翻译点（原 LegacyBlockingDecisionAdapter 平移）。
//
// 这层翻译属于适配器职责：线上 ClientInput.key 的字节是 sf::Keyboard::Scancode 数值，
// 而领域层只认 contract::Key。放在 UNOOL.Adapters 工程里，与其它端口适配器同层；
// 服务端（Server）链接 Adapters + Core，符号在链接期解析。
//
// 未列出的键（含 Unknown）一律回落 Key::Unknown，与重构前行为一致。
// 升级 SFML 导致 Scan 编号变化时，只需维护本函数。

namespace unool::shell {

	unool::contract::Key toDomainKey(sf::Keyboard::Scancode sc) {
		using Sc = sf::Keyboard::Scancode;
		using K = unool::contract::Key;
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
