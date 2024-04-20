#include <mutex>
#include <sys/socket.h>
#include <thread>

#include "loader.hpp"
#include "stream_socket.hpp"

namespace proxy {

LoadBalancer::LoadBalancer()
    : pub_id_(::pubsub::ID::pub_id())
    , sub_id_(::pubsub::ID::sub_id())
    , center_(::pubsub::PubSubCenter::instance())
    , backend_()
    , stop_(false)
    , sockes_()
    , sock_queue_()
    , checked_()
    , text_()
{
    center_->registerPub(pub_id_, this);
}

LoadBalancer::~LoadBalancer()
{
    center_->unRegisterPub(pub_id_);
    stop();
}

uint16_t LoadBalancer::pubID() const { return pub_id_; }

void LoadBalancer::notify(::pubsub::PubType type, std::shared_ptr<::pubsub::Context> ctx)
{
    center_->notifySubscriber(pub_id_, pubsub::FORWARD, ctx);
}

uint16_t LoadBalancer::subID() const { return sub_id_; }

void LoadBalancer::subscribe(uint16_t pubID, ::pubsub::PubType type)
{
    center_->subscribe(pubID, type, this);
}

void LoadBalancer::update(std::shared_ptr<::pubsub::Context> ctx)
{
    switch (ctx->event_type_) {
        case pubsub::BALANCE:
        {
            auto forward_ctx = std::dynamic_pointer_cast<context::LoadBalanceCtx>(ctx);
            updateForLoadbalance(forward_ctx);
            break;
        }
        default:
            break;
    }
}

void LoadBalancer::updateForLoadbalance(std::shared_ptr<context::LoadBalanceCtx> ctx)
{
    std::lock_guard<std::mutex> guard(mx_);
    sock_queue_.push_back(std::move(ctx->sockes_));
}

void LoadBalancer::addBackend(reactor::InetAddr addr, uint16_t const & check_port)
{
    std::lock_guard<std::mutex> lk(mx_);
    auto const & str = addr.toString();
    backend_.emplace(str, std::move(addr));
    checked_.emplace(str, check_port);
}

void LoadBalancer::removeBackend(reactor::InetAddr addr)
{
    if (backend_.find(addr.toString()) == backend_.end())
        return;

    std::lock_guard<std::mutex> lk(mx_);
    backend_.erase(addr.toString());
}

void LoadBalancer::checkAllBackend()
{
    for (auto & pair : checked_)
    {
        int time = 3;
        int sock = 0;
        auto const & ip = backend_.at(pair.first).ip();
        auto const & addr = reactor::InetAddr(ip, pair.second);
        // Verify the correctness of the connection
        do
        {
            sock = reactor::StreamSocket::connectTo(addr);
            if (sock > 0)
                break;
        } while (--time);
        if (sock < 0)
        {
            std::lock_guard<std::mutex> guard(mx_);
            backend_.erase(pair.first);
            continue;
        }

        time = 3;
        auto len = text_.length();
        // Verify the legitimacy for sending data
        do
        {
            auto sent = reactor::StreamSocket::write(sock, text_.c_str(), len);
            if (sent == len)
                break;
        } while (--time);
        if (time < 0)
        {
            std::lock_guard<std::mutex> guard(mx_);
            backend_.erase(pair.first);
            continue;
        }

        // Verify the legitimacy for receiving data
        time = 3;
        char buf[len];
        do
        {
            ::recv(sock, buf, len, 0);
            if (text_ == buf)
                break;
        } while (--time);
        if (time < 0)
        {
            std::lock_guard<std::mutex> guard(mx_);
            backend_.erase(pair.first);
            continue;
        }
    }
}

void LoadBalancer::specifyCheckText(std::string const & text)
{
    text_ = text;
}

void LoadBalancer::loadbalance()
{
    while (!stop_.load(std::memory_order_acquire))
    {
        while (sock_queue_.empty())
            std::this_thread::yield();

        {
            std::lock_guard<std::mutex> guard(mx_);
            sockes_ = sock_queue_.front();
            sock_queue_.pop_front();
        }

        std::shared_ptr<::pubsub::Context> forward_ctx =
            std::make_shared<context::ForwardContext>(
                ::pubsub::FORWARD);
        auto forward_ptr = std::dynamic_pointer_cast<context::ForwardContext>(forward_ctx);

        auto bk_2_sock = new std::unordered_map<int, ::reactor::InetAddr>;
        balancing(sockes_, *bk_2_sock);
        forward_ptr->sock_to_backend_.reset(bk_2_sock);

        center_->notifySubscriber(pub_id_,
            pubsub::FORWARD,
             forward_ctx);
    }
}

void LoadBalancer::balancing(
            std::vector<int> & sockes,
            std::unordered_map<int, ::reactor::InetAddr> & sock_map)
{
    auto iter = backend_.begin();
    for (auto & sock : sockes)
    {
        sock_map.emplace(sock, iter->second);
        if (++iter == backend_.end())
            iter = backend_.begin();
    }
}

void LoadBalancer::stop()
{
    stop_.store(true, std::memory_order_release);
}

} // namespace