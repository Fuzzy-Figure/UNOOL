package com.unool.android.server

import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

/** 服务进程内的共享状态：Service 写，Activity 订阅。 */
object ServerState {
    data class LogLine(val text: String, val error: Boolean = false)

    private val _running = MutableStateFlow(false)
    val running: StateFlow<Boolean> = _running

    private val _logs = MutableStateFlow<List<LogLine>>(emptyList())
    val logs: StateFlow<List<LogLine>> = _logs

    private val _lastError = MutableStateFlow<String?>(null)
    val lastError: StateFlow<String?> = _lastError

    fun setRunning(value: Boolean) {
        _running.value = value
    }

    fun appendLog(text: String, error: Boolean = false) {
        val line = LogLine(text.trimEnd(), error)
        _logs.value = (_logs.value + line).takeLast(200)
        if (error) _lastError.value = line.text
    }

    fun clearLogs() {
        _logs.value = emptyList()
        _lastError.value = null
    }
}
