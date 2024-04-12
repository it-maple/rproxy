#pragma once

#include "context.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace reactor {

class Demultiplex;

namespace context {

struct AcceptContext : ::pubsub::Context
{
    AcceptContext(uint16_t event, int fd)
        : Context(event)
        , fd_(fd) {}

    int fd_;
};

struct DmpRegisterContext : ::pubsub::Context
{
    DmpRegisterContext(uint16_t event)
        : Context(event)
        , fd_(-1)
        , events_()
        , dmp_ptr_()
    {}
    
    int                     fd_;
    uint32_t                events_;
    std::shared_ptr<Demultiplex> dmp_ptr_;
};

struct DmpModifyContext : ::pubsub::Context
{
    DmpModifyContext(uint16_t event)
        : Context(event)
        , fd_(-1)
        , events_()
    {}

    int                     fd_;
    uint32_t                events_;
};

struct DmpDeleteContext : ::pubsub::Context
{
    DmpDeleteContext(uint16_t event)
        : Context(event)
        , fd_(-1)
    {}

    int fd_;
};

struct DmpWaitContext : ::pubsub::Context
{
    DmpWaitContext(uint16_t event)
        : Context(event)
        , resp_events_()
    {}

    struct resp
    {
        int fd;
        uint32_t events;
    };
    std::vector<resp> resp_events_;
};

} // namespace pubsub
} // namespace reactor