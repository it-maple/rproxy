#pragma once

#include <stddef.h>
#include <sys/epoll.h>

#include "pub_sub.hpp"

namespace reactor {

class Demultiplex : public ::pubsub::Publisher
                    , public std::enable_shared_from_this<Demultiplex>
{
public:
    enum EventT
    {
        READABLE = EPOLLIN,
        WRITABLE = EPOLLOUT,
        CLOSABLE = EPOLLERR | EPOLLHUP | EPOLLRDHUP
    };

    static const size_t MAX_SIZE = 1024;
    static const uint32_t DefaultEvents = READABLE | CLOSABLE | EPOLLET;

public:
    Demultiplex();
    Demultiplex(const Demultiplex &) = delete;
    Demultiplex & operator=(const Demultiplex &) = delete;
    Demultiplex(Demultiplex && demultiplex) = delete;
    Demultiplex & operator=(Demultiplex && demultiplex) = delete;
    virtual ~Demultiplex();

    void enableNotify(bool enabled);

    uint16_t pubID() const override;

    void notify(::pubsub::PubType type,
            std::shared_ptr<::pubsub::Context> ctx) override;

    std::shared_ptr<::pubsub::PubSubCenter> center() noexcept;

    int demultiplexRegister(int sockfd, bool notify = false);

    int demultiplexModify(std::shared_ptr<::pubsub::Context> ctx, bool notify = false);

    int demultiplexRemove(std::shared_ptr<::pubsub::Context> ctx, bool notify = false);

    void demultiplexWait(std::shared_ptr<::pubsub::Context> ctx, bool notify = false);

private:
    uint16_t                                id_;
    int                                     dmpfd_;
    bool                                    notify_;
    std::shared_ptr<::pubsub::PubSubCenter> center_;
};

} // namespace reactor end