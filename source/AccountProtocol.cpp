#include "../header/AccountProtocol.h"

namespace AccountProtocol {

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

}
