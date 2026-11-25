package com.unool.android.server

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * 服务端配置的可编辑部分。
 * 文件位置与 native 侧 Platform::exeDir() 一致：App filesDir/server_config.json。
 * 未打开过编辑界面时沿用 APK assets 里的默认模板。
 */
data class ServerConfig(
    val mode: String = "normal",
    val maxClients: Int = 32,
    val banCount: Int = 1,
    val normalCandidateCount: Int = 5,
    val normalInitHandCount: Int = 8,
    val doubleCandidateCount: Int = 5,
    val doubleInitHandCount: Int = 10,
    val assign: List<String> = emptyList(),
    val shieldedCharacters: List<String> = emptyList(),
    val shieldedGroups: List<String> = listOf("实验")
) {
    companion object {
        private const val FILE_NAME = "server_config.json"

        // 来源：source/core/Character.cpp 的 Character::infos 分组（2026-11-25 提取）。
        // 服务端 App 不链接 C++ 角色表，这里内置分组清单供配置界面勾选。
        val allGroups: List<String> = listOf(
            "元首", "其他", "原神", "实验", "山海经",
            "希腊神话", "新三国", "网络", "荒野乱斗"
        )

        private fun configFile(context: Context): File = File(context.filesDir, FILE_NAME)

        private fun defaultRoot(): JSONObject = JSONObject().apply {
            put("mode", "normal")
            put("characters", JSONObject().apply {
                put("shielded", JSONObject().apply {
                    put("characters", JSONArray())
                    put("groups", JSONArray().put("实验"))
                })
            })
        }

        private fun toStringList(array: JSONArray): List<String> =
            (0 until array.length()).map { index -> array.optString(index) }
                .filter { it.isNotBlank() }

        private fun toJsonArray(items: List<String>): JSONArray = JSONArray().apply {
            items.forEach { put(it) }
        }

        fun load(context: Context): ServerConfig {
            val file = configFile(context)
            if (!file.exists()) return ServerConfig()
            return try {
                val root = JSONObject(file.readText(Charsets.UTF_8))
                val network = root.optJSONObject("network")
                val rules = root.optJSONObject("rules")
                val normal = rules?.optJSONObject("normal")
                val double = rules?.optJSONObject("double")
                val characters = root.optJSONObject("characters")
                val shielded = characters?.optJSONObject("shielded")
                ServerConfig(
                    mode = root.optString("mode", "normal"),
                    maxClients = network?.optInt("maxClients", 32) ?: 32,
                    banCount = rules?.optInt("banCount", 1) ?: 1,
                    normalCandidateCount = normal?.optInt("candidateCount", 5) ?: 5,
                    normalInitHandCount = normal?.optInt("initHandCount", 8) ?: 8,
                    doubleCandidateCount = double?.optInt("candidateCount", 5) ?: 5,
                    doubleInitHandCount = double?.optInt("initHandCount", 10) ?: 10,
                    assign = characters?.optJSONArray("assign")?.let(::toStringList) ?: emptyList(),
                    shieldedCharacters = shielded?.optJSONArray("characters")?.let(::toStringList) ?: emptyList(),
                    shieldedGroups = shielded?.optJSONArray("groups")?.let(::toStringList) ?: listOf("实验")
                )
            }
            catch (_: Exception) {
                ServerConfig()
            }
        }

        fun save(context: Context, config: ServerConfig): Boolean = try {
            val file = configFile(context)
            val root = if (file.exists()) {
                try {
                    JSONObject(file.readText(Charsets.UTF_8))
                }
                catch (_: Exception) {
                    defaultRoot()
                }
            }
            else {
                defaultRoot()
            }

            // 保留用户已有的其它字段，只覆盖界面暴露的这几项。
            root.put("mode", config.mode)

            val characters = root.optJSONObject("characters") ?: JSONObject().also {
                root.put("characters", it)
            }
            // 指定角色留空时删除 assign 键，绝不能写空数组：
            // native 侧只要键存在就会走指定角色分支并因数量不符抛异常。
            if (config.assign.isEmpty()) {
                characters.remove("assign")
            }
            else {
                characters.put("assign", toJsonArray(config.assign))
            }

            val shielded = characters.optJSONObject("shielded") ?: JSONObject().also {
                characters.put("shielded", it)
            }
            shielded.put("characters", toJsonArray(config.shieldedCharacters))
            shielded.put("groups", toJsonArray(config.shieldedGroups))

            val network = root.optJSONObject("network") ?: JSONObject().also {
                root.put("network", it)
            }
            network.put("maxClients", config.maxClients)
            if (!network.has("maxReconnectAttempts")) network.put("maxReconnectAttempts", 100)
            if (!network.has("reconnectTimeoutSec")) network.put("reconnectTimeoutSec", 600)

            val rules = root.optJSONObject("rules") ?: JSONObject().also {
                root.put("rules", it)
            }
            rules.put("banCount", config.banCount)

            val normal = rules.optJSONObject("normal") ?: JSONObject().also {
                rules.put("normal", it)
            }
            normal.put("candidateCount", config.normalCandidateCount)
            normal.put("initHandCount", config.normalInitHandCount)

            val double = rules.optJSONObject("double") ?: JSONObject().also {
                rules.put("double", it)
            }
            double.put("candidateCount", config.doubleCandidateCount)
            double.put("initHandCount", config.doubleInitHandCount)

            file.writeText(root.toString(2), Charsets.UTF_8)
            true
        }
        catch (_: Exception) {
            false
        }
    }
}
