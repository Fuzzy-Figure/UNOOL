package com.unool.android.net

import android.util.Log
import com.unool.android.protocol.ClientMessage
import com.unool.android.protocol.ProtocolCodec
import com.unool.android.protocol.ProtocolException
import com.unool.android.protocol.ServerMessage
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.launch
import java.io.DataInputStream
import java.net.InetSocketAddress
import java.net.Socket
import java.nio.ByteBuffer

sealed interface ConnectionState {
    data object Disconnected : ConnectionState
    data object Connecting : ConnectionState
    data object Connected : ConnectionState
    data class Failed(val reason: String) : ConnectionState
}

/**
 * 与 PC 服务端保持一条 TCP 连接：按 SFML 帧头拆包、解码、回心跳。
 * 连接与读包在 IO 线程池中执行，写包用锁串行化。
 */
class GameConnection {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val sendLock = Any()
    private val outbox = Channel<ClientMessage>(Channel.UNLIMITED)

    init {
        // 所有写 socket 都在 IO 线程；Android 禁止主线程网络 IO。
        scope.launch(Dispatchers.IO) {
            for (message in outbox) {
                val stream = output ?: continue
                synchronized(sendLock) {
                    try {
                        val frame = ProtocolCodec.encode(message)
                        Log.d("UNOOL_NET", "send ${message::class.simpleName} bytes=${frame.size}")
                        stream.write(frame)
                        stream.flush()
                    } catch (e: Exception) {
                        Log.e("UNOOL_NET", "send failed: ${e.message}", e)
                    }
                }
            }
        }
    }

    @Volatile
    private var socket: Socket? = null

    @Volatile
    private var output: java.io.OutputStream? = null

    @Volatile
    private var closing = false

    val state = MutableStateFlow<ConnectionState>(ConnectionState.Disconnected)

    val messages = MutableSharedFlow<ServerMessage>(
        extraBufferCapacity = 512,
        onBufferOverflow = BufferOverflow.DROP_OLDEST
    )

    fun connect(host: String, port: Int) {
        closing = false
        closeSocket()
        scope.launch {
            state.value = ConnectionState.Connecting
            try {
                val s = Socket()
                s.tcpNoDelay = true
                s.soTimeout = 0
                s.connect(InetSocketAddress(host, port), 3000)
                socket = s
                output = s.getOutputStream()
                state.value = ConnectionState.Connected

                val input = DataInputStream(s.getInputStream())
                val header = ByteArray(4)
                while (!s.isClosed) {
                    input.readFully(header)
                    val size = ByteBuffer.wrap(header).int
                    if (size <= 0 || size > 16 * 1024 * 1024) {
                        throw ProtocolException("非法包长：$size")
                    }
                    val payload = ByteArray(size)
                    input.readFully(payload)
                    val message = try {
                        ProtocolCodec.decode(payload)
                    } catch (e: Exception) {
                        Log.e("UNOOL_NET", "decode failed: ${e.message}", e)
                        throw e
                    }
                    Log.d("UNOOL_NET", "recv size=$size msg=$message")
                    messages.emit(message)
                    if (message is ServerMessage.Heartbeat) {
                        send(ClientMessage.Heartbeat)
                    }
                }
            } catch (e: java.io.EOFException) {
                Log.d("UNOOL_NET", "eof")
                // 服务端正常关闭连接
                state.value = ConnectionState.Disconnected
            } catch (e: Exception) {
                Log.e("UNOOL_NET", "connection error: ${e.message}", e)
                state.value = if (closing) {
                    ConnectionState.Disconnected
                } else {
                    ConnectionState.Failed(e.message ?: e::class.simpleName ?: "连接失败")
                }
            } finally {
                closeSocket()
                messages.emit(ServerMessage.Disconnected)
            }
        }
    }

    fun send(message: ClientMessage) {
        // 从 UI 线程调用，只入队，不碰 socket。
        outbox.trySend(message)
    }

    fun disconnect() {
        closing = true
        closeSocket()
        state.value = ConnectionState.Disconnected
    }

    fun close() {
        closing = true
        closeSocket()
        scope.cancel()
    }

    private fun closeSocket() {
        output = null
        socket?.let {
            try {
                if (!it.isClosed) it.close()
            } catch (_: Exception) {
            }
        }
        socket = null
    }
}
