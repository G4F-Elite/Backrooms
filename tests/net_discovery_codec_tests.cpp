#include <cassert>
#include <cstring>
#include <iostream>

#include "../src/net_discovery_codec.h"

void testDiscoveryRequestRoundtrip() {
    char buf[DISCOVERY_REQ_LEN];
    memset(buf, 0, sizeof(buf));
    assert(encodeDiscoveryRequest(buf, (int)sizeof(buf)));
    assert(decodeDiscoveryRequest(buf, (int)sizeof(buf)));
}

void testDiscoveryRequestBounds() {
    char small[DISCOVERY_REQ_LEN - 1];
    memset(small, 0, sizeof(small));
    assert(!encodeDiscoveryRequest(nullptr, DISCOVERY_REQ_LEN));
    assert(!encodeDiscoveryRequest(small, (int)sizeof(small)));
    assert(!decodeDiscoveryRequest(nullptr, DISCOVERY_REQ_LEN));
    assert(!decodeDiscoveryRequest(small, (int)sizeof(small)));
}

void testDiscoveryResponseRoundtrip() {
    char buf[DISCOVERY_RESP_LEN];
    memset(buf, 0, sizeof(buf));
    
    DiscoveryHostPayload in{};
    in.gamePort = 0x1234;
    in.playerCount = 3;
    in.maxPlayers = 4;
    in.gameStarted = true;
    strncpy(in.hostName, "Host", DISCOVERY_NAME_LEN - 1);
    
    assert(encodeDiscoveryHostPayload(buf, (int)sizeof(buf), in));
    DiscoveryHostPayload out{};
    assert(decodeDiscoveryHostPayload(buf, (int)sizeof(buf), out));
    
    assert(out.gamePort == in.gamePort);
    assert(out.playerCount == in.playerCount);
    assert(out.maxPlayers == in.maxPlayers);
    assert(out.gameStarted == in.gameStarted);
    assert(strcmp(out.hostName, in.hostName) == 0);
}

void testDiscoveryResponseBounds() {
    char small[DISCOVERY_RESP_LEN - 1];
    memset(small, 0, sizeof(small));
    DiscoveryHostPayload payload{};
    DiscoveryHostPayload out{};
    assert(!encodeDiscoveryHostPayload(nullptr, DISCOVERY_RESP_LEN, payload));
    assert(!encodeDiscoveryHostPayload(small, (int)sizeof(small), payload));
    assert(!decodeDiscoveryHostPayload(nullptr, DISCOVERY_RESP_LEN, out));
    assert(!decodeDiscoveryHostPayload(small, (int)sizeof(small), out));
}

int main() {
    testDiscoveryRequestRoundtrip();
    testDiscoveryRequestBounds();
    testDiscoveryResponseRoundtrip();
    testDiscoveryResponseBounds();
    std::cout << "All LAN discovery codec tests passed.\n";
    return 0;
}
