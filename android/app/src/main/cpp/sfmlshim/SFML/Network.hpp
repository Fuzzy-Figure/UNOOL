#pragma once
// Android 构建用的 SFML 3 网络层最小实现。
// 只覆盖 UNOOL 服务端实际使用的 API；线格式与 SFML 3.1 保持一致：
//   TcpSocket::send(Packet&) = 4 字节大端长度 + Packet 原始字节；
//   Packet 整数大端、bool 1 字节、std::string = 4 字节大端长度 + UTF-8。
// 这样 PC 客户端、现有 Kotlin 安卓客户端和本机服务端三者仍可互通。
#include <algorithm>
#include <arpa/inet.h>
#include <bit>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace sf {

class Time {
public:
    constexpr explicit Time(const float seconds = 0.f) : seconds_(seconds) {}
    [[nodiscard]] constexpr float asSeconds() const { return seconds_; }
    [[nodiscard]] constexpr std::int32_t asMilliseconds() const {
        return static_cast<std::int32_t>(seconds_ * 1000.f);
    }
private:
    float seconds_{};
};

inline Time milliseconds(const float value) { return Time(value / 1000.f); }
inline Time seconds(const float value) { return Time(value); }

class Clock {
public:
    Clock() : start_(std::chrono::steady_clock::now()) {}
    [[nodiscard]] Time getElapsedTime() const {
        return Time(std::chrono::duration<float>(std::chrono::steady_clock::now() - start_).count());
    }
    Time restart() {
        const auto now = std::chrono::steady_clock::now();
        const Time elapsed(std::chrono::duration<float>(now - start_).count());
        start_ = now;
        return elapsed;
    }
private:
    std::chrono::steady_clock::time_point start_;
};

class TcpSocket;

class Packet {
public:
    Packet() = default;
    virtual ~Packet() = default;
    Packet(const Packet&) = default;
    Packet& operator=(const Packet&) = default;
    Packet(Packet&&) noexcept = default;
    Packet& operator=(Packet&&) noexcept = default;

    void append(const void* data, const std::size_t sizeInBytes) {
        if (data == nullptr || sizeInBytes == 0) return;
        const auto* first = static_cast<const std::byte*>(data);
        m_data.insert(m_data.end(), first, first + sizeInBytes);
    }

    [[nodiscard]] std::size_t getReadPosition() const { return m_readPos; }

    void clear() {
        m_data.clear();
        m_readPos = 0;
        m_sendPos = 0;
        m_isValid = true;
    }

    [[nodiscard]] const void* getData() const {
        return m_data.empty() ? nullptr : static_cast<const void*>(m_data.data());
    }

    [[nodiscard]] std::size_t getDataSize() const { return m_data.size(); }

    [[nodiscard]] bool endOfPacket() const { return m_readPos >= m_data.size(); }

    explicit operator bool() const { return m_isValid; }

    Packet& operator>>(bool& data) {
        std::uint8_t raw{};
        *this >> raw;
        data = raw != 0;
        return *this;
    }

    Packet& operator>>(std::int8_t& data) {
        data = static_cast<std::int8_t>(readUInt<std::uint8_t>());
        return *this;
    }
    Packet& operator>>(std::uint8_t& data) { data = readUInt<std::uint8_t>(); return *this; }
    Packet& operator>>(std::int16_t& data) {
        data = static_cast<std::int16_t>(readUInt<std::uint16_t>());
        return *this;
    }
    Packet& operator>>(std::uint16_t& data) { data = readUInt<std::uint16_t>(); return *this; }
    Packet& operator>>(std::int32_t& data) {
        data = static_cast<std::int32_t>(readUInt<std::uint32_t>());
        return *this;
    }
    Packet& operator>>(std::uint32_t& data) { data = readUInt<std::uint32_t>(); return *this; }
    Packet& operator>>(std::int64_t& data) {
        data = static_cast<std::int64_t>(readUInt<std::uint64_t>());
        return *this;
    }
    Packet& operator>>(std::uint64_t& data) { data = readUInt<std::uint64_t>(); return *this; }
    Packet& operator>>(float& data) { data = std::bit_cast<float>(readUInt<std::uint32_t>()); return *this; }
    Packet& operator>>(double& data) { data = std::bit_cast<double>(readUInt<std::uint64_t>()); return *this; }

