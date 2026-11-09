#pragma once
#include <cstddef>
#include <string>

namespace unool::application {

	// 客户端会话端口（阶段 4 · S1）。
	// 把 ClientNetwork 混合门面里「连接 / 轮询 / 会话状态」这一组纯标准库语义
	// 收敛为应用层可见的接口；不含 sf::Packet / sf::Keyboard 等第三方类型，
	// 保持 Application 层纯标准库（Q8）。
	// 与 IAccountSession 不同：后者是服务端会话端口（由 ServerNetwork 实现），
	// 本端口是客户端会话端口（由 ClientNetwork 实现）。
	// 收发包分发与两个 Scene 的端口化分别留 S2 / S3；本批不改变任何调用链行为。
	class IClientSession {
	public:
		virtual ~IClientSession() = default;

		virtual bool connect(const std::string& ip, unsigned short port) = 0;
		virtual void disconnect() = 0;
		virtual void update() = 0;

		virtual bool isConnected() const = 0;
		virtual std::size_t getPlayerId() const = 0;
		virtual const std::string& getUsername() const = 0;
	};

}
