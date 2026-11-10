#pragma once
#include "IAssetCatalog.h"
#include "IClock.h"
#include "IConfigProvider.h"
#include "IRandom.h"
#include "IUserRepository.h"

namespace unool::application {

	// 出站端口聚合：作为构造注入的单一单元，避免每个领域对象持有五个引用。
	// 与第一步的 IServerView / IDecisionPort 一样，由 Composition Root 装配。
	struct DomainServices {
		IConfigProvider& config;
		IUserRepository& users;
		IRandom& random;
		IClock& clock;
		IAssetCatalog& assets;
	};

	// B+ 组合根状态：供无法构造注入的入口使用（Character 的静态工具、
	// utils 的全局随机门面）。三个 exe 都必须在 main() 中调用
	// installServices() 显式装配；未装配时 services() 直接报错，
	// 不再静默回退到一组硬编码的遗留实现。
	// 两者定义在 source/application/Services.cpp：该单元由 UNOOL.Core 静态库
	// 编译一次、3 个 exe 共同链接，不引入新的链接依赖，
	// 端口头不认识任何具体适配器。
	DomainServices& services();
	void installServices(DomainServices& s);
}
