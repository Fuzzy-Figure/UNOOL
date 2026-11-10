#pragma once
#include <cstddef>
#include <string>
#include "MessageType.h"


namespace unool::application {

	// 账号用例所需的会话出站端口（阶段 3 第九批 / W5 第一段）。
	//
	// 由 ServerNetwork 实现：只暴露「会话状态查询 / 变更 / 发包」，
	// 不把 ClientSlot、sf::SocketSelector、sf::Clock 等传输细节暴露给
	// Application 层。方法内不含业务判断，行为与原
	// ServerNetwork::handleAccountPacket / trySpendPoints 逐字等价。
	class IAccountSession {
	public:
		virtual ~IAccountSession() = default;

		// 连接与槽位状态；越界一律返回 false / 空串
		virtual bool isConnected(std::size_t clientIdx) const = 0;
		virtual bool isLoggedIn(std::size_t clientIdx) const = 0;
		virtual bool isDisconnected(std::size_t clientIdx) const = 0;
		virtual std::string usernameOf(std::size_t clientIdx) const = 0;

		// 座位被其他账号占用时，只清理 socket（保留 username 与 disconnected 标记供重连识别）
		virtual void dropConnection(std::size_t clientIdx) = 0;

		// 登录成功后写回槽位、重置心跳计时并打印日志（原逻辑整段平移）
		virtual void onLoginSucceeded(std::size_t clientIdx, const std::string& username,
									  int points, int wins, int losses) = 0;

		// 积分消费成功后同步 ClientSlot.points 缓存
		virtual void onPointsSpent(std::size_t clientIdx, int amount) = 0;

		// 「开始游戏」：置 playerReady、双方齐备且未开局时发 GameStart 并打印日志
		virtual void onReadyToPlay(std::size_t clientIdx) = 0;

	// 回送已编码的账号响应包（适配器按既有 AccountProtocol 线格式打包，Application 不接触 sf::Packet）
	virtual void sendAccountResponse(std::size_t clientIdx, MessageType type, bool ok,
									 const std::string& msg, int points, int wins, int losses) = 0;
	virtual void sendCheckUsernameResponse(std::size_t clientIdx, bool exists) = 0;

		// === 商城展示 / 签到用例（W5 第二段）===
		// 把槽位积分同步为绝对值；与 onPointsSpent 的「减 amount」语义不同
		// （签到返回签到后积分、商城购买返回购买后积分，都是全量值）。
		virtual void onPointsChanged(std::size_t clientIdx, int points) = 0;

	// 回送商城 / 技能描述响应包：jsonPayload 为已序列化 JSON 字符串，消息号由适配器封装，
	// 保持原 bool 返回值（调用点忽略）。签到响应字段较多，单独方法保证线格式一致。
	virtual bool sendPayload(std::size_t clientIdx, MessageType type, const std::string& jsonPayload) = 0;
	virtual void sendCheckinResponse(std::size_t clientIdx, const std::string& msg, bool ok, int points) = 0;

		// === 购买决策用例（W5 第二段第二段）===
		// 回送已序列化为 JSON 字符串的购买结果。MessageType::ShopResult 消息号
		// 与连接早退检查都由适配器封装，Application 层不接触协议枚举；
		// 返回是否发送成功（两个调用点均忽略返回值，与原行为一致）。
		virtual bool sendShopResult(std::size_t clientIdx, const std::string& resultJson) = 0;
	};
}