    Packet& operator>>(char* data) {
        std::string value;
        *this >> value;
        if (m_isValid && data != nullptr) {
            std::memcpy(data, value.c_str(), value.size() + 1);
        }
        return *this;
    }

    Packet& operator>>(std::string& data) {
        const std::uint32_t length = readUInt<std::uint32_t>();
        if (!m_isValid || !checkSize(length)) return *this;
        data.assign(reinterpret_cast<const char*>(m_data.data() + m_readPos), length);
        m_readPos += length;
        return *this;
    }

    Packet& operator>>(wchar_t* data) {
        std::wstring value;
        *this >> value;
        if (m_isValid && data != nullptr) {
            std::memcpy(data, value.c_str(), (value.size() + 1) * sizeof(wchar_t));
        }
        return *this;
    }

    Packet& operator>>(std::wstring& data) {
        const std::uint32_t length = readUInt<std::uint32_t>();
        data.clear();
        if (!m_isValid) return *this;
        data.reserve(length);
        for (std::uint32_t i = 0; i < length; ++i) {
            const auto code = readUInt<std::uint32_t>();
            if (!m_isValid) return *this;
            data.push_back(static_cast<wchar_t>(code));
        }
        return *this;
    }

    Packet& operator<<(const bool data) { writeUInt<std::uint8_t>(data ? 1 : 0); return *this; }
    Packet& operator<<(const std::int8_t data) { writeUInt<std::uint8_t>(static_cast<std::uint8_t>(data)); return *this; }
    Packet& operator<<(const std::uint8_t data) { writeUInt<std::uint8_t>(data); return *this; }
    Packet& operator<<(const std::int16_t data) { writeUInt<std::uint16_t>(static_cast<std::uint16_t>(data)); return *this; }
    Packet& operator<<(const std::uint16_t data) { writeUInt<std::uint16_t>(data); return *this; }
    Packet& operator<<(const std::int32_t data) { writeUInt<std::uint32_t>(static_cast<std::uint32_t>(data)); return *this; }
    Packet& operator<<(const std::uint32_t data) { writeUInt<std::uint32_t>(data); return *this; }
    Packet& operator<<(const std::int64_t data) { writeUInt<std::uint64_t>(static_cast<std::uint64_t>(data)); return *this; }
    Packet& operator<<(const std::uint64_t data) { writeUInt<std::uint64_t>(data); return *this; }
    Packet& operator<<(const float data) { writeUInt<std::uint32_t>(std::bit_cast<std::uint32_t>(data)); return *this; }
    Packet& operator<<(const double data) { writeUInt<std::uint64_t>(std::bit_cast<std::uint64_t>(data)); return *this; }

    Packet& operator<<(const char* data) {
        if (data == nullptr) return *this << std::string{};
        return *this << std::string(data);
    }
    Packet& operator<<(const std::string& data) {
        writeUInt<std::uint32_t>(static_cast<std::uint32_t>(data.size()));
        append(data.data(), data.size());
        return *this;
    }
    Packet& operator<<(const wchar_t* data) {
        if (data == nullptr) return *this << std::wstring{};
        return *this << std::wstring(data);
    }
    Packet& operator<<(const std::wstring& data) {
        writeUInt<std::uint32_t>(static_cast<std::uint32_t>(data.size()));
        for (const wchar_t ch : data) {
            writeUInt<std::uint32_t>(static_cast<std::uint32_t>(ch));
        }
        return *this;
    }

protected:
    friend class TcpSocket;

    virtual const void* onSend(std::size_t& size) {
        size = m_data.size();
        return m_data.empty() ? nullptr : static_cast<const void*>(m_data.data());
    }
    virtual void onReceive(const void* data, const std::size_t size) {
        append(data, size);
    }

private:
    bool checkSize(const std::size_t size) {
        if (size > m_data.size() - std::min(m_readPos, m_data.size())) {
            m_isValid = false;
            return false;
        }
        return true;
    }

