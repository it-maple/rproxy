#pragma once

#include <memory>

#include "context.hpp"

namespace pubsub {

class Subscriber
{
public:
    Subscriber() = default;
    virtual ~Subscriber() = default;

    virtual uint16_t subID() const =0;

    virtual void update(std::shared_ptr<Context> ctx) =0;
};

} // namespace