#include "application/Services.h"
#include "adapters/LegacyServicesAdapter.h"

// 默认组合的实例化：从 source/application/Services.cpp 的 legacyBundle() 剥离
// 到本单元（阶段 3 第十三批）。理由：LegacyServicesAdapter.h 会传递包含
// <json.hpp> / UserDB.h / Character.h 等 adapters 与第三方头，而 Domain /
// Application 两库的 PCH 是纯标准库、包含目录也不含 dep\nlohmann；把实例化
// 留在这里，Services.cpp 就能保持纯端口依赖，具备划入领域 / 应用层库的条件。
//
// 仍然是进程内唯一的一份静态局部对象，惰性构造；五个适配器各一份、random
// 与随机门面共用同一实例，与重构前逐字等价。仅 ServerMain 显式装配时走
// installServices()，未装配的客户端仍落到这一份默认组合。
namespace unool::application {
	DomainServices& defaultBundle() {
		static adapters::LegacyServicesAdapter adapter;
		return adapter.bundle;
	}
}
