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
 *
 * 注意：不要把 frpc 的 stdout/stderr 接到 Java 管道再逐行读。
 * 读端一旦关闭，frpc 后续写日志会收到 SIGPIPE，退出码就是 141。
 * 现在改为重定向到 filesDir/frpc.log，再由本类轮询文件增量显示。
 */
object FrpcController {
    private val _running = MutableStateFlow(false)
    val running: StateFlow<Boolean> = _running

    private var process: Process? = null
    private var watcher: Thread? = null

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

        val logFile = File(context.filesDir, "frpc.log")
        try {
            if (logFile.exists()) logFile.writeText("")
        }
        catch (_: Exception) {
            ServerState.appendLog("[SakuraFrp] 无法清空旧 frpc.log，将追加写入", true)
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
            .redirectOutput(logFile)
            .start()

        process = started
        _running.value = true
        ServerState.appendLog("[SakuraFrp] 隧道已启动（ID: $trimmedIds）")

        watcher?.interrupt()
        watcher = Thread({
            var lastLines = emptyList<String>()

            fun drainLog() {
                val lines = try {
                    if (logFile.exists()) logFile.readLines() else emptyList()
                }
                catch (_: Exception) {
                    emptyList()
                }
                val fresh = when {
                    lines.size < lastLines.size -> lines
                    lines.size == lastLines.size && lines.lastOrNull() != lastLines.lastOrNull() ->
                        lines.takeLast(1)
                    else -> lines.drop(lastLines.size)
                }
                fresh.filter { it.isNotBlank() }.forEach { line ->
                    ServerState.appendLog("[SakuraFrp] $line")
                }
                lastLines = lines
            }

            while (process === started && started.isAlive) {
                drainLog()
                try {
                    Thread.sleep(400)
                }
                catch (_: InterruptedException) {
                    break
                }
            }

            drainLog()
            val code = try { started.waitFor() } catch (_: Exception) { -1 }
            if (process === started) process = null
            _running.value = false
            val message = if (code == 141) {
                "[SakuraFrp] 进程退出（code=141）：旧版 stdout 管道问题已改文件日志；若再次出现请检查账号/隧道状态"
            }
            else {
                "[SakuraFrp] 进程退出（code=$code）"
            }
            ServerState.appendLog(message, code != 0)
        }, "unool-frpc").also { it.start() }

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
