#pragma once

#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "group.hpp"
#include "nw_ctx.hpp"
#include "packet.hpp"
#include "proxy_ctx.hpp"
#include "pub_type.hpp"
#include "publisher.hpp"

namespace proxy {

class ProxyServer : public ::pubsub::Subscriber,
                      public ::pubsub::Publisher
{
public:
    // for send() wtih MSG_ZEROCOPY option
    constexpr static int const SEND_PROPER_SIZE = 10 * 1024;

public:
    ProxyServer(int n_thread = 1);
    ProxyServer(const ProxyServer &) = delete;
    ProxyServer & operator=(const ProxyServer &) = delete;
    ProxyServer(ProxyServer &&) = delete;
    ProxyServer & operator=(ProxyServer &&) = delete;
    ~ProxyServer();

    virtual uint16_t subID() const override;

    virtual uint16_t pubID() const override;

    virtual void subscribe(uint16_t pubID, ::pubsub::PubType type) override;

    virtual void notify(::pubsub::PubType type, std::shared_ptr<::pubsub::Context> ctx) override;

    virtual void update(std::shared_ptr<::pubsub::Context> ctx) override;

private:
    void updateForWait(std::shared_ptr<::reactor::context::DmpWaitContext> ctx);

    /**
     * @brief if the cached connection establish with backend gather than
     * the incoming client. don't create new connection, otherwise
     * establish new connection and then assign to the rest
     * incoming client.
     * 
     * @param ctx context that contain all data for forwarding request
     */
    void updateForForward(std::shared_ptr<context::ForwardContext> ctx);

private:
    int establishWith(::reactor::InetAddr addr);

    /**
     * @brief zero-copy to forward buffer in kernel space 
     * forward data from back-end side to cient
     * 
     * @param in_fd     input fd
     * @param out_fd    output fd
     * @return ssize_t  transferred size
     */
    ssize_t zeroCopy(int in_fd, int out_fd);

    ssize_t forward(int in_fd, int out_fd, Packet * packet);

public:
    void stop();

    void pooling();

private:
    using SocketSet = std::unordered_set<int>;
    using AllSocket = std::unordered_map<std::string, SocketSet>;
    using SocketMap = std::unordered_map<int, int>;

private:
    std::mutex                                      mx_;
    std::condition_variable                         cv_;
    uint16_t                                        sub_id_;
    uint16_t                                        pub_id_;
    std::shared_ptr<::pubsub::PubSubCenter>         center_;

    int                                             pipe_[2];
    std::atomic_bool                                stop_;
    std::atomic_bool                                flag_;
    std::unordered_map<int, int>                    sock_map_;
    std::unordered_map<int, int>                    reversed_map_;
    reactor::LoopThreadGroup                        group_;
};

} // namespace