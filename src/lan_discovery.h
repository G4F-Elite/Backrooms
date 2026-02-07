#pragma once

#include <cstdio>
#include <cstring>
#include "net_types.h"
#include "net_discovery_codec.h"

constexpr int MAX_DISCOVERED_ROOMS = 16;
constexpr float DISCOVERY_HOST_TIMEOUT = 6.0f;
constexpr float DISCOVERY_SCAN_INTERVAL = 1.5f;
constexpr float DISCOVERY_ANNOUNCE_INTERVAL = 1.0f;

struct LanRoomInfo {
    char ip[64];
    unsigned short gamePort;
    unsigned char playerCount;
    unsigned char maxPlayers;
    bool gameStarted;
    char hostName[DISCOVERY_NAME_LEN];
    float lastSeen;
};

class LanDiscoveryManager {
public:
    SOCKET sock = INVALID_SOCKET;
    bool initialized = false;
    bool hostMode = false;
    float lastScanAt = 0.0f;
    float lastAnnounceAt = 0.0f;
    char localIP[64] = "0.0.0.0";
    LanRoomInfo rooms[MAX_DISCOVERED_ROOMS];
    int roomCount = 0;
    int selectedRoom = 0;
    
    void ensureLocalIP() {
        if (strcmp(localIP, "0.0.0.0") != 0) return;
        char host[256];
        if (gethostname(host, sizeof(host)) != 0) return;
        addrinfo hints;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_DGRAM;
        addrinfo* result = nullptr;
        if (getaddrinfo(host, nullptr, &hints, &result) != 0) return;
        for (addrinfo* it = result; it; it = it->ai_next) {
            sockaddr_in* addr = (sockaddr_in*)it->ai_addr;
            const char* ip = inet_ntoa(addr->sin_addr);
            if (!ip) continue;
            if (strcmp(ip, "127.0.0.1") == 0) continue;
            strncpy(localIP, ip, sizeof(localIP) - 1);
            localIP[sizeof(localIP) - 1] = 0;
            break;
        }
        freeaddrinfo(result);
    }
    
    bool startClient() {
        if (!initSocket(false)) return false;
        hostMode = false;
        ensureLocalIP();
        return true;
    }
    
    bool startHost() {
        if (!initSocket(true)) return false;
        hostMode = true;
        ensureLocalIP();
        return true;
    }
    
    void stop() {
        if (sock != INVALID_SOCKET) closesocket(sock);
        sock = INVALID_SOCKET;
        roomCount = 0;
        selectedRoom = 0;
        if (initialized) {
            WSACleanup();
            initialized = false;
        }
    }
    
    void requestScan() {
        if (sock == INVALID_SOCKET) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_DISCOVER_REQ;
        if (!encodeDiscoveryRequest(buf, PACKET_SIZE)) return;
        
        sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_port = htons(NET_DISCOVERY_PORT);
        dest.sin_addr.s_addr = INADDR_BROADCAST;
        sendto(sock, buf, DISCOVERY_REQ_LEN, 0, (sockaddr*)&dest, sizeof(dest));
    }
    
    void announceHost(int playerCount, bool gameStarted, float nowTime) {
        if (!hostMode || sock == INVALID_SOCKET) return;
        if (nowTime - lastAnnounceAt < DISCOVERY_ANNOUNCE_INTERVAL) return;
        lastAnnounceAt = nowTime;
        sendHostPacket(PKT_HOST_ANNOUNCE, playerCount, gameStarted, nullptr);
    }
    
    void updateClient(float nowTime) {
        if (sock == INVALID_SOCKET || hostMode) return;
        if (nowTime - lastScanAt >= DISCOVERY_SCAN_INTERVAL) {
            lastScanAt = nowTime;
            requestScan();
        }
        recvPackets(nowTime, false, 0, false);
        purgeExpired(nowTime);
    }
    
    void updateHost(int playerCount, bool gameStarted, float nowTime) {
        if (sock == INVALID_SOCKET || !hostMode) return;
        recvPackets(nowTime, true, playerCount, gameStarted);
        announceHost(playerCount, gameStarted, nowTime);
        purgeExpired(nowTime);
    }
    
    void selectNextRoom() {
        if (roomCount <= 0) return;
        selectedRoom = (selectedRoom + 1) % roomCount;
    }
    
