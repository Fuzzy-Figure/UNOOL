#include "AccountProtocol.h"
#include "AccountEvent.h"
#include <SFML/Network.hpp>

namespace unool::protocol {

	sf::Packet makeRegisterRequest(const std::string& username, const std::string& password) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::RegisterRequest);
		packet << kProtocolVersion;
		packet << username;
		packet << password;
		return packet;
	}

	sf::Packet makeLoginRequest(const std::string& username, const std::string& password) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::LoginRequest);
		packet << kProtocolVersion;
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

	sf::Packet makeVersionMismatchResponse() {
		return makeAccountResponse(MessageType::LoginResponse, false,
		                           std::string(kProtocolVersionMismatch));
	}

	// 顶下线通知：服务端在断开旧连接之前直发这一条，包体只有一个原因串。
	sf::Packet makeSessionReplaced(const std::string& reason) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::SessionReplaced);
		packet << reason;
		return packet;
	}

	std::optional<std::string> parseSessionReplaced(sf::Packet& packet) {
		std::string reason;
		if (!(packet >> reason)) return std::nullopt;
		return reason;
	}

	std::optional<std::uint32_t> peekProtocolVersion(sf::Packet& packet) {
		std::uint32_t version{};
		if (!(packet >> version)) return std::nullopt;
		return version;
	}

	std::optional<AccountRequest> parseAccountRequest(sf::Packet& packet, bool withVersion) {
		AccountRequest req;
		if (withVersion) {
			const auto version = peekProtocolVersion(packet);
			if (!version) return std::nullopt;
			req.versionMatches = isProtocolVersionCompatible(*version);
		}
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
