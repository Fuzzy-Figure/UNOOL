#pragma once
#include <string>
#include <vector>
#include <json.hpp>
#include "Character.h"

// 让 nlohmann::json 支持 Character::Level 枚举序列化
NLOHMANN_JSON_SERIALIZE_ENUM(Character::Level, {
	{Character::Level::F, "F"},
	{Character::Level::D, "D"},
	{Character::Level::C, "C"},
	{Character::Level::B, "B"},
	{Character::Level::A, "A"},
	{Character::Level::S, "S"},
})

// 皮肤信息（商城展示用）
struct ShopSkinInfo {
	std::string name;           // 皮肤名
	bool unlocked = false;      // 是否已解锁
	int price = 0;              // 售价（-1=不可购买）
	std::string quality;        // 品质中文名

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShopSkinInfo, name, unlocked, price, quality)
};

// 角色信息（商城展示用）
struct ShopCharacterInfo {
	std::string name;               // 角色名
	Character::Level level = Character::Level::F; // 等级
	bool unlocked = false;          // 是否已解锁英雄
	int price = 0;                  // 英雄售价（-1=不可购买如S档）
	std::vector<ShopSkinInfo> skins; // 皮肤列表（不含默认皮肤）

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShopCharacterInfo, name, level, unlocked, price, skins)
};

// 道具信息（商城展示用）
struct ShopItemInfo {
	std::string name;           // 道具名
	int count = 0;              // 当前数量
	int price = 0;              // 当前售价
	bool available = true;      // 是否可购买

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShopItemInfo, name, count, price, available)
};

// 完整商城数据
struct ShopData {
	int points = 0;                          // 用户当前积分
	std::vector<ShopCharacterInfo> characters; // 角色列表（按F→S排序）
	std::vector<ShopItemInfo> items;           // 道具列表

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShopData, points, characters, items)
};

// 购买请求（客户端→服务器），通过 MessageType::ShopPurchase 发送
// type 取值："request"(请求刷新商城数据) / "character" / "skin" / "item"
struct ShopPurchaseRequest {
	std::string type;        // 购买类型
	std::string name;        // 角色名/道具名（type=character/item 时用）
	std::string charName;    // 皮肤所属角色名（type=skin 时用）
	std::string skinName;    // 皮肤名（type=skin 时用）

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShopPurchaseRequest, type, name, charName, skinName)
};

// 购买结果（服务器→客户端），通过 MessageType::ShopResult 发送
struct ShopPurchaseResult {
	bool ok = false;         // 是否成功
	std::string msg;         // 结果消息
	int points = 0;          // 购买后剩余积分

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(ShopPurchaseResult, ok, msg, points)
};
