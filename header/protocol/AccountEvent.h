#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include "MessageType.h"

// 阶段 4 · S3b2（53.9 口径①A + ②A）：账号相关响应的纯值对象与单点解析。
// 只依赖标准库与 MessageType；与 AccountProtocol.h 一样只用 sf::Packet 前向声明，
// 完整定义由 source/protocol/AccountProtocol.cpp 提供，不把 SFML 带进 Application。
namespace sf { class Packet; }

namespace AccountProtocol {

	// 5 类账号响应统一的值对象：按消息类型填充下列字段的子集。
	// 字段顺序与线上包体逐字一致，解析规则见 parseAccountEvent()。
	struct AccountEvent {
		MessageType type{};
		bool ok{};
		bool exists{};
		std::string msg;
		int points{};
		int wins{};
		int losses{};
		std::size_t pid{};
	};

	// packet 已由 PacketDispatcher 消费掉开头的 int 消息号。
	// 解析失败返回 std::nullopt（与原 View 的「响应解析失败」分支一一对应）；
	// CheckinResponse 与原 View 一致、不检查 >> 结果，恒返回值对象。
	std::optional<AccountEvent> parseAccountEvent(MessageType type, sf::Packet& packet);

}
