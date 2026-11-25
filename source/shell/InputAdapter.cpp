#include "shell/InputAdapter.h"

#include "Socket.h"
#include "GameRenderer.h"

namespace unool::shell {

	namespace {
		std::optional<int> digitFromScancode(const sf::Keyboard::Scancode sc) {
			using enum sf::Keyboard::Scancode;
			switch (sc) {
				case Num0: case Numpad0: return 0;
				case Num1: case Numpad1: return 1;
				case Num2: case Numpad2: return 2;
				case Num3: case Numpad3: return 3;
				case Num4: case Numpad4: return 4;
				case Num5: case Numpad5: return 5;
				case Num6: case Numpad6: return 6;
				case Num7: case Numpad7: return 7;
				case Num8: case Numpad8: return 8;
				case Num9: case Numpad9: return 9;
				default: return std::nullopt;
			}
		}
	}

	InputAdapter::InputAdapter(ClientNetwork& net, GameRenderer& renderer)
		: net_{ net }, renderer_{ renderer } {}

	// 阶段 4 · S4：以下事件循环从 ClientApp::gamePhase 原样平移，逻辑、键位判定、
	// 线上 ClientInput.key 字节（由 net_.sendClientInput 产生）均未改动；
	// 仅把「窗口关闭 → 退出 gamePhase」改为返回值语义，调用方收到 false 即 return。
	bool InputAdapter::pollAndDispatch() {
		for (auto event = renderer_.pollEvent(); event; event = renderer_.pollEvent()) {
			if (event->is<sf::Event::Closed>()) {
				renderer_.closeWindow();
				return false;
			}
			if (event->is<sf::Event::KeyPressed>()) {
				auto keyEvent = event->getIf<sf::Event::KeyPressed>();
				if (keyEvent) {
					auto sc = keyEvent->scancode;
					// Windows 客户端本地分页：<-- / A 和 --> / D 只翻显示页，不发服务端。
					if (renderer_.hasChoiceOptions() &&
						(sc == sf::Keyboard::Scancode::Left || sc == sf::Keyboard::Scancode::A)) {
						renderer_.previousChoicePage();
					}
					else if (renderer_.hasChoiceOptions() &&
						(sc == sf::Keyboard::Scancode::Right || sc == sf::Keyboard::Scancode::D)) {
						renderer_.nextChoicePage();
					}
					else if (renderer_.hasChoiceOptions() && digitFromScancode(sc).has_value()) {
						// 数字键映射到当前页的真实选项序号，再按真实序号发包。
						const auto index = renderer_.choiceIndexForDigit(digitFromScancode(sc).value());
						if (index.has_value()) {
							net_.sendChoiceSelect(*index, renderer_.getSelectedIndex(net_.getPlayerId()));
						}
					}
					else if (renderer_.canSelectLocal() && (sc == sf::Keyboard::Scancode::Left || sc == sf::Keyboard::Scancode::A)) {
						renderer_.movePointerLeft(net_.getPlayerId());
					}
					else if (renderer_.canSelectLocal() && (sc == sf::Keyboard::Scancode::Right || sc == sf::Keyboard::Scancode::D)) {
						renderer_.movePointerRight(net_.getPlayerId());
					}
					else {
						net_.sendClientInput(sc, renderer_.getSelectedIndex(net_.getPlayerId()));
					}
				}
			}
			//BP 预览：鼠标悬停选项行切换角色详情；纯本地事件，不发任何 ClientInput
			if (event->is<sf::Event::MouseMoved>()) {
				auto moveEvent = event->getIf<sf::Event::MouseMoved>();
				if (moveEvent) {
					renderer_.handleMouseMove(static_cast<sf::Vector2f>(moveEvent->position));
				}
			}
			if (event->is<sf::Event::MouseButtonPressed>()) {
				auto mouseEvent = event->getIf<sf::Event::MouseButtonPressed>();
				if (mouseEvent && mouseEvent->button == sf::Mouse::Button::Left) {
					sf::Vector2f mousePos = static_cast<sf::Vector2f>(mouseEvent->position);
					renderer_.handleMouseClick(mousePos);
				}
			}
		}
		return true;
	}

}
