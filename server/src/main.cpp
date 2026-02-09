#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <ctime>

#include "../../shared/net/UdpSocketLite.h"
#include "../../shared/protocol/Packets.h"

namespace {

using backrooms::net::UdpAddress;
using backrooms::net::UdpSocketLite;
using namespace backrooms::protocol;

// Compatibility with existing client net packet ids.
constexpr std::uint8_t LEGACY_PKT_PING = 1;
constexpr std::uint8_t LEGACY_PKT_PONG = 2;
constexpr std::uint8_t LEGACY_PKT_JOIN = 3;
constexpr std::uint8_t LEGACY_PKT_WELCOME = 4;
constexpr std::uint8_t LEGACY_PKT_GAME_START = 14;

struct LegacyClient {
    UdpAddress addr;
    std::uint8_t id;
};

bool sameAddress(const UdpAddress& a, const UdpAddress& b) {
    return a.ipv4HostOrder == b.ipv4HostOrder && a.portHostOrder == b.portHostOrder;
}

std::uint8_t findOrAssignClientId(std::vector<LegacyClient>& clients, const UdpAddress& from) {
    for (const auto& c : clients) {
        if (sameAddress(c.addr, from)) return c.id;
    }
    std::uint8_t nextId = (std::uint8_t)(clients.size() + 1);
    if (nextId >= (std::uint8_t)kMaxPlayersDedicated) nextId = (std::uint8_t)(kMaxPlayersDedicated - 1);
    clients.push_back({from, nextId});
    return nextId;
}

std::uint32_t parseIpv4HostOrder(const std::string& ip) {
    std::uint32_t a = 127, b = 0, c = 0, d = 1;
    if (sscanf(ip.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return (127u << 24) | 1u;
    if (a > 255 || b > 255 || c > 255 || d > 255) return (127u << 24) | 1u;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

int parseIntArg(char** argv, int argc, const char* key, int fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (std::string(argv[i]) == key) return std::atoi(argv[i + 1]);
    }
    return fallback;
}

std::string parseStringArg(char** argv, int argc, const char* key, const std::string& fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (std::string(argv[i]) == key) return argv[i + 1];
    }
    return fallback;
}

}  // namespace

int main(int argc, char** argv) {
    const int listenPort = parseIntArg(argv, argc, "--port", kDefaultGamePort);
    const std::string masterIp = parseStringArg(argv, argc, "--master-ip", "127.0.0.1");
    const int masterPort = parseIntArg(argv, argc, "--master-port", kDefaultMasterPort);
    const std::string serverName = parseStringArg(argv, argc, "--name", "Backrooms Dedicated");

    UdpSocketLite socket;
    if (!socket.open((std::uint16_t)listenPort, true)) {
        std::cerr << "Failed to open server UDP socket on port " << listenPort << "\n";
        return 1;
    }

    UdpAddress masterAddr{};
    masterAddr.ipv4HostOrder = parseIpv4HostOrder(masterIp);
    masterAddr.portHostOrder = (std::uint16_t)masterPort;

    std::cout << "Backrooms Dedicated Server\n";
    std::cout << "Listen UDP: " << listenPort << "\n";
    std::cout << "Master: " << masterIp << ":" << masterPort << "\n";

    std::uint32_t seq = 1;
    auto lastHeartbeat = std::chrono::steady_clock::now() - std::chrono::seconds(2);
    std::uint8_t playersNow = 0;
    const std::uint8_t playersMax = (std::uint8_t)kMaxPlayersDedicated;
    const std::uint32_t worldSeed = (std::uint32_t)std::time(nullptr);
    const float spawnPos[3] = {0.0f, 1.7f, 0.0f};
    std::vector<LegacyClient> legacyClients;

    for (;;) {
        std::uint8_t buf[1400] = {};
        UdpAddress from{};
        int got = socket.recvFrom(from, buf, (int)sizeof(buf));
        if (got > 0) {
            // Legacy client compatibility path (existing netMgr).
            if ((std::uint8_t)buf[0] == LEGACY_PKT_JOIN) {
                const std::uint8_t assignedId = findOrAssignClientId(legacyClients, from);

                std::uint8_t welcome[16] = {};
                welcome[0] = LEGACY_PKT_WELCOME;
                welcome[1] = assignedId;
                std::memcpy(welcome + 2, &worldSeed, 4);
                socket.sendTo(from, welcome, 16);

                std::uint8_t gameStart[32] = {};
                gameStart[0] = LEGACY_PKT_GAME_START;
                std::memcpy(gameStart + 1, &worldSeed, 4);
                std::memcpy(gameStart + 5, &spawnPos[0], sizeof(spawnPos));
                socket.sendTo(from, gameStart, 32);

                if (playersNow + 1 < playersMax) playersNow++;
                continue;
            }
            if ((std::uint8_t)buf[0] == LEGACY_PKT_PING && got >= 7) {
                std::uint8_t pong[8] = {};
                pong[0] = LEGACY_PKT_PONG;
                std::memcpy(pong + 1, buf + 1, 6);
                socket.sendTo(from, pong, 8);
                continue;
            }

            MessageHeader header{};
            if (decodeHeader(buf, got, header) && header.protocolVersion == kProtocolVersion) {
                if (header.type == (std::uint8_t)MessageType::HandshakeHello) {
                    HandshakeHello hello{};
                    if (decodeHandshakeHello(buf, got, hello)) {
                        HandshakeWelcome welcome{};
                        welcome.accepted = 1;
                        welcome.playerId = playersNow;
                        welcome.tickRateHz = 30;
                        int outLen = 0;
                        std::uint8_t out[64] = {};
                        if (encodeHandshakeWelcome(out, (int)sizeof(out), seq++, welcome, outLen)) {
                            socket.sendTo(from, out, outLen);
                        }
                        if (playersNow + 1 < playersMax) playersNow++;
                    }
                }
            }
        } else if (got < 0 && !socket.wouldBlock()) {
            std::cerr << "Server recv error.\n";
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeat).count() >= 1000) {
            Heartbeat hb{};
            hb.gamePort = (std::uint16_t)listenPort;
            hb.currentPlayers = playersNow;
            hb.maxPlayers = playersMax;
            hb.flags = 0;
            std::memset(hb.serverName, 0, sizeof(hb.serverName));
            std::strncpy(hb.serverName, serverName.c_str(), sizeof(hb.serverName) - 1);
            int outLen = 0;
            std::uint8_t out[128] = {};
            if (encodeHeartbeat(out, (int)sizeof(out), seq++, hb, outLen)) {
                socket.sendTo(masterAddr, out, outLen);
            }
            lastHeartbeat = now;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    return 0;
}
