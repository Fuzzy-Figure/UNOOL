#pragma once
// B+ 进程级默认服务组合：给客户端外壳（ClientMain）提供与重构前逐字等价的一组实例。
//
// 五个出站端口各有独立适配器；服务端组合根（ServerMain）直接构造五个对象并
// 自行组装 DomainServices，可单独替换 / 注入假实现。本文件不再实现任何端口，
// 只把五个适配器汇集为一个可直接构造的门面（header-only，无独立编译单元）。
//
// 与旧版的区别：不再由 services() 在未装配时惰性创建这一份实例。
// 现在由各 exe 的 main() 显式构造本类并调用 installServices(bundle)，
// 因此进程里「谁实现了端口」只有组合根一处决定，不存在隐藏的默认回退。
// services() / installServices() 的定义仍在 source/application/Services.cpp。
#include "adapters/JsonConfigProvider.h"
#include "adapters/JsonUserRepository.h"
#include "adapters/Mt19937Random.h"
#include "adapters/ChronoClock.h"
#include "adapters/FilesystemAssetCatalog.h"
#include "application/Services.h"

namespace unool::adapters {

	class LegacyServicesAdapter final {
	public:
		JsonConfigProvider config{};
		JsonUserRepository users{};
		Mt19937Random random{};
		ChronoClock clock{};
		FilesystemAssetCatalog assets{};
		// 引用成员按声明顺序绑定到上面五个实例；随机门面与 DomainServices::random
		// 指向同一个 Mt19937Random，随机序列与重构前逐字等价。
		application::DomainServices bundle{ config, users, random, clock, assets };
	};
}
