#pragma once

class GameLogic;
class Player;
class Card;

class Effect {
public:
	Effect() = delete;

	static void rev(Card& card, GameLogic& game);
	static void ban(Card& card, Player& source, Player& target);
	static void draw2(Card& card, Player& source, Player& target);
	static void pal(Card& card, GameLogic& game, Player& source);
	static void draw4(Card& card, GameLogic& game, Player& source, Player& target);
	//秘技牌效果：毒酒选一名其他角色造成10点伤害，药酒令打出者回复10点体力
	static void poison(Player& source);
	static void heal(Player& source);
};