    template <class UInt>
    [[nodiscard]] UInt readUInt() {
        UInt value{};
        if (!checkSize(sizeof(UInt))) return value;
        const auto* bytes = reinterpret_cast<const unsigned char*>(m_data.data() + m_readPos);
        for (std::size_t i = 0; i < sizeof(UInt); ++i) {
            value = static_cast<UInt>((value << 8) | bytes[i]);
        }
        m_readPos += sizeof(UInt);
        return value;
    }

    template <class UInt>
    void writeUInt(const UInt value) {
        unsigned char bytes[sizeof(UInt)]{};
        for (std::size_t i = 0; i < sizeof(UInt); ++i) {
            bytes[i] = static_cast<unsigned char>(
                (value >> (8 * (sizeof(UInt) - 1 - i))) & static_cast<UInt>(0xff));
        }
        append(bytes, sizeof(UInt));
    }

    std::vector<std::byte> m_data;
    std::size_t m_readPos{};
    std::size_t m_sendPos{};
    bool m_isValid{true};
};

class Socket {
public:
    enum class Status { Done, NotReady, Partial, Disconnected, Error, ConnectionRefused };

    virtual ~Socket() = default;
    [[nodiscard]] virtual int nativeHandle() const = 0;
    [[nodiscard]] virtual bool hasPendingFrame() const { return false; }
};

class TcpSocket final : public Socket {
public:
    TcpSocket() = default;
    ~TcpSocket() override { disconnect(); }

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    void attach(const int fd) {
        disconnect();
        fd_ = fd;
    }

    [[nodiscard]] int nativeHandle() const override { return fd_; }

    void setBlocking(const bool blocking) {
        const int flags = ::fcntl(fd_, F_GETFL, 0);
        if (flags < 0) return;
        const int next = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        (void)::fcntl(fd_, F_SETFL, next);
        blocking_ = blocking;
    }

    Status send(Packet& packet) {
        if (fd_ < 0) return Status::Disconnected;
        std::size_t bodySize = 0;
        const void* body = packet.onSend(bodySize);
        std::vector<unsigned char> frame(sizeof(std::uint32_t) + bodySize);
        const std::uint32_t wireSize = static_cast<std::uint32_t>(bodySize);
        for (std::size_t i = 0; i < sizeof(std::uint32_t); ++i) {
            frame[i] = static_cast<unsigned char>(wireSize >> (8 * (3 - i)));
        }
        if (bodySize > 0 && body != nullptr) {
            std::memcpy(frame.data() + sizeof(std::uint32_t), body, bodySize);
        }

        std::size_t sent = 0;
        while (sent < frame.size()) {
            const ssize_t written = ::send(fd_, frame.data() + sent, frame.size() - sent, MSG_NOSIGNAL);
            if (written > 0) {
                sent += static_cast<std::size_t>(written);
                continue;
            }
            if (written < 0 && errno == EINTR) continue;
            if (written < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                return sent == 0 ? Status::NotReady : Status::Partial;
            }
            return Status::Error;
        }
        return Status::Done;
    }

    Status receive(Packet& packet) {
        pumpIncoming();
        if (inbound_.size() >= sizeof(std::uint32_t)) {
            const std::uint32_t length = readFrameSize();
            if (length > 16u * 1024u * 1024u) return Status::Error;
            if (inbound_.size() >= sizeof(std::uint32_t) + length) {
                packet.onReceive(inbound_.data() + sizeof(std::uint32_t), length);
                inbound_.erase(inbound_.begin(),
                               inbound_.begin() + sizeof(std::uint32_t) + length);
                return Status::Done;
            }
        }
        return disconnected_ ? Status::Disconnected : Status::NotReady;
    }

    [[nodiscard]] bool hasPendingFrame() const override {
        if (inbound_.size() < sizeof(std::uint32_t)) return false;
        return inbound_.size() >= sizeof(std::uint32_t) + readFrameSize();
    }

    void disconnect() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
        inbound_.clear();
        disconnected_ = false;
    }

