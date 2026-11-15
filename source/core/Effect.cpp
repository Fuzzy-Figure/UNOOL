#include "Effect.h"
#include "GameLogic.h"
#include "Player.h"
#include "Card.h"
#include "Skill.h"
#include <functional>

void Effect::ban(Card& card, Player& source, Player& target) {
	std::println("{}封禁了{}", source.displayName(), target.displayName());
	target.ban(source, card);
}
void Effect::rev(Card& card, GameLogic& game) {
	game.reverse();
}
void Effect::draw2(Card& card, Player& source, Player& target) {
	target.draw(2, DrawReason::card_draw2);
	target.ban(source, card);
}

void Effect::pal(Card& card, GameLogic& game, Player& source) {
	std::vector<Card::Color> colorVec(Card::fourColors.begin(), Card::fourColors.end());
	const Card::Color newColor = source.chooseCardColor("请选择颜色", true, colorVec).value();
	game.setCurrentColor(newColor);
	std::println("{}将当前颜色变为{}", source.displayName(), newColor);
}
void Effect::draw4(Card& card, GameLogic& game, Player& source, Player& target) {
	const Card::Color& colorBeforeDraw4 = game.getCurrentColor();
	pal(card, game, source);
	const std::size_t choice = target.ask(
		std::format("{}对你使用了[+4]，是否质疑？", source.characterName()), {
		"质疑",
		"不质疑"
		}, true);
	if (choice == 1) { //质疑
		if (source.handInclude([&colorBeforeDraw4](const Card& card) {
			return card.is(colorBeforeDraw4);
		})) { //质疑成功
			source.draw(4, DrawReason::card_draw4);
		}
		else { //质疑失败
			target.draw(6, DrawReason::card_draw4);
			target.ban(source, card);
		}
	}
	else if (choice == 2) { //不质疑
		target.draw(4, DrawReason::card_draw4);
		target.ban(source, card);
	}
	else throw std::runtime_error(std::format("意外的ask返回值：{}", choice));
}

// ==================== 秘技牌效果 ====================
void Effect::poison(Player& source) {
	const opt_ref<Player> target = source.chooseOtherPlayer(
		"【毒酒】选择一名其他角色，对其造成10点伤害", false);
	if (!target.has_value()) {
		std::println("<秘技> {}取消了毒酒的目标选择", source.displayName());
		return;
	}
	Player& victim = target.value().get();
	victim.damage(10, std::ref(source));
	std::println("<秘技> {}的毒酒对{}造成10点伤害", source.displayName(), victim.displayName());
}
void Effect::heal(Player& source) {
	source.recover(10);
	std::println("<秘技> {}的药酒回复10点体力", source.displayName());
}
