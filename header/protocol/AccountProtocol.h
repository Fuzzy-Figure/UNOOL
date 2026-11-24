#pragma once
#include <cstdint>
#include <string>
#include <optional>
#include "MessageType.h"
#include "Protocol.h"

// 头文件自足化（第十四批）：协议声明只需 sf::Packet 的类型声明；完整定义由
// 各 .cpp 显式包含 <SFML/Network.hpp>，写法与 header/core/Card.h 一致。
namespace sf { class Packet; }

namespace unool::protocol {
	// 构造请求 packet（客户端用）
	// 阶段 4 · S3b2（53.9 ①A）：补齐两个原本在 View 手写包的 helper，5 个账号请求
	// 全部由本层构造；消息号与字段顺序逐字不变。
	sf::Packet makeRegisterRequest(const std::string& username, const std::string& password);
	sf::Packet makeLoginRequest(const std::string& username, const std::string& password);
	// 版本不匹配时的明确拒绝响应（ok=false + 可读原因），不进入任何业务分支。
	sf::Packet makeVersionMismatchResponse();
	sf::Packet makeCheckUsernameRequest(const std::string& username);
	sf::Packet makeCheckinRequest();
	// 房间功能上线后匹配下线：ReadyToPlay / CancelMatchmaking / MatchmakingStarted /
	// MatchmakingCancelled 四个构造与解析已随消息号一并删除，组队改由 RoomProtocol 承担。

	// 构造响应 packet（服务端用）
	sf::Packet makeAccountResponse(MessageType type, bool ok,
								   const std::string& msg, int points = 0, int wins = 0, int losses = 0);
	sf::Packet makeCheckUsernameResponse(bool exists);
	// 顶下线通知（SessionReplaced）：同账号在另一端登录时，服务端断开旧连接前直发，
	// 包体只有一个可读原因串。旧客户端不认识该消息号会直接丢弃，不影响其它流程。
	sf::Packet makeSessionReplaced(const std::string& reason);
	std::optional<std::string> parseSessionReplaced(sf::Packet& packet);

	// 解析请求（服务端用）—— packet 已 >> 过 msgType
	struct AccountRequest {
		std::string username;
		std::string password;
		bool versionMatches = true; // LoginRequest/RegisterRequest 首字段版本校验结果
	};
	// withVersion=true 时先读 uint32 版本字段（LoginRequest/RegisterRequest），
	// 之后读 username/password。只适用于这两个「用户名 + 口令」的请求；
	// CheckUsernameRequest 的包体只有用户名，必须用 parseCheckUsernameRequest。
	std::optional<AccountRequest> parseAccountRequest(sf::Packet& packet, bool withVersion = false);
	// 从 LoginRequest/RegisterRequest 读取版本字段（包已消费消息号）。
	// 返回 nullopt 表示包缺少版本字段（视为旧版本，应拒绝）。
	std::optional<std::uint32_t> peekProtocolVersion(sf::Packet& packet);
	std::optional<std::string> parseCheckUsernameRequest(sf::Packet& packet);

	// 解析响应（客户端用）—— packet 已 >> 过 msgType
	struct AccountResponse {
		bool ok = false;
		std::string msg;
		int points = 0;
		int wins = 0;
		int losses = 0;
	};
	std::optional<AccountResponse> parseAccountResponse(sf::Packet& packet);
	std::optional<bool> parseCheckUsernameResponse(sf::Packet& packet);
}
