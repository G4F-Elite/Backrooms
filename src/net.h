#pragma once
// Network manager - host/join/sync logic
#include "net_types.h"
#include "net_sync_codec.h"
#include <cstring>

struct NetEntitySnapshotEntry {
    int id;
    int type;
    Vec3 pos;
    float yaw;
    int state;
    bool active;
};

struct NetWorldItemSnapshotEntry {
    int id;
    int type;
    Vec3 pos;
    bool active;
};

struct NetInteractRequest {
    int playerId;
    int requestType;
    int targetId;
    bool valid;
};

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
    unsigned int reshuffleSeed;
    unsigned char reshuffleCells[RESHUFFLE_CELL_COUNT];
    
    // Extended multiplayer sync
    NetEntitySnapshotEntry entitySnapshot[MAX_SYNC_ENTITIES];
    int entitySnapshotCount;
    bool entitySnapshotReceived;
    
    bool objectiveSwitches[2];
    bool objectiveDoorOpen;
    bool objectiveStateReceived;
    
    NetWorldItemSnapshotEntry itemSnapshot[MAX_SYNC_ITEMS];
    int itemSnapshotCount;
    bool itemSnapshotReceived;
    
    int inventoryBattery[MAX_PLAYERS];
    int inventoryMedkit[MAX_PLAYERS];
    int inventoryBait[MAX_PLAYERS];
    bool inventorySyncReceived;
    
    int roamEventType;
    int roamEventA;
    int roamEventB;
    float roamEventDuration;
    bool roamEventReceived;
    
    NetInteractRequest interactRequests[16];
    int interactRequestCount;
    
    int packetsSent;
    int packetsRecv;
    int bytesSent;
    int bytesRecv;
    float rttMs;
    float lastPingTime;
    unsigned short pingSeq;
    float lastPacketRecvTime;
    
    NetworkManager() {
        sock = INVALID_SOCKET;
        isHost = false;
        connected = false;
        gameStarted = false;
        myId = 0;
        worldSeed = 0;
        spawnPos = Vec3(0, 1.7f, 0);
        reshuffleReceived = false;
        reshuffleSeed = 0;
        memset(reshuffleCells, 0, sizeof(reshuffleCells));
        entitySnapshotCount = 0;
        entitySnapshotReceived = false;
        objectiveSwitches[0] = objectiveSwitches[1] = false;
        objectiveDoorOpen = false;
        objectiveStateReceived = false;
        itemSnapshotCount = 0;
        itemSnapshotReceived = false;
        inventorySyncReceived = false;
        roamEventType = 0;
        roamEventA = roamEventB = 0;
        roamEventDuration = 0.0f;
        roamEventReceived = false;
        interactRequestCount = 0;
        packetsSent = packetsRecv = 0;
        bytesSent = bytesRecv = 0;
        rttMs = 0.0f;
        lastPingTime = 0.0f;
        pingSeq = 0;
        lastPacketRecvTime = 0.0f;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            players[i] = NetPlayer();
            players[i].active = false;
            players[i].hasValidPos = false;
            players[i].flashlightOn = false;
            inventoryBattery[i] = 0;
            inventoryMedkit[i] = 0;
            inventoryBait[i] = 0;
        }
        for (int i = 0; i < MAX_SYNC_ENTITIES; i++) entitySnapshot[i].active = false;
        for (int i = 0; i < MAX_SYNC_ITEMS; i++) itemSnapshot[i].active = false;
        for (int i = 0; i < 16; i++) interactRequests[i].valid = false;
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
        lastPacketRecvTime = (float)glfwGetTime();
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
        lastPacketRecvTime = (float)glfwGetTime();
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
        long long key = ((long long)chunkX << 32) | (chunkZ & 0xFFFFFFFF);
        auto it = chunks.find(key);
        if (it == chunks.end()) return;
        
        char buf[PACKET_SIZE];
        buf[0] = PKT_RESHUFFLE;
        ReshuffleSyncData data;
        data.chunkX = chunkX;
        data.chunkZ = chunkZ;
        data.seed = seed;
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                int idx = x * CHUNK_SIZE + z;
                data.cells[idx] = (unsigned char)it->second.cells[x][z];
            }
        }
        if (!encodeReshufflePayload(buf, PACKET_SIZE, data)) return;
        broadcast(buf, RESHUFFLE_PACKET_LEN);
    }
    
    void sendScare(int sourcePlayerId) {
        if (!connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_SCARE;
        if (!encodeScarePayload(buf, PACKET_SIZE, sourcePlayerId)) return;
        broadcast(buf, SCARE_PACKET_LEN);
    }
    
    void sendEntitySnapshot(const NetEntitySnapshotEntry* entries, int count) {
        if (!isHost || !connected || !entries) return;
        if (count < 0) return;
        if (count > MAX_SYNC_ENTITIES) count = MAX_SYNC_ENTITIES;
        char buf[PACKET_SIZE];
        buf[0] = PKT_ENTITY_SNAPSHOT;
        buf[1] = (char)count;
        int off = 2;
        for (int i = 0; i < count; i++) {
            buf[off++] = (char)entries[i].id;
            buf[off++] = (char)entries[i].type;
            memcpy(buf + off, &entries[i].pos, sizeof(Vec3)); off += (int)sizeof(Vec3);
            memcpy(buf + off, &entries[i].yaw, 4); off += 4;
            buf[off++] = (char)entries[i].state;
            buf[off++] = entries[i].active ? 1 : 0;
        }
        broadcast(buf, off);
    }
    
    void sendObjectiveState(bool sw0, bool sw1, bool doorOpen) {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_OBJECTIVE_STATE;
        buf[1] = sw0 ? 1 : 0;
        buf[2] = sw1 ? 1 : 0;
        buf[3] = doorOpen ? 1 : 0;
        broadcast(buf, 8);
    }
    
    void sendItemSnapshot(const NetWorldItemSnapshotEntry* entries, int count) {
        if (!isHost || !connected || !entries) return;
        if (count < 0) return;
        if (count > MAX_SYNC_ITEMS) count = MAX_SYNC_ITEMS;
        char buf[PACKET_SIZE];
        buf[0] = PKT_ITEM_SNAPSHOT;
        buf[1] = (char)count;
        int off = 2;
        for (int i = 0; i < count; i++) {
            buf[off++] = (char)entries[i].id;
            buf[off++] = (char)entries[i].type;
            memcpy(buf + off, &entries[i].pos, sizeof(Vec3)); off += (int)sizeof(Vec3);
            buf[off++] = entries[i].active ? 1 : 0;
            if (off + 20 >= PACKET_SIZE) break;
        }
        broadcast(buf, off);
    }
    
    void sendInventorySync() {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_INVENTORY_SYNC;
        int off = 1;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            buf[off++] = (char)i;
            buf[off++] = (char)inventoryBattery[i];
            buf[off++] = (char)inventoryMedkit[i];
            buf[off++] = (char)inventoryBait[i];
        }
        broadcast(buf, off);
    }
    
    void sendInteractRequest(int requestType, int targetId) {
        if (!connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_INTERACT_REQ;
        buf[1] = (char)myId;
        buf[2] = (char)requestType;
        buf[3] = (char)targetId;
        broadcast(buf, 8);
    }
    
    void sendRoamEvent(int eventType, int a, int b, float duration) {
        if (!isHost || !connected) return;
        char buf[PACKET_SIZE];
        buf[0] = PKT_ROAM_EVENT;
        buf[1] = (char)eventType;
        buf[2] = (char)a;
        buf[3] = (char)b;
        memcpy(buf + 4, &duration, 4);
        broadcast(buf, 12);
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
                packetsSent++;
                bytesSent += len;
            }
        } else {
            sockaddr_in dest;
            dest.sin_family = AF_INET;
            inet_pton(AF_INET, hostIP, &dest.sin_addr);
            dest.sin_port = htons(NET_PORT);
            sendto(sock, buf, len, 0, (sockaddr*)&dest, sizeof(dest));
            packetsSent++;
            bytesSent += len;
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
            packetsRecv++;
            bytesRecv += recv;
            lastPacketRecvTime = (float)glfwGetTime();
            handlePacket(buf, recv, from);
        }
    }
    
    void handlePacket(char* buf, int len, sockaddr_in& from) {
        PacketType type = (PacketType)buf[0];
        if (type == PKT_JOIN && isHost) handleJoin(buf, from);
        else if (type == PKT_PING) handlePing(buf, len, from);
        else if (type == PKT_PONG) handlePong(buf, len);
        else if (type == PKT_WELCOME) handleWelcome(buf);
        else if (type == PKT_PLAYER_STATE) handlePlayerState(buf);
        else if (type == PKT_GAME_START) handleGameStart(buf);
        else if (type == PKT_RESHUFFLE) handleReshuffle(buf, len);
        else if (type == PKT_SCARE) handleScare(buf, len);
        else if (type == PKT_NOTE_COLLECT) handleNoteCollect(buf);
        else if (type == PKT_ENTITY_SNAPSHOT) handleEntitySnapshot(buf, len);
        else if (type == PKT_OBJECTIVE_STATE) handleObjectiveState(buf, len);
        else if (type == PKT_ITEM_SNAPSHOT) handleItemSnapshot(buf, len);
        else if (type == PKT_INVENTORY_SYNC) handleInventorySync(buf, len);
        else if (type == PKT_INTERACT_REQ) handleInteractRequest(buf, len);
        else if (type == PKT_ROAM_EVENT) handleRoamEvent(buf, len);
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
            packetsSent++;
            bytesSent += 16;
            if (gameStarted) {
                char gs[PACKET_SIZE];
                gs[0] = PKT_GAME_START;
                memcpy(gs + 1, &worldSeed, 4);
                memcpy(gs + 5, &spawnPos, sizeof(Vec3));
                sendto(sock, gs, 32, 0, (sockaddr*)&from, sizeof(from));
                packetsSent++;
                bytesSent += 32;
            }
            break;
        }
    }
    
    void sendPing(float nowTime) {
        if (!connected || isHost) return;
        if (nowTime - lastPingTime < 1.0f) return;
        lastPingTime = nowTime;
        char buf[PACKET_SIZE];
        buf[0] = PKT_PING;
        memcpy(buf + 1, &pingSeq, 2);
        memcpy(buf + 3, &nowTime, 4);
        pingSeq++;
        broadcast(buf, 8);
    }
    
    bool clientTimedOut(float nowTime) const {
        if (!connected || isHost) return false;
        return (nowTime - lastPacketRecvTime) > 6.0f;
    }
    
    void handlePing(char* buf, int len, sockaddr_in& from) {
        if (!isHost || len < 7) return;
        char resp[PACKET_SIZE];
        resp[0] = PKT_PONG;
        memcpy(resp + 1, buf + 1, 6);
        sendto(sock, resp, 8, 0, (sockaddr*)&from, sizeof(from));
        packetsSent++;
        bytesSent += 8;
    }
    
    void handlePong(char* buf, int len) {
        if (len < 7 || isHost) return;
        float sentTime = 0.0f;
        memcpy(&sentTime, buf + 3, 4);
        float nowTime = (float)glfwGetTime();
        float ms = (nowTime - sentTime) * 1000.0f;
        if (ms < 0) ms = 0;
        rttMs = rttMs <= 0.0f ? ms : (rttMs * 0.8f + ms * 0.2f);
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
    
    void handleReshuffle(char* buf, int len) {
        ReshuffleSyncData data;
        if (!decodeReshufflePayload(buf, len, data)) return;
        reshuffleChunkX = data.chunkX;
        reshuffleChunkZ = data.chunkZ;
        reshuffleSeed = data.seed;
        memcpy(reshuffleCells, data.cells, RESHUFFLE_CELL_COUNT);
        // Signal to game_loop to reshuffle this chunk
        reshuffleReceived = true;
    }
    
    void handleScare(char* buf, int len) {
        int sourcePlayerId = -1;
        if (!decodeScarePayload(buf, len, sourcePlayerId)) return;
        
        if (isHost) {
            // Relay scare event to all clients (including sender).
            for (int i = 1; i < MAX_PLAYERS; i++) {
                if (!players[i].active) continue;
                sockaddr_in dest;
                dest.sin_family = AF_INET;
                dest.sin_addr.s_addr = players[i].addr;
                dest.sin_port = players[i].port;
                sendto(sock, buf, SCARE_PACKET_LEN, 0, (sockaddr*)&dest, sizeof(dest));
            }
        }
        
        if (sourcePlayerId >= 0 && sourcePlayerId < MAX_PLAYERS) {
            triggerScare();
        }
    }
    
    void handleNoteCollect(char* buf) {
        // Handled in story manager
    }
    
    void handleEntitySnapshot(char* buf, int len) {
        if (len < 2) return;
        int count = (unsigned char)buf[1];
        if (count > MAX_SYNC_ENTITIES) count = MAX_SYNC_ENTITIES;
        int off = 2;
        for (int i = 0; i < count; i++) {
            if (off + 20 > len) break;
            entitySnapshot[i].id = (unsigned char)buf[off++];
            entitySnapshot[i].type = (unsigned char)buf[off++];
            memcpy(&entitySnapshot[i].pos, buf + off, sizeof(Vec3)); off += (int)sizeof(Vec3);
            memcpy(&entitySnapshot[i].yaw, buf + off, 4); off += 4;
            entitySnapshot[i].state = (unsigned char)buf[off++];
            entitySnapshot[i].active = buf[off++] != 0;
        }
        entitySnapshotCount = count;
        entitySnapshotReceived = true;
    }
    
    void handleObjectiveState(char* buf, int len) {
        if (len < 4) return;
        objectiveSwitches[0] = buf[1] != 0;
        objectiveSwitches[1] = buf[2] != 0;
        objectiveDoorOpen = buf[3] != 0;
        objectiveStateReceived = true;
    }
    
    void handleItemSnapshot(char* buf, int len) {
        if (len < 2) return;
        int count = (unsigned char)buf[1];
        if (count > MAX_SYNC_ITEMS) count = MAX_SYNC_ITEMS;
        int off = 2;
        for (int i = 0; i < count; i++) {
            if (off + 15 > len) break;
            itemSnapshot[i].id = (unsigned char)buf[off++];
            itemSnapshot[i].type = (unsigned char)buf[off++];
            memcpy(&itemSnapshot[i].pos, buf + off, sizeof(Vec3)); off += (int)sizeof(Vec3);
            itemSnapshot[i].active = buf[off++] != 0;
        }
        itemSnapshotCount = count;
        itemSnapshotReceived = true;
    }
    
    void handleInventorySync(char* buf, int len) {
        if (len < 1 + MAX_PLAYERS * 4) return;
        int off = 1;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            int pid = (unsigned char)buf[off++];
            int bat = (unsigned char)buf[off++];
            int med = (unsigned char)buf[off++];
            int bait = (unsigned char)buf[off++];
            if (pid < 0 || pid >= MAX_PLAYERS) continue;
            inventoryBattery[pid] = bat;
            inventoryMedkit[pid] = med;
            inventoryBait[pid] = bait;
        }
        inventorySyncReceived = true;
    }
    
    void handleInteractRequest(char* buf, int len) {
        if (!isHost || len < 4) return;
        if (interactRequestCount >= 16) return;
        int idx = interactRequestCount++;
        interactRequests[idx].playerId = (unsigned char)buf[1];
        interactRequests[idx].requestType = (unsigned char)buf[2];
        interactRequests[idx].targetId = (unsigned char)buf[3];
        interactRequests[idx].valid = true;
    }
    
    void handleRoamEvent(char* buf, int len) {
        if (len < 8) return;
        roamEventType = (unsigned char)buf[1];
        roamEventA = (unsigned char)buf[2];
        roamEventB = (unsigned char)buf[3];
        memcpy(&roamEventDuration, buf + 4, 4);
        roamEventReceived = true;
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
        reshuffleSeed = 0;
        memset(reshuffleCells, 0, sizeof(reshuffleCells));
        entitySnapshotCount = 0;
        entitySnapshotReceived = false;
        itemSnapshotCount = 0;
        itemSnapshotReceived = false;
        objectiveSwitches[0] = objectiveSwitches[1] = false;
        objectiveDoorOpen = false;
        objectiveStateReceived = false;
        inventorySyncReceived = false;
        roamEventType = 0;
        roamEventA = roamEventB = 0;
        roamEventDuration = 0.0f;
        roamEventReceived = false;
        interactRequestCount = 0;
        packetsSent = packetsRecv = 0;
        bytesSent = bytesRecv = 0;
        rttMs = 0.0f;
        lastPingTime = 0.0f;
        pingSeq = 0;
        lastPacketRecvTime = 0.0f;
        for (int i = 0; i < MAX_PLAYERS; i++) {
            players[i] = NetPlayer();
            players[i].active = false;
            inventoryBattery[i] = 0;
            inventoryMedkit[i] = 0;
            inventoryBait[i] = 0;
        }
        for (int i = 0; i < MAX_SYNC_ENTITIES; i++) entitySnapshot[i].active = false;
        for (int i = 0; i < MAX_SYNC_ITEMS; i++) itemSnapshot[i].active = false;
        for (int i = 0; i < 16; i++) interactRequests[i].valid = false;
    }
};

inline NetworkManager netMgr;
