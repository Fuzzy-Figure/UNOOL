#pragma once
#include <string>
#include "domain/Types.h"

namespace unool::application {

	using domain::player_id;

	// 服务端出站端口（第一阶段骨架）。
	//
	// onState / onCharInfo 承接原 GameLogic::broadcastState() / flushCharInfo()
	// 的发包实现；usernameOf / spendPoints 承接原 ServerNetwork 的会话查询与
	// 积分消费。端口签名不含任何线上 DTO，Application 不反向依赖 Protocol。
	//
	// GameState / CharInfo 的组装与映射留到阶段 2 的 NetworkServerView +
	// GameSnapshot，本阶段只平移调用位置，不改包内容。
	class IServerView {
	public:
		virtual ~IServerView() = default;

		virtual void onState() = 0;
		virtual void onCharInfo() = 0;

		virtual std::string usernameOf(player_id) const = 0;
		virtual bool spendPoints(player_id, int amount) = 0;
	};
}
