#include "utils.h"
#include "application/Services.h"

// 默认服务定位：从 source/utils.cpp 归位到本单元（阶段 3 第十二批）。
// 本文件仍由 UNOOL.Core 静态库编译一次、3 个 exe 共同链接。
// 阶段 3 第十三批：默认组合的实例化剥离到 source/adapters/LegacyBundle.cpp，
// 本单元不再包含任何 adapters / 第三方头，只依赖端口头与标准库，具备划入
// Domain / Application 库（纯标准库 PCH）的条件。未装配时 services() 惰性调用
// defaultBundle()，仍是进程内唯一一份、与重构前逐字等价的遗留实现。
namespace unool::application {
	// 定义在 source/adapters/LegacyBundle.cpp，由 UNOOL.Core 同一静态库解析；
	// 只在此前置声明，避免把 LegacyServicesAdapter.h 的传递依赖带进本单元。
	DomainServices& defaultBundle();

	namespace detail {
		opt_ref<DomainServices>& installedSlot() {
			static opt_ref<DomainServices> slot{};
			return slot;
		}
	}

	DomainServices& services() {
		auto& installed{detail::installedSlot()};
		if (installed.has_value()) return installed->get();
		return defaultBundle();
	}
	void installServices(DomainServices& s) {
		detail::installedSlot() = std::ref(s);
	}
}
