#pragma once

#include <cstdint>

namespace pubsub {

enum PubType : uint16_t
{
    ACCEPTED,
    DMPREGISTER,
    DMPMODIFY,
    DMPDELETE,
    DMPWAIT,

    // proxy
    FORWARD
};
} // namespace