#include "shell/ClientApp.h"
#include "Socket.h"
#include "utils.h"
#include "shell/JsonConfig.h"
#include "shell/LegacyServicesAdapter.h"
#include "core/Services.h"
#include "platform/Platform.h"
#include <charconv>
#include <cstdlib>
#include <print>
#include <string>
#include <string_view>

namespace {

// Arch-C P2-3：配置外置。优先级 命令行 > 环境变量 > 配置文件。
// 默认值保持工单指定：客户端 14341（经内网穿透映射，与服务端 8888 有意不同）。
std::string envOr(std::string_view name, std::string fallback) {
	if (auto v = unool::envVar(name.data())) return *v;
	return fallback;
}

int parseInt(std::string_view text, int fallback) {
	int value{};
	const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
	return ec == std::errc{} && ptr == text.data() + text.size() ? value : fallback;
}

// 解析 --ip <addr> / --port <n>，返回是否成功（未知参数忽略，便于双击启动）。
bool parseArgs(int argc, char* argv[], std::string& ip, int& port) {
	for (int i = 1; i < argc; ++i) {
		const std::string_view arg = argv[i];
		if ((arg == "--ip" || arg == "--port") && i + 1 < argc) {
			const std::string_view val = argv[++i];
			if (arg == "--ip") ip = val;
			else port = parseInt(val, port);
		}
	}
	return true;
}

} // namespace

// B+ 组合根：进程级服务组合在这里显式装配，不再依赖 services() 的隐式默认回退。
// 控制台编码是进程级平台能力，经 platform 接缝取得，本文件不含平台头。
int main(int argc, char* argv[]) {
	// 必须先设置控制台编码，再构造服务组合。UserDB 的 load() 首次运行会打印中文日志。
	unool::platform::setupConsoleUtf8();

	unool::shell::LegacyServicesAdapter defaultServices{};
	unool::core::installServices(defaultServices.bundle);

	const std::string windowTitle = "Client";
	const std::string windowTitleWithBrackets = "[Client]";
	std::println("{} 启动客户端...", windowTitleWithBrackets);

	ClientNetwork clientNetwork;
	const auto& config = unool::shell::getClientConfig();
	std::string ipAddress = config["server"]["ip"];
	int port = config["server"]["port"];

	// 环境变量覆盖（仅当设置时）。
	ipAddress = envOr("UNOOL_SERVER_IP", ipAddress);
	if (auto p = unool::envVar("UNOOL_SERVER_PORT")) port = parseInt(*p, port);

	// 命令行覆盖（最高优先级）。
	parseArgs(argc, argv, ipAddress, port);

	unool::shell::ClientApp app(clientNetwork, windowTitle, windowTitleWithBrackets, ipAddress,
	                            static_cast<unsigned short>(port));
	return app.run();
}
