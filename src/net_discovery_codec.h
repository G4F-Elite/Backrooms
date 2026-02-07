#pragma once

#include <cstring>

constexpr int DISCOVERY_NAME_LEN = 32;
constexpr int DISCOVERY_REQ_LEN = 2;
constexpr int DISCOVERY_RESP_LEN = 1 + 2 + 1 + 1 + 1 + DISCOVERY_NAME_LEN;
constexpr unsigned char DISCOVERY_PROTO_VER = 1;

struct DiscoveryHostPayload {
    unsigned short gamePort;
    unsigned char playerCount;
    unsigned char maxPlayers;
    bool gameStarted;
    char hostName[DISCOVERY_NAME_LEN];
};

inline bool encodeDiscoveryRequest(char* payload, int payloadLen) {
    if (!payload || payloadLen < DISCOVERY_REQ_LEN) return false;
    payload[1] = (char)DISCOVERY_PROTO_VER;
    return true;
}

inline bool decodeDiscoveryRequest(const char* payload, int payloadLen) {
    if (!payload || payloadLen < DISCOVERY_REQ_LEN) return false;
    return (unsigned char)payload[1] == DISCOVERY_PROTO_VER;
}

inline bool encodeDiscoveryHostPayload(char* payload, int payloadLen, const DiscoveryHostPayload& in) {
    if (!payload || payloadLen < DISCOVERY_RESP_LEN) return false;
    memcpy(payload + 1, &in.gamePort, 2);
    payload[3] = (char)in.playerCount;
    payload[4] = (char)in.maxPlayers;
    payload[5] = in.gameStarted ? 1 : 0;
    memset(payload + 6, 0, DISCOVERY_NAME_LEN);
    size_t hostLen = strnlen(in.hostName, DISCOVERY_NAME_LEN - 1);
    memcpy(payload + 6, in.hostName, hostLen);
    return true;
}

inline bool decodeDiscoveryHostPayload(const char* payload, int payloadLen, DiscoveryHostPayload& out) {
    if (!payload || payloadLen < DISCOVERY_RESP_LEN) return false;
    memcpy(&out.gamePort, payload + 1, 2);
    out.playerCount = (unsigned char)payload[3];
    out.maxPlayers = (unsigned char)payload[4];
    out.gameStarted = payload[5] != 0;
    memset(out.hostName, 0, DISCOVERY_NAME_LEN);
    memcpy(out.hostName, payload + 6, DISCOVERY_NAME_LEN);
    out.hostName[DISCOVERY_NAME_LEN - 1] = 0;
    return true;
}
