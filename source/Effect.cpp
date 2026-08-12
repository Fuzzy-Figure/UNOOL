#include "../header/Effect.h"
#include "../header/GameLogic.h"
#include "../header/Player.h"
#include "../header/Card.h"

void Effect::ban(Card& card, Player& source, Player& target) {
	std::cout << "玩家" << source.getId() << "封禁了玩家" << target.getId() << std::endl;
	target.ban(source, card);
}
void Effect::rev(Card& card, GameLogic& game) {
	game.reverse();
}
void Effect::draw2(Card& card, Player& source, Player& target) {
	target.draw(2);
	target.ban(source, card);
}

void Effect::pal(Card& card, GameLogic& game, Player& source) {
	std::vector<std::wstring> options = {
		Card::to_wstring(Card::Color::blue),
		Card::to_wstring(Card::Color::red),
		Card::to_wstring(Card::Color::green),
		Card::to_wstring(Card::Color::yellow)
	};

	std::size_t choice = source.ask(L"请选择颜色", options, true);

	Card::Color newColor;
	switch (choice) {
	case 1: newColor = Card::Color::blue; break;
	case 2: newColor = Card::Color::red; break;
	case 3: newColor = Card::Color::green; break;
	case 4: newColor = Card::Color::yellow; break;
	default: newColor = Card::Color::blue; break;
	}

	game.setCurrentColor(newColor);
	std::cout << "玩家" << source.getId() << "选择了颜色：" << Card::to_string(newColor) << std::endl;
}
void Effect::draw4(Card& card, GameLogic& game, Player& source, Player& target) {
	const Card::Color& colorBeforeDraw4 = game.getCurrentColor();
	pal(card, game, source);
	const std::size_t choice = target.ask(
		source.characterNameW() + L"对你使用了[+4]，是否质疑？", {
		L"质疑",
		L"不质疑"
		}, true);
	if (choice == 1) { //质疑
		if (source.handInclude([&colorBeforeDraw4](const Card& card) {
			return card.is(colorBeforeDraw4);
		})) { //质疑成功
			source.draw(4);
		}
		else { //质疑失败
			target.draw(6);
			target.ban(source, card);
		}
	}
	else if (choice == 2) { //不质疑
		target.draw(4);
		target.ban(source, card);
	}
	else throw std::runtime_error("意外的ask返回值：" + std::to_string(choice));
}
