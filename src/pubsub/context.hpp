#pragma once

#include <cstdint>

namespace pubsub {

struct Context
{
    Context(uint16_t event)
        : event_type_(event) {}

    virtual ~Context() {}

    uint16_t event_type_;
};

} // namespace ::pubsub
