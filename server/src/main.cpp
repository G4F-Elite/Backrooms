#include <chrono>
#include <array>
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

// Legacy packet compatibility with current game client (src/net_types.h).
constexpr std::uint8_t LEGACY_PKT_PING = 1;
constexpr std::uint8_t LEGACY_PKT_PONG = 2;
constexpr std::uint8_t LEGACY_PKT_JOIN = 3;
constexpr std::uint8_t LEGACY_PKT_WELCOME = 4;
constexpr std::uint8_t LEGACY_PKT_PLAYER_STATE = 6;
constexpr std::uint8_t LEGACY_PKT_PLAYER_NAME = 12;
constexpr std::uint8_t LEGACY_PKT_GAME_START = 14;
constexpr std::uint8_t LEGACY_PKT_ENTITY_SNAPSHOT = 21;
constexpr std::uint8_t LEGACY_PKT_OBJECTIVE_STATE = 22;
constexpr std::uint8_t LEGACY_PKT_ITEM_SNAPSHOT = 23;
constexpr std::uint8_t LEGACY_PKT_INVENTORY_SYNC = 24;
constexpr std::uint8_t LEGACY_PKT_INTERACT_REQ = 25;
constexpr std::uint8_t LEGACY_PKT_ROAM_EVENT = 26;
constexpr int LEGACY_MAX_PLAYERS = 4;
constexpr int LEGACY_PACKET_SIZE = 512;
constexpr double CLIENT_PACKET_BUDGET_MAX = 240.0;
constexpr double CLIENT_PACKET_BUDGET_REFILL_PER_SEC = 140.0;

struct LegacyClientState {
    bool active = false;
    UdpAddress addr{};
    std::uint8_t id = 255;
    char name[kPlayerNameLen] = {};
    bool hasPlayerState = false;
    std::array<std::uint8_t, 32> lastPlayerState{};
    std::chrono::steady_clock::time_point lastSeen{};
    std::chrono::steady_clock::time_point budgetTs{};
    double packetBudget = CLIENT_PACKET_BUDGET_MAX;
};

bool sameAddress(const UdpAddress& a, const UdpAddress& b) {
    return a.ipv4HostOrder == b.ipv4HostOrder && a.portHostOrder == b.portHostOrder;
}

int findClientIndexByAddress(const std::array<LegacyClientState, LEGACY_MAX_PLAYERS>& clients, const UdpAddress& from) {
    for (int i = 0; i < LEGACY_MAX_PLAYERS; i++) {
        if (clients[i].active && sameAddress(clients[i].addr, from)) return i;
    }
    return -1;
}

std::uint8_t countActiveClients(const std::array<LegacyClientState, LEGACY_MAX_PLAYERS>& clients) {
    std::uint8_t c = 0;
    for (const auto& cl : clients) if (cl.active) c++;
    return c;
}

int allocateClientSlot(const std::array<LegacyClientState, LEGACY_MAX_PLAYERS>& clients) {
    for (int i = 0; i < LEGACY_MAX_PLAYERS; i++) {
        if (!clients[i].active) return i;
    }
    return -1;
}

bool isRelayPacket(std::uint8_t type) {
    if (type == LEGACY_PKT_PING || type == LEGACY_PKT_PONG || type == LEGACY_PKT_JOIN ||
        type == LEGACY_PKT_WELCOME || type == LEGACY_PKT_GAME_START) {
        return false;
    }
    return type >= LEGACY_PKT_PLAYER_STATE;
}

bool isKnownLegacyPacketType(std::uint8_t type) {
    switch (type) {
        case LEGACY_PKT_PING:
        case LEGACY_PKT_PONG:
        case LEGACY_PKT_JOIN:
        case LEGACY_PKT_WELCOME:
        case LEGACY_PKT_PLAYER_STATE:
        case LEGACY_PKT_PLAYER_NAME:
        case LEGACY_PKT_GAME_START:
        case LEGACY_PKT_ENTITY_SNAPSHOT:
        case LEGACY_PKT_OBJECTIVE_STATE:
        case LEGACY_PKT_ITEM_SNAPSHOT:
        case LEGACY_PKT_INVENTORY_SYNC:
        case LEGACY_PKT_INTERACT_REQ:
        case LEGACY_PKT_ROAM_EVENT:
            return true;
        default:
            return false;
    }
}

bool isValidLegacyPacketSize(std::uint8_t type, int len) {
    if (len <= 0 || len > LEGACY_PACKET_SIZE) return false;
    switch (type) {
        case LEGACY_PKT_PING: return len >= 7 && len <= 8;
        case LEGACY_PKT_PONG: return len == 8;
        case LEGACY_PKT_JOIN: return len == 64;
        case LEGACY_PKT_WELCOME: return len == 16;
        case LEGACY_PKT_PLAYER_STATE: return len == 32;
        case LEGACY_PKT_PLAYER_NAME: return len == 2 + kPlayerNameLen;
        case LEGACY_PKT_GAME_START: return len == 32;
        case LEGACY_PKT_ENTITY_SNAPSHOT: return len >= 2;
        case LEGACY_PKT_OBJECTIVE_STATE: return len >= 4 && len <= 8;
        case LEGACY_PKT_ITEM_SNAPSHOT: return len >= 2;
        case LEGACY_PKT_INVENTORY_SYNC: return len >= 17;
        case LEGACY_PKT_INTERACT_REQ: return len >= 4 && len <= 8;
        case LEGACY_PKT_ROAM_EVENT: return len >= 8 && len <= 12;
        default: return false;
    }
}

