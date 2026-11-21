#include "test_shopviewmodel.h"

#include "shell/ShopViewModel.h"

#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using unool::shell::ShopPresentationCache;
using unool::shell::ShopViewModel;

namespace {

	ShopSkinInfo makeSkin(std::string name, const bool unlocked, ShopPrice price) {
		ShopSkinInfo s;
		s.name = std::move(name);
		s.unlocked = unlocked;
		s.price = price;
		s.quality = "普通";
		return s;
	}

	ShopCharacterInfo makeHero(std::string name, const bool unlocked, ShopPrice price) {
		ShopCharacterInfo c;
		c.name = std::move(name);
		c.level = ShopLevel::F;
		c.unlocked = unlocked;
		c.price = price;
		return c;
	}

	ShopItemInfo makeItem(std::string name, const std::size_t price, const bool available,
						  std::optional<int> maxCount = std::nullopt) {
		ShopItemInfo i;
		i.name = std::move(name);
		i.count = 0;
		i.price = price;
		i.available = available;
		i.description = i.name + " 的效果说明";
		i.maxCount = maxCount;
		return i;
	}

	// 25 个英雄（3 页）+ 3 个道具（其中一个 available=false）：
	//   h00 已解锁（无价）且带两个皮肤（免费已解锁 + 售价 60）、
	//   h01 不可购买（无价）、h02 未解锁售价 100、
	//   h03 未解锁且有售价 80 的皮肤（验证「请先购买角色」），其余 h04..h24 售价 100。
	//   points = 150：买得起 100 / 60 / 30，买不起 200。
	ShopData makeData() {
		ShopData d;
		d.points = 150;
		for (int i = 0; i < 25; ++i) {
			const std::string name = std::format("h{:02}", i);
			if (i == 0) {
				d.characters.push_back(makeHero(name, true, std::nullopt));
			}
			else if (i == 1) {
				d.characters.push_back(makeHero(name, false, std::nullopt));
			}
			else {
				d.characters.push_back(makeHero(name, false, std::size_t{ 100 }));
			}
		}
		// 皮肤只能给已解锁的角色买，所以可购买的皮肤挂在 h00 上
		d.characters[0].skins.push_back(makeSkin("s_free", true, std::nullopt));
		d.characters[0].skins.push_back(makeSkin("s_buy", false, std::size_t{ 60 }));
		d.characters[3].skins.push_back(makeSkin("s_locked", false, std::size_t{ 80 }));

		d.items.push_back(makeItem("道具A", 30, true, 5));
		d.items.push_back(makeItem("道具B", 200, true));
		d.items.push_back(makeItem("道具C", 10, false));
		return d;
	}

	const ShopViewModel::Action* findType(const std::vector<ShopViewModel::Action>& actions,
										  const ShopViewModel::ActionType type) {
		for (const auto& a : actions) {
			if (a.type == type) return &a;
		}
		return nullptr;
	}

	// 把 cursor 移到第一个满足 pred 的 HeroCollapsed 行
	void moveCursorToHero(ShopViewModel& vm, const std::size_t heroIdx) {
		const auto& rows = vm.rows();
		for (std::size_t i = 0; i < rows.size(); ++i) {
			if (rows[i].type == ShopViewModel::Row::Type::HeroCollapsed && rows[i].heroIdx == heroIdx) {
				vm.clickRow(i);
				return;
			}
		}
	}

	// ===== 用例 =====

	void testInitialState(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);

