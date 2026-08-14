#pragma once
#include <SFML/Network.hpp>
#include <string>
#include <optional>
#include "../header/Socket.h"

namespace AccountProtocol {
	// 构造请求 packet（客户端用）
	sf::Packet makeRegisterRequest(const std::string& username, const std::string& password);
	sf::Packet makeLoginRequest(const std::string& username, const std::string& password);
	sf::Packet makeCheckUsernameRequest(const std::string& username);

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
