#pragma once
// Network types and constants
#include <winsock2.h>
#include <ws2tcpip.h>
#include "math.h"

#pragma comment(lib, "ws2_32.lib")

const int NET_PORT = 27015;
const int MAX_PLAYERS = 4;
const int PACKET_SIZE = 512;

// Packet types
enum PacketType {
    PKT_PING = 1,
    PKT_PONG,
    PKT_JOIN,
    PKT_WELCOME,
    PKT_LEAVE,
    PKT_PLAYER_STATE,    // Position, yaw, pitch, flashlight
    PKT_ENTITY_STATE,    // Monster position/state
    PKT_ENTITY_SPAWN,    // New monster spawned
    PKT_ENTITY_REMOVE,   // Monster removed
    PKT_WORLD_SEED,
    PKT_CHAT,
    PKT_SCARE,
    PKT_GAME_START,      // Host sends to start game for all
    PKT_TELEPORT,        // Request teleport to another player
    PKT_NOTE_SPAWN,      // Note spawned
    PKT_NOTE_COLLECT,    // Note collected
    PKT_RESHUFFLE        // Map changed
};

// Network player data
struct NetPlayer {
    Vec3 pos;
    float yaw, pitch;
    int id;
    bool active;
    bool isHost;
    bool hasValidPos;    // True when player has sent at least one position update
    bool flashlightOn;   // Is flashlight on
    char name[32];
    float lastUpdate;
    unsigned long addr;
    unsigned short port;
};