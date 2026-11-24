#include "MessageCodec.h"

#include "GameState.h"
#include "core/IDecisionPort.h"
#include <SFML/Network.hpp>

// Arch-C P1-2：游戏态消息的成对序列化实现。
// 字段顺序、类型与收敛前逐字段照抄，唯一统一点是消息号一律用 int 写入
// （与所有读端的 int 读法对齐；原 Choice 用 to_underlying 写 2 字节是隐患）。
// 收发两端均改为走本 Codec，因此字节自洽。

namespace unool::protocol {

	std::optional<MessageType> readMessageType(sf::Packet& packet) {
		int raw{};
		if (!(packet >> raw)) return std::nullopt;
		return static_cast<MessageType>(raw);
	}

	// ---- 客户端 -> 服务端 ----
	sf::Packet makeClientInput(int key, std::size_t playerId, std::size_t selectedIndex) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::ClientInput)
			<< key << playerId << selectedIndex;
		return packet;
	}

	std::optional<ClientInputData> parseClientInput(sf::Packet& packet) {
		ClientInputData data;
		if (!(packet >> data.key >> data.playerId >> data.selectedIndex)) return std::nullopt;
		return data;
	}

	// ---- 服务端 -> 客户端 ----
	sf::Packet makeConnectionInfo(std::size_t playerId) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::ConnectionInfo) << playerId;
		return packet;
	}

	std::optional<ConnectionInfoData> parseConnectionInfo(sf::Packet& packet) {
		ConnectionInfoData data;
		if (!(packet >> data.playerId)) return std::nullopt;
		return data;
	}

	sf::Packet makeGameStart(std::size_t seatId) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::GameStart) << seatId;
		return packet;
	}

	std::optional<std::size_t> parseGameStart(sf::Packet& packet) {
		std::size_t seatId{};
		if (!(packet >> seatId)) return std::nullopt;
		return seatId;
	}

	sf::Packet makeHeartbeat() {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::Heartbeat);
		return packet;
	}

	sf::Packet makeCheckinResponse(const std::string& msg, bool ok, int points) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::CheckinResponse);
		packet << msg << ok << points;
		return packet;
	}

	sf::Packet makeGameEnd(std::optional<std::size_t> winnerId, const std::string& winnerName) {
		sf::Packet packet;
		const bool hasWinner = winnerId.has_value();
		packet << static_cast<int>(MessageType::GameEnd) << hasWinner;
		if (hasWinner) packet << winnerId.value() << winnerName;
		return packet;
	}

	std::optional<GameEndData> parseGameEnd(sf::Packet& packet) {
		GameEndData data;
		if (!(packet >> data.hasWinner)) return std::nullopt;
		if (data.hasWinner && !(packet >> data.winnerId >> data.winnerName)) return std::nullopt;
		return data;
	}

	sf::Packet makeGameStateMessage(const GameState& state) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::GameState) << state;
		return packet;
	}

	bool parseGameState(sf::Packet& packet, GameState& state) {
		return static_cast<bool>(packet >> state);
	}

	sf::Packet makeCharInfoMessage(const CharInfo& info) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::CharInfo) << info;
		return packet;
	}

	bool parseCharInfo(sf::Packet& packet, CharInfo& info) {
		return static_cast<bool>(packet >> info);
	}

	sf::Packet makeChoice(const unool::core::ChoicePrompt& prompt) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::Choice);
		packet << prompt.title;
		packet << prompt.options.size();
		for (const auto& option : prompt.options) packet << option;
		packet << prompt.forced;
		packet << prompt.errorMsg;
		const bool hasTimeout = prompt.timeoutMs.has_value();
		packet << hasTimeout;
		if (hasTimeout) packet << prompt.timeoutMs.value();
		packet << prompt.currentPage;
		packet << prompt.totalPages;
		packet << prompt.optionMeta.size();
		for (const auto& meta : prompt.optionMeta) {
			packet << meta.characterName << meta.level << meta.hp << meta.skinName << meta.imagePath;
		}
		return packet;
	}

	std::optional<unool::core::ChoicePrompt> parseChoice(sf::Packet& packet) {
		using unool::core::ChoiceOptionMeta;
		using unool::core::ChoicePrompt;
		ChoicePrompt prompt;
		if (!(packet >> prompt.title)) return std::nullopt;

		std::size_t optionCount{};
		if (!(packet >> optionCount)) return std::nullopt;
		prompt.options.resize(optionCount);
		for (std::size_t i = 0; i < optionCount; ++i) {
			if (!(packet >> prompt.options[i])) return std::nullopt;
		}

		if (!(packet >> prompt.forced >> prompt.errorMsg)) return std::nullopt;

		bool hasTimeout{};
		if (!(packet >> hasTimeout)) return std::nullopt;
		if (hasTimeout) {
			std::size_t timeoutMs{};
			if (!(packet >> timeoutMs)) return std::nullopt;
			prompt.timeoutMs = timeoutMs;
		}

		if (!(packet >> prompt.currentPage >> prompt.totalPages)) return std::nullopt;

		std::size_t metaCount{};
		if (!(packet >> metaCount)) return std::nullopt;
		prompt.optionMeta.reserve(metaCount);
		for (std::size_t i = 0; i < metaCount; ++i) {
			ChoiceOptionMeta meta;
			if (!(packet >> meta.characterName >> meta.level >> meta.hp >> meta.skinName >> meta.imagePath)) break;
			prompt.optionMeta.push_back(std::move(meta));
		}
		return prompt;
	}

	sf::Packet makePointerUpdate(std::size_t playerId, std::size_t selectedIndex) {
		sf::Packet packet;
		packet << static_cast<int>(MessageType::PointerUpdate) << playerId << selectedIndex;
		return packet;
	}

	std::optional<PointerUpdateData> parsePointerUpdate(sf::Packet& packet) {
		PointerUpdateData data;
		if (!(packet >> data.playerId >> data.selectedIndex)) return std::nullopt;
		return data;
	}

	sf::Packet makeStringMessage(MessageType type, const std::string& text) {
		sf::Packet packet;
		packet << static_cast<int>(type) << text;
		return packet;
	}

	std::optional<StringPayload> parseStringPayload(sf::Packet& packet) {
		StringPayload payload;
		if (!(packet >> payload.text)) return std::nullopt;
		return payload;
	}

}
