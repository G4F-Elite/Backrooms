#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "../../shared/net/UdpSocketLite.h"
#include "../../shared/protocol/Packets.h"

namespace {

using backrooms::net::UdpAddress;
using backrooms::net::UdpSocketLite;
using namespace backrooms::protocol;

struct RegistryEntry {
    std::uint32_t ipv4HostOrder;
    std::uint16_t gamePort;
    std::uint8_t currentPlayers;
    std::uint8_t maxPlayers;
    std::uint8_t flags;
    char serverName[kServerNameLen];
    std::uint64_t lastSeenMs;
};

std::uint64_t nowMs() {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return (std::uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void upsertServer(std::vector<RegistryEntry>& entries, std::uint32_t ip, const Heartbeat& hb, std::uint64_t ts) {
    for (auto& e : entries) {
        if (e.ipv4HostOrder == ip && e.gamePort == hb.gamePort) {
            e.currentPlayers = hb.currentPlayers;
            e.maxPlayers = hb.maxPlayers;
            e.flags = hb.flags;
            std::memset(e.serverName, 0, sizeof(e.serverName));
            std::strncpy(e.serverName, hb.serverName, sizeof(e.serverName) - 1);
            e.lastSeenMs = ts;
            return;
        }
    }
    RegistryEntry e{};
    e.ipv4HostOrder = ip;
    e.gamePort = hb.gamePort;
    e.currentPlayers = hb.currentPlayers;
    e.maxPlayers = hb.maxPlayers;
    e.flags = hb.flags;
    std::memset(e.serverName, 0, sizeof(e.serverName));
    std::strncpy(e.serverName, hb.serverName, sizeof(e.serverName) - 1);
    e.lastSeenMs = ts;
    entries.push_back(e);
}

}  // namespace

int main(int argc, char** argv) {
    int listenPort = kDefaultMasterPort;
    for (int i = 1; i + 1 < argc; i++) {
        if (std::string(argv[i]) == "--port") listenPort = std::atoi(argv[i + 1]);
    }

    UdpSocketLite socket;
    if (!socket.open((std::uint16_t)listenPort, true)) {
        std::cerr << "Failed to open master UDP socket on port " << listenPort << "\n";
        return 1;
    }

    std::cout << "Backrooms Master Service\n";
    std::cout << "Listen UDP: " << listenPort << "\n";

    std::vector<RegistryEntry> servers;
    std::uint32_t seq = 1;

    for (;;) {
        std::uint8_t in[1400] = {};
        UdpAddress from{};
        int got = socket.recvFrom(from, in, (int)sizeof(in));
        if (got > 0) {
            MessageHeader header{};
            if (decodeHeader(in, got, header) && header.protocolVersion == kProtocolVersion) {
                if (header.type == (std::uint8_t)MessageType::Heartbeat) {
                    Heartbeat hb{};
                    if (decodeHeartbeat(in, got, hb)) {
                        upsertServer(servers, from.ipv4HostOrder, hb, nowMs());
                    }
                } else if (header.type == (std::uint8_t)MessageType::ServerListQuery) {
                    if (decodeServerListQuery(in, got)) {
                        ServerListResponse response{};
                        response.count = 0;
                        for (const auto& s : servers) {
                            if (response.count >= kMaxServerListEntries) break;
                            auto& dst = response.entries[response.count++];
                            dst.ipv4HostOrder = s.ipv4HostOrder;
                            dst.gamePort = s.gamePort;
                            dst.currentPlayers = s.currentPlayers;
                            dst.maxPlayers = s.maxPlayers;
                            dst.flags = s.flags;
                            std::memset(dst.serverName, 0, sizeof(dst.serverName));
                            std::strncpy(dst.serverName, s.serverName, sizeof(dst.serverName) - 1);
                        }
                        std::uint8_t out[1400] = {};
                        int outLen = 0;
                        if (encodeServerListResponse(out, (int)sizeof(out), seq++, response, outLen)) {
                            socket.sendTo(from, out, outLen);
                        }
                    }
                }
            }
        } else if (got < 0 && !socket.wouldBlock()) {
            std::cerr << "Master recv error.\n";
        }

        const std::uint64_t ts = nowMs();
        servers.erase(
            std::remove_if(
                servers.begin(),
                servers.end(),
                [ts](const RegistryEntry& e) { return (ts - e.lastSeenMs) > 10'000; }
            ),
            servers.end()
        );

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}
