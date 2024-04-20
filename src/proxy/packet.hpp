#pragma once

#include <sys/types.h>

namespace proxy {

struct Packet
{
    constexpr static const int MAX_IP_SIZE      = 10;
    constexpr static const int MAX_PORT_SIZE    = 6;
    constexpr static const int MAX_PACKET_SIZE  = 64 * 1024;

    char    ip_[MAX_IP_SIZE];
    char    port_[MAX_PORT_SIZE];
    size_t  length_;
    char    segment_[MAX_PACKET_SIZE];
};

} // namespace