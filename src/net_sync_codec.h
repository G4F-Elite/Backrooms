#pragma once

#include <cstring>

constexpr int RESHUFFLE_CELL_COUNT = 16 * 16;
constexpr int RESHUFFLE_PACKET_LEN = 1 + 4 + 4 + 4 + RESHUFFLE_CELL_COUNT;
constexpr int SCARE_PACKET_LEN = 2;

struct ReshuffleSyncData {
    int chunkX;
    int chunkZ;
    unsigned int seed;
    unsigned char cells[RESHUFFLE_CELL_COUNT];
};

inline bool encodeReshufflePayload(char* payload, int payloadLen, const ReshuffleSyncData& data) {
    if (!payload || payloadLen < RESHUFFLE_PACKET_LEN) return false;
    memcpy(payload + 1, &data.chunkX, 4);
    memcpy(payload + 5, &data.chunkZ, 4);
    memcpy(payload + 9, &data.seed, 4);
    memcpy(payload + 13, data.cells, RESHUFFLE_CELL_COUNT);
    return true;
}

inline bool decodeReshufflePayload(const char* payload, int payloadLen, ReshuffleSyncData& out) {
    if (!payload || payloadLen < RESHUFFLE_PACKET_LEN) return false;
    memcpy(&out.chunkX, payload + 1, 4);
    memcpy(&out.chunkZ, payload + 5, 4);
    memcpy(&out.seed, payload + 9, 4);
    memcpy(out.cells, payload + 13, RESHUFFLE_CELL_COUNT);
    return true;
}

inline bool encodeScarePayload(char* payload, int payloadLen, int sourcePlayerId) {
    if (!payload || payloadLen < SCARE_PACKET_LEN) return false;
    payload[1] = (char)sourcePlayerId;
    return true;
}

inline bool decodeScarePayload(const char* payload, int payloadLen, int& outSourcePlayerId) {
    if (!payload || payloadLen < SCARE_PACKET_LEN) return false;
    outSourcePlayerId = (int)(unsigned char)payload[1];
    return true;
}
