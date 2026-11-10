#include "ShopProtocol.h"
#include <SFML/Network.hpp>

namespace unool::protocol {

	// 阶段 4 · S3c（46.4 / 评估口径②A）：3 处 json::parse + get<T>() 与 4 处 dump() 上移。
	// 解析失败统一返回 std::nullopt，由 ShopScene 保留各自的分流（成功赋值 / 静默 /
	// 提示「解析失败」），与原 View 的 try/catch 分支一一对应；提取门控由调用方保持。
	std::optional<ShopData> parseShopData(const std::string& jsonStr) {
		try {
			nlohmann::json j = nlohmann::json::parse(jsonStr);
			return j.get<ShopData>();
		} catch (const std::exception&) {
			return std::nullopt;
		}
	}

	std::optional<SkillInfoResponseBody> parseSkillInfoResponse(const std::string& jsonStr) {
		try {
			nlohmann::json j = nlohmann::json::parse(jsonStr);
			return j.get<SkillInfoResponseBody>();
		} catch (const std::exception&) {
			return std::nullopt;
		}
	}

	std::optional<ShopPurchaseResult> parseShopPurchaseResult(const std::string& jsonStr) {
		try {
			nlohmann::json j = nlohmann::json::parse(jsonStr);
			return j.get<ShopPurchaseResult>();
		} catch (const std::exception&) {
			return std::nullopt;
		}
	}

	std::string makeShopPurchaseJson(ShopRequestType type,
	                                 const std::string& name,
	                                 const std::string& charName,
	                                 const std::string& skinName) {
		ShopPurchaseRequest req;
		req.type = type;
		req.name = name;
		req.charName = charName;
		req.skinName = skinName;
		nlohmann::json j = req;
		return j.dump();
	}

	// 阶段 4 · S3c（②A）：原 requestSkillInfo 手写 SkillInfoRequest 包（消息号 + dump）
	// 逐字平移；pendingSkills 去重集合仍由 ShopScene 持有，本函数只接收去重后的 names。
	sf::Packet makeSkillInfoRequestPacket(const std::vector<std::string>& names) {
		SkillInfoRequestBody req;
		req.names = names;
		nlohmann::json j = req;
		sf::Packet packet;
		packet << static_cast<int>(MessageType::SkillInfoRequest);
		packet << j.dump();
		return packet;
	}

}
