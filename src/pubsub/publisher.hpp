#pragma once

#include <memory>

#include "context.hpp"
#include "pub_type.hpp"

namespace pubsub {

class Publisher
{
public:
    Publisher() = default;
    Publisher(Publisher &&) = default;
    Publisher(const Publisher &) = delete;
    Publisher & operator=(const Publisher &) = delete;
    Publisher & operator=(Publisher &&) = delete;
    virtual ~Publisher() = default;

    virtual uint16_t pubID() const =0;

    virtual void notify(PubType type, std::shared_ptr<Context> ctx) =0;
};

} // namespace