#include <errno.h>
#include <fcntl.h>

#include <iostream>

#include "proxy_server.hpp"
#include "pub_type.hpp"

#ifdef MSG_ATTACH
#   include <string.h>
#   include <sys/mman.h>
#   include <sys/socket.h>
#   include "packet.hpp"
#endif

namespace proxy {

ProxyServer::ProxyServer(int n_thread)
    : sub_id_(::pubsub::ID::sub_id())
    , pub_id_(::pubsub::ID::pub_id())
    , center_(::pubsub::PubSubCenter::instance())
    , pipe_()
    , stop_(false)
    , flag_(false)
    , sock_map_()
    , group_()
{
    if (::pipe2(pipe_, O_NONBLOCK) == -1)
        throw std::runtime_error("can't open two pipe to transfer data in kernel space");

    group_.init(n_thread);

    auto const & dmpes = group_.dmp();
    for (auto & dmp : dmpes)
    {
        dmp->enableNotify(false);
        center_->subscribe(dmp->pubID(), pubsub::DMPWAIT, this);
    }

    center_->registerPub(pub_id_, this);
}

ProxyServer::~ProxyServer()
{
    ::close(pipe_[0]);
    ::close(pipe_[1]);

    auto const & dmpes = group_.dmp();
    for (auto & dmp : dmpes)
        center_->unSubscribe(dmp->pubID(), pubsub::DMPWAIT, this);
}

uint16_t ProxyServer::subID() const
{
    return sub_id_;
}

uint16_t ProxyServer::pubID() const { return pub_id_; }

void ProxyServer::subscribe(uint16_t pubID, ::pubsub::PubType type)
{
    center_->subscribe(pubID, type, this);
}

void ProxyServer::notify(::pubsub::PubType type, std::shared_ptr<::pubsub::Context> ctx)
{
    center_->notifySubscriber(pub_id_, pubsub::FORWARD, ctx);
}

void ProxyServer::update(std::shared_ptr<::pubsub::Context> ctx)
{
    switch (ctx->event_type_) {
        case ::pubsub::DMPWAIT:
        {
            auto wait_ctx = std::dynamic_pointer_cast<::reactor::context::DmpWaitContext>(ctx);
            updateForWait(wait_ctx);
            break;
        }
        case ::pubsub::FORWARD:
        {
            auto forward_ctx = std::dynamic_pointer_cast<context::ForwardContext>(ctx);
            updateForForward(forward_ctx);
            break;
        }
        default:
            break;
    }
}

void ProxyServer::updateForWait(std::shared_ptr<::reactor::context::DmpWaitContext> ctx)
{
#ifdef MSG_ATTACH
    auto packet = new Packet;
#endif
    for (auto & resp : ctx->resp_events_)
    {
        std::cout << "resp.fd, " << resp.fd
                  << ", backend fd: " << sock_map_.at(resp.fd)
                  << "\n";
        if ((resp.events & ::reactor::Demultiplex::READABLE)
                && !(resp.events & ::reactor::Demultiplex::CLOSABLE)
            )
        {
#ifdef MSG_ATTACH
            forward(resp.fd, sock_map_.at(resp.fd), packet);
#else
            zeroCopy(resp.fd, sock_map_.at(resp.fd));
#endif
        }
        if (resp.events & ::reactor::Demultiplex::CLOSABLE)
        {
            ::close(resp.fd);

            std::lock_guard<std::mutex> guard(mx_);
            if (sock_map_.find(resp.fd) == sock_map_.end())
                continue;
            reversed_map_.erase(sock_map_.at(resp.fd));
            sock_map_.erase(resp.fd);
            std::cout << "remove backend fd: " << resp.fd << "\n";
        }
    }
}

void ProxyServer::updateForForward(std::shared_ptr<context::ForwardContext> ctx)
{
#ifdef MSG_ATTACH
    auto packet = new Packet;
#endif

    for (auto & pair : *ctx->sock_to_backend_)
    {
        int && sock          = -1;
        int const & incoming = pair.first;
        if (reversed_map_.find(incoming) == reversed_map_.end())
        {
            std::lock_guard<std::mutex> guard(mx_);
            if (reversed_map_.find(incoming) == reversed_map_.end())
            {
                sock = establishWith(pair.second);
                if (sock < 0)
                    continue;

                sock_map_.emplace(sock, incoming);
                reversed_map_.emplace(incoming, sock);
                group_.registerFd(sock);
            }
        }
        sock = reversed_map_.at(incoming);

        auto && addr        = reactor::StreamSocket::getPeerAddr(pair.first);
        auto && addr_str = addr.toString();

        // TODO: make strategy to prevent large traffic incoming
        // for specified client, like closing conection or limitting traffic
        if (center_->isLismited(addr_str))
            continue;

#ifdef MSG_ATTACH
        std::string && ip   = addr.ip();
        std::string && port = addr.port();
        std::copy(ip.begin(), ip.end(), packet->ip_);
        std::copy(port.begin(), port.end(), packet->port_);
        auto total = forward(incoming, sock, packet);
        // accounting traffic
        if (total > 0)
            center_->account(addr_str, total);
#else
        auto bytes = zeroCopy(incoming, sock);
        if (bytes > 0)
            center_->account(addr_str, bytes);
#endif
    }
}

int ProxyServer::establishWith(::reactor::InetAddr addr)
{
    auto sock = reactor::StreamSocket::connectTo(addr);
    std::cout << "establish new connection with backend: " << sock << "\n";
    reactor::StreamSocket::setNonBlocking(sock);
    group_.registerFd(sock);

    return sock;
}

ssize_t ProxyServer::zeroCopy(int in_fd, int out_fd)
{
    auto len = ::splice(in_fd, nullptr, pipe_[1], nullptr, INT_MAX, SPLICE_F_MOVE);
    if (len < 0)
    {
        ::perror("splice(): ");
        return len;
    }
    if (len == 0)
        return len;
    
    ssize_t slen    = 0;
    ssize_t total   = 0;
    while (len > 0)
    {
        slen = ::splice(pipe_[0], nullptr, out_fd, nullptr, len, SPLICE_F_MORE | SPLICE_F_MOVE);
        if (slen < 0)
        {
            ::perror("splice() to out_fd:");
            break;
        }

        len -= slen;
        total += slen;
    }

    // if (len != 0)
    //     throw std::runtime_error("missing data for splice()");

    return total;
}

ssize_t ProxyServer::forward(int in_fd, int out_fd, Packet * packet)
{
#ifdef MSG_ATTACH
    ::memset(packet, 0, sizeof(*packet));
#endif
    // read from incoming request
    auto recved = ::read(in_fd, packet->segment_, packet->MAX_PACKET_SIZE);
    std::cout << "[forward] recv from: " << in_fd
              << ", len: " << recved << "\n";
    if (recved == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
        return recved;

    ssize_t total = 0;
    // TODO: send() with MSG_ZEROCOPY Suitable for data less than about 10KB
    if (recved > SEND_PROPER_SIZE)
    {
    }
    // else
    {
        // re-packing the received data packeta with specified format
        packet->length_ = recved;
        ssize_t sent    = 0;
        // BUG: the exactly size for Packet maybe more than
        // the socket buffer size
        while (true)
        {
#ifdef MSG_ATTACH
            sent = ::send(out_fd, packet->segment_ + total, packet->length_ - total, MSG_ZEROCOPY);
#endif
            if (sent == -1)
            {
                ::perror("send() with MSG_ZEROCOPY: ");
                break;
            }

            total += sent;
            if (total == recved)
                break;
        }
        std::cout << "[forward] sent to: " << out_fd
                  << ", len: " << sent << "\n";
    }
    
    return total;
}

void ProxyServer::stop()
{
    stop_.store(true, std::memory_order_release);
}

void ProxyServer::pooling()
{
    while (!stop_.load(std::memory_order_acquire))
    {
        group_.joinAll();
    }
}

} // namespace