#pragma once
#include <string>
#include "application/IClientSession.h"

class ClientNetwork;

namespace unool::adapters {

	// 客户端组合根（阶段 4 · S1）：唯一出现「谁构造谁、按什么顺序驱动」的地方。
	// 三个 Scene 本批零改动，仍直接持有 ClientNetwork&；ClientApp 同时持有
	// IClientSession& 端口视图（与 net_ 指向同一个对象），S2 的 PacketDispatcher
	// 收拢收包后再由端口全面接管。S1 只把原 ClientMain.cpp 的装配与游戏阶段
	// 主循环平移到这里，不改任何函数体的行为。
	class ClientApp {
	public:
		ClientApp(ClientNetwork& net, const std::string& windowTitle,
		          const std::string& titleBrackets, const std::string& ipAddress,
		          unsigned short port);

		int run();

	private:
		ClientNetwork& net_;
		application::IClientSession& session_;
		std::string windowTitle_{};
		std::string titleBrackets_{};
		std::string ipAddress_{};
		unsigned short port_{};
	};

}
