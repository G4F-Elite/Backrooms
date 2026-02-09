#include <cassert>
#include <cstring>
#include <iostream>

#include "../shared/protocol/Packets.h"

using namespace backrooms::protocol;

void testHandshakeRoundtrip() {
    std::uint8_t buf[256] = {};
    HandshakeHello in{};
    in.nonce = 0xAABBCCDDu;
    std::strncpy(in.playerName, "PlayerOne", sizeof(in.playerName) - 1);
    int written = 0;
    assert(encodeHandshakeHello(buf, (int)sizeof(buf), 7, in, written));
    assert(written > 0);

    HandshakeHello out{};
    assert(decodeHandshakeHello(buf, written, out));
    assert(out.nonce == in.nonce);
    assert(std::strcmp(out.playerName, "PlayerOne") == 0);
}

void testHeartbeatRoundtrip() {
    std::uint8_t buf[256] = {};
    Heartbeat in{};
    in.gamePort = 27015;
    in.currentPlayers = 3;
    in.maxPlayers = 8;
    in.flags = kServerFlagInGame;
    std::strncpy(in.serverName, "EU-1", sizeof(in.serverName) - 1);
    int written = 0;
    assert(encodeHeartbeat(buf, (int)sizeof(buf), 9, in, written));

    Heartbeat out{};
    assert(decodeHeartbeat(buf, written, out));
    assert(out.gamePort == in.gamePort);
    assert(out.currentPlayers == in.currentPlayers);
    assert(out.maxPlayers == in.maxPlayers);
    assert(out.flags == in.flags);
    assert(std::strcmp(out.serverName, "EU-1") == 0);
}

void testServerListRoundtrip() {
    ServerListResponse in{};
    in.count = 2;
    in.entries[0].ipv4HostOrder = 0x7F000001u;
    in.entries[0].gamePort = 27015;
    in.entries[0].currentPlayers = 1;
    in.entries[0].maxPlayers = 8;
    in.entries[0].flags = 0;
    std::strncpy(in.entries[0].serverName, "Local", sizeof(in.entries[0].serverName) - 1);

    in.entries[1].ipv4HostOrder = 0x0A000005u;
    in.entries[1].gamePort = 27016;
    in.entries[1].currentPlayers = 6;
    in.entries[1].maxPlayers = 8;
    in.entries[1].flags = kServerFlagInGame;
    std::strncpy(in.entries[1].serverName, "LAN-2", sizeof(in.entries[1].serverName) - 1);

    std::uint8_t buf[1400] = {};
    int written = 0;
    assert(encodeServerListResponse(buf, (int)sizeof(buf), 11, in, written));

    ServerListResponse out{};
    assert(decodeServerListResponse(buf, written, out));
    assert(out.count == 2);
    assert(out.entries[1].gamePort == 27016);
    assert(out.entries[1].flags == kServerFlagInGame);
    assert(std::strcmp(out.entries[1].serverName, "LAN-2") == 0);
}

int main() {
    testHandshakeRoundtrip();
    testHeartbeatRoundtrip();
    testServerListRoundtrip();
    std::cout << "All protocol packet tests passed.\n";
    return 0;
}
