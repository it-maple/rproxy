#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "proxy_ctx.hpp"
#include "pub_sub.hpp"
#include "publisher.hpp"
#include "stream_socket.hpp"
#include "subscriber.hpp"

namespace proxy {

class LoadBalancer : public ::pubsub::Publisher,
                     public ::pubsub::Subscriber
{
public:
    LoadBalancer();
    ~LoadBalancer();

    virtual uint16_t pubID() const override;

    virtual void notify(::pubsub::PubType type, std::shared_ptr<::pubsub::Context> ctx) override;

    virtual uint16_t subID() const override;

    virtual void subscribe(uint16_t pubID, ::pubsub::PubType type) override;

    virtual void update(std::shared_ptr<::pubsub::Context> ctx) override;

private:
    void updateForLoadbalance(std::shared_ptr<context::LoadBalanceCtx> ctx);

public:
    /**
     * @brief add new backend that is online
     * 
     * @param backend backend's address, got by InetAddr that load from configuration file
     */
    void addBackend(reactor::InetAddr addr, uint16_t const & check_port);

    /**
     * @brief remove backend that was offline
     * 
     * @param backend backend's address, got by InetAddr
     */
    void removeBackend(reactor::InetAddr addr);

    /**
     * @brief Timer Task. check whether all backend is alive or dead.
     * if dead, remove the backend from cache.
     * 
     */
    void checkAllBackend();

    void specifyCheckText(std::string const & text);

    void loadbalance();

    void stop();

private:
    /**
     * @brief implementation for Load Balancing algorithm.
     * default to Round Robin.
     * 
     */
    void balancing(std::vector<int> & sockes,
            std::unordered_map<int, ::reactor::InetAddr> & sock_map);

private:
    uint16_t                                pub_id_;
    uint16_t                                sub_id_;
    std::shared_ptr<pubsub::PubSubCenter>   center_;
    std::unordered_map<std::string,
            reactor::InetAddr>              backend_;
    std::mutex                              mx_;
    std::atomic_bool                        stop_;
    std::vector<int>                        sockes_;
    std::deque<std::vector<int>>            sock_queue_;
    std::unordered_map<std::string,
                        uint16_t>        checked_;
    std::string                             text_;
};

} // namespace