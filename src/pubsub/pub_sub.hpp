#pragma once

#include <cstddef>
#include <limits.h>

#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "publisher.hpp"
#include "subscriber.hpp"

namespace pubsub {

namespace ID {

static std::atomic<uint16_t> SubscriberID(0);
static std::atomic<uint16_t> PublisherID(0);

static uint16_t sub_id()
{

    int id = 0;
    do {
        id = SubscriberID.load(std::memory_order_acquire);
    } while (id != SubscriberID.fetch_add(1));

    return id;
}

static uint16_t pub_id()
{
    int id = 0;
    do {
        id = PublisherID.load(std::memory_order_acquire);
    } while (id != PublisherID.fetch_add(1));

    return id;
}

} // namespace ID
constexpr static const auto MAX_TRAFFIC= LONG_LONG_MAX;

class PubSubCenter
{
public:
    PubSubCenter();
    ~PubSubCenter();

    void registerPub(uint16_t id, Publisher * const pub);

    void unRegisterPub(uint16_t id);

    void subscribe(uint16_t pubID, PubType type, Subscriber * const sub);

    void unSubscribe(uint16_t pubID, PubType type, Subscriber * const sub);

    void notifySubscriber(uint16_t pubID, PubType type, std::shared_ptr<Context> ctx);

    void account(std::string const & addr, size_t traffic);

    size_t traffic(std::string const & addr);

    bool isLismited(std::string const & addr);

    static std::shared_ptr<PubSubCenter> instance();

private:
    std::mutex                                  mx_;
    std::unordered_map<uint16_t, Publisher *>   publishers_;
    std::unordered_map<uint16_t,
        std::unordered_map<PubType,
            std::unordered_set<Subscriber *>>>  subcribers_;

    std::unordered_map<std::string, size_t>  traffic_;
};

} // namespace ::pubsub