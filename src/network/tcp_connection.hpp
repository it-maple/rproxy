#pragma once

#include <cstddef>
#include <memory>

#include "buffer.hpp"
#include "channel.hpp"
#include "context.hpp"
#include "stream_socket.hpp"

namespace reactor {

class TcpServer;

class TcpConnection
{
public:
    using ChannelPtr = std::shared_ptr<Channel>;
    using ContextPtr = std::shared_ptr<::pubsub::Context>;

    enum TcpState
    {
        CONNECTED = 0x00,
        DISCONNECT
    };

public:
    TcpConnection(ChannelPtr const & ptr, ContextPtr const & cxt_ptr);
    // TcpConnection(EventLoop * const loop, ChannelPtr const & ptr);
    TcpConnection(const TcpConnection &) = default;
    TcpConnection(TcpConnection &&) = default;
    TcpConnection & operator=(TcpConnection &&) = default;
    TcpConnection & operator=(const TcpConnection &) = default;
    ~TcpConnection();

    ChannelPtr & channel() noexcept;

    ContextPtr & context() noexcept;

    void disconnect();

    // connected or disconnected
    bool alive() noexcept;

    std::string getTcpInfo() noexcept;

    ArrayBuffer & recvBuffer() noexcept;

    ArrayBuffer & sendBuffer() noexcept;

    void read();

    void shutdownWrite();

    void read(void * const buf, size_t len);

    void send(void const * const buf, size_t len);

    void sendInLoop(TcpConnection & conn);

private:
    void sendInLoop();

private:
    int         sockfd_;
    // EventLoop * loop_;
    ChannelPtr  channel_;

    ArrayBuffer recv_buffer_;
    ArrayBuffer send_buffer_;

    InetAddr    addr_;
    TcpState    state_;

    ContextPtr  context_;
};

} // namespace reactor