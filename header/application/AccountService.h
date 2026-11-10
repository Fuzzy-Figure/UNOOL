#pragma once
#include <cstddef>
#include "MessageType.h"
#include "AccountProtocol.h"

namespace unool::application {

	class IAccountSession;
	class IUserRepository;

	// 账号用例服务（阶段 3 第九批 / W5 第一段）。
	//
	// 承接原 ServerNetwork::handleAccountPacket 与 trySpendPoints 的业务判断：
	// 注册、登录（含重连顶替与另一端重复登录校验）、查重、开始游戏、积分消费。
	// 只依赖 IAccountSession / IUserRepository 两个出站端口，不依赖
	// ServerNetwork、UserDB 或 SFML 传输细节；线上包内容由 AccountProtocol 负责。
	// B+：座位容量由组合根构造注入，用例服务不再读取进程级全局常量。
	class AccountService {
	public:
		AccountService(IAccountSession& session, IUserRepository& users, std::size_t seatCount);

		void handleAccountRequest(std::size_t clientIdx, MessageType type, const unool::protocol::AccountRequest& req);
		bool spendPoints(std::size_t clientIdx, int amount);

	private:
		IAccountSession& session_;
		IUserRepository& users_;
		std::size_t seatCount_;
	};
}
