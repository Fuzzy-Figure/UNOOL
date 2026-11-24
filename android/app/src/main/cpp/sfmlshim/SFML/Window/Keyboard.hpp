#pragma once
// Android 服务端只用到扫描码的数值：ClientInput.key 直接承载 sf::Keyboard::Scancode。
// 枚举编号照抄 SFML 3.1 的 Scan，保证与 PC 客户端 / 安卓客户端线上字节一致。
namespace sf {
namespace Keyboard {

enum class Scan {
    Unknown = -1,
    A = 0, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9, Num0,
    Enter, Escape, Backspace, Tab, Space,
    Hyphen, Equal, LBracket, RBracket, Backslash, Semicolon,
    Apostrophe, Grave, Comma, Period, Slash,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,
    CapsLock, PrintScreen, ScrollLock, Pause,
    Insert, Home, PageUp, Delete, End, PageDown,
    Right, Left, Down, Up,
    NumLock, NumpadDivide, NumpadMultiply, NumpadMinus, NumpadPlus,
    NumpadEqual, NumpadEnter, NumpadDecimal,
    Numpad1, Numpad2, Numpad3, Numpad4, Numpad5,
    Numpad6, Numpad7, Numpad8, Numpad9, Numpad0
};

using Scancode = Scan;

} // namespace Keyboard
} // namespace sf
