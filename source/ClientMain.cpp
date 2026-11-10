#include "adapters/ClientApp.h"
#include "Socket.h"
#include "utils.h"
#include "adapters/JsonConfig.h"
#include "adapters/LegacyServicesAdapter.h"
#include "application/Services.h"
#ifdef _WIN32
#include <Windows.h>
#endif
#include <print>
#include <string>

// B+ 组合根：进程级服务组合在这里显式装配，不再依赖 services() 的隐式默认回退。
// 本文件属于客户端外壳，与平台相关的控制台编码用 _WIN32 保护，
// 以便将来替换外壳时不必改动共享编译单元。
int main() {
	// 生命周期覆盖 app.run()：必须在任何代码首次调用 services() 之前装配完成。
	unool::adapters::LegacyServicesAdapter defaultServices{};
	unool::application::installServices(defaultServices.bundle);

#ifdef _WIN32
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
#endif

	const std::string windowTitle = "Client";
	const std::string windowTitleWithBrackets = "[Client]";
	std::println("{} 启动客户端...", windowTitleWithBrackets);

	ClientNetwork clientNetwork;
	const auto& config = unool::getClientConfig();
	std::string ipAddress = config["server"]["ip"];
	unsigned short port = config["server"]["port"];

	unool::adapters::ClientApp app(clientNetwork, windowTitle, windowTitleWithBrackets, ipAddress, port);
	return app.run();
}
