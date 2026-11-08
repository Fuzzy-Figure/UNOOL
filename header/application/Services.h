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

	// 默认门面：供无法构造注入的入口使用（Character 的静态工具、
	// utils 的全局随机门面）。未装配时返回与重构前逐字等价的遗留实现，
	// 因此客户端工程不装配也不会改变任何行为。
	// installServices 只应由 Composition Root（ServerMain）调用。
	// 两者定义在 source/utils.cpp：三个 exe 工程原本都编译该文件，
	// 因此不新增 .cpp、不改动客户端工程、不引入新的链接依赖。
	DomainServices& services();
	void installServices(DomainServices& s);
}
