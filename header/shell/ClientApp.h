#pragma once
#include <string>
#include "core/IClientSession.h"

class ClientNetwork;

namespace unool::shell {

	// 客户端组合根（阶段 4 · S1）：唯一出现「谁构造谁、按什么顺序驱动」的地方。
	// ClientApp 同时持有 IClientSession& 端口视图（与 net_ 指向同一个对象），
	// Scene 侧只经由端口收发；收包统一由 ConnectionState 的显式队列承担。
	// 装配与游戏阶段主循环在这里，不改任何函数体的行为。
	class ClientApp {
	public:
		ClientApp(ClientNetwork& net, const std::string& windowTitle,
		          const std::string& titleBrackets, const std::string& ipAddress,
		          unsigned short port);

		int run();

	private:
		ClientNetwork& net_;
		core::IClientSession& session_;
		std::string windowTitle_{};
		std::string titleBrackets_{};
		std::string ipAddress_{};
		unsigned short port_{};
	};

}
