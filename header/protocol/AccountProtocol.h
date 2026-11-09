#pragma once
#include <string>
#include <optional>
#include "MessageType.h"

// 头文件自足化（第十四批）：协议声明只需 sf::Packet 的类型声明；完整定义由
// 各 .cpp 显式包含 <SFML/Network.hpp>，写法与 header/domain/Card.h 一致。
namespace sf { class Packet; }

namespace AccountProtocol {
	// 构造请求 packet（客户端用）
	// 阶段 4 · S3b2（53.9 ①A）：补齐两个原本在 View 手写包的 helper，5 个账号请求
	// 全部由本层构造；消息号与字段顺序逐字不变。
	sf::Packet makeRegisterRequest(const std::string& username, const std::string& password);
	sf::Packet makeLoginRequest(const std::string& username, const std::string& password);
	sf::Packet makeCheckUsernameRequest(const std::string& username);
	sf::Packet makeCheckinRequest();
	sf::Packet makeReadyToPlay();

	// 构造响应 packet（服务端用）
	sf::Packet makeAccountResponse(MessageType type, bool ok,
								   const std::string& msg, int points = 0, int wins = 0, int losses = 0);
	sf::Packet makeCheckUsernameResponse(bool exists);

	// 解析请求（服务端用）—— packet 已 >> 过 msgType
	struct AccountRequest {
		std::string username;
		std::string password;
	};
	std::optional<AccountRequest> parseAccountRequest(sf::Packet& packet);
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
