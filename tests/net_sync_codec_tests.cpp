#include <cassert>
#include <cstring>
#include <iostream>

#include "../src/net_sync_codec.h"

void testReshuffleRoundtrip() {
    char buffer[RESHUFFLE_PACKET_LEN];
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = 0x7F;
    
    ReshuffleSyncData in{};
    in.chunkX = -12;
    in.chunkZ = 34;
    in.seed = 123456789u;
    for (int i = 0; i < RESHUFFLE_CELL_COUNT; i++) in.cells[i] = (unsigned char)(i % 2);
    
    assert(encodeReshufflePayload(buffer, (int)sizeof(buffer), in));
    
    ReshuffleSyncData out{};
    assert(decodeReshufflePayload(buffer, (int)sizeof(buffer), out));
    assert(out.chunkX == in.chunkX);
    assert(out.chunkZ == in.chunkZ);
    assert(out.seed == in.seed);
    assert(memcmp(out.cells, in.cells, RESHUFFLE_CELL_COUNT) == 0);
}

void testReshuffleBounds() {
    char tooSmall[RESHUFFLE_PACKET_LEN - 1];
    ReshuffleSyncData data{};
    assert(!encodeReshufflePayload(nullptr, RESHUFFLE_PACKET_LEN, data));
    assert(!encodeReshufflePayload(tooSmall, (int)sizeof(tooSmall), data));
    
    ReshuffleSyncData out{};
    assert(!decodeReshufflePayload(nullptr, RESHUFFLE_PACKET_LEN, out));
    assert(!decodeReshufflePayload(tooSmall, (int)sizeof(tooSmall), out));
}

void testScareRoundtrip() {
    char buffer[SCARE_PACKET_LEN];
    memset(buffer, 0, sizeof(buffer));
    buffer[0] = 0x33;
    
    assert(encodeScarePayload(buffer, (int)sizeof(buffer), 3));
    int sourcePlayerId = -1;
    assert(decodeScarePayload(buffer, (int)sizeof(buffer), sourcePlayerId));
    assert(sourcePlayerId == 3);
}

void testScareBounds() {
    char tooSmall[SCARE_PACKET_LEN - 1];
    int sourcePlayerId = -1;
    assert(!encodeScarePayload(nullptr, SCARE_PACKET_LEN, 0));
    assert(!encodeScarePayload(tooSmall, (int)sizeof(tooSmall), 0));
    assert(!decodeScarePayload(nullptr, SCARE_PACKET_LEN, sourcePlayerId));
    assert(!decodeScarePayload(tooSmall, (int)sizeof(tooSmall), sourcePlayerId));
}

int main() {
    testReshuffleRoundtrip();
    testReshuffleBounds();
    testScareRoundtrip();
    testScareBounds();
    std::cout << "All net sync codec tests passed.\n";
    return 0;
}
