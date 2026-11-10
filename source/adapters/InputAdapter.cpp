#include "adapters/InputAdapter.h"

#include "Socket.h"
#include "GameRenderer.h"

namespace unool::adapters {

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
					if (renderer_.canSelectLocal() && (sc == sf::Keyboard::Scancode::Left || sc == sf::Keyboard::Scancode::A)) {
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
