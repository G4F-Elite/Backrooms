#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "../../shared/net/UdpSocketLite.h"
#include "../../shared/protocol/Packets.h"

namespace {

using backrooms::net::UdpAddress;
using backrooms::net::UdpSocketLite;
using namespace backrooms::protocol;

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

    for (;;) {
        std::uint8_t buf[1400] = {};
        UdpAddress from{};
        int got = socket.recvFrom(from, buf, (int)sizeof(buf));
        if (got > 0) {
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
