package com.unool.android.server

import android.content.Context
import android.os.Build
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import java.io.File
import java.util.concurrent.TimeUnit

/**
 * 随 APK 打包的 SakuraFrp frpc 进程控制。
 * frpc 二进制放在 jniLibs/<abi>/libfrpc.so，安装后位于 nativeLibraryDir，
 * 该目录可执行；访问密钥只作为进程参数传入，绝不写进日志。
 */
object FrpcController {
    private val _running = MutableStateFlow(false)
    val running: StateFlow<Boolean> = _running

    private var process: Process? = null

    fun isAvailable(context: Context): Boolean =
        File(context.applicationInfo.nativeLibraryDir, "libfrpc.so").exists()

    fun start(context: Context, token: String, tunnelIds: String): Boolean {
        if (process?.isAlive == true) return true

        val trimmedToken = token.trim()
        val trimmedIds = tunnelIds.trim()
        if (trimmedToken.isEmpty() || trimmedIds.isEmpty()) {
            ServerState.appendLog("[SakuraFrp] 未填写访问密钥或隧道 ID，跳过内网穿透", true)
            return false
        }

        val executable = File(context.applicationInfo.nativeLibraryDir, "libfrpc.so")
        if (!executable.exists()) {
            ServerState.appendLog("[SakuraFrp] 未找到 frpc 可执行文件（libfrpc.so）", true)
            return false
        }

        // 官方启动参数：frpc --disable_log_color -f <访问密钥>:<隧道ID>
        // 多条同节点隧道可用英文逗号分隔，原样透传。
        val command = listOf(
            executable.absolutePath,
            "--disable_log_color",
            "-f",
            "$trimmedToken:$trimmedIds"
        )
        val started = ProcessBuilder(command)
            .directory(context.filesDir)
            .redirectErrorStream(true)
            .start()

        process = started
        _running.value = true
        ServerState.appendLog("[SakuraFrp] 隧道已启动（ID: $trimmedIds）")

        Thread({
            try {
                started.inputStream.bufferedReader().useLines { lines ->
                    lines.forEach { line ->
                        if (line.isNotBlank()) ServerState.appendLog("[SakuraFrp] $line")
                    }
                }
            }
            catch (_: Exception) {
                // 进程被停止时流会关闭，属正常退出路径
            }
            finally {
                val code = try { started.waitFor() } catch (_: Exception) { -1 }
                if (process === started) process = null
                _running.value = false
                ServerState.appendLog("[SakuraFrp] 进程退出（code=$code）", code != 0)
            }
        }, "unool-frpc").start()

        return true
    }

    fun stop() {
        val current = process ?: return
        process = null
        _running.value = false
        current.destroy()
        ServerState.appendLog("[SakuraFrp] 正在关闭隧道...")

        Thread({
            try {
                val exited = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    current.waitFor(2, TimeUnit.SECONDS)
                }
                else {
                    current.waitFor()
                    true
                }
                if (!exited) current.destroyForcibly()
            }
            catch (_: Exception) {
                // 忽略关闭过程中的异常
            }
        }, "unool-frpc-stop").start()
    }
}
