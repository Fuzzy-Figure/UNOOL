#include "shell/ShopViewModel.h"

#include <algorithm>

namespace unool::shell {

	ShopViewModel::ShopViewModel(ShopPresentationCache& cache) : cache_(cache) {
		rebuildDisplayedIndices();
	}

	// ===== 出站队列 =====

	std::vector<ShopViewModel::Action> ShopViewModel::takeActions() {
		std::vector<Action> out{};
		out.swap(actions_);
		return out;
	}

	// ===== 入站 =====

	void ShopViewModel::onShopData(ShopData data) {
		shopData_ = std::move(data);
		dataReady_ = true;
		// 每次 ShopData 到达都用服务端下发的展示元数据全量刷新缓存：
		// 服务端改了文案 / 上限，客户端下一帧就能显示新值。
		for (const auto& item : shopData_.items) {
			cache_.itemPresentation[item.name] =
				ShopItemPresentation{ item.description, item.maxCount };
		}
		// 数据刷新后重建显示并夹紧导航状态
		rebuildDisplayedIndices();
		clampPage();
		rebuildRows();
		clampCursor();
		// 预取缓存缺失的角色技能描述（一次性批量请求）
		std::vector<std::string> missing;
		for (const auto& c : shopData_.characters) {
			if (!cache_.skills.contains(c.name)) missing.push_back(c.name);
		}
		requestSkills(missing);
	}

	void ShopViewModel::onShopDataParseFailed() {
		setPurchaseMsg("商城数据解析失败");
	}

	void ShopViewModel::onSkillInfoResponse(const std::vector<std::pair<std::string, std::string>>& skills) {
		for (const auto& [name, text] : skills) {
			cache_.skills[name] = text;
			pendingSkills_.erase(name);
		}
	}

	void ShopViewModel::onPurchaseResult(bool ok, std::string_view msg) {
		if (ok) {
			setPurchaseMsg("购买成功");
			return;
		}
		// 失败：积分不足或其它，统一提示
		setPurchaseMsg(msg.empty() ? std::string("积分不足！") : std::string(msg));
	}

	// ===== 输入 =====

	void ShopViewModel::enterSearch() {
		if (currentView_ != View::Hero) return;
		mode_ = Mode::Search;
	}

	void ShopViewModel::leaveSearch() {
		if (mode_ != Mode::Search) return;
		mode_ = Mode::Browse;
		searchKeyword_.clear();
		rebuildDisplayedIndices();
		pageIdx_ = 0;
		rebuildRows();
		clampCursor();
	}

	void ShopViewModel::typeChar(char32_t ch) {
		if (mode_ != Mode::Search) return;
		if (ch < 33) return;  // 排除控制字符与空格
		if (searchKeyword_.size() >= kKeywordMaxBytes) return;
		// 将码位编码为 UTF-8 追加
		if (ch < 0x80) {
			searchKeyword_.push_back(static_cast<char>(ch));
		}
		else if (ch < 0x800) {
			searchKeyword_.push_back(static_cast<char>(0xC0 | (ch >> 6)));
			searchKeyword_.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
		}
		else if (ch < 0x10000) {
			searchKeyword_.push_back(static_cast<char>(0xE0 | (ch >> 12)));
			searchKeyword_.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
			searchKeyword_.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
		}
		else {
			searchKeyword_.push_back(static_cast<char>(0xF0 | (ch >> 18)));
			searchKeyword_.push_back(static_cast<char>(0x80 | ((ch >> 12) & 0x3F)));
			searchKeyword_.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
			searchKeyword_.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
		}
		rebuildDisplayedIndices();
		pageIdx_ = 0;
		rebuildRows();
		clampCursor();
	}

	void ShopViewModel::backspace() {
		if (mode_ != Mode::Search || searchKeyword_.empty()) return;
		// 删除最后一个 UTF-8 字符（连读所有 10xxxxxx 续字节）
		std::size_t n = searchKeyword_.size();
		while (n > 0 && (static_cast<unsigned char>(searchKeyword_[n - 1]) & 0xC0) == 0x80) --n;
		if (n > 0) --n;
		searchKeyword_.erase(n);
		rebuildDisplayedIndices();
		pageIdx_ = 0;
		rebuildRows();
		clampCursor();
	}

