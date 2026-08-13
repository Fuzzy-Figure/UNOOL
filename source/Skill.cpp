#include "../header/Skill.h"
#include "../header/Player.h"
#include "../header/GameLogic.h"
#include "../header/utils.h"
#include <iostream>
#include <cmath>



Skill::Skill(const std::string& _name, const std::string& _info, const std::optional<std::size_t>& _limit)
	:name(_name), info(_info), limit(_limit) {}

void Skill::reset() {
	count = 0;
}


const std::unordered_map<std::string, PSkill::Factory> PSkill::registry = {
	{"粪怒", PSkill::粪怒},
	{"隐身", PSkill::隐身},
	{"顶置", PSkill::顶置},
	{"带派", PSkill::带派},
	{"寒魄", PSkill::寒魄},
	{"割腕", PSkill::割腕},
	{"丑皇", PSkill::丑皇},
	{"军国", PSkill::军国},
	{"家暴", PSkill::家暴},
	{"健身", PSkill::健身},
	{"做题", PSkill::做题},
	{"棍击", PSkill::棍击},
	{"神木", PSkill::神木},
	{"雷剑", PSkill::雷剑},
	{"买棋", PSkill::买棋},
	{"卖棋", PSkill::卖棋},
	{"耐克", PSkill::耐克},
	{"轰炸", PSkill::轰炸},
	{"爆破", PSkill::爆破},
	{"电音", PSkill::电音},
	{"蒙面", PSkill::蒙面},
	{"锐刻", PSkill::锐刻},
	{"巨富", PSkill::巨富},
	{"破产", PSkill::破产},
	{"假酒", PSkill::假酒},
	{"窃观", PSkill::窃观},
	{"生存", PSkill::生存},
	{"创造", PSkill::创造},
	{"炼兵", PSkill::炼兵},
	{"好火", PSkill::好火},
};

// **********************
//         被动技
// **********************
const std::unordered_map<std::string, std::string> PSkill::descriptions = {
	{"粪怒", "限定技，一名角色手牌数变为1时，你可令其摸你手牌数张牌（至多摸五张）。"},
	{"隐身", "当你成为【+2】/【+4】的目标时，改为你的下家摸1张牌。"},
	{"顶置", "锁定技，回合开始时，你观看牌堆底一张牌，然后可将此牌置于牌堆顶。"},
	{"带派", "每局游戏开始时，选择获得一张【变色】或【+4】；也可选择两张均获得并失去25点体力。"},
	{"寒魄", "限定技，你打出牌后，若你手牌数为1，你可令最后一张手牌变为你打出的牌。"},
	{"割腕", "锁定技，你打出红色牌后失去随机1~5点体力。"},
	{"丑皇", "锁定技，你打出万能牌后，选择一项：回复10点体力或弃置一张非数字牌。"},
	{"军国", "锁定技，当一名角色被封禁时，其失去1%最大体力（向上取整），自己被封禁改为失去1体力。"},
	{"家暴", "限定技，回合开始时，你可以令一名体力小于你的角色失去10%最大体力（向上取整）。"},
	{"健身", "锁定技，每局游戏开始时，回复5点体力。"},
	{"做题", "限定技，你打出数字牌/非数字牌后，可弃置一张非数字牌/数字牌。"},
	{"棍击", "每局限9次，当你打出万能牌后，你可以对一名其他角色造成2^X点伤害（X为本局此技能发动次数）。"},
	{"神木", "锁定技，游戏开始时，从游戏外将九张【变色】和九张【+4】加入牌堆。"},
	{"雷剑", "你打出【反转】后，可弃置一张同色数字牌并回复此牌分值点体力。"},
	{"买棋", "回合开始时，你可以失去10X点体力（X为此技能发动次数-1），从游戏外获得随机一张万能牌。"},
	{"卖棋", "回合结束时，你可以弃置一张万能牌，回复10点体力。"},
	{"耐克", "锁定技，若上一张牌为蓝色或万能牌，【封禁】、【+2】和【+4】对你无效"},
	{"轰炸", "锁定技，当你打出【+2】时，目标失去2%最大体力。"},
	{"爆破", "你对其他角色造成封禁效果后，可随机获得其一张牌并对其造成此牌分值点伤害。"},
	{"电音", "回合开始时，你可以令手牌中所有数字牌变成随机数字。"},
	{"蒙面", "锁定技，当你失去体力时，失去体力的数值减少30% （向下取整）。"},
	{"锐刻", "当你打出【5】时，你可以令一名角色摸1张牌；你可改为令其摸5张牌并失去此技能（至本局结束）。"},
	{"巨富", "游戏开始时，你的初始手牌改为十二张；你执行摸牌阶段额外摸一张牌。"},
	{"破产", "锁定技，若你打出牌后手牌数全场最多，随机弃置一张牌。"},
	{"假酒", "你打出功能牌后，可随机获得一张不同颜色的非数字牌。"},
	{"窃观", "锁定技，其他角色从牌堆一次性获得一张牌时，你得知其颜色牌名并需要输入确认。"},
	{"生存", "锁定技，回合开始时，选择一张手牌变为随机颜色的【X】（X为本局发动次数，至多9）。"},
	{"创造", "每局每种颜色限一次，你打出【9】后，可从游戏外获得一张同色的任意牌名的牌。"},
	{"炼兵", "每种颜色限一次，回合开始时，你可以弃置2张同色同名牌，从游戏外获得一张同色【+2】。"},
	{"好火", "每名角色限一次，当其他角色打出红色牌后，若其体力值大于你，你可以交给其一张手牌。"},
};

