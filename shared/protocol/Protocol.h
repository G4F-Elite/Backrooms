#pragma once

#include <cstdint>

namespace backrooms::protocol {

constexpr std::uint16_t kProtocolVersion = 1;
constexpr std::uint16_t kDefaultGamePort = 27015;
constexpr std::uint16_t kDefaultMasterPort = 27017;
constexpr int kMaxPlayersDedicated = 8;
constexpr std::uint8_t kMaxServerListEntries = 32;
constexpr std::uint8_t kServerFlagInGame = 1 << 0;

enum class MessageType : std::uint8_t {
    HandshakeHello = 1,
    HandshakeWelcome = 2,
    ClientInput = 3,
    ServerSnapshot = 4,
    Heartbeat = 5,
    ServerListQuery = 6,
    ServerListResponse = 7
};

struct MessageHeader {
    std::uint16_t protocolVersion;
    std::uint8_t type;
    std::uint32_t sequence;
};

}  // namespace backrooms::protocol
