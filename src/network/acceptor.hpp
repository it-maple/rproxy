#pragma once

#include <arpa/inet.h>
#include <stdexcept>

#include "pub_sub.hpp"
#include "nw_ctx.hpp"
#include "stream_socket.hpp"

namespace reactor {

class Acceptor : public ::pubsub::Publisher
{
public:
    Acceptor(int serverfd)
        : id_(::pubsub::ID::pub_id())
        , server_(serverfd)
        , center_(::pubsub::PubSubCenter::instance())
    {
        StreamSocket::listen(server_);
        center_->registerPub(id_, this);
    }

    Acceptor(const Acceptor &) = delete;
    Acceptor(Acceptor &&) = default;
    Acceptor & operator=(const Acceptor &) = delete;
    Acceptor & operator=(Acceptor &&) = delete;

    virtual ~Acceptor()
    {
        center_->unRegisterPub(id_);
    }

    int server() { return server_; }

    int accept()
    {
        int sockfd = accept(server_);

        if (sockfd < 0)
            throw std::runtime_error("can not accept connection");

        std::shared_ptr<::pubsub::Context> ctx = std::make_shared<context::AcceptContext>(::pubsub::ACCEPTED, sockfd);
        center_->notifySubscriber(id_, ::pubsub::ACCEPTED, ctx);

        return sockfd;
    }

private:
    int accept(int fd) {
        socklen_t addrLen;
        struct sockaddr_in clientAddr;
        return ::accept(fd, (struct sockaddr *) &clientAddr, &addrLen);
    }

public:
    virtual void notify(::pubsub::PubType type,
                std::shared_ptr<::pubsub::Context> ctx) override
    {
        center_->notifySubscriber(id_, ::pubsub::ACCEPTED, ctx);
    }

    virtual uint16_t pubID() const override { return id_; }

private:
    uint16_t                                id_;
    int                                     server_;
    // std::function<void(int)> &              accept_callback_;
    std::shared_ptr<::pubsub::PubSubCenter>   center_;
    // std::unordered_map<PubType, PairVec>    subcribers_;
};

} // namespace reactor