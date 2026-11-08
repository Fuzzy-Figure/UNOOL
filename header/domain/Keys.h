#pragma once
#include <cstdint>

namespace unool::domain {

	// 领域键码：数值与 SFML 3.1 sf::Keyboard::Scan 逐字对应
	// （取自 dep/SFML/include/SFML/Window/Keyboard.hpp 的 Scan 枚举）。
	// 端口透传的是 ClientInput.key 的原始数值，因此所有按键判定、
	// 数字键解析与线上包字节都不变；领域层不再需要任何 SFML 头。
	enum class Key : std::uint32_t {
		Unknown = 0xFFFFFFFFu,
		A = 0,
		B = 1,
		D = 3,
		S = 18,
		W = 22,
		Num1 = 26,
		Num2 = 27,
		Num3 = 28,
		Num4 = 29,
		Num5 = 30,
		Num6 = 31,
		Num7 = 32,
		Num8 = 33,
		Num9 = 34,
		Num0 = 35,
		Space = 40,
		Right = 86,
		Left = 87,
		Down = 88,
		Up = 89,
		Numpad1 = 98,
		Numpad2 = 99,
		Numpad3 = 100,
		Numpad4 = 101,
		Numpad5 = 102,
		Numpad6 = 103,
		Numpad7 = 104,
		Numpad8 = 105,
		Numpad9 = 106,
		Numpad0 = 107
	};
}
