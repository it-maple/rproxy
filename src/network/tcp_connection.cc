#include "tcp_connection.hpp"
#include "nw_ctx.hpp"
#include <sys/socket.h>

namespace reactor {

TcpConnection::TcpConnection(ChannelPtr const & ptr, ContextPtr const & cxt_ptr)
    : sockfd_(ptr->socket())
    , channel_(ptr)
    , recv_buffer_()
    , send_buffer_()
    , addr_(StreamSocket::getPeerAddr(sockfd_))
    , state_(CONNECTED)
    , context_(cxt_ptr)
    {}

TcpConnection::~TcpConnection()
{
    state_ = DISCONNECT;

    StreamSocket::close(sockfd_);
}

TcpConnection::ChannelPtr & TcpConnection::channel() noexcept
{
    return channel_;
}

TcpConnection::ContextPtr & TcpConnection::context() noexcept
{
    return context_;
}

void TcpConnection::disconnect()
{
    state_ = DISCONNECT;
    channel_->leave();
}

bool TcpConnection::alive() noexcept
{
    return state_ == CONNECTED;
}

std::string TcpConnection::getTcpInfo() noexcept
{
    return addr_();
}

ArrayBuffer & TcpConnection::recvBuffer() noexcept
{
    return recv_buffer_;
}

ArrayBuffer & TcpConnection::sendBuffer() noexcept
{
    return send_buffer_;
}

void TcpConnection::read()
{
    read(recv_buffer_.peek(), recv_buffer_.writableBytes());
}

void TcpConnection::shutdownWrite()
{
    ::shutdown(sockfd_, SHUT_WR);
    channel_->disableWrite(context_);
}

void TcpConnection::read(void * const buf, size_t len)
{
    if (state_ == DISCONNECT)
        return;

    auto ctx = std::dynamic_pointer_cast<context::DmpRegisterContext>(context_);
    if (!(ctx->events_ & Demultiplex::READABLE))
        return;

    ssize_t recv = StreamSocket::read(sockfd_, buf, len);
    if (recv < 0)
    {
        disconnect();
    }
    else
    {
        recv_buffer_.hasWritten(recv);
    }
}

void TcpConnection::send(void const * const buf, size_t len)
{
    if (state_ == DISCONNECT)
        return;

    auto ctx = std::dynamic_pointer_cast<context::DmpRegisterContext>(context_);

    ssize_t wrote = 0;
    ssize_t remain = len;
    if (!(Demultiplex::WRITABLE & ctx->events_) &&
        send_buffer_.readableBytes() == 0)
    {
        wrote = StreamSocket::write(sockfd_, buf, len);
        if (wrote >= 0)
        {
            remain = len - wrote;
            // FIXME: maybe can do something when data was comepletely sent.
            if (remain == 0)
            {
            }
        }
        else
        {
            // FIXME: Maybe need to handle specified error.
            wrote = 0;
        }
    }

    if (remain > 0)
    {
        send_buffer_.append(static_cast<char const *>(buf) + wrote, remain);
        auto modify = new context::DmpModifyContext(::pubsub::DMPMODIFY);
        if (!(Demultiplex::WRITABLE & ctx->events_))
            channel_->enableWrite(std::shared_ptr<::pubsub::Context>(modify));
    }
}

void TcpConnection::sendInLoop()
{
    if (send_buffer_.readableBytes() == 0)
        return;

    auto ctx = std::dynamic_pointer_cast<context::DmpRegisterContext>(context_);
    if (Demultiplex::READABLE & ctx->events_)
    {

        auto wrote = StreamSocket::write(sockfd_, send_buffer_.peek(), send_buffer_.readableBytes());
        if (wrote > 0)
        {
            send_buffer_.retrieve(wrote);
            if (send_buffer_.readableBytes() == 0)
            {
                channel_->disableWrite(ctx);
            }
        }
        // FIXME: maybe need to handle error
    }
}

void TcpConnection::sendInLoop(TcpConnection & conn)
{
    conn.sendInLoop();
}

} // namespace reactor