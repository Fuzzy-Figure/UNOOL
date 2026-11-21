#pragma once

// Arch-C P1-4：商城的状态机与全部迁移规则从 ShopScene 抽出。
//
// 与 LoginViewModel 同一套模式：本类纯标准库（只依赖 ShopData DTO 与
// ShopPresentationCache），无 SFML、无窗口、无网络——事件命中判定、发包、
// 绘制全部留在 View，因此整台商城交互可以在 Tests 里 headless 驱动。
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "protocol/ShopData.h"
#include "shell/ShopPresentationCache.h"

namespace unool::shell {
	class ShopViewModel {
	public:
		enum class View { Hero, Item };
		enum class Mode { Browse, Search };

		// 扁平行表：渲染与导航共用。HeroCollapsed = 英雄主行，SkinEntry = 展开后的
		// 皮肤行（缩进显示），ItemEntry = 道具行。
		struct Row {
			enum class Type { HeroCollapsed, SkinEntry, ItemEntry };
			Type type{ Type::HeroCollapsed };
			std::size_t heroIdx{};
			std::size_t skinIdx{};
			std::size_t itemIdx{};
		};

		// 出站动作：View 逐条执行。VM 不持有 ConnectionState，也不碰协议字节。
		enum class ActionType {
			RequestShopData,
			RequestSkillInfo,
			BuyCharacter,
			BuySkin,
			BuyItem,
		};
		struct Action {
			ActionType type{ ActionType::RequestShopData };
			std::string name;                // 角色名 / 道具名
			std::string skin;                // 皮肤名（BuySkin 用）
			std::vector<std::string> names;  // 角色名列表（RequestSkillInfo 用）
		};

		static constexpr std::size_t kPerPage = 10;         // 英雄视图每页行数
		static constexpr std::int64_t kMessageMs = 2500;    // 购买提示显示时长
		static constexpr std::size_t kKeywordMaxBytes = 96; // 搜索关键词上限（UTF-8 字节）

		explicit ShopViewModel(ShopPresentationCache& cache);

		// ----- 时间注入（提示消息的 2.5 秒淡出由外部驱动）-----
		void setNow(std::int64_t ms) { nowMs_ = ms; }

		// ----- 出站队列 -----
		// 进入商城时调用一次：排入 RequestShopData（View 在 run() 开头执行）。
		void requestShopData() { queue(Action{ ActionType::RequestShopData }); }
		std::vector<Action> takeActions();

		// ----- 入站：由 View 解析协议后喂进来 -----
		void onShopData(ShopData data);
		void onShopDataParseFailed();
		void onSkillInfoResponse(const std::vector<std::pair<std::string, std::string>>& skills);
		void onPurchaseResult(bool ok, std::string_view msg);

		// ----- 导航与输入（View 把 Scancode / 鼠标命中翻译成这些方法）-----
		void enterSearch();
		void leaveSearch();
		void typeChar(char32_t ch);
		void backspace();
		void moveUp();
		void moveDown();
		void previousPage();
		void nextPage();
		void showHeroes();
		void showItems();
		void activateCurrentRow();  // Enter：仅展开/收起英雄，不购买
		// 鼠标点中第 index 行：先选中再执行 Enter 语义（越界忽略）
		void clickRow(std::size_t index);
		void buyCurrent();          // 「购买」按钮
		void requestExit();         // 浏览模式 ESC
		bool exiting() const { return exiting_; }

		// ----- 只读视图（渲染与命中检测用）-----
		const ShopData& shopData() const { return shopData_; }
		bool dataReady() const { return dataReady_; }
		View view() const { return currentView_; }
		Mode mode() const { return mode_; }
		bool searching() const { return mode_ == Mode::Search; }
		std::size_t page() const { return pageIdx_; }
		std::size_t totalPages() const;
		std::size_t cursor() const { return cursor_; }
		std::size_t expandedHero() const { return expandedHeroIdx_; }
		const std::string& keyword() const { return searchKeyword_; }
		const std::vector<std::size_t>& displayedIndices() const { return displayedIndices_; }
		const std::vector<Row>& rows() const { return rows_; }
		// 当前选中行是否可购买（购买按钮可用态）
		bool currentRowBuyable() const;
		// 积分是否够付这个价（nullopt = 不售卖）
		bool affordable(unool::contract::price_t price) const;

		// ----- 购买提示 -----
		const std::string& purchaseMessage() const { return purchaseMsg_; }
		bool messageVisible() const;

		// ----- 技能描述（渲染时按需请求，不阻塞绘制）-----
		const std::string* skillText(const std::string& charName) const;
		// 缓存缺失时排入一次 RequestSkillInfo；已在途中则不再重复排队。
		void ensureSkillRequested(const std::string& charName);

		// 展示缓存（道具说明 / 购买上限、技能全文）由 View 直接读
		const ShopPresentationCache& cache() const { return cache_; }

	private:
		void rebuildDisplayedIndices();
		void rebuildRows();
		void clampPage();
		void clampCursor();
		void setPurchaseMsg(std::string msg);
		void requestSkills(const std::vector<std::string>& names);
		void queue(Action action) { actions_.push_back(std::move(action)); }
		void resetNavigation();  // 切视图 / 换页时：page、cursor、展开态归零

		ShopPresentationCache& cache_;

		ShopData shopData_{};
		bool dataReady_ = false;
		std::string purchaseMsg_;
		bool msgActive_ = false;
		std::int64_t nowMs_ = 0;
		std::int64_t msgShownAtMs_ = 0;
		bool exiting_ = false;

		View currentView_ = View::Hero;
		Mode mode_ = Mode::Browse;

		std::size_t pageIdx_ = 0;
		std::size_t cursor_ = 0;
		std::size_t expandedHeroIdx_ = static_cast<std::size_t>(-1);
		std::string searchKeyword_;
		std::vector<std::size_t> displayedIndices_;
		std::vector<Row> rows_;

		std::unordered_set<std::string> pendingSkills_;
		std::vector<Action> actions_;
	};
}
