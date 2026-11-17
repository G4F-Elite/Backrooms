#pragma once
// Player-to-squad packets that the host only relays: ping markers and squad banter.
#include "net_core.h"

inline void NetworkManager::relayToClients(char* buf, int len, int exceptId) {
    if (!isHost) return;
    for (int i = 1; i < MAX_PLAYERS; i++) {
        if (!players[i].active || i == exceptId) continue;
        sockaddr_in dest;
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = players[i].addr;
        dest.sin_port = players[i].port;
        sendto(sock, buf, len, 0, (sockaddr*)&dest, sizeof(dest));
    }
}

inline void NetworkManager::handlePingMark(char* buf, int len) {
    if (len < 2 + (int)sizeof(Vec3)) return;
    int fromId = (unsigned char)buf[1];
    Vec3 p;
    memcpy(&p, buf + 2, sizeof(Vec3));
    pingMarkFrom = fromId;
    pingMarkPos = p;
    pingMarkTtl = 6.0f;
    pingMarkReceived = true;
    relayToClients(buf, 32, fromId);
}

inline void NetworkManager::sendSquadFx(const SquadFxEvent& fx, const ShiftStats& stats) {
    if (!connected) return;
    char buf[PACKET_SIZE];
    encodeSquadFxPacket(buf, PKT_SQUAD_FX, fx, stats);
    broadcast(buf, SQUAD_FX_PACKET_LEN);
}

inline void NetworkManager::handleSquadFx(char* buf, int len) {
    SquadFxEvent fx;
    ShiftStats stats = {};
    if (!decodeSquadFxPacket(buf, len, fx, stats)) return;
    if (fx.from == myId || !players[fx.from].active) return;
    relayToClients(buf, SQUAD_FX_PACKET_LEN, fx.from);
    if (fx.kind == SQUAD_FX_STATS) {
        squadStats[fx.from] = stats;
        return;
    }
    if (fxQueueCount < SQUAD_FX_QUEUE_LEN) fxQueue[fxQueueCount++] = fx;
}