bool consumePacketBudget(LegacyClientState& client, const std::chrono::steady_clock::time_point& now) {
    if (client.budgetTs.time_since_epoch().count() == 0) {
        client.budgetTs = now;
    }
    const double elapsedSec = std::chrono::duration<double>(now - client.budgetTs).count();
    if (elapsedSec > 0.0) {
        client.packetBudget += elapsedSec * CLIENT_PACKET_BUDGET_REFILL_PER_SEC;
        if (client.packetBudget > CLIENT_PACKET_BUDGET_MAX) client.packetBudget = CLIENT_PACKET_BUDGET_MAX;
        client.budgetTs = now;
    }
    if (client.packetBudget < 1.0) return false;
    client.packetBudget -= 1.0;
    return true;
}

void relayToOthers(
    UdpSocketLite& socket,
    const std::array<LegacyClientState, LEGACY_MAX_PLAYERS>& clients,
    int senderIdx,
    const std::uint8_t* packet,
    int len
) {
    for (int i = 0; i < LEGACY_MAX_PLAYERS; i++) {
        if (!clients[i].active || i == senderIdx) continue;
        socket.sendTo(clients[i].addr, packet, len);
    }
}

void sendPlayerName(UdpSocketLite& socket, const UdpAddress& to, std::uint8_t id, const char* name) {
    std::uint8_t pkt[2 + kPlayerNameLen] = {};
    pkt[0] = LEGACY_PKT_PLAYER_NAME;
    pkt[1] = id;
    std::memcpy(pkt + 2, name, kPlayerNameLen);
    socket.sendTo(to, pkt, (int)sizeof(pkt));
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
    std::array<LegacyClientState, LEGACY_MAX_PLAYERS> clients{};
    std::vector<std::uint8_t> cachedEntitySnapshot;
    std::vector<std::uint8_t> cachedObjectiveState;
    std::vector<std::uint8_t> cachedItemSnapshot;
    std::vector<std::uint8_t> cachedInventorySync;

    const std::uint8_t playersMax = (std::uint8_t)LEGACY_MAX_PLAYERS;
    const std::uint32_t worldSeed = (std::uint32_t)std::time(nullptr);
    const float spawnPos[3] = {0.0f, 1.7f, 0.0f};
    const auto clientTimeout = std::chrono::seconds(25);

    for (;;) {
        for (;;) {
            std::uint8_t buf[1400] = {};
            UdpAddress from{};
            int got = socket.recvFrom(from, buf, (int)sizeof(buf));
            if (got <= 0) {
                if (got < 0 && !socket.wouldBlock()) {
                    std::cerr << "Server recv error.\n";
                }
                break;
            }

            const auto now = std::chrono::steady_clock::now();
            const std::uint8_t pktType = (std::uint8_t)buf[0];
            int senderIdx = findClientIndexByAddress(clients, from);
            const bool looksProtocolPacket = (got >= kHeaderSize && buf[0] == 0 && buf[1] == (std::uint8_t)kProtocolVersion);

            if (!looksProtocolPacket && pktType <= LEGACY_PKT_ROAM_EVENT) {
                if (!isKnownLegacyPacketType(pktType)) continue;
                if (!isValidLegacyPacketSize(pktType, got)) continue;
            }

            if (senderIdx >= 0 && !consumePacketBudget(clients[senderIdx], now)) {
                continue;
            }

            if (!looksProtocolPacket && pktType == LEGACY_PKT_JOIN) {
                if (senderIdx < 0) {
                    senderIdx = allocateClientSlot(clients);
                    if (senderIdx < 0) {
                        continue;
                    }
                    clients[senderIdx].active = true;
                    clients[senderIdx].addr = from;
                    clients[senderIdx].id = (std::uint8_t)senderIdx;
                    std::memset(clients[senderIdx].name, 0, kPlayerNameLen);
                    std::strncpy(clients[senderIdx].name, "Player", kPlayerNameLen - 1);
                    clients[senderIdx].hasPlayerState = false;
                    clients[senderIdx].packetBudget = CLIENT_PACKET_BUDGET_MAX;
                    clients[senderIdx].budgetTs = now;
                }
                clients[senderIdx].lastSeen = now;

                if (got >= 1 + kPlayerNameLen) {
                    std::memcpy(clients[senderIdx].name, buf + 1, kPlayerNameLen);
                    clients[senderIdx].name[kPlayerNameLen - 1] = '\0';
                }

                const std::uint8_t assignedId = clients[senderIdx].id;

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

                for (int i = 0; i < LEGACY_MAX_PLAYERS; i++) {
                    if (!clients[i].active) continue;
                    sendPlayerName(socket, from, clients[i].id, clients[i].name);
                }

                for (int i = 0; i < LEGACY_MAX_PLAYERS; i++) {
                    if (!clients[i].active || !clients[i].hasPlayerState || i == senderIdx) continue;
                    socket.sendTo(from, clients[i].lastPlayerState.data(), (int)clients[i].lastPlayerState.size());
                }

                if (!cachedEntitySnapshot.empty()) socket.sendTo(from, cachedEntitySnapshot.data(), (int)cachedEntitySnapshot.size());
                if (!cachedObjectiveState.empty()) socket.sendTo(from, cachedObjectiveState.data(), (int)cachedObjectiveState.size());
                if (!cachedItemSnapshot.empty()) socket.sendTo(from, cachedItemSnapshot.data(), (int)cachedItemSnapshot.size());
                if (!cachedInventorySync.empty()) socket.sendTo(from, cachedInventorySync.data(), (int)cachedInventorySync.size());

                for (int i = 0; i < LEGACY_MAX_PLAYERS; i++) {
                    if (!clients[i].active || i == senderIdx) continue;
                    sendPlayerName(socket, clients[i].addr, assignedId, clients[senderIdx].name);
                }
                continue;
            }

            if (senderIdx >= 0) {
                clients[senderIdx].lastSeen = now;
            }

            if (!looksProtocolPacket && pktType == LEGACY_PKT_PING && got >= 7) {
                std::uint8_t pong[8] = {};
                pong[0] = LEGACY_PKT_PONG;
                std::memcpy(pong + 1, buf + 1, 6);
                socket.sendTo(from, pong, 8);
                continue;
            }

            if (senderIdx < 0) {
                continue;
            }

            if (!looksProtocolPacket && pktType == LEGACY_PKT_PLAYER_NAME && got >= 2 + kPlayerNameLen) {
                std::memcpy(clients[senderIdx].name, buf + 2, kPlayerNameLen);
                clients[senderIdx].name[kPlayerNameLen - 1] = '\0';
                buf[1] = clients[senderIdx].id;
                relayToOthers(socket, clients, senderIdx, buf, got);
                continue;
            }

            if (!looksProtocolPacket && pktType == LEGACY_PKT_PLAYER_STATE && got >= 32) {
                buf[1] = clients[senderIdx].id;
                std::memcpy(clients[senderIdx].lastPlayerState.data(), buf, 32);
                clients[senderIdx].hasPlayerState = true;
                relayToOthers(socket, clients, senderIdx, buf, 32);
                continue;
            }

            if (!looksProtocolPacket && pktType == LEGACY_PKT_ENTITY_SNAPSHOT && got > 2 && got <= LEGACY_PACKET_SIZE) {
                cachedEntitySnapshot.assign(buf, buf + got);
            } else if (!looksProtocolPacket && pktType == LEGACY_PKT_OBJECTIVE_STATE && got >= 4 && got <= LEGACY_PACKET_SIZE) {
                cachedObjectiveState.assign(buf, buf + got);
            } else if (!looksProtocolPacket && pktType == LEGACY_PKT_ITEM_SNAPSHOT && got > 2 && got <= LEGACY_PACKET_SIZE) {
                cachedItemSnapshot.assign(buf, buf + got);
            } else if (!looksProtocolPacket && pktType == LEGACY_PKT_INVENTORY_SYNC && got > 1 && got <= LEGACY_PACKET_SIZE) {
                cachedInventorySync.assign(buf, buf + got);
            }

            if (!looksProtocolPacket && isRelayPacket(pktType)) {
                relayToOthers(socket, clients, senderIdx, buf, got);
                continue;
            }

            MessageHeader header{};
            if (decodeHeader(buf, got, header) && header.protocolVersion == kProtocolVersion) {
                if (header.type == (std::uint8_t)MessageType::HandshakeHello) {
                    HandshakeHello hello{};
                    if (decodeHandshakeHello(buf, got, hello)) {
                        HandshakeWelcome welcome{};
                        welcome.accepted = 1;
                        welcome.playerId = countActiveClients(clients);
                        welcome.tickRateHz = 30;
                        int outLen = 0;
                        std::uint8_t out[64] = {};
                        if (encodeHandshakeWelcome(out, (int)sizeof(out), seq++, welcome, outLen)) {
                            socket.sendTo(from, out, outLen);
                        }
                    }
                }
            }
        }

        auto now = std::chrono::steady_clock::now();
        for (auto& c : clients) {
            if (!c.active) continue;
            if (now - c.lastSeen > clientTimeout) {
                c.active = false;
                c.hasPlayerState = false;
            }
        }

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeartbeat).count() >= 1000) {
            Heartbeat hb{};
            hb.gamePort = (std::uint16_t)listenPort;
            hb.currentPlayers = countActiveClients(clients);
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
