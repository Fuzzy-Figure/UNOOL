#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "contract/Types.h"
#include "contract/Keys.h"

namespace unool::core {

	using contract::player_id;

	// 一次客户端输入。key 是适配器已翻译好的领域键码，
	// 端口与领域层不再接触底层扫描码数值；线上 ClientInput.key 的字节
	// 仍由传输层按 sf::Keyboard::Scancode 数值收发，保持不变。
	struct DecisionInput {
		player_id     player{};
		contract::Key   key{};
		std::size_t   selectedIndex{};
	};

	// 一次选项页面。字段与 ServerNetwork::sendPlayerChoice 的形参一一对应，
	// 默认值也与其一致，保证包内容逐字节不变。
	struct ChoicePrompt {
		std::string                title;
		std::vector<std::string>   options;
		bool                       forced = false;
		std::string                errorMsg;
		std::optional<std::size_t> timeoutMs;
		std::size_t                currentPage = 0;
		std::size_t                totalPages = 1;
	};

	// 决策通道（出站端口）。
	//
	// 只提供原子 I/O 能力：领域层保留 ask / chooseCard 的循环骨架，以及分页、
	// 强制选项、超时、600 秒重连、数字键发动技能、选中索引同步等全部既有流程。
	// 网络轮询、发包、连接状态判定、配置读取全部由适配器承担。
	//
	// 确认口径：不把整个 while 循环搬进适配器（循环内含 collectAvailableSkills、
	// tryActivate、tryUseBombCard、OperatingGuard、broadcastState 等领域调用），
	// 也不让端口承载 std::function 谓词（谓词留领域侧判定）。
	class IDecisionPort {
	public:
		virtual ~IDecisionPort() = default;

		// ServerNetwork::update() / receiveClientInput()
		virtual void poll() = 0;
		virtual std::optional<DecisionInput> takeInput() = 0;

		// ServerNetwork::sendPlayerChoice() / clearPlayerChoice()
		virtual void showChoice(player_id, const ChoicePrompt&) = 0;
		virtual void clearChoice(player_id) = 0;

		// ServerNetwork::isClientConnected() / isClientLoggedIn()
		virtual bool isConnected(player_id) const = 0;
		virtual bool isLoggedIn(player_id) const = 0;

		// 掉线重连上限：原先在 Player 中直接读 server_config.json 的
		// network.reconnectTimeoutSec（默认 600），现在由适配器读配置。
		virtual int reconnectTimeoutSec() const = 0;
	};
}
