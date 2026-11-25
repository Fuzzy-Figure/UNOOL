#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "MessageType.h"

// Arch-C P1-2：游戏态消息的统一序列化层。
// 每种消息一对 makeXxx / parseXxx；make 返回含消息号的完整 sf::Packet，
// parse 接收“消息号已消费”的载荷包（与 AccountProtocol 的 parse 系列约定一致）。
//
// 头文件只前向声明 sf::Packet，不引入 SFML / nlohmann 头；DTO 均为纯值类型，
// 不把第三方依赖外泄到 header/core 闭包。线上字节布局与收敛前逐字一致。
namespace sf { class Packet; }

struct GameState;
struct CharInfo;
namespace unool::core { struct ChoicePrompt; }

namespace unool::protocol {

	// ---- 纯值 DTO ----
	struct ConnectionInfoData {
		std::size_t playerId{};
	};
	struct GameEndData {
		bool hasWinner{};
		std::size_t winnerId{};
		std::string winnerName;
	};
	struct PointerUpdateData {
		std::size_t playerId{};
		std::size_t selectedIndex{};
	};
	struct ClientInputData {
		int key{};                  // sf::Keyboard::Scancode 的数值
		std::size_t playerId{};
		std::size_t selectedIndex{};
	};
	struct ChoiceSelectData {
		std::size_t choiceIndex{};  // 1-based 真实选项序号；0=取消
		std::size_t selectedIndex{};
	};
	struct StringPayload {
		std::string text;           // ShopResult / SkillInfoResponse / ShopData 的 JSON 载荷
	};

	// ---- 客户端 -> 服务端 ----
	sf::Packet makeClientInput(int key, std::size_t playerId, std::size_t selectedIndex);
	std::optional<ClientInputData> parseClientInput(sf::Packet& packet);
	sf::Packet makeChoiceSelect(std::size_t choiceIndex, std::size_t selectedIndex);
	std::optional<ChoiceSelectData> parseChoiceSelect(sf::Packet& packet);

	// ---- 服务端 -> 客户端 ----
	sf::Packet makeConnectionInfo(std::size_t playerId);
	std::optional<ConnectionInfoData> parseConnectionInfo(sf::Packet& packet);

	// 房间功能（协议版本 3）：开局包携带「本局座位号」（0/1）。
	// 座位是桌面内的玩法身份，与连接槽位号无关：房间把两名成员映射到两个座位，
	// 客户端据此从 GameState 里认出自己的手牌。
	sf::Packet makeGameStart(std::size_t seatId);
	std::optional<std::size_t> parseGameStart(sf::Packet& packet);
	sf::Packet makeHeartbeat();

	// 签到响应：msg / ok / points 三字段（与 parseAccountEvent 的 CheckinResponse 分支对应）。
	sf::Packet makeCheckinResponse(const std::string& msg, bool ok, int points);

	sf::Packet makeGameEnd(std::optional<std::size_t> winnerId, const std::string& winnerName);
	std::optional<GameEndData> parseGameEnd(sf::Packet& packet);

	sf::Packet makeGameStateMessage(const GameState& state);
	bool parseGameState(sf::Packet& packet, GameState& state);

	sf::Packet makeCharInfoMessage(const CharInfo& info);
	bool parseCharInfo(sf::Packet& packet, CharInfo& info);

	sf::Packet makeChoice(const unool::core::ChoicePrompt& prompt);
	std::optional<unool::core::ChoicePrompt> parseChoice(sf::Packet& packet);

	sf::Packet makePointerUpdate(std::size_t playerId, std::size_t selectedIndex);
	std::optional<PointerUpdateData> parsePointerUpdate(sf::Packet& packet);

	// 单一字符串载荷的消息（ShopResult / SkillInfoResponse / ShopData）。
	sf::Packet makeStringMessage(MessageType type, const std::string& text);
	std::optional<StringPayload> parseStringPayload(sf::Packet& packet);

	// 从完整包中读取并校验消息号；成功消费首字段并返回该类型。
	std::optional<MessageType> readMessageType(sf::Packet& packet);
}
