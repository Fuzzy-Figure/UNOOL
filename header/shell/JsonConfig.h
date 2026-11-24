#pragma once
// JSON 配置缓存门面：4 个声明从 header/utils.h 逐字平移而来。
//
// 与 JsonConfigProvider（IConfigProvider 端口实现）不同，本头只提供
// getServerConfig / getClientConfig / reload* 的底层声明，供未走端口注入的
// 遗留调用点使用；实现在 source/shell/JsonConfig.cpp，配置字节与缓存语义未变。
//
// 只 include 前向声明头 json_fwd.hpp（内部仅 5 个标准库头），让 utils.h 与
// 被它传递包含的领域头闭包保持纯标准库。调用 .value() / operator[] / get<>()
// 的编译单元另需完整 json.hpp（各自显式 include，或由所属工程的 PCH 提供）。
#include <json_fwd.hpp>
#include <mutex>

namespace unool::shell {
	//服务器专用配置：读取 server_config.json 并缓存；reload 可强制重读
	nlohmann::json& getServerConfig();
	//强制重新读取 server_config.json，刷新缓存（bp 阶段每局前调用）
	void reloadServerConfig();

	// 多房间并发（协议版本 3）：每个房间一个线程跑对局，而 server_config.json
	// 每局前会 reload 一次，读方与写方必须串行化，否则读到的 json 可能正被替换。
	// 服务端唯一的读方是 JsonConfigProvider（它在每个方法里持本锁），
	// reloadServerConfig 也持同一把锁；客户端配置不涉及并发 reload，不加锁。
	std::mutex& serverConfigMutex();

	//客户端专用配置：读取 client_config.json 并缓存；reload 可强制重读
	nlohmann::json& getClientConfig();
	//强制重新读取 client_config.json，刷新缓存
	void reloadClientConfig();
}
