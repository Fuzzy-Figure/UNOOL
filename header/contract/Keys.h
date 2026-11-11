#pragma once
#include <cstdint>

namespace unool::contract {

	// 领域键码：只表达「哪个键」，不携带任何第三方库的数值。
	//
	// SFML 扫描码 -> 领域键码的翻译集中在适配器 toDomainKey()，
	// 因此升级 SFML 时只需维护那一个函数；端口层与领域层都不认识 SFML，
	// 也不会因为第三方库调整枚举编号而静默失配。
	//
	// 这里的数值只是领域内部自增序号，与线上 ClientInput.key 的字节完全无关：
	// 传输层仍旧收发 sf::Keyboard::Scancode 的数值，线上包内容不变。
	// A 保持为 0，使 DecisionInput{} 的默认构造语义与重构前 key = 0 一致。
	enum class Key : std::uint32_t {
		A, B, D, S, W,
		Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
		Space,
		Right, Left, Down, Up,
		Numpad1, Numpad2, Numpad3, Numpad4, Numpad5,
		Numpad6, Numpad7, Numpad8, Numpad9, Numpad0,
		Unknown
	};
}
