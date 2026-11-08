#pragma once
// 默认遗留服务组合：给未装配的客户端（Client1/Client2）提供与重构前
// 逐字等价的一组实例。
//
// 阶段 2 起，五个出站端口各自有独立适配器；Composition Root（ServerMain）
// 直接构造五个对象并自行组装 DomainServices，可单独替换 / 注入假实现。
// 本文件不再实现任何端口，只把五个适配器汇集为一个可直接静态构造的默认门面。
// 采用 header-only 内联实现，避免给 Client1/Client2 引入新的 .cpp 与链接依赖；
// services() / installServices() 的定义仍在所有 exe 都已编译的 source/utils.cpp。
#include "adapters/JsonConfigProvider.h"
#include "adapters/JsonUserRepository.h"
#include "adapters/Mt19937Random.h"
#include "adapters/SfmlClock.h"
#include "adapters/FilesystemAssetCatalog.h"
#include "application/Services.h"

namespace unool::adapters {

	class LegacyServicesAdapter final {
	public:
		JsonConfigProvider config{};
		JsonUserRepository users{};
		Mt19937Random random{};
		SfmlClock clock{};
		FilesystemAssetCatalog assets{};
		// 引用成员按声明顺序绑定到上面五个实例；随机门面与 DomainServices::random
		// 指向同一个 Mt19937Random，随机序列与重构前逐字等价。
		application::DomainServices bundle{ config, users, random, clock, assets };
	};
}