    const LanRoomInfo* getSelectedRoom() const {
        if (roomCount <= 0) return nullptr;
        int idx = selectedRoom;
        if (idx < 0 || idx >= roomCount) idx = 0;
        return &rooms[idx];
    }
    
private:
    bool initSocket(bool bindDiscoveryPort) {
        if (sock != INVALID_SOCKET) return true;
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
        initialized = true;
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) return false;
        
        BOOL on = TRUE;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        
        sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = htons(bindDiscoveryPort ? NET_DISCOVERY_PORT : 0);
        if (bind(sock, (sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
            closesocket(sock);
            sock = INVALID_SOCKET;
            return false;
        }
        return true;
    }
    
    void recvPackets(float nowTime, bool canReply, int playerCount, bool gameStarted) {
        char buf[PACKET_SIZE];
        sockaddr_in from;
        int fromLen = sizeof(from);
        while (true) {
            int recvLen = recvfrom(sock, buf, PACKET_SIZE, 0, (sockaddr*)&from, &fromLen);
            if (recvLen <= 0) break;
            PacketType type = (PacketType)buf[0];
            if (type == PKT_DISCOVER_REQ) {
                if (canReply && decodeDiscoveryRequest(buf, recvLen)) {
                    sendHostPacket(PKT_DISCOVER_RESP, playerCount, gameStarted, &from);
                }
            } else if (type == PKT_DISCOVER_RESP || type == PKT_HOST_ANNOUNCE) {
                DiscoveryHostPayload payload;
                if (!decodeDiscoveryHostPayload(buf, recvLen, payload)) continue;
                upsertRoom(from, payload, nowTime);
            }
        }
    }
    
    void sendHostPacket(PacketType type, int playerCount, bool gameStarted, sockaddr_in* target) {
        char buf[PACKET_SIZE];
        buf[0] = (char)type;
        DiscoveryHostPayload payload{};
        payload.gamePort = htons((unsigned short)NET_PORT);
        payload.playerCount = (unsigned char)playerCount;
        payload.maxPlayers = (unsigned char)MAX_PLAYERS;
        payload.gameStarted = gameStarted;
        strncpy(payload.hostName, "Host", DISCOVERY_NAME_LEN - 1);
        if (!encodeDiscoveryHostPayload(buf, PACKET_SIZE, payload)) return;
        
        if (target) {
            sendto(sock, buf, DISCOVERY_RESP_LEN, 0, (sockaddr*)target, sizeof(*target));
            return;
        }
        sockaddr_in dest;
        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;
        dest.sin_port = htons(NET_DISCOVERY_PORT);
        dest.sin_addr.s_addr = INADDR_BROADCAST;
        sendto(sock, buf, DISCOVERY_RESP_LEN, 0, (sockaddr*)&dest, sizeof(dest));
    }
    
    void upsertRoom(const sockaddr_in& from, const DiscoveryHostPayload& payload, float nowTime) {
        char ip[64] = {0};
        inet_ntop(AF_INET, (void*)&from.sin_addr, ip, sizeof(ip));
        int idx = -1;
        for (int i = 0; i < roomCount; i++) {
            if (strcmp(rooms[i].ip, ip) == 0) { idx = i; break; }
        }
        if (idx < 0) {
            if (roomCount >= MAX_DISCOVERED_ROOMS) return;
            idx = roomCount++;
        }
        snprintf(rooms[idx].ip, sizeof(rooms[idx].ip), "%s", ip);
        rooms[idx].gamePort = ntohs(payload.gamePort);
        rooms[idx].playerCount = payload.playerCount;
        rooms[idx].maxPlayers = payload.maxPlayers;
        rooms[idx].gameStarted = payload.gameStarted;
        snprintf(rooms[idx].hostName, sizeof(rooms[idx].hostName), "%s", payload.hostName);
        rooms[idx].lastSeen = nowTime;
        if (selectedRoom >= roomCount) selectedRoom = 0;
    }
    
    void purgeExpired(float nowTime) {
        for (int i = 0; i < roomCount;) {
            if (nowTime - rooms[i].lastSeen <= DISCOVERY_HOST_TIMEOUT) { i++; continue; }
            for (int j = i; j < roomCount - 1; j++) rooms[j] = rooms[j + 1];
            roomCount--;
            if (selectedRoom >= roomCount) selectedRoom = roomCount > 0 ? roomCount - 1 : 0;
        }
    }
};

inline LanDiscoveryManager lanDiscovery;
