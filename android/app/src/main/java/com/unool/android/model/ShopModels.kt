package com.unool.android.model

import org.json.JSONObject

/** 商城等级显示：兼容服务端下发 "F" 或枚举数字 "0"~"5"。 */
object ShopLevel {
    fun label(value: String): String = when (value.trim()) {
        "0", "F" -> "F"
        "1", "D" -> "D"
        "2", "C" -> "C"
        "3", "B" -> "B"
        "4", "A" -> "A"
        "5", "S" -> "S"
        else -> value.ifBlank { "?" }
    }
}

data class ShopSkin(
    val name: String,
    val unlocked: Boolean,
    val price: Long?,
    val quality: String,
    val imagePath: String
)

data class ShopHero(
    val name: String,
    val level: String,
    val unlocked: Boolean,
    val price: Long?,
    val skins: List<ShopSkin>,
    val defaultImagePath: String
)

data class ShopItem(
    val name: String,
    val count: Int,
    val price: Long,
    val available: Boolean,
    val description: String,
    val maxCount: Int?
)

data class ShopSnapshot(
    val points: Int,
    val heroes: List<ShopHero>,
    val items: List<ShopItem>
)

data class ShopResult(val ok: Boolean, val msg: String, val points: Int)

object ShopParser {

    fun purchaseJson(type: String, name: String, charName: String, skinName: String): String {
        return JSONObject()
            .put("type", type)
            .put("name", name)
            .put("charName", charName)
            .put("skinName", skinName)
            .toString()
    }

    fun skillRequestJson(names: List<String>): String {
        val array = org.json.JSONArray()
        names.forEach { array.put(it) }
        return JSONObject().put("names", array).toString()
    }

    fun parseShopData(json: String): ShopSnapshot {
        val root = JSONObject(json)
        val heroes = mutableListOf<ShopHero>()
        val heroArray = root.optJSONArray("characters")
        if (heroArray != null) {
            for (i in 0 until heroArray.length()) {
                val obj = heroArray.getJSONObject(i)
                val skinArray = obj.optJSONArray("skins")
                val skins = mutableListOf<ShopSkin>()
                if (skinArray != null) {
                    for (j in 0 until skinArray.length()) {
                        val s = skinArray.getJSONObject(j)
                        skins += ShopSkin(
                            name = s.optString("name"),
                            unlocked = s.optBoolean("unlocked"),
                            price = optNullableLong(s, "price"),
                            quality = s.optString("quality"),
                            imagePath = s.optString("imagePath")
                        )
                    }
                }
                heroes += ShopHero(
                    name = obj.optString("name"),
                    level = obj.optString("level"),
                    unlocked = obj.optBoolean("unlocked"),
                    price = optNullableLong(obj, "price"),
                    skins = skins,
                    defaultImagePath = obj.optString("defaultImagePath")
                )
            }
        }

        val items = mutableListOf<ShopItem>()
        val itemArray = root.optJSONArray("items")
        if (itemArray != null) {
            for (i in 0 until itemArray.length()) {
                val obj = itemArray.getJSONObject(i)
                items += ShopItem(
                    name = obj.optString("name"),
                    count = obj.optInt("count"),
                    price = obj.optLong("price"),
                    available = obj.optBoolean("available"),
                    description = obj.optString("description"),
                    maxCount = optNullableInt(obj, "maxCount")
                )
            }
        }

        return ShopSnapshot(root.optInt("points"), heroes, items)
    }

    fun parseResult(json: String): ShopResult {
        val obj = JSONObject(json)
        return ShopResult(
            ok = obj.optBoolean("ok"),
            msg = obj.optString("msg"),
            points = obj.optInt("points")
        )
    }

    fun parseSkills(json: String): Map<String, String> {
        val obj = JSONObject(json).optJSONObject("skills") ?: return emptyMap()
        val result = HashMap<String, String>()
        obj.keys().forEach { result[it] = obj.optString(it) }
        return result
    }

    private fun optNullableLong(obj: JSONObject, key: String): Long? {
        if (obj.isNull(key)) return null
        return obj.optLong(key)
    }

    private fun optNullableInt(obj: JSONObject, key: String): Int? {
        if (obj.isNull(key)) return null
        return obj.optInt(key)
    }
}
