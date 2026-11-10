#include "utils.h"
#include "application/Services.h"
#include <stdexcept>

// B+ 组合根显式化。
//
// 旧行为：services() 在未装配时惰性调用 defaultBundle()，于是「客户端不装配」
// 会静默落回一组硬编码的遗留适配器。这条隐式路径既是隐藏的全局状态，
// 也让多桌 / 多实例无法为每一桌提供独立的服务组合。
//
// 新行为：services() 只返回组合根显式安装的那一份；未装配时立即报错，
// 而不是悄悄退化。Client1 / Client2 与 Server 都在各自 main() 里装配，
// 装配顺序与所用实例和重构前一致，随机序列与可观察行为不变。
//
// 本单元由 UNOOL.Core 编译一次、3 个 exe 共同链接。它只依赖端口头与标准库，
// 不含任何具体适配器、第三方头或平台头。
namespace unool::application {
	namespace detail {
		opt_ref<DomainServices>& installedSlot() {
			static opt_ref<DomainServices> slot{};
			return slot;
		}
	}

	DomainServices& services() {
		auto& installed{detail::installedSlot()};
		if (!installed.has_value()) {
			throw std::logic_error(
				"[UNOOL] 组合根尚未装配服务：请在 main() 中调用 installServices()");
		}
		return installed->get();
	}

	void installServices(DomainServices& s) {
		detail::installedSlot() = std::ref(s);
	}
}
