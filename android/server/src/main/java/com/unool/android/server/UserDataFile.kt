package com.unool.android.server

import android.content.Context
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
            JSONObject(file.readText(Charsets.UTF_8)).toString(2)
        }
        catch (_: Exception) {
            file.readText(Charsets.UTF_8)
        }
    }

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
            val pretty = JSONObject(text).toString(2)
            file.writeText(pretty, Charsets.UTF_8)
            null
        }
        catch (e: Exception) {
            "保存失败：${e.message ?: "无法写入本地文件"}"
        }
    }
}
