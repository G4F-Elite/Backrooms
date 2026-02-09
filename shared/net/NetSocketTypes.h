#pragma once

#include <cstdint>

namespace backrooms::net {

using Port = std::uint16_t;

struct Endpoint {
    std::uint32_t ipv4HostOrder;
    Port portHostOrder;
};

enum class SocketStatus {
    Ok = 0,
    WouldBlock,
    Closed,
    Error
};

struct SocketIoResult {
    SocketStatus status;
    int bytes;
};

}  // namespace backrooms::net