PSkill::PSkill(const std::string& _name,
			   const std::optional<std::size_t> _limit, const bool _forced,
			   const TriggerPlayer& _triggerPlayer, const TriggerTime& _triggerTime,
			   const Filter& _filter, const Content& _content)
	:Skill(_name, descriptions.at(_name), _limit),
	forced(_forced),
	triggerPlayer(_triggerPlayer),
	triggerTime(_triggerTime),
	filter(_filter),
	content(_content) {}

bool PSkill::matchTrigger(const TriggerTime& currentTriggerTime,
						  const Trigger& trigger) const {
	return triggerTime == currentTriggerTime && (
		triggerTime == TriggerTime::game_begin ||
		triggerTime == TriggerTime::game_end ||
		triggerPlayer == TriggerPlayer::anybody ||
		(triggerPlayer == TriggerPlayer::self && trigger.get_carrier() == trigger.get_player()) ||
		(triggerPlayer == TriggerPlayer::others && trigger.get_carrier() != trigger.get_player())
		);
}

void PSkill::launch(Trigger& trigger) {
	//不满足条件，或达到次数限制：不发动
	if ((limit != unlimited && count >= limit) || !filter(trigger)) return;
	//如果不是锁定技，询问玩家是否发动
	if (!forced) {
		const std::size_t choice = trigger.get_carrier().ask(
			L"是否发动 [" + getNameW() + L"]？",
			{ L"发动", L"不发动" },
			true
		);
		if (choice == 2) return;
	}
	//发动技能
	count += 1;
	trigger.set_count(count);
	std::cout << "<技能> " << trigger.get_carrier().characterName() << "发动了" << name << "！" << std::endl;
	content(trigger);
}

void PSkill::reset() {
	Skill::reset();
	if (disabledFlag) *disabledFlag = false;
	if (usedColorsFlag) usedColorsFlag->clear();
	if (usedPlayerIdsFlag) usedPlayerIdsFlag->clear();
}


// **********************
//      技能工厂函数
// **********************