private:
    [[nodiscard]] std::uint32_t readFrameSize() const {
        std::uint32_t size = 0;
        for (std::size_t i = 0; i < sizeof(std::uint32_t); ++i) {
            size = (size << 8) | inbound_[i];
        }
        return size;
    }

    void pumpIncoming() {
        unsigned char buffer[8192]{};
        while (fd_ >= 0) {
            const ssize_t count = ::recv(fd_, buffer, sizeof(buffer), 0);
            if (count > 0) {
                inbound_.insert(inbound_.end(), buffer, buffer + count);
                if (inbound_.size() > 64u * 1024u * 1024u) return;
                continue;
            }
            if (count == 0) {
                disconnected_ = true;
                return;
            }
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            disconnected_ = true;
            return;
        }
    }

    int fd_ = -1;
    bool blocking_ = true;
    bool disconnected_ = false;
    std::vector<unsigned char> inbound_;
};

class TcpListener final : public Socket {
public:
    TcpListener() = default;
    ~TcpListener() override { close(); }

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;

    [[nodiscard]] int nativeHandle() const override { return fd_; }

    Status listen(const unsigned short port) {
        close();
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) return Status::Error;
        const int enabled = 1;
        (void)::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);
        if (::bind(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0 ||
            ::listen(fd_, 128) < 0) {
            close();
            return Status::Error;
        }
        return Status::Done;
    }

    void setBlocking(const bool blocking) {
        const int flags = ::fcntl(fd_, F_GETFL, 0);
        if (flags < 0) return;
        const int next = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
        (void)::fcntl(fd_, F_SETFL, next);
    }

    Status accept(TcpSocket& socket) {
        if (fd_ < 0) return Status::Error;
        sockaddr_in address{};
        socklen_t length = sizeof(address);
        const int client = ::accept(fd_, reinterpret_cast<sockaddr*>(&address), &length);
        if (client < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return Status::NotReady;
            if (errno == EINTR) return Status::NotReady;
            return Status::Error;
        }
        socket.attach(client);
        return Status::Done;
    }

    void close() {
        if (fd_ >= 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    int fd_ = -1;
};

class SocketSelector final {
public:
    SocketSelector() = default;

    void add(Socket& socket) {
        if (std::find(sockets_.begin(), sockets_.end(), &socket) == sockets_.end()) {
            sockets_.push_back(&socket);
        }
    }

    void remove(Socket& socket) {
        sockets_.erase(std::remove(sockets_.begin(), sockets_.end(), &socket), sockets_.end());
        ready_.erase(std::remove(ready_.begin(), ready_.end(), &socket), ready_.end());
    }

    void clear() {
        sockets_.clear();
        ready_.clear();
    }

    bool wait(const Time timeout) {
        ready_.clear();
        for (Socket* socket : sockets_) {
            if (socket->hasPendingFrame()) {
                ready_.push_back(socket);
                return true;
            }
        }

        fd_set descriptors;
        FD_ZERO(&descriptors);
        int maxFd = -1;
        for (Socket* socket : sockets_) {
            const int fd = socket->nativeHandle();
            if (fd >= 0) {
                FD_SET(fd, &descriptors);
                maxFd = std::max(maxFd, fd);
            }
        }
        if (maxFd < 0) {
            ::usleep(static_cast<useconds_t>(timeout.asSeconds() * 1000000.f));
            return false;
        }

        const float totalSeconds = timeout.asSeconds();
        const auto whole = static_cast<time_t>(totalSeconds);
        timeval duration{};
        duration.tv_sec = whole;
        duration.tv_usec = static_cast<suseconds_t>((totalSeconds - static_cast<float>(whole)) * 1000000.f);
        const int result = ::select(maxFd + 1, &descriptors, nullptr, nullptr, &duration);
        if (result <= 0) return false;
        for (Socket* socket : sockets_) {
            const int fd = socket->nativeHandle();
            if (fd >= 0 && FD_ISSET(fd, &descriptors)) ready_.push_back(socket);
        }
        return !ready_.empty();
    }

    [[nodiscard]] bool isReady(Socket& socket) const {
        if (socket.hasPendingFrame()) return true;
        return std::find(ready_.begin(), ready_.end(), &socket) != ready_.end();
    }

private:
    std::vector<Socket*> sockets_;
    std::vector<Socket*> ready_;
};

} // namespace sf
