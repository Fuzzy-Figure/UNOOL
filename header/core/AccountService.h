#pragma once
#include <cstddef>
#include "MessageType.h"
#include "AccountProtocol.h"

namespace unool::core {

	// MessageType 归 protocol 层，此处只转发类型名。
	using protocol::MessageType;

	class IAccountSession;
	class IUserRepository;

	// 账号用例服务（阶段 3 第九批 / W5 第一段）。
	//
	// 承接原 ServerNetwork::handleAccountPacket 与 trySpendPoints 的业务判断：
	// 注册、登录（含重连顶替与另一端重复登录校验）、查重、积分消费。
	// 只依赖 IAccountSession / IUserRepository 两个出站端口，不依赖
	// ServerNetwork、UserDB 或 SFML 传输细节；线上包内容由 AccountProtocol 负责。
	//
	// 房间功能（协议版本 3）：「开始游戏 / 退出匹配」不再是账号用例——组队与准备
	// 由 RoomManager 负责，本服务只认注册 / 登录 / 查重 / 积分。
	// 连接槽位上限由组合根注入（多房间并发后不再是 2）。
	class AccountService {
	public:
		AccountService(IAccountSession& session, IUserRepository& users, std::size_t capacity);

		void handleAccountRequest(std::size_t clientIdx, MessageType type, const unool::protocol::AccountRequest& req);
		bool spendPoints(std::size_t clientIdx, int amount);

	private:
		IAccountSession& session_;
		IUserRepository& users_;
		std::size_t capacity_;
	};
}