std::unique_ptr<PSkill> PSkill::粪怒() {
	const std::string   name = "粪怒";
	const limit_t      limit = 1;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::others;
	const auto   triggerTime = TriggerTime::lose_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_player().handCount() == 1;
	};
	const Content content = [](Trigger& trigger) {
		trigger.get_player().draw(
			std::min(trigger.get_carrier().handCount(), 5ull)
		);
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::隐身() {
	const std::string   name = "隐身";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::card_target_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_card().is(Card::Name::action_draw2, Card::Name::wild_draw4);
	};
	const Content content = [](Trigger& trigger) {
		trigger.get_card().cancelEffect();
		trigger.get_source().draw(1);
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::顶置() {
	const std::string   name = "顶置";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::phase_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		GameLogic& game = trigger.get_game();
		Pile& pile = game.getPile();

		if (pile.empty()) return;

		const Card& bottomCard = pile.back();
		std::size_t choice = trigger.get_carrier().ask(
			L"牌堆底是" + Card::to_wstring(bottomCard.getColor()) + L" " + Card::to_wstring(bottomCard.getName()) + L"，是否顶置？",
			{ L"顶置", L"不顶置" },
			true
		);

		if (choice == 1) {
			std::unique_ptr<Card> card = pile.take_back(game.getDiscardPile());
			pile.push_front(std::move(card));
			std::cout << "<技能> 顶置成功！" << std::endl;
		}
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::带派() {
	const std::string   name = "带派";
	const limit_t      limit = 1;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::game_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		std::size_t choice = carrier.ask(
			L"发动[带派]，选择一项：", {
			L"获得一张变色",
			L"获得一张+4",
			L"都获得并失去25体力"
			}, true);
		switch (choice) {
		case 1:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
			break;
		case 2:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
			break;
		case 3:
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
			carrier.takeDamage(25, carrier);
			break;
		}
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::寒魄() {
	const std::string   name = "寒魄";
	const limit_t      limit = 1;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_carrier().handCount() == 1;
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		Card& card = trigger.get_card();
		carrier.getCardByIndex(0).set(card);
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::割腕() {
	const std::string   name = "割腕";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_card().getColor() == Card::Color::red;
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		carrier.takeDamage(unool::random::randomSize_t(1, 5), carrier);
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::丑皇() {
	const std::string   name = "丑皇";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_card().isWild();
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();

		std::size_t choice = 1;
		if (carrier.handInclude(&Card::isNotNumber)) { //有非数字牌
			choice = carrier.ask(L"选择一项：", { L"回复10体力", L"弃置一张非数字牌" }, true);
		}
		else {
			carrier.ask(L"手中没有非数字牌，已自动选择回复10体力", { L"确定", }, true);
			choice = 1;
		}
		switch (choice) {
		case 1:
			carrier.recover(10);
			break;
		case 2:
			carrier.chooseToDiscard(
				1, true,
				[](const Card& card)->bool {
				return !card.isNumber();
			});
			break;
		}
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::军国() {
	const std::string   name = "军国";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::anybody;
	const auto   triggerTime = TriggerTime::ban_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		Player& player = trigger.get_player();
		Player& carrier = trigger.get_carrier();
		if (player != carrier) //其他角色：失去 1% 最大体力，向上取整
			player.takeDamage(unool::math::ceil(player.getMaxHp() * 0.01), carrier);
		else //自己：失去1体力
			player.takeDamage(1, carrier);
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::家暴() {
	const std::string   name = "家暴";
	const limit_t      limit = 1;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::phase_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_game().playersSatisfy(
			//有玩家的血量 < 携带者
			[&trigger](const std::vector<std::unique_ptr<Player>>& players)->bool {
			for (const auto& player : players) {
				if (player->getHp() < trigger.get_carrier().getHp()) return true;
			}
			return false;
		});
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		GameLogic& game = trigger.get_game();
		const auto& candidates = game.getPlayersIf([&carrier](const Player& p) {
			return p.getHp() < carrier.getHp();
		});

		if (candidates.empty()) {
			game.broadcastState();
			return;
		}

		std::size_t targetId;

		std::vector<std::wstring> options;
		for (auto& p : candidates) {
			options.push_back(p.get().characterNameW());
		}
		std::size_t choice = carrier.ask(L"选择家暴目标：", options, true);
		targetId = candidates[choice - 1].get().getId();


		Player& target = game.getPlayerById(targetId);
		std::size_t damage = unool::math::ceil(target.getMaxHp() * 0.1);
		target.takeDamage(damage, trigger.get_carrier());
		std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName() << "发动家暴，造成" << damage << "点伤害！" << std::endl;

		game.broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::健身() {
	const std::string   name = "健身";
	const limit_t      limit = 1;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::game_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		trigger.get_carrier().recover(5);
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::做题() {
	const std::string   name = "做题";
	const limit_t      limit = 1;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger) ->bool {
		Card& card = trigger.get_card();
		Player& carrier = trigger.get_carrier();
		if (card.isNumber()) return carrier.handInclude(&Card::isNotNumber);
		else return carrier.handInclude(&Card::isNumber);
	};
	const Content content = [](Trigger& trigger) {
		Card& card = trigger.get_card();
		Player& carrier = trigger.get_carrier();
		if (card.isNumber()) {
			carrier.chooseToDiscard(1, true,
									[](const Card& c)->bool { return !c.isNumber(); });
		}
		else {
			carrier.chooseToDiscard(1, true,
									[](const Card& c)->bool { return c.isNumber(); });
		}
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::棍击() {
	const std::string   name = "棍击";
	const limit_t      limit = 9;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_card().isWild();
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		GameLogic& game = trigger.get_game();
		const auto& candidates = game.getPlayersExcludeId(carrier.getId());

		if (candidates.empty()) return;

		std::vector<std::wstring> options;
		for (const auto& p : candidates) {
			options.push_back(p.get().characterNameW());
		}
		const std::size_t choice = carrier.ask(L"选择棍击目标：", options, true);
		const std::size_t targetId = candidates[choice - 1].get().getId();


		Player& target = game.getPlayerById(targetId);
		std::size_t damage = unool::math::pow(2, trigger.get_count());
		target.takeDamage(damage, trigger.get_carrier());
		std::cout << "<技能> " << carrier.characterName() << "对" << target.characterName()
			<< "发动棍击，造成" << damage << "点伤害！" << std::endl;

		game.broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::神木() {
	const std::string   name = "神木";
	const limit_t      limit = 1;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::game_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		GameLogic& game = trigger.get_game();
		game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_pal), 9);
		game.getPile().push_front(Card::make(Card::Color::black, Card::Name::wild_draw4), 9);
		game.getPile().shuffle();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::雷剑() {
	const std::string   name = "雷剑";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_card().getName() == Card::Name::action_rev;
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		Card& card = trigger.get_card();
		Card::Color color = card.getColor();

		auto discarded = carrier.chooseToDiscard(1, false,
												 [color](const Card& c)->bool {
			return c.isNumber() && c.getColor() == color;
		});

		if (!discarded.empty()) {
			std::size_t value = discarded.front().get().value();
			carrier.recover(value);
			std::cout << "<技能> " << carrier.characterName()
				<< "发动雷剑，弃置 [" << discarded.front().get() << "] 并回复" << value << "点体力！" << std::endl;
			trigger.get_game().broadcastState();
		}
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::买棋() {
	const std::string   name = "买棋";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::phase_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		carrier.takeDamage(10 * (trigger.get_count() - 1), trigger.get_carrier());
		if (unool::random::probability(0.5)) { //万能
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_pal));
		}
		else { //+4
			carrier.gainCard(Card::make(Card::Color::black, Card::Name::wild_draw4));
		}
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::卖棋() {
	const std::string   name = "卖棋";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::phase_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_carrier().handInclude(&Card::isWild);
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		carrier.chooseToDiscard(1, true, [](const Card& card) {
			return card.isWild();
		});
		carrier.recover(10);
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::耐克() {
	const std::string   name = "耐克";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::card_target_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		Card& card = trigger.get_card();
		if (!card.is(Card::Name::action_ban, Card::Name::action_draw2, Card::Name::wild_draw4))
			return false;
		auto lastOpt = trigger.get_game().lastCard();
		if (!lastOpt.has_value()) return false;
		const Card& last = lastOpt.value();
		return last.is(Card::Color::blue) || last.isWild();
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		trigger.get_card().cancelEffect();
		std::cout << carrier.characterName() << "触发技能，使此牌无效" << std::endl;
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::轰炸() {
	const std::string   name = "轰炸";
	const auto         limit = PSkill::unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_card().is(Card::Name::action_draw2, Card::Name::wild_draw4);
	};
	const Content content = [](Trigger& trigger) {
		Player& target = trigger.get_player().next();
		if (trigger.get_card().is(Card::Name::action_draw2)) {
			target.takeDamage(unool::math::ceil(target.getMaxHp() * 0.02), trigger.get_carrier());
		}
		else {
			target.takeDamage(unool::math::ceil(target.getMaxHp() * 0.04), trigger.get_carrier());
		}
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::爆破() {
	const std::string   name = "爆破";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::others;
	const auto   triggerTime = TriggerTime::ban_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_source() == trigger.get_carrier();
	};
	const Content content = [](Trigger& trigger) {
		Player& player = trigger.get_player();
		Hand& hand = player.getHand();
		auto card = hand.takeCardByIndex(unool::random::randomSize_t(0, hand.count() - 1));
		player.takeDamage(card->value(), trigger.get_carrier());
		std::cout << "爆破获取了 [" << *card << "]，造成了" << card->value() << "点伤害！" << std::endl;
		trigger.get_carrier().gainCard(std::move(card));
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::电音() {
	const std::string   name = "电音";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::phase_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		Hand& hand = trigger.get_carrier().getHand();
		hand.forEachIf(&Card::isNumber, [](Card& card) {
			static const std::vector numbers = {
				Card::Name::number_0, Card::Name::number_1, Card::Name::number_2, Card::Name::number_3, Card::Name::number_4,
				Card::Name::number_5, Card::Name::number_6, Card::Name::number_7, Card::Name::number_8, Card::Name::number_9
			};
			card.setName(numbers[unool::random::randomSize_t(0, 9)]);
		});
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::蒙面() {
	const std::string   name = "蒙面";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::lose_hp_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		trigger.get_number() = unool::math::floor(trigger.get_number() * 0.7);
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::锐刻() {
	const std::string   name = "锐刻";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	auto disabled = std::make_shared<bool>(false);

	const Filter filter = [disabled](const Trigger& trigger)->bool {
		if (*disabled) return false;
		return trigger.get_card().getName() == Card::Name::number_5;
	};
	const Content content = [disabled](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		GameLogic& game = trigger.get_game();

		std::size_t choice = carrier.ask(L"发动[锐刻]，选择一项：", {
			L"令一名角色摸1张牌",
			L"令一名角色摸5张牌并失去此技能至本局结束"
										 }, false);

		if (choice == 0) return;

		const auto& candidates = game.getPlayers();
		std::vector<std::wstring> options;
		for (const auto& p : candidates) {
			options.push_back(p.get().characterNameW());
		}
		std::size_t targetChoice = carrier.ask(L"选择目标角色：", options, true);
		Player& target = candidates[targetChoice - 1].get();

		if (choice == 1) {
			target.draw(1);
			std::cout << "<技能> " << carrier.characterName() << "发动锐刻，令" << target.characterName() << "摸1张牌" << std::endl;
		}
		else {
			target.draw(5);
			*disabled = true;
			std::cout << "<技能> " << carrier.characterName() << "发动锐刻，令" << target.characterName()
				<< "摸5张牌，失去此技能至本局结束" << std::endl;
		}
		game.broadcastState();
	};

	auto skill = std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
	skill->disabledFlag = disabled;
	return skill;
}
std::unique_ptr<PSkill> PSkill::巨富() {
	const std::string   name = "巨富";
	const limit_t      limit = 1;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::game_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return true;
	};
	const Content content = [](Trigger& trigger) {
		trigger.get_carrier().draw(4); //初始8张 + 4张 = 12张
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::破产() {
	const std::string   name = "破产";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		Player& carrier = trigger.get_carrier();
		GameLogic& game = trigger.get_game();
		std::size_t myCount = carrier.handCount();
		if (myCount == 0) return false;
		for (const auto& p : game.getPlayers()) {
			if (p.get().handCount() > myCount) return false;
		}
		return true;
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		Hand& hand = carrier.getHand();
		std::size_t idx = unool::random::randomSize_t(0, hand.count() - 1);
		carrier.discardByIndex(idx);
		std::cout << "<技能> " << carrier.characterName() << "触发破产，随机弃置一张牌" << std::endl;
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::假酒() {
	const std::string   name = "假酒";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_card().isNotNumber();
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		Card::Color playedColor = trigger.get_card().getColor();

		static const std::vector nonNumberNames = {
			Card::Name::action_ban, Card::Name::action_rev, Card::Name::action_draw2,
			Card::Name::wild_pal, Card::Name::wild_draw4
		};
		static const std::vector colors = {
			Card::Color::red, Card::Color::yellow, Card::Color::blue, Card::Color::green
		};

		//构建候选：不同颜色的非数字牌
		std::vector<std::unique_ptr<Card>> candidates;
		for (auto n : nonNumberNames) {
			if (n == Card::Name::wild_pal || n == Card::Name::wild_draw4) {
				//万能牌(黑色)：打出牌非黑色时候选
				if (playedColor != Card::Color::black)
					candidates.push_back(Card::make(Card::Color::black, n));
			}
			else {
				//有色功能牌：颜色 != 打出牌颜色
				for (auto c : colors) {
					if (c != playedColor)
						candidates.push_back(Card::make(c, n));
				}
			}
		}

		if (candidates.empty()) return;

		std::size_t idx = unool::random::randomSize_t(0, candidates.size() - 1);
		carrier.gainCard(std::move(candidates[idx]));
		std::cout << "<技能> " << carrier.characterName() << "发动假酒，获得了一张不同颜色的非数字牌" << std::endl;
		trigger.get_game().broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::窃观() {
	const std::string   name = "窃观";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::others;
	const auto   triggerTime = TriggerTime::draw_end;

	const Filter filter = [](const Trigger& trigger)->bool {
		if (!trigger.has_number()) return false;
		if (trigger.get_number() != 1) return false;
		return trigger.has_card();
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		Player& drawer = trigger.get_player();
		const Card& card = trigger.get_card();
		std::wstring title = L"【窃观】" + drawer.characterNameW()
			+ L"获得了 " + Card::to_wstring(card.getColor())
			+ L" " + Card::to_wstring(card.getName()) + L"（需确认）";
		carrier.ask(title, { L"确认" }, true);
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::生存() {
	const std::string   name = "生存";
	const limit_t      limit = unlimited;
	const bool        forced = true;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::phase_begin;

	const Filter filter = [](const Trigger& trigger)->bool {
		return trigger.get_carrier().handCount() > 0;
	};
	const Content content = [](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		GameLogic& game = trigger.get_game();

		std::size_t x = trigger.get_count();
		if (x > 9) x = 9;
		Card::Name targetName = static_cast<Card::Name>(static_cast<int>(Card::Name::number_0) + x);

		static const std::vector<Card::Color> colors = {
			Card::Color::red, Card::Color::yellow, Card::Color::blue, Card::Color::green
		};
		Card::Color targetColor = colors[unool::random::randomSize_t(0, colors.size() - 1)];

		auto target = Card::make(targetColor, targetName);
		std::wstring title = L"请选择一张牌变为【" + std::to_wstring(x) + L"】，↑确认（不能取消）";
		auto opt = carrier.chooseToChange(*target, title);
		if (opt.has_value()) {
			std::cout << "<技能> " << carrier.characterName() << "发动生存，将一张手牌改为【"
				<< Card::to_string(targetColor) << Card::to_string(targetName) << "】" << std::endl;
		}
		game.broadcastState();
	};
	return std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
}
std::unique_ptr<PSkill> PSkill::创造() {
	const std::string   name = "创造";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::use_card_end;

	auto usedColors = std::make_shared<std::unordered_set<Card::Color>>();

	const Filter filter = [usedColors](const Trigger& trigger)->bool {
		const Card& c = trigger.get_card();
		if (c.getName() != Card::Name::number_9) return false;
		if (c.is(Card::Color::black)) return false;
		return usedColors->find(c.getColor()) == usedColors->end();
	};
	const Content content = [usedColors](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		const Card& nine = trigger.get_card();
		Card::Color targetColor = nine.getColor();
		std::vector<std::wstring> opts = {
			L"1", L"2", L"3", L"4", L"5",
			L"6", L"7", L"8", L"9", L"0",
			L"反转", L"封禁", L"+2"
		};
		std::size_t idx = carrier.ask(L"【创造】选择获得的牌名（" + Card::to_wstring(targetColor) + L"色）：", opts, true);
		static const Card::Name nameMap[] = {
			Card::Name::number_1, Card::Name::number_2, Card::Name::number_3,
			Card::Name::number_4, Card::Name::number_5, Card::Name::number_6,
			Card::Name::number_7, Card::Name::number_8, Card::Name::number_9,
			Card::Name::number_0, Card::Name::action_rev, Card::Name::action_ban,
			Card::Name::action_draw2
		};
		constexpr std::size_t nameCount = sizeof(nameMap) / sizeof(nameMap[0]);
		if (idx < 1 || idx - 1 >= nameCount) return;
		Card::Name name = nameMap[idx - 1];
		carrier.gainCard(Card::make(targetColor, name));
		usedColors->insert(targetColor);
		std::cout << "<技能> " << carrier.characterName() << "发动创造，获得一张【"
			<< Card::to_string(targetColor) << Card::to_string(name) << "】" << std::endl;
		trigger.get_game().broadcastState();
	};
	auto skill = std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
	skill->usedColorsFlag = usedColors;
	return skill;
}
std::unique_ptr<PSkill> PSkill::炼兵() {
	const std::string   name = "炼兵";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::self;
	const auto   triggerTime = TriggerTime::phase_begin;

	auto usedColors = std::make_shared<std::unordered_set<Card::Color>>();

	using CN = std::pair<Card::Color, Card::Name>;
	auto buildPairs = [](Player& carrier) -> std::map<CN, std::size_t> {
		std::map<CN, std::size_t> cnt;
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			const Card& c = carrier.getHand().getCardByIndex(i);
			if (c.getColor() == Card::Color::black) continue;
			cnt[{c.getColor(), c.getName()}]++;
		}
		return cnt;
	};

	const Filter filter = [usedColors, buildPairs](const Trigger& trigger)->bool {
		Player& carrier = trigger.get_carrier();
		if (carrier.handCount() < 2) return false;
		auto cnt = buildPairs(carrier);
		for (const auto& kv : cnt) {
			if (kv.second >= 2 && usedColors->find(kv.first.first) == usedColors->end()) {
				return true;
			}
		}
		return false;
	};

	const Content content = [usedColors, buildPairs](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		GameLogic& game = trigger.get_game();

		auto cnt = buildPairs(carrier);
		std::vector<CN> validPairs;
		std::vector<std::wstring> opts;
		for (const auto& kv : cnt) {
			if (kv.second >= 2 && usedColors->find(kv.first.first) == usedColors->end()) {
				validPairs.push_back(kv.first);
				opts.push_back(Card::to_wstring(kv.first.first) + L" "
							   + Card::to_wstring(kv.first.second) + L"(" + std::to_wstring(static_cast<unsigned long long>(kv.second)) + L"张)");
			}
		}
		if (validPairs.empty()) return;

		std::size_t idx = carrier.ask(L"【炼兵】选择要弃的同色同名牌对：", opts, false);
		if (idx == 0) return;
		if (idx - 1 >= validPairs.size()) return;

		CN target = validPairs[idx - 1];
		std::size_t i1 = carrier.handCount(), i2 = carrier.handCount();
		for (std::size_t i = 0; i < carrier.handCount(); ++i) {
			const Card& c = carrier.getHand().getCardByIndex(i);
			if (c.getColor() == target.first && c.getName() == target.second) {
				if (i1 >= carrier.handCount()) i1 = i;
				else { i2 = i; break; }
			}
		}
		if (i1 >= carrier.handCount() || i2 >= carrier.handCount()) return;
		if (i1 > i2) std::swap(i1, i2);
		carrier.discardByIndex(i2);
		carrier.discardByIndex(i1);
		carrier.gainCard(Card::make(target.first, Card::Name::action_draw2));
		usedColors->insert(target.first);
		std::cout << "<技能> " << carrier.characterName() << "发动炼兵，弃两张"
			<< Card::to_string(target.first) << Card::to_string(target.second)
			<< "，获得一张" << Card::to_string(target.first) << "+2" << std::endl;
		game.broadcastState();
	};
	auto skill = std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
	skill->usedColorsFlag = usedColors;
	return skill;
}
std::unique_ptr<PSkill> PSkill::好火() {
	const std::string   name = "好火";
	const limit_t      limit = unlimited;
	const bool        forced = false;
	const auto triggerPlayer = TriggerPlayer::others;
	const auto   triggerTime = TriggerTime::use_card_end;

	auto usedPlayerIds = std::make_shared<std::unordered_set<std::size_t>>();

	const Filter filter = [usedPlayerIds](const Trigger& trigger)->bool {
		if (!trigger.has_card() || !trigger.has_player()) return false;
		const Card& c = trigger.get_card();
		if (!c.is(Card::Color::red)) return false;
		Player& player = trigger.get_player();
		Player& carrier = trigger.get_carrier();
		if (player == carrier) return false;
		if (player.getHp() <= carrier.getHp()) return false;
		if (usedPlayerIds->find(player.getId()) != usedPlayerIds->end()) return false;
		if (carrier.handCount() == 0) return false;
		return true;
	};
	const Content content = [usedPlayerIds](Trigger& trigger) {
		Player& carrier = trigger.get_carrier();
		Player& target = trigger.get_player();

		auto result = carrier.chooseToGive(target, false);
		if (!result.has_value()) return;

		usedPlayerIds->insert(target.getId());
		std::cout << "<技能> " << carrier.characterName() << "发动好火，交给"
			<< target.characterName() << "一张" << result.value().get().toString() << std::endl;
	};
	auto skill = std::make_unique<PSkill>(name, limit, forced, triggerPlayer, triggerTime, filter, content);
	skill->usedPlayerIdsFlag = usedPlayerIds;
	return skill;
}

std::wstring PSkill::getInfoW(const std::string& skillName) {
	return unool::string::to_utf16(descriptions.at(skillName));
}


PSkill::Trigger::Trigger(GameLogic& _game, Player& _carrier,
						 opt_ref<Player> _player,
						 opt_ref<Card> _card,
						 opt_ref<Player> _source,
						 opt_ref<std::size_t> _number)
	:game(_game), carrier(_carrier), player(_player),
	card(_card), source(_source), number(_number) {}




// **********************
//         主动技
// **********************
ASkill::ASkill(const std::string& _name, const std::string& _info, const std::optional<std::size_t>& _limit)
	:Skill(_name, _info, _limit) {}

std::wstring ASkill::getInfoW(const std::string& skillName) {
	return L"";
}

const std::unordered_map<std::string, ASkill::Factory> ASkill::registry = {};

