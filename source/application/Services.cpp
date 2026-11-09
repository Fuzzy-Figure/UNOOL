#include "utils.h"
#include "application/Services.h"
#include "adapters/LegacyServicesAdapter.h"

// 默认服务定位：从 source/utils.cpp 归位到本单元（阶段 3 第十二批）。
// 本文件仍由 UNOOL.Core 静态库编译一次、3 个 exe 共同链接，因此定义放这里
// 不会新增链接依赖。静态工具（Character）与全局随机门面（utils）在未装配时
// 自动落到与重构前逐字等价的遗留实现；仅 ServerMain 显式装配。
namespace unool::application {
	namespace detail {
		opt_ref<DomainServices>& installedSlot() {
			static opt_ref<DomainServices> slot{};
			return slot;
		}
		DomainServices& legacyBundle() {
			// 默认组合门面：五个独立适配器各一份，random 与随机门面共用同一实例。
			static adapters::LegacyServicesAdapter adapter;
			return adapter.bundle;
		}
	}

	DomainServices& services() {
		auto& installed{detail::installedSlot()};
		if (installed.has_value()) return installed->get();
		return detail::legacyBundle();
	}
	void installServices(DomainServices& s) {
		detail::installedSlot() = std::ref(s);
	}
}