	void ShopViewModel::moveUp() {
		if (cursor_ > 0) --cursor_;
	}

	void ShopViewModel::moveDown() {
		if (cursor_ + 1 < rows_.size()) ++cursor_;
	}

	void ShopViewModel::previousPage() {
		// 搜索模式下不翻页（原 View 只在浏览模式绑定了 A/左）
		if (mode_ == Mode::Search) return;
		if (pageIdx_ == 0) return;
		--pageIdx_;
		expandedHeroIdx_ = static_cast<std::size_t>(-1);
		rebuildRows();
		clampCursor();
	}

	void ShopViewModel::nextPage() {
		if (mode_ == Mode::Search) return;
		if (pageIdx_ + 1 >= totalPages()) return;
		++pageIdx_;
		expandedHeroIdx_ = static_cast<std::size_t>(-1);
		rebuildRows();
		clampCursor();
	}

	void ShopViewModel::showHeroes() {
		currentView_ = View::Hero;
		resetNavigation();
		rebuildRows();
	}

	void ShopViewModel::showItems() {
		currentView_ = View::Item;
		resetNavigation();
		rebuildRows();
	}

	void ShopViewModel::activateCurrentRow() {
		if (rows_.empty()) return;
		const Row row = rows_[cursor_];  // 值拷贝，避免 rebuildRows 后悬空引用
		if (currentView_ != View::Hero) return;
		if (row.type != Row::Type::HeroCollapsed) return;
		if (expandedHeroIdx_ == row.heroIdx) {
			expandedHeroIdx_ = static_cast<std::size_t>(-1);
			rebuildRows();
			clampCursor();
			return;
		}
		expandedHeroIdx_ = row.heroIdx;
		rebuildRows();
		// 展开新英雄后，将 cursor 定位到该英雄行
		for (std::size_t i = 0; i < rows_.size(); ++i) {
			if (rows_[i].type == Row::Type::HeroCollapsed && rows_[i].heroIdx == row.heroIdx) {
				cursor_ = i;
				break;
			}
		}
	}

	void ShopViewModel::clickRow(std::size_t index) {
		if (index >= rows_.size()) return;
		cursor_ = index;
		activateCurrentRow();
	}

	void ShopViewModel::buyCurrent() {
		if (rows_.empty()) return;
		const Row row = rows_[cursor_];  // 值拷贝，避免消息 / 请求期间表被重建
		if (currentView_ == View::Hero) {
			if (row.type == Row::Type::HeroCollapsed) {
				const auto& hero = shopData_.characters[row.heroIdx];
				if (!hero.unlocked && hero.price.has_value()) {
					queue(Action{ ActionType::BuyCharacter, hero.name, {} , {} });
					setPurchaseMsg("正在购买...");
				}
				else {
					setPurchaseMsg(hero.unlocked ? "已解锁" : "不可购买");
				}
				return;
			}
			if (row.type == Row::Type::SkinEntry) {
				const auto& hero = shopData_.characters[row.heroIdx];
				const auto& skin = hero.skins[row.skinIdx];
				if (!hero.unlocked) {
					setPurchaseMsg("请先购买角色");
					return;
				}
				if (!skin.unlocked && skin.price.has_value()) {
					queue(Action{ ActionType::BuySkin, hero.name, skin.name, {} });
					setPurchaseMsg("正在购买...");
				}
				else {
					setPurchaseMsg(skin.unlocked ? "已解锁" : "不售卖");
				}
			}
			return;
		}
		if (row.type == Row::Type::ItemEntry) {
			const auto& item = shopData_.items[row.itemIdx];
			if (item.available) {
				queue(Action{ ActionType::BuyItem, item.name, {}, {} });
				setPurchaseMsg("正在购买...");
			}
			else {
				setPurchaseMsg("不可购买");
			}
		}
	}

	void ShopViewModel::requestExit() {
		exiting_ = true;
	}

	// ===== 只读查询 =====

	std::size_t ShopViewModel::totalPages() const {
		if (currentView_ == View::Item) return 1;
		const std::size_t n = displayedIndices_.size();
		return (n + kPerPage - 1) / kPerPage;
	}

