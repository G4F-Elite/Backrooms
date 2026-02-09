#pragma once

#include <cstdio>
#include <cstring>

#include "../shared/net/UdpSocketLite.h"
#include "../shared/protocol/Packets.h"

constexpr int MAX_DEDICATED_ROOMS = backrooms::protocol::kMaxServerListEntries;
constexpr float DEDICATED_SCAN_INTERVAL = 1.5f;

struct DedicatedRoomInfo {
    char ip[64];
    unsigned short gamePort;
    unsigned char playerCount;
    unsigned char maxPlayers;
    bool inGame;
    char serverName[backrooms::protocol::kServerNameLen];
};

inline std::uint32_t parseIpv4HostOrder(const char* ip) {
    unsigned int a = 127, b = 0, c = 0, d = 1;
    if (!ip) return (127u << 24) | 1u;
    if (sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) return (127u << 24) | 1u;
    if (a > 255 || b > 255 || c > 255 || d > 255) return (127u << 24) | 1u;
    return (a << 24) | (b << 16) | (c << 8) | d;
}

inline void formatIpv4HostOrder(std::uint32_t ipHostOrder, char* out, int outLen) {
    if (!out || outLen < 8) return;
    std::snprintf(
        out,
        outLen,
        "%u.%u.%u.%u",
        (ipHostOrder >> 24) & 0xFF,
        (ipHostOrder >> 16) & 0xFF,
        (ipHostOrder >> 8) & 0xFF,
        ipHostOrder & 0xFF
    );
}

class DedicatedDirectoryClient {
public:
    backrooms::net::UdpSocketLite socket;
    bool active = false;
    char masterIP[64] = "127.0.0.1";
    unsigned short masterPort = backrooms::protocol::kDefaultMasterPort;
    float lastScanAt = 0.0f;
    std::uint32_t sequence = 1;

    DedicatedRoomInfo rooms[MAX_DEDICATED_ROOMS];
    int roomCount = 0;
    int selectedRoom = 0;

    bool start(const char* ip, unsigned short port) {
        stop();
        std::snprintf(masterIP, sizeof(masterIP), "%s", ip ? ip : "127.0.0.1");
        masterPort = port;
        active = socket.open(0, true);
        lastScanAt = 0.0f;
        return active;
    }

    void stop() {
        socket.close();
        active = false;
        roomCount = 0;
        selectedRoom = 0;
    }

    void requestScan() {
        if (!active) return;
        std::uint8_t packet[64] = {};
        int written = 0;
        if (!backrooms::protocol::encodeServerListQuery(packet, (int)sizeof(packet), sequence++, written)) return;
        backrooms::net::UdpAddress master{};
        master.ipv4HostOrder = parseIpv4HostOrder(masterIP);
        master.portHostOrder = masterPort;
        socket.sendTo(master, packet, written);
    }

    void selectNextRoom() {
        if (roomCount <= 0) return;
        selectedRoom = (selectedRoom + 1) % roomCount;
    }

    const DedicatedRoomInfo* getSelectedRoom() const {
        if (roomCount <= 0) return nullptr;
        int idx = selectedRoom;
        if (idx < 0 || idx >= roomCount) idx = 0;
        return &rooms[idx];
    }

    void update(float nowTime) {
        if (!active) return;
        if (nowTime - lastScanAt >= DEDICATED_SCAN_INTERVAL) {
            lastScanAt = nowTime;
            requestScan();
        }
        recvResponses();
    }

private:
    void recvResponses() {
        std::uint8_t packet[1600] = {};
        backrooms::net::UdpAddress from{};
        while (true) {
            int got = socket.recvFrom(from, packet, (int)sizeof(packet));
            if (got <= 0) {
                if (!socket.wouldBlock()) {
                    // ignore transient socket errors and keep polling
                }
                break;
            }
            backrooms::protocol::ServerListResponse resp{};
            if (!backrooms::protocol::decodeServerListResponse(packet, got, resp)) continue;
            roomCount = (int)resp.count;
            if (roomCount > MAX_DEDICATED_ROOMS) roomCount = MAX_DEDICATED_ROOMS;
            for (int i = 0; i < roomCount; i++) {
                formatIpv4HostOrder(resp.entries[i].ipv4HostOrder, rooms[i].ip, (int)sizeof(rooms[i].ip));
                rooms[i].gamePort = resp.entries[i].gamePort;
                rooms[i].playerCount = resp.entries[i].currentPlayers;
                rooms[i].maxPlayers = resp.entries[i].maxPlayers;
                rooms[i].inGame = (resp.entries[i].flags & backrooms::protocol::kServerFlagInGame) != 0;
                std::snprintf(rooms[i].serverName, sizeof(rooms[i].serverName), "%s", resp.entries[i].serverName);
            }
            if (selectedRoom >= roomCount) selectedRoom = roomCount > 0 ? roomCount - 1 : 0;
        }
    }
};

inline DedicatedDirectoryClient dedicatedDirectory;
