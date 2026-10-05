#include "Effect.h"
#include "GameLogic.h"
#include "Player.h"
#include "Card.h"

void Effect::ban(Card& card, Player& source, Player& target) {
	std::println("玩家{}封禁了玩家{}", source.getId(), target.getId());
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
	std::vector<Card::Color> colorVec(Card::fourColors.begin(), Card::fourColors.end());
	const Card::Color newColor = source.chooseCardColor("请选择颜色", true, colorVec).value();
	game.setCurrentColor(newColor);
	std::println("玩家{}选择了颜色：{}", source.getId(), Card::to_string(newColor));
}
void Effect::draw4(Card& card, GameLogic& game, Player& source, Player& target) {
	const Card::Color& colorBeforeDraw4 = game.getCurrentColor();
	pal(card, game, source);
	const std::size_t choice = target.ask(
		source.characterName() + "对你使用了[+4]，是否质疑？", {
		"质疑",
		"不质疑"
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
	else throw std::runtime_error(std::format("意外的ask返回值：{}", choice));
}
