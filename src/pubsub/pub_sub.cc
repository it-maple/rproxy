#include <stdexcept>

#include "pub_sub.hpp"
#include "publisher.hpp"

namespace pubsub {

PubSubCenter::PubSubCenter() {}

PubSubCenter::~PubSubCenter() {}

void PubSubCenter::registerPub(uint16_t id, Publisher * const pub)
{
    if (!pub)
        throw std::logic_error("invalid pointer.");

    std::lock_guard<std::mutex> lk(mx_);
    publishers_.emplace(id, pub);
}

void PubSubCenter::unRegisterPub(uint16_t id)
{
    auto iter = publishers_.find(id);
    // assert(iter == publishers_.end());
    if (iter == publishers_.end())
        return;

    std::lock_guard<std::mutex> lk(mx_);
    publishers_.erase(iter);
    subcribers_.erase(id);
}

void PubSubCenter::subscribe(uint16_t pubID, PubType type, Subscriber * const sub)
{
    if (!sub)
        throw std::logic_error("invalid pointer.");

    if (publishers_.find(pubID) == publishers_.end())
        return;

    auto & puber = subcribers_[pubID];
    static std::lock_guard<std::mutex> lk(mx_);
    puber[type].emplace(sub);
}

void PubSubCenter::unSubscribe(uint16_t pubID, PubType type, Subscriber * const sub)
{
    if (!sub)
        throw std::logic_error("invalid pointer.");

    if (publishers_.find(pubID) == publishers_.end())
        return;

    static std::lock_guard<std::mutex> lk(mx_);
    auto & map = subcribers_[pubID];
    auto & set = map[type];
    set.erase(sub);
}

void PubSubCenter::notifySubscriber(uint16_t pubID, PubType type, std::shared_ptr<Context> ctx) noexcept
{
    if (publishers_.find(pubID) == publishers_.end())
        return;

    auto & map = subcribers_[pubID];
    auto & set = map[type];
    for (auto & sub : set)
    {
        sub->update(ctx);
    }
}

std::shared_ptr<PubSubCenter> PubSubCenter::instance()
{
    static std::mutex mx;
    static std::shared_ptr<PubSubCenter> center;
    if (!center)
    {
        std::lock_guard<std::mutex> lk(mx);
        if (!center)
        {
            center = std::make_shared<PubSubCenter>();
        }
    }

    return center;
}

} // namespace