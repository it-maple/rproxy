#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "context.hpp"
#include "stream_socket.hpp"

namespace proxy {
namespace context {

struct LoadBalanceCtx : ::pubsub::Context
{
    LoadBalanceCtx(uint16_t event, size_t /* forward socket */ cnt = 0)
        : ::pubsub::Context(event)
        , sockes_(cnt)
    {}

    std::vector<int> sockes_;
};

struct ForwardContext : ::pubsub::Context
{
    ForwardContext(uint16_t event)
        : ::pubsub::Context(event)
    {}
    
    std::shared_ptr<
        std::unordered_map<
            int, reactor::InetAddr>> sock_to_backend_;
};

} // namespace
} // namespace