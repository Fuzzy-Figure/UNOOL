#include "ShopService.h"
#include "IAccountSession.h"
#include "IUserRepository.h"
#include "ShopData.h"
#include "Character.h"
#include "MessageType.h"

#include <algorithm>
#include <print>
#include <ranges>
#include <string>
#include <utility>

#include <json.hpp>
#include <SFML/Network.hpp>

namespace unool::application {

	ShopService::ShopService(IAccountSession& session, IUserRepository& users)
		: session_(session), users_(users) {}

	// W5 第二段（分段第一段）：以下三个函数体原位于 source/adapters/Socket.cpp
	// 的 ServerNetwork::sendShopData / handleSkillInfoRequest / handleCheckin，
	// 除「会话状态与存档访问改走端口」外逐字平移；线上包内容与日志文本不变。

	bool ShopService::sendShopData(std::size_t clientIndex, const std::string& username) {
		if (!session_.isConnected(clientIndex)) return false;

		// 从用户仓储获取商城展示所需的存档投影
		const ShopUserInfo userInfo = users_.getShopUserInfo(username);
		// 是否集齐 ABCDF 所有角色（决定 S 档角色可购买性）
		const bool allNonSCollected = users_.hasAllNonSRank(username);

		ShopData shopData;
		shopData.points = userInfo.points;

		// 遍历 Character::infos，跳过"白板"，为每个角色构建 ShopCharacterInfo
		for (const auto& [name, info] : Character::infos) {
			if (name == "白板") continue;

			ShopCharacterInfo charInfo;
			charInfo.name = name;
			charInfo.level = info.level;
			charInfo.unlocked = userInfo.ownedCharacters.contains(name);
			charInfo.price = Character::getCharacterPrice(name);
			// S 档角色：集齐 ABCDF 后售价 488，否则不可购买（nullopt）
			if (info.level == Character::Level::S && allNonSCollected) {
				charInfo.price = static_cast<Character::price_t>(488);
			}

			// 获取皮肤列表，跳过"默认"皮肤
			auto skins = Character::getSkins(name);
			for (const auto& skinName : skins) {
				if (skinName == "默认") continue;

				ShopSkinInfo skinInfo;
				skinInfo.name = skinName;
				// 检查皮肤是否已解锁（从 ownedCharacters[charName] 的皮肤集合中查找）
				auto charIt = userInfo.ownedCharacters.find(name);
				if (charIt != userInfo.ownedCharacters.end()) {
					skinInfo.unlocked = charIt->second.contains(skinName);
				}
				skinInfo.price = Character::getSkinPrice(name, skinName);
				skinInfo.quality = Character::to_string(Character::getSkinQuality(name, skinName));
				charInfo.skins.push_back(std::move(skinInfo));
			}

			shopData.characters.push_back(std::move(charInfo));
		}

		// 按 Level F→S 排序（F=0, D=1, C=2, B=3, A=4, S=5）
		std::ranges::sort(shopData.characters, [](const ShopCharacterInfo& a, const ShopCharacterInfo& b) {
			return std::to_underlying(a.level) < std::to_underlying(b.level);
		});

		// 构建道具列表
		// 选将扩充卡
		{
			ShopItemInfo itemInfo;
			itemInfo.name = "选将扩充卡";
			itemInfo.count = users_.getItemCount(username, "选将扩充卡");
			itemInfo.price = (itemInfo.count < 5) ? static_cast<Character::price_t>(88 + 100 * itemInfo.count * (itemInfo.count + 1) / 2) : Character::price_t{ 0 };
			itemInfo.available = itemInfo.count < 5;
			shopData.items.push_back(std::move(itemInfo));
		}
		// 增分卡
		{
			ShopItemInfo itemInfo;
			itemInfo.name = "增分卡";
			itemInfo.count = users_.getItemCount(username, "增分卡");
			itemInfo.price = 10;
			itemInfo.available = true;
			shopData.items.push_back(std::move(itemInfo));
		}

		// 不死图腾
		{
			ShopItemInfo itemInfo;
			itemInfo.name = "不死图腾";
			itemInfo.count = users_.getItemCount(username, "不死图腾");
			itemInfo.price = 10;
			itemInfo.available = true;
			shopData.items.push_back(std::move(itemInfo));
		}

		// 炸弹卡
		{
			ShopItemInfo itemInfo;
			itemInfo.name = "炸弹卡";
			itemInfo.count = users_.getItemCount(username, "炸弹卡");
			itemInfo.price = 5;
			itemInfo.available = true;
			shopData.items.push_back(std::move(itemInfo));
		}

		// 序列化 ShopData 为 JSON 并通过 sf::Packet 发送
		nlohmann::json j = shopData;
		std::string jsonStr = j.dump();

		sf::Packet packet;
		packet << static_cast<int>(MessageType::ShopData);
		packet << jsonStr;

		std::println("[ServerNetwork] 发送商城数据给客户端{}（账号 {}）：积分{}，角色{}个，道具{}个",
					 clientIndex, username, shopData.points, shopData.characters.size(), shopData.items.size());

		return session_.sendPayload(clientIndex, packet);
	}

	void ShopService::handleSkillInfoRequest(std::size_t clientIndex, const std::string& requestData) {
		if (!session_.isConnected(clientIndex)) return;

		SkillInfoRequestBody req;
		try {
			nlohmann::json j = nlohmann::json::parse(requestData);
			req = j.get<SkillInfoRequestBody>();
		}
		catch (const std::exception&) {
			return; // 解析失败直接忽略
		}

		// 逐角色生成技能描述文本（未定义的角色跳过）
		SkillInfoResponseBody resp;
		for (const auto& name : req.names) {
			try {
				resp.skills[name] = Character::make(name)->getSkillsText();
			}
			catch (const std::exception&) {}
		}

		nlohmann::json j = resp;
		sf::Packet packet;
		packet << static_cast<int>(MessageType::SkillInfoResponse);
		packet << j.dump();
		session_.sendPayload(clientIndex, packet);
	}

	void ShopService::handleCheckin(std::size_t clientIndex) {
		if (!session_.isConnected(clientIndex)) return;

		const std::string username = session_.usernameOf(clientIndex);
		if (username.empty()) return;

		const CheckinOutcome result = users_.checkin(username);

		sf::Packet packet;
		packet << static_cast<int>(MessageType::CheckinResponse);
		packet << result.msg;
		packet << result.ok;
		packet << result.points;
		session_.sendPayload(clientIndex, packet);

		//同步积分
		session_.onPointsChanged(clientIndex, result.points);
		std::println("[ServerNetwork] 客户端{}（账号 {}）签到：{}（当前积分 {}）",
					 clientIndex, username, (result.ok ? "成功" : "已签到"), result.points);
	}
}
