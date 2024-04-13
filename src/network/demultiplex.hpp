#pragma once

#include <stddef.h>
#include <sys/epoll.h>

#include "pub_sub.hpp"

namespace reactor {

class Demultiplex : public ::pubsub::Publisher
                    , public std::enable_shared_from_this<Demultiplex>
{
public:
    /**
     * FIXME: Maybe need to handle the concrete error for robust feature
     * 1. Connection Timeouts:
     *      Set a timeout for the socket using the setsockopt function 
     *      with the SO_RCVTIMEO and SO_SNDTIMEO options to specify the
     *      timeout for receive and send operations.
     *      Use non-blocking sockets with select, poll, or epoll to implement
     *      a timeout mechanism for socket operations.
     * 
     * 2. Connection Refused:
     *      Handle the ECONNREFUSED error code when attempting to connect to
     *      a remote host. You can catch this specific error code and take
     *      appropriate action, such as retrying the connection or notifying the user.
     * 
     * 3. Network Unreachable:
     *      Check for network availability using platform-specific network
     *      status APIs or libraries. For example, on Linux, you can use the
     *      getaddrinfo function to check network availability before attempting the connection.
     */
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

    bool demultiplexAccepted();

    int demultiplexRegister(int sockfd, bool notify = true);

    int demultiplexModify(std::shared_ptr<::pubsub::Context> ctx, bool notify = true);

    int demultiplexRemove(std::shared_ptr<::pubsub::Context> ctx, bool notify = true);

    void demultiplexWait(std::shared_ptr<::pubsub::Context> ctx, bool notify = true);

private:
    uint16_t                                id_;
    int                                     dmpfd_;
    bool                                    notify_;
    std::shared_ptr<::pubsub::PubSubCenter> center_;
};

} // namespace reactor end