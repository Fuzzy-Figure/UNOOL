#include "AccountProtocol.h"
#include "AccountEvent.h"
#include <SFML/Network.hpp>

namespace unool::protocol {

	sf::Packet makeRegisterRequest(const std::string& username, const std::string& password) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::RegisterRequest);
		packet << username;
		packet << password;
		return packet;
	}

	sf::Packet makeLoginRequest(const std::string& username, const std::string& password) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::LoginRequest);
		packet << username;
		packet << password;
		return packet;
	}

	sf::Packet makeCheckUsernameRequest(const std::string& username) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::CheckUsernameRequest);
		packet << username;
		return packet;
	}

	// 阶段 4 · S3b2（53.9 ①A）：原 LoginScene.cpp 两处手写包逐字平移，
	// 包体只有开头的 int 消息号，不带其它字段。
	sf::Packet makeCheckinRequest() {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::CheckinRequest);
		return packet;
	}

	sf::Packet makeReadyToPlay() {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::ReadyToPlay);
		return packet;
	}

	// 退出匹配功能：两个包都由本层构造，View / Application 不手写消息号。
	sf::Packet makeCancelMatchmaking() {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::CancelMatchmaking);
		return packet;
	}

	sf::Packet makeMatchmakingCancelled(const std::string& username) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::MatchmakingCancelled);
		packet << username;
		return packet;
	}

	// 与解散广播同构：只有「int 消息号 + std::string 账号名」两个字段。
	sf::Packet makeMatchmakingStarted(const std::string& username) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::MatchmakingStarted);
		packet << username;
		return packet;
	}

	sf::Packet makeAccountResponse(MessageType type, bool ok, const std::string& msg, int points, int wins, int losses) {
		sf::Packet packet;
		packet << static_cast<int>(type);
		packet << ok;
		packet << msg;
		packet << points;
		packet << wins;
		packet << losses;
		return packet;
	}

	sf::Packet makeCheckUsernameResponse(bool exists) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::CheckUsernameResponse);
		packet << exists;
		return packet;
	}

	std::optional<AccountRequest> parseAccountRequest(sf::Packet& packet) {
		AccountRequest req;
		if (!(packet >> req.username >> req.password)) return std::nullopt;
		return req;
	}

	std::optional<std::string> parseCheckUsernameRequest(sf::Packet& packet) {
		std::string username;
		if (!(packet >> username)) return std::nullopt;
		return username;
	}

	std::optional<AccountResponse> parseAccountResponse(sf::Packet& packet) {
		AccountResponse resp;
		if (!(packet >> resp.ok >> resp.msg >> resp.points >> resp.wins >> resp.losses)) return std::nullopt;
		return resp;
	}

	std::optional<bool> parseCheckUsernameResponse(sf::Packet& packet) {
		bool exists;
		if (!(packet >> exists)) return std::nullopt;
		return exists;
	}

	std::optional<std::string> parseMatchmakingStarted(sf::Packet& packet) {
		std::string username;
		if (!(packet >> username)) return std::nullopt;
		return username;
	}

	// 阶段 4 · S3b2（53.9 ②A）：4 处响应解析与 ConnectionInfo 的 packet >> pid
	// 收拢到单点，字段顺序与 View 原解析逐字一致；packet 已 >> 过开头消息号。
	// CheckinResponse 沿用原 View 的写法：不检查 >> 结果，字段保留默认 / 部分解析值，
	// 因此该类型恒返回值对象（与旧代码「坏包也弹窗、积分取 0」的行为一致）。
	std::optional<AccountEvent> parseAccountEvent(MessageType type, sf::Packet& packet) {
		AccountEvent ev{};
		ev.type = type;
		switch (type) {
			case MessageType::ConnectionInfo:
				if (!(packet >> ev.pid)) return std::nullopt;
				break;
			case MessageType::CheckUsernameResponse:
				if (!(packet >> ev.exists)) return std::nullopt;
				break;
			case MessageType::RegisterResponse:
			case MessageType::LoginResponse:
				if (!(packet >> ev.ok >> ev.msg >> ev.points >> ev.wins >> ev.losses)) return std::nullopt;
				break;
			case MessageType::CheckinResponse:
				packet >> ev.msg >> ev.ok >> ev.points;
				break;
			default:
				return std::nullopt;
		}
		return ev;
	}

}
