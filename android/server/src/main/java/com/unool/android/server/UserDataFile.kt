package com.unool.android.server

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import java.io.File

/**
 * 账号存档 userDatas.json 的读取、校验与保存。
 * 路径与 native 的 Platform::exeDir() 一致：App filesDir/userDatas.json。
 */
object UserDataFile {
    private const val FILE_NAME = "userDatas.json"
    private const val BACKUP_NAME = "userDatas.json.bak"

    fun file(context: Context): File = File(context.filesDir, FILE_NAME)

    fun loadText(context: Context): String {
        val file = file(context)
        if (!file.exists()) return "{}"
        return try {
            prettyJson(JSONObject(file.readText(Charsets.UTF_8)))
        }
        catch (_: Exception) {
            file.readText(Charsets.UTF_8)
        }
    }

    /**
     * 与 C++ UserDB::dumpJson 对齐：
     * ownedCharacters 下每个角色的皮肤数组保持单行：
     *   "角色名": [ "皮肤1", "皮肤2" ]
     */
    private fun prettyJson(value: Any?): String = buildPretty(value, 0, false)

    private fun buildPretty(value: Any?, indent: Int, compactArray: Boolean): String = when (value) {
        is JSONObject -> {
            if (value.length() == 0) {
                "{}"
            }
            else {
                val keys = value.keys()
                val lines = mutableListOf<String>()
                while (keys.hasNext()) {
                    val key = keys.next()
                    val child = value.opt(key)
                    val childCompact = key == "ownedCharacters" || compactArray
                    lines += "${pad(indent + 1)}${JSONObject.quote(key)}: ${buildPretty(child, indent + 1, childCompact)}"
                }
                "{\n${lines.joinToString(",\n")}\n${pad(indent)}}"
            }
        }

        is JSONArray -> {
            if (value.length() == 0) {
                "[]"
            }
            else if (compactArray) {
                val items = (0 until value.length()).map { index ->
                    compactValue(value.opt(index))
                }
                "[ ${items.joinToString(", ")} ]"
            }
            else {
                val lines = mutableListOf<String>()
                for (index in 0 until value.length()) {
                    lines += "${pad(indent + 1)}${buildPretty(value.opt(index), indent + 1, false)}"
                }
                "[\n${lines.joinToString(",\n")}\n${pad(indent)}]"
            }
        }

        is String -> JSONObject.quote(value)
        JSONObject.NULL -> "null"
        null -> "null"
        else -> value.toString()
    }

    private fun compactValue(value: Any?): String = when (value) {
        is String -> JSONObject.quote(value)
        JSONObject.NULL, null -> "null"
        is JSONArray -> {
            val items = (0 until value.length()).map { index -> compactValue(value.opt(index)) }
            "[ ${items.joinToString(", ")} ]"
        }
        is JSONObject -> {
            val keys = value.keys()
            val items = mutableListOf<String>()
            while (keys.hasNext()) {
                val key = keys.next()
                items += "${JSONObject.quote(key)}: ${compactValue(value.opt(key))}"
            }
            "{ ${items.joinToString(", ")} }"
        }
        else -> value.toString()
    }

    private fun pad(indent: Int): String = "  ".repeat(indent)

    /** 返回 null 表示校验通过；否则返回给界面显示的错误原因。 */
    private fun validate(text: String): String? {
        if (text.isBlank()) return "内容为空，不能保存。"

        val root = try {
            JSONObject(text)
        }
        catch (e: Exception) {
            return "JSON 格式错误：${e.message ?: "无法解析"}"
        }

        val keys = root.keys()
        while (keys.hasNext()) {
            val username = keys.next()
            val value = root.opt(username)
            if (value !is JSONObject) {
                return "账号 [$username] 的值必须是 JSON 对象。"
            }

            val hash = value.optString("passwordHash", "")
            val salt = value.optString("salt", "")
            if (hash.isBlank() || salt.isBlank()) {
                return "账号 [$username] 缺少 passwordHash 或 salt，保存后会无法登录。"
            }

            for (key in listOf("points", "wins", "losses", "checkinStreak", "checkinPrize3Today")) {
                if (!value.has(key)) continue
                val number = value.opt(key)
                if (number !is Number) {
                    return "账号 [$username] 的 $key 必须是数字。"
                }
            }

            for (key in listOf("items", "ownedCharacters")) {
                if (!value.has(key)) continue
                if (value.opt(key) !is JSONObject) {
                    return "账号 [$username] 的 $key 必须是 JSON 对象。"
                }
            }
        }
        return null
    }

    /** 返回 null 表示保存成功；否则返回给界面显示的错误原因。 */
    fun saveText(context: Context, text: String): String? {
        val validationError = validate(text)
        if (validationError != null) return validationError

        return try {
            val file = file(context)
            if (file.exists()) {
                file.copyTo(File(context.filesDir, BACKUP_NAME), overwrite = true)
            }
            file.writeText(prettyJson(JSONObject(text)), Charsets.UTF_8)
            null
        }
        catch (e: Exception) {
            "保存失败：${e.message ?: "无法写入本地文件"}"
        }
    }
}
