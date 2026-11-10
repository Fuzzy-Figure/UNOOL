#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>
#include "MessageType.h"
#include "ShopData.h"

// 阶段 4 · S3c（46.4 / 评估口径①A + ②A）：商城相关协议的纯构造与解析收拢到协议层。
// 头文件只依赖标准库、MessageType 与 ShopData（含 nlohmann 宏定义）；sf::Packet 仅前向
// 声明（仅 makeSkillInfoRequestPacket 返回完整类型需要），完整定义由
// source/protocol/ShopProtocol.cpp 提供，不把 SFML 带进 Application。
namespace sf { class Packet; }

namespace unool::protocol {

	// 解析服务器下发的商城数据 JSON。packet 已由 PacketDispatcher 消费开头 int 消息号，
	// 此处取已读出的 json 字符串，与 View 原「if (packet >> jsonStr)」门控保持一致：
	// 提取失败由调用方保持静默（与原行为一致），解析异常由调用方转「商城数据解析失败」。
	std::optional<ShopData> parseShopData(const std::string& jsonStr);

	// 解析技能描述响应 JSON。提取失败由调用方保持静默（原 catch 为空，不提示）。
	std::optional<SkillInfoResponseBody> parseSkillInfoResponse(const std::string& jsonStr);

	// 解析购买结果 JSON。提取失败由调用方转「购买结果解析失败」。
	std::optional<ShopPurchaseResult> parseShopPurchaseResult(const std::string& jsonStr);

	// 构造购买请求 JSON 字符串（经 IClientGateway::sendShopPurchase 发送）。
	// 覆盖 4 处手写 ShopPurchaseRequest + j.dump()（Request / Character / Skin / Item），
	// 字段顺序与线上包体逐字一致。
	std::string makeShopPurchaseJson(ShopRequestType type,
	                                 const std::string& name,
	                                 const std::string& charName,
	                                 const std::string& skinName);

	// 构造技能描述请求 packet（经 IClientGateway::send 发送）。
	// names 应为调用方已去重的角色名集合（pendingSkills 去重仍在 ShopScene）。
	sf::Packet makeSkillInfoRequestPacket(const std::vector<std::string>& names);

}