		r.check(vm.view() == ShopViewModel::View::Hero);
		r.check(vm.mode() == ShopViewModel::Mode::Browse);
		r.check(!vm.dataReady());
		r.check(vm.rows().empty());
		r.check(vm.displayedIndices().empty());
		r.check(vm.page() == 0);
		r.check(vm.cursor() == 0);
		r.check(vm.totalPages() == 0);
		r.check(!vm.currentRowBuyable());
		r.check(vm.purchaseMessage().empty());
		r.check(!vm.messageVisible());
		r.check(!vm.exiting());
		r.check(vm.skillText("h00") == nullptr);
		r.check(vm.takeActions().empty(), "未请求数据前不出站");
	}

	void testDataArrival(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());

		r.check(vm.dataReady());
		r.check(vm.shopData().points == 150);
		r.check(vm.displayedIndices().size() == 25);
		r.check(vm.totalPages() == 3, "25 个英雄 / 每页 10 = 3 页");
		r.check(vm.rows().size() == 10);
		r.check(vm.page() == 0);
		r.check(vm.cursor() == 0);

		// 每次 ShopData 到达都全量刷新道具展示缓存
		r.check(cache.itemPresentation.size() == 3);
		r.check(cache.itemPresentation["道具A"].maxCount.has_value());
		r.check(*cache.itemPresentation["道具A"].maxCount == 5);
		r.check(!cache.itemPresentation["道具B"].maxCount.has_value());
		r.check(cache.itemPresentation["道具A"].description == "道具A 的效果说明");

		// 技能缓存全缺失 → 一次性批量请求
		const auto actions = vm.takeActions();
		r.check(actions.size() == 1);
		const auto* req = findType(actions, ShopViewModel::ActionType::RequestSkillInfo);
		r.check(req != nullptr);
		r.check(req != nullptr && req->names.size() == 25);
	}

	void testAffordable(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());

		r.check(vm.affordable(std::size_t{ 150 }));
		r.check(vm.affordable(std::size_t{ 100 }));
		r.check(!vm.affordable(std::size_t{ 200 }));
		r.check(!vm.affordable(std::nullopt), "无价 = 不售卖，不算买得起");

		// 负积分（异常存档）不应被当成「买得起」
		ShopData negative = makeData();
		negative.points = -5;
		vm.onShopData(negative);
		r.check(!vm.affordable(std::size_t{ 1 }));
	}

	void testPagination(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());

		vm.nextPage();
		r.check(vm.page() == 1);
		r.check(vm.rows().size() == 10);
		vm.nextPage();
		r.check(vm.page() == 2);
		r.check(vm.rows().size() == 5, "最后一页只剩 5 行");
		vm.nextPage();
		r.check(vm.page() == 2, "已在最后一页，再翻不动");

		vm.previousPage();
		r.check(vm.page() == 1);
		vm.previousPage();
		vm.previousPage();
		r.check(vm.page() == 0, "已在第一页，再翻不动");

		// 展开后翻页要收起（expanded 状态跨页无意义）
		moveCursorToHero(vm, 0);
		r.check(vm.expandedHero() == 0);
		r.check(vm.rows().size() == 12, "展开后插入 2 行皮肤");
		vm.nextPage();
		r.check(vm.page() == 1);
		r.check(vm.expandedHero() == static_cast<std::size_t>(-1), "翻页重置展开态");
		r.check(vm.rows().size() == 10);
	}

	void testSearch(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());

		vm.enterSearch();
		r.check(vm.searching());
		r.check(vm.keyword().empty());

		vm.typeChar(U'h');
		r.check(vm.keyword() == "h");
		r.check(vm.displayedIndices().size() == 25);
		r.check(vm.totalPages() == 3);

		// 输入会重置页码：先在 "h" 下翻到第 2 页
		vm.nextPage();
		r.check(vm.page() == 1);
		vm.typeChar(U'0');
		r.check(vm.keyword() == "h0");
		r.check(vm.page() == 0, "继续输入回到第 1 页");
		r.check(vm.displayedIndices().size() == 10, "h0 匹配 h00..h09");
		r.check(vm.rows().size() == 10);

		// 控制字符与空格被忽略
		const std::string before = vm.keyword();
		vm.typeChar(U' ');
		vm.typeChar(U'\t');
		r.check(vm.keyword() == before);

		// UTF-8：中文按 3 字节写入，backspace 整体删除
		vm.typeChar(U'你');
		r.check(vm.keyword().ends_with("你"));
		r.check(vm.keyword().size() == before.size() + 3);
		vm.backspace();
		r.check(vm.keyword() == before);

		// 关键词上限 96 字节
		ShopViewModel cap(cache);
		cap.onShopData(makeData());
		cap.enterSearch();
		for (int i = 0; i < 120; ++i) cap.typeChar(U'a');
		r.check(cap.keyword().size() == ShopViewModel::kKeywordMaxBytes);
		for (int i = 0; i < 120; ++i) cap.backspace();
		r.check(cap.keyword().empty());
		cap.backspace();  // 空关键词再按退格不崩
		r.check(cap.keyword().empty());

		// 退出搜索清空关键词并恢复全量列表
		vm.leaveSearch();
		r.check(!vm.searching());
		r.check(vm.keyword().empty());
		r.check(vm.displayedIndices().size() == 25);

		// 搜索无结果：列表空、页码 0、光标 0
		vm.enterSearch();
		for (const char32_t ch : std::u32string(U"zzz")) vm.typeChar(ch);
		r.check(vm.displayedIndices().empty());
		r.check(vm.rows().empty());
		r.check(vm.totalPages() == 0);
		r.check(vm.cursor() == 0);
		vm.moveDown();  // 空表导航不崩
		r.check(vm.cursor() == 0);
	}

	void testSearchOnlyInHeroView(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());
		vm.showItems();
		vm.enterSearch();
		r.check(!vm.searching(), "道具视图没有搜索框");
		// 非搜索状态下打字无效
		vm.typeChar(U'x');
		r.check(vm.keyword().empty());
	}

	void testExpandCollapse(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());

		// h00 在首行，展开后两个皮肤紧跟其后
		moveCursorToHero(vm, 0);
		r.check(vm.cursor() == 0);
		r.check(vm.expandedHero() == 0);
		r.check(vm.rows().size() == 12);
		r.check(vm.rows()[1].type == ShopViewModel::Row::Type::SkinEntry);
		r.check(vm.rows()[1].heroIdx == 0 && vm.rows()[1].skinIdx == 0);
		r.check(vm.rows()[2].type == ShopViewModel::Row::Type::SkinEntry);
		r.check(vm.rows()[2].skinIdx == 1);

		// 皮肤行上按 Enter 不改变展开态（只有英雄主行可展开）
		vm.clickRow(2);
		r.check(vm.expandedHero() == 0);
		r.check(vm.cursor() == 2);

		// 再次点英雄主行 → 收起
		vm.clickRow(0);
		r.check(vm.expandedHero() == static_cast<std::size_t>(-1));
		r.check(vm.rows().size() == 10);

		// 越界点击忽略
		vm.clickRow(999);
		r.check(vm.cursor() == 0);
	}

	void testBuyHero(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());
		vm.setNow(1000);
		(void)vm.takeActions();  // 清掉技能预取

		// h00 已解锁：只提示，不发包
		vm.clickRow(0);
		r.check(!vm.currentRowBuyable());
		vm.buyCurrent();
		r.check(vm.purchaseMessage() == "已解锁");
		r.check(vm.takeActions().empty());

		// h01 无价：不可购买
		vm.clickRow(1);
		r.check(!vm.currentRowBuyable());
		vm.buyCurrent();
		r.check(vm.purchaseMessage() == "不可购买");
		r.check(vm.takeActions().empty());

		// h02 未解锁、售价 100、积分 150：可买
		vm.clickRow(2);
		r.check(vm.currentRowBuyable());
		vm.buyCurrent();
		r.check(vm.purchaseMessage() == "正在购买...");
		const auto actions = vm.takeActions();
		r.check(actions.size() == 1);
		const auto* buy = findType(actions, ShopViewModel::ActionType::BuyCharacter);
		r.check(buy != nullptr && buy->name == "h02");
	}

	void testBuySkin(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());
		(void)vm.takeActions();

		// h00 的免费皮肤：已解锁（h00 本身已解锁，走到皮肤分支）
		moveCursorToHero(vm, 0);
		vm.clickRow(1);  // s_free
		r.check(!vm.currentRowBuyable());
		vm.buyCurrent();
		r.check(vm.purchaseMessage() == "已解锁");

		// h00 的付费皮肤：可买，动作带角色名 + 皮肤名
		vm.clickRow(2);  // s_buy
		r.check(vm.currentRowBuyable());
		vm.buyCurrent();
		const auto actions = vm.takeActions();
		const auto* buy = findType(actions, ShopViewModel::ActionType::BuySkin);
		r.check(buy != nullptr);
		r.check(buy != nullptr && buy->name == "h00" && buy->skin == "s_buy");

		// h03 未解锁：皮肤买不了，先提示买角色
		vm.clickRow(0);  // 收起 h00
		moveCursorToHero(vm, 3);
		r.check(vm.expandedHero() == 3);
		// h03 的皮肤行紧跟在它后面
		vm.clickRow(vm.cursor() + 1);
		r.check(vm.currentRowBuyable(), "皮肤本身可买（价格存在、未解锁）");
		vm.buyCurrent();
		r.check(vm.purchaseMessage() == "请先购买角色");
		r.check(vm.takeActions().empty());
	}

	void testItemView(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());
		(void)vm.takeActions();

		vm.showItems();
		r.check(vm.view() == ShopViewModel::View::Item);
		r.check(vm.page() == 0);
		r.check(vm.cursor() == 0);
		r.check(vm.totalPages() == 1);
		r.check(vm.rows().size() == 2, "available=false 的道具从列表消失");
		r.check(vm.rows()[0].itemIdx == 0);
		r.check(vm.rows()[1].itemIdx == 1);

		// 道具视图不翻页
		vm.nextPage();
		r.check(vm.page() == 0);

		// 道具A：买得起、可买
		r.check(vm.currentRowBuyable());
		vm.buyCurrent();
		const auto actions = vm.takeActions();
		const auto* buy = findType(actions, ShopViewModel::ActionType::BuyItem);
		r.check(buy != nullptr && buy->name == "道具A");

		// 道具B：available 但买不起（仍可点，由服务端拒绝）
		vm.moveDown();
		r.check(vm.cursor() == 1);
		r.check(vm.currentRowBuyable());
		vm.buyCurrent();
		r.check(vm.purchaseMessage() == "正在购买...");
		const auto second = vm.takeActions();
		r.check(second.size() == 1);
		r.check(second[0].name == "道具B");

		// 回到英雄视图：页码与光标归零
		vm.showHeroes();
		r.check(vm.view() == ShopViewModel::View::Hero);
		r.check(vm.page() == 0);
		r.check(vm.cursor() == 0);
		r.check(vm.rows().size() == 10);
	}

	void testPurchaseMessage(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());
		(void)vm.takeActions();

		vm.setNow(1000);
		vm.clickRow(2);
		vm.buyCurrent();
		r.check(vm.purchaseMessage() == "正在购买...");
		r.check(vm.messageVisible());

		vm.setNow(1000 + ShopViewModel::kMessageMs - 1);
		r.check(vm.messageVisible(), "2.5 秒内仍显示");
		vm.setNow(1000 + ShopViewModel::kMessageMs);
		r.check(!vm.messageVisible(), "到点即隐藏");

		// 结果消息替换「正在购买...」，并从新的时间点重新计时
		vm.setNow(5000);
		vm.onPurchaseResult(true, "");
		r.check(vm.purchaseMessage() == "购买成功");
		r.check(vm.messageVisible());
		vm.onPurchaseResult(false, "");
		r.check(vm.purchaseMessage() == "积分不足！", "空 msg 的失败默认积分不足");
		vm.onPurchaseResult(false, "余额不足");
		r.check(vm.purchaseMessage() == "余额不足", "服务端文案优先");
		vm.onPurchaseResult(false, "购买结果解析失败");
		r.check(vm.purchaseMessage() == "购买结果解析失败");
		vm.onShopDataParseFailed();
		r.check(vm.purchaseMessage() == "商城数据解析失败");
	}

	void testSkillCache(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(makeData());
		const auto first = vm.takeActions();
		r.check(first.size() == 1);
		r.check(first[0].names.size() == 25);

		// 已在途中：不重复排队
		vm.ensureSkillRequested("h00");
		r.check(vm.takeActions().empty());

		// 响应填缓存、清 pending，之后可直接命中
		vm.onSkillInfoResponse({ { "h00", "h00 的技能" } });
		const auto* text = vm.skillText("h00");
		r.check(text != nullptr && *text == "h00 的技能");
		vm.ensureSkillRequested("h00");
		r.check(vm.takeActions().empty(), "已缓存不再请求");

		// 其余 24 个仍在途中：刷新商城数据不会重复排同样的请求
		vm.onShopData(makeData());
		r.check(vm.takeActions().empty(), "在途请求不重复排队");

		// 全部到位后再刷新：无缺失，不再请求
		std::vector<std::pair<std::string, std::string>> all;
		for (const auto& c : makeData().characters) all.emplace_back(c.name, c.name + " 的技能");
		vm.onSkillInfoResponse(all);
		vm.onShopData(makeData());
		r.check(vm.takeActions().empty(), "全部缓存后不再请求");
	}

	void testExit(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		r.check(!vm.exiting());
		vm.requestExit();
		r.check(vm.exiting());
	}

	void testEmptyData(unool::tests::TestResult& r) {
		ShopPresentationCache cache;
		ShopViewModel vm(cache);
		vm.onShopData(ShopData{});

		r.check(vm.dataReady());
		r.check(vm.rows().empty());
		r.check(vm.totalPages() == 0);
		r.check(!vm.currentRowBuyable());
		vm.buyCurrent();  // 空表购买 / 展开不崩
		vm.activateCurrentRow();
		vm.moveDown();
		vm.moveUp();
		r.check(vm.cursor() == 0);
		r.check(vm.purchaseMessage().empty());
		r.check(vm.takeActions().empty(), "没有角色就不请求技能");
	}

} // namespace

unool::tests::TestResult runShopViewModelTests() {
	unool::tests::TestResult r;
	r.section("shop.initial", testInitialState);
	r.section("shop.dataArrival", testDataArrival);
	r.section("shop.affordable", testAffordable);
	r.section("shop.pagination", testPagination);
	r.section("shop.search", testSearch);
	r.section("shop.searchOnlyInHeroView", testSearchOnlyInHeroView);
	r.section("shop.expandCollapse", testExpandCollapse);
	r.section("shop.buyHero", testBuyHero);
	r.section("shop.buySkin", testBuySkin);
	r.section("shop.itemView", testItemView);
	r.section("shop.purchaseMessage", testPurchaseMessage);
	r.section("shop.skillCache", testSkillCache);
	r.section("shop.exit", testExit);
	r.section("shop.emptyData", testEmptyData);
	return r;
}
