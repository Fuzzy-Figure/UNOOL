package com.unool.android.server

import android.content.Context

/**
 * SakuraFrp 隧道配置。访问密钥属于敏感信息，只存在应用私有 SharedPreferences，
 * 不写入日志，也不随 APK 分发。
 */
data class FrpcConfig(
    val token: String = "",
    val tunnelIds: String = "",
    val autoStart: Boolean = false
) {
    companion object {
        private const val PREFS = "unool_sakurafrp"
        private const val KEY_TOKEN = "token"
        private const val KEY_TUNNELS = "tunnel_ids"
        private const val KEY_AUTO = "auto_start"

        fun load(context: Context): FrpcConfig {
            val prefs = context.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
            return FrpcConfig(
                token = prefs.getString(KEY_TOKEN, "") ?: "",
                tunnelIds = prefs.getString(KEY_TUNNELS, "") ?: "",
                autoStart = prefs.getBoolean(KEY_AUTO, false)
            )
        }

        fun save(context: Context, config: FrpcConfig) {
            context.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().apply {
                putString(KEY_TOKEN, config.token.trim())
                putString(KEY_TUNNELS, config.tunnelIds.trim())
                putBoolean(KEY_AUTO, config.autoStart)
                apply()
            }
        }
    }
}
