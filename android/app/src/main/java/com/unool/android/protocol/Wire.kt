package com.unool.android.protocol

import java.io.ByteArrayOutputStream
import java.nio.ByteBuffer

/**
 * SFML 3.1 sf::Packet 线格式：
 * - 帧头：4 字节大端 uint32，表示后续 payload 的字节数（由 TcpSocket::send(Packet&) 添加）
 * - 整数：16/32/64 位均为大端（htons/htonl/手写 htonll）
 * - bool：1 字节（先转 uint8）
 * - std::string：4 字节大端 uint32 字节长度 + UTF-8 原始字节
 * - std::size_t（x64）：8 字节大端 uint64
 *
 * 注意：不要按 SFML 2.x 的小端 / uint64 字符串长度实现，连不上 PC 服务端。
 */
class ProtocolException(message: String) : Exception(message)

class PacketWriter {
    private val payload = ByteArrayOutputStream()

    fun u8(value: Int): PacketWriter {
        payload.write(value and 0xff)
        return this
    }

    fun bool(value: Boolean): PacketWriter = u8(if (value) 1 else 0)

    fun i32(value: Int): PacketWriter {
        payload.write(ByteBuffer.allocate(4).putInt(value).array())
        return this
    }

    fun u32(value: Long): PacketWriter {
        payload.write(ByteBuffer.allocate(4).putInt(value.toInt()).array())
        return this
    }

    fun u64(value: Long): PacketWriter {
        payload.write(ByteBuffer.allocate(8).putLong(value).array())
        return this
    }

    fun str(value: String): PacketWriter {
        val bytes = value.toByteArray(Charsets.UTF_8)
        u32(bytes.size.toLong())
        payload.write(bytes, 0, bytes.size)
        return this
    }

    /** 生成带 4 字节大端帧头的完整 TCP 帧 */
    fun frame(): ByteArray {
        val body = payload.toByteArray()
        val header = ByteBuffer.allocate(4).putInt(body.size).array()
        return header + body
    }
}

class PacketReader(payload: ByteArray) {
    private val data = payload
    private var pos = 0

    val eof: Boolean get() = pos >= data.size
    val remaining: Int get() = data.size - pos

    private fun take(size: Int): ByteArray {
        if (size < 0 || size > remaining) {
            throw ProtocolException("包提前结束：需要 $size 字节，剩余 $remaining 字节")
        }
        val result = data.copyOfRange(pos, pos + size)
        pos += size
        return result
    }

    fun u8(): Int = take(1)[0].toInt() and 0xff
    fun bool(): Boolean = u8() != 0
    fun i32(): Int = ByteBuffer.wrap(take(4)).int
    fun u32(): Long = ByteBuffer.wrap(take(4)).int.toLong() and 0xffffffffL
    fun u64(): Long = ByteBuffer.wrap(take(8)).long

    fun count(): Int {
        val n = u64()
        if (n < 0 || n > 1_000_000L) throw ProtocolException("非法容器长度：$n")
        return n.toInt()
    }

    fun str(): String {
        val length = u32()
        if (length > Int.MAX_VALUE.toLong()) throw ProtocolException("字符串过长：$length")
        return String(take(length.toInt()), Charsets.UTF_8)
    }

    fun assertEnd() {
        if (!eof) throw ProtocolException("包尾残留 $remaining 字节")
    }
}
