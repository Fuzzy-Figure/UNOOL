package com.unool.android.server

import android.content.Context
import android.content.res.AssetManager

/**
 * Java/Kotlin 与 C++ 服务端的唯一边界。
 * nativeRun 会阻塞调用线程，必须由 Service 的工作线程调用。
 */
object ServerBridge {
    init {
        System.loadLibrary("unool-server")
    }

    @JvmStatic
    external fun nativeRun(port: Int, dataDir: String, assets: AssetManager): Boolean

    @JvmStatic
    external fun nativeStop()

    @JvmStatic
    external fun nativeIsRunning(): Boolean

    /** 由 C++ 服务端回调，把原生日志送进 UI。 */
    @JvmStatic
    fun onNativeLog(message: String, isError: Boolean) {
        ServerState.appendLog(message, isError)
    }

    fun start(context: Context, port: Int): Boolean {
        ServerState.clearLogs()
        return nativeRun(port, context.filesDir.absolutePath, context.assets)
    }
}