	bool ShopViewModel::currentRowBuyable() const {
		if (rows_.empty()) return false;
		const auto& row = rows_[cursor_];
		if (currentView_ == View::Hero) {
			if (row.type == Row::Type::HeroCollapsed) {
				const auto& hero = shopData_.characters[row.heroIdx];
				return !hero.unlocked && hero.price.has_value();
			}
			if (row.type == Row::Type::SkinEntry) {
				const auto& hero = shopData_.characters[row.heroIdx];
				const auto& skin = hero.skins[row.skinIdx];
				return !skin.unlocked && skin.price.has_value();
			}
			return false;
		}
		if (row.type == Row::Type::ItemEntry) {
			return shopData_.items[row.itemIdx].available;
		}
		return false;
	}

	bool ShopViewModel::affordable(unool::contract::price_t price) const {
		return price.has_value() && shopData_.points >= 0
			&& static_cast<std::size_t>(shopData_.points) >= *price;
	}

	bool ShopViewModel::messageVisible() const {
		return msgActive_ && !purchaseMsg_.empty() && nowMs_ - msgShownAtMs_ < kMessageMs;
	}

	const std::string* ShopViewModel::skillText(const std::string& charName) const {
		if (auto it = cache_.skills.find(charName); it != cache_.skills.end()) return &it->second;
		return nullptr;
	}

	void ShopViewModel::ensureSkillRequested(const std::string& charName) {
		if (cache_.skills.contains(charName)) return;
		requestSkills({ charName });
	}

	// ===== 内部 =====

	void ShopViewModel::rebuildDisplayedIndices() {
		displayedIndices_.clear();
		const bool filter = (mode_ == Mode::Search && !searchKeyword_.empty());
		for (std::size_t i = 0; i < shopData_.characters.size(); ++i) {
			if (filter && shopData_.characters[i].name.find(searchKeyword_) == std::string::npos) continue;
			displayedIndices_.push_back(i);
		}
	}

	void ShopViewModel::rebuildRows() {
		rows_.clear();
		if (currentView_ == View::Item) {
			for (std::size_t i = 0; i < shopData_.items.size(); ++i) {
				// 达到服务端下发的购买上限后（available=false）从列表消失
				if (!shopData_.items[i].available) continue;
				Row r;
				r.type = Row::Type::ItemEntry;
				r.itemIdx = i;
				rows_.push_back(r);
			}
			return;
		}
		// 英雄视图：对 displayedIndices 分页
		const std::size_t start = pageIdx_ * kPerPage;
		const std::size_t end = std::min(start + kPerPage, displayedIndices_.size());
		for (std::size_t i = start; i < end; ++i) {
			const std::size_t hi = displayedIndices_[i];
			Row r;
			r.type = Row::Type::HeroCollapsed;
			r.heroIdx = hi;
			rows_.push_back(r);
			// 展开皮肤列表：仅显示皮肤项
			if (hi == expandedHeroIdx_) {
				for (std::size_t s = 0; s < shopData_.characters[hi].skins.size(); ++s) {
					Row sr;
					sr.type = Row::Type::SkinEntry;
					sr.heroIdx = hi;
					sr.skinIdx = s;
					rows_.push_back(sr);
				}
			}
		}
	}

	void ShopViewModel::clampPage() {
		const std::size_t pages = totalPages();
		if (pageIdx_ >= pages) pageIdx_ = (pages == 0) ? 0 : pages - 1;
	}

	void ShopViewModel::clampCursor() {
		if (rows_.empty()) {
			cursor_ = 0;
			return;
		}
		if (cursor_ >= rows_.size()) cursor_ = rows_.size() - 1;
	}

	void ShopViewModel::resetNavigation() {
		pageIdx_ = 0;
		cursor_ = 0;
		expandedHeroIdx_ = static_cast<std::size_t>(-1);
	}

	void ShopViewModel::setPurchaseMsg(std::string msg) {
		purchaseMsg_ = std::move(msg);
		msgShownAtMs_ = nowMs_;
		msgActive_ = true;
	}

	void ShopViewModel::requestSkills(const std::vector<std::string>& names) {
		std::vector<std::string> filtered;
		for (const auto& name : names) {
			if (pendingSkills_.contains(name)) continue;
			filtered.push_back(name);
			pendingSkills_.insert(name);
		}
		if (filtered.empty()) return;
		Action action{};
		action.type = ActionType::RequestSkillInfo;
		action.names = std::move(filtered);
		queue(std::move(action));
	}

} // namespace unool::shell
