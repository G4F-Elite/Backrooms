#pragma once
// Network manager - host/join/sync logic
#include "net_types.h"
#include <cstring>

class NetworkManager {
public:
    SOCKET sock;
    bool isHost;
    bool connected;
    bool gameStarted;
    int myId;
    NetPlayer players[MAX_PLAYERS];
    unsigned int worldSeed;
    Vec3 spawnPos;
    char hostIP[64];
    
    // Sync flags
    bool reshuffleReceived;
    int reshuffleChunkX, reshuffleChunkZ;
    
    NetworkManager() {
        sock = INVALID_SOCKET;
        isHost = false;
        connected = false;
        gameStarted = false;
        myId = 0;
        worldSeed = 0;
        spawnPos = Vec3(0, 1.7f, 0);
        reshuffleReceived = false;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            players[i] = NetPlayer();
            players[i].active = false;
            players[i].hasValidPos = false;
            players[i].flashlightOn = false;
        }
        strcpy(hostIP, "192.168.0.1");
    }
    
    bool init() {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (sock == INVALID_SOCKET) return false;
        u_long mode = 1;
        ioctlsocket(sock, FIONBIO, &mode);
        return true;
    }
    
    bool hostGame(unsigned int seed) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(NET_PORT);
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) return false;
        isHost = true;
        connected = true;
        myId = 0;
        worldSeed = seed;
        players[0].active = true;
        players[0].isHost = true;
        players[0].id = 0;
        players[0].hasValidPos = false;
        strcpy(players[0].name, "Host");
        return true;
    }
    
    bool joinGame(const char* ip) {
        strcpy(hostIP, ip);
        sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = 0;
        bind(sock, (sockaddr*)&local, sizeof(local));
        sendJoinRequest();
        connected = true;
        return true;
    }
    
    void sendJoinRequest() {
        char buf[PACKET_SIZE];
        buf[0] = PKT_JOIN;
        strcpy(buf + 1, "Player");
        sockaddr_in dest;
        dest.sin_family = AF_INET;
        inet_pton(AF_INET, hostIP, &dest.sin_addr);
        dest.sin_port = htons(NET_PORT);
        sendto(sock, buf, 64, 0, (sockaddr*)&dest, sizeof(dest));
    }
    
    void sendPlayerState(Vec3 pos, float yaw, float pitch, bool flashlight) {
        if (!connected) return;
        
        players[myId].pos = pos;
        players[myId].yaw = yaw;
        players[myId].pitch = pitch;
        players[myId].hasValidPos = true;
        players[myId].flashlightOn = flashlight;
        
        char buf[PACKET_SIZE];
        buf[0] = PKT_PLAYER_STATE;
        buf[1] = (char)myId;
        memcpy(buf + 2, &pos, sizeof(Vec3));
        memcpy(buf + 14, &yaw, 4);
        memcpy(buf + 18, &pitch, 4);
        buf[22] = flashlight ? 1 : 0;
        broadcast(buf, 32);
    }
    
    // Host sends entity state to all
    void sendEntityState(int entityId, Vec3 pos, int state) {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_ENTITY_STATE;
        buf[1] = (char)entityId;
        memcpy(buf + 2, &pos, sizeof(Vec3));
        buf[14] = (char)state;
        broadcast(buf, 32);
    }
    
    // Host sends entity spawn
    void sendEntitySpawn(int entityId, int type, Vec3 pos) {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_ENTITY_SPAWN;
        buf[1] = (char)entityId;
        buf[2] = (char)type;
        memcpy(buf + 3, &pos, sizeof(Vec3));
        broadcast(buf, 32);
    }
    
    // Host sends entity remove
    void sendEntityRemove(int entityId) {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_ENTITY_REMOVE;
        buf[1] = (char)entityId;
        broadcast(buf, 8);
    }
    
    // Note spawned
    void sendNoteSpawn(int noteId, Vec3 pos) {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_NOTE_SPAWN;
        buf[1] = (char)noteId;
        memcpy(buf + 2, &pos, sizeof(Vec3));
        broadcast(buf, 32);
    }
    
    // Note collected
    void sendNoteCollect(int noteId) {
        if (!connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_NOTE_COLLECT;
        buf[1] = (char)noteId;
        broadcast(buf, 8);
    }
    
    // Reshuffle happened (map changed)
    void sendReshuffle(int chunkX, int chunkZ, unsigned int seed) {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_RESHUFFLE;
        memcpy(buf + 1, &chunkX, 4);
        memcpy(buf + 5, &chunkZ, 4);
        memcpy(buf + 9, &seed, 4);
        broadcast(buf, 16);
    }
    
    void broadcast(char* buf, int len) {
        if (isHost) {
            for (int i = 1; i < MAX_PLAYERS; i++) {
                if (!players[i].active) continue;
                sockaddr_in dest;
                dest.sin_family = AF_INET;
                dest.sin_addr.s_addr = players[i].addr;
                dest.sin_port = players[i].port;
                sendto(sock, buf, len, 0, (sockaddr*)&dest, sizeof(dest));
            }
        } else {
            sockaddr_in dest;
            dest.sin_family = AF_INET;
            inet_pton(AF_INET, hostIP, &dest.sin_addr);
            dest.sin_port = htons(NET_PORT);
            sendto(sock, buf, len, 0, (sockaddr*)&dest, sizeof(dest));
        }
    }
    
    void update() {
        if (!connected) return;
        char buf[PACKET_SIZE];
        sockaddr_in from;
        int fromLen = sizeof(from);
        while (true) {
            int recv = recvfrom(sock, buf, PACKET_SIZE, 0, (sockaddr*)&from, &fromLen);
            if (recv <= 0) break;
            handlePacket(buf, recv, from);
        }
    }
    
    void handlePacket(char* buf, int len, sockaddr_in& from) {
        PacketType type = (PacketType)buf[0];
        if (type == PKT_JOIN && isHost) handleJoin(buf, from);
        else if (type == PKT_WELCOME) handleWelcome(buf);
        else if (type == PKT_PLAYER_STATE) handlePlayerState(buf);
        else if (type == PKT_GAME_START) handleGameStart(buf);
        else if (type == PKT_RESHUFFLE) handleReshuffle(buf);
        else if (type == PKT_NOTE_COLLECT) handleNoteCollect(buf);
        // Entities handled in entity manager
    }
    
    void sendGameStart(Vec3 spawn) {
        if (!isHost) return;
        spawnPos = spawn;
        players[0].pos = spawn;
        players[0].hasValidPos = true;
        
        char buf[PACKET_SIZE];
        buf[0] = PKT_GAME_START;
        memcpy(buf + 1, &worldSeed, 4);
        memcpy(buf + 5, &spawn, sizeof(Vec3));
        broadcast(buf, 32);
        gameStarted = true;
    }
    
    void handleGameStart(char* buf) {
        memcpy(&worldSeed, buf + 1, 4);
        memcpy(&spawnPos, buf + 5, sizeof(Vec3));
        gameStarted = true;
    }
    
    Vec3 getOtherPlayerPos() {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (i == myId || !players[i].active) continue;
            if (players[i].hasValidPos) return players[i].pos;
        }
        return spawnPos;
    }
    
    bool hasOtherPlayersWithPos() {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (i == myId || !players[i].active) continue;
            if (players[i].hasValidPos) return true;
        }
        return false;
    }
    
    void handleJoin(char* buf, sockaddr_in& from) {
        for (int i = 1; i < MAX_PLAYERS; i++) {
            if (players[i].active) continue;
            players[i].active = true;
            players[i].id = i;
            players[i].addr = from.sin_addr.s_addr;
            players[i].port = from.sin_port;
            players[i].hasValidPos = false;
            strcpy(players[i].name, buf + 1);
            char resp[PACKET_SIZE];
            resp[0] = PKT_WELCOME;
            resp[1] = (char)i;
            memcpy(resp + 2, &worldSeed, 4);
            sendto(sock, resp, 16, 0, (sockaddr*)&from, sizeof(from));
            break;
        }
    }
    
    void handleWelcome(char* buf) {
        myId = buf[1];
        memcpy(&worldSeed, buf + 2, 4);
        players[myId].active = true;
        players[myId].id = myId;
        players[myId].hasValidPos = false;
    }
    
    void handlePlayerState(char* buf) {
        int id = buf[1];
        if (id < 0 || id >= MAX_PLAYERS || id == myId) return;
        memcpy(&players[id].pos, buf + 2, sizeof(Vec3));
        memcpy(&players[id].yaw, buf + 14, 4);
        memcpy(&players[id].pitch, buf + 18, 4);
        players[id].flashlightOn = buf[22] != 0;
        players[id].active = true;
        players[id].hasValidPos = true;
        
        // Host relays to other clients
        if (isHost) {
            for (int i = 1; i < MAX_PLAYERS; i++) {
                if (!players[i].active || i == id) continue;
                sockaddr_in dest;
                dest.sin_family = AF_INET;
                dest.sin_addr.s_addr = players[i].addr;
                dest.sin_port = players[i].port;
                sendto(sock, buf, 32, 0, (sockaddr*)&dest, sizeof(dest));
            }
        }
    }
    
    void handleReshuffle(char* buf) {
        memcpy(&reshuffleChunkX, buf + 1, 4);
        memcpy(&reshuffleChunkZ, buf + 5, 4);
        unsigned int seed;
        memcpy(&seed, buf + 9, 4);
        // Signal to game_loop to reshuffle this chunk
        reshuffleReceived = true;
    }
    
    void handleNoteCollect(char* buf) {
        // Handled in story manager
    }
    
    int getPlayerCount() {
        int c = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) if (players[i].active) c++;
        return c;
    }
    
    void shutdown() {
        if (sock != INVALID_SOCKET) closesocket(sock);
        sock = INVALID_SOCKET;
        WSACleanup();
        connected = false;
        gameStarted = false;
        isHost = false;
        myId = 0;
        reshuffleReceived = false;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            players[i] = NetPlayer();
            players[i].active = false;
        }
    }
};

inline NetworkManager netMgr;