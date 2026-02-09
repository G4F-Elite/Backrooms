#pragma once
#include "net_core.h"

inline void NetworkManager::sendPlayerNamePacketTo(int playerId, const sockaddr_in* target) {
    if (!isHost || !connected || !target) return;
    if (playerId < 0 || playerId >= MAX_PLAYERS || !players[playerId].active) return;
    char buf[PACKET_SIZE] = {};
    buf[0] = PKT_PLAYER_NAME;
    buf[1] = (char)playerId;
    memcpy(buf + 2, players[playerId].name, PLAYER_NAME_BUF_LEN);
    sendto(sock, buf, 2 + PLAYER_NAME_BUF_LEN, 0, (sockaddr*)target, sizeof(*target));
    packetsSent++;
    bytesSent += 2 + PLAYER_NAME_BUF_LEN;
}

inline void NetworkManager::broadcastPlayerName(int playerId) {
    if (!isHost || !connected) return;
    if (playerId < 0 || playerId >= MAX_PLAYERS || !players[playerId].active) return;
    char buf[PACKET_SIZE] = {};
    buf[0] = PKT_PLAYER_NAME;
    buf[1] = (char)playerId;
    memcpy(buf + 2, players[playerId].name, PLAYER_NAME_BUF_LEN);
    broadcast(buf, 2 + PLAYER_NAME_BUF_LEN);
}

inline void NetworkManager::sendPlayerState(Vec3 pos, float yaw, float pitch, bool flashlight) {
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

inline void NetworkManager::sendEntityState(int entityId, Vec3 pos, int state) {
    if (!isHost || !connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_ENTITY_STATE;
    buf[1] = (char)entityId;
    memcpy(buf + 2, &pos, sizeof(Vec3));
    buf[14] = (char)state;
    broadcast(buf, 32);
}

inline void NetworkManager::sendEntitySpawn(int entityId, int type, Vec3 pos) {
    if (!isHost || !connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_ENTITY_SPAWN;
    buf[1] = (char)entityId;
    buf[2] = (char)type;
    memcpy(buf + 3, &pos, sizeof(Vec3));
    broadcast(buf, 32);
}

inline void NetworkManager::sendEntityRemove(int entityId) {
    if (!isHost || !connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_ENTITY_REMOVE;
    buf[1] = (char)entityId;
    broadcast(buf, 8);
}

inline void NetworkManager::sendNoteSpawn(int noteId, Vec3 pos) {
    if (!isHost || !connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_NOTE_SPAWN;
    buf[1] = (char)noteId;
    memcpy(buf + 2, &pos, sizeof(Vec3));
    broadcast(buf, 32);
}

inline void NetworkManager::sendNoteCollect(int noteId) {
    if (!connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_NOTE_COLLECT;
    buf[1] = (char)noteId;
    broadcast(buf, 8);
}

inline void NetworkManager::sendReshuffle(int chunkX, int chunkZ, unsigned int seed) {
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

inline void NetworkManager::sendScare(int sourcePlayerId) {
    if (!connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_SCARE;
    if (!encodeScarePayload(buf, PACKET_SIZE, sourcePlayerId)) return;
    broadcast(buf, SCARE_PACKET_LEN);
}

inline void NetworkManager::sendEntitySnapshot(const NetEntitySnapshotEntry* entries, int count) {
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

inline void NetworkManager::sendObjectiveState(bool sw0, bool sw1, bool doorOpen) {
    if (!isHost || !connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_OBJECTIVE_STATE;
    buf[1] = sw0 ? 1 : 0;
    buf[2] = sw1 ? 1 : 0;
    buf[3] = doorOpen ? 1 : 0;
    broadcast(buf, 8);
}

inline void NetworkManager::sendItemSnapshot(const NetWorldItemSnapshotEntry* entries, int count) {
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

inline void NetworkManager::sendInventorySync() {
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

inline void NetworkManager::sendInteractRequest(int requestType, int targetId) {
    if (!connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_INTERACT_REQ;
    buf[1] = (char)myId;
    buf[2] = (char)requestType;
    buf[3] = (char)targetId;
    broadcast(buf, 8);
}

inline void NetworkManager::sendRoamEvent(int eventType, int a, int b, float duration) {
    if (!isHost || !connected) return;
    char buf[PACKET_SIZE];
    buf[0] = PKT_ROAM_EVENT;
    buf[1] = (char)eventType;
    buf[2] = (char)a;
    buf[3] = (char)b;
    memcpy(buf + 4, &duration, 4);
    broadcast(buf, 12);
}

inline void NetworkManager::sendGameStart(Vec3 spawn) {
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

inline void NetworkManager::sendPing(float nowTime) {
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

inline void NetworkManager::update() {
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

inline void NetworkManager::handlePacket(char* buf, int len, sockaddr_in& from) {
    PacketType type = (PacketType)buf[0];
    if (type == PKT_JOIN && isHost) handleJoin(buf, from);
    else if (type == PKT_PING) handlePing(buf, len, from);
    else if (type == PKT_PONG) handlePong(buf, len);
    else if (type == PKT_WELCOME) handleWelcome(buf);
    else if (type == PKT_PLAYER_STATE) handlePlayerState(buf);
    else if (type == PKT_GAME_START) handleGameStart(buf);
    else if (type == PKT_RESHUFFLE) handleReshuffle(buf, len);
    else if (type == PKT_PLAYER_NAME) handlePlayerName(buf, len);
    else if (type == PKT_SCARE) handleScare(buf, len);
    else if (type == PKT_NOTE_COLLECT) handleNoteCollect(buf);
    else if (type == PKT_ENTITY_SNAPSHOT) handleEntitySnapshot(buf, len);
    else if (type == PKT_OBJECTIVE_STATE) handleObjectiveState(buf, len);
    else if (type == PKT_ITEM_SNAPSHOT) handleItemSnapshot(buf, len);
    else if (type == PKT_INVENTORY_SYNC) handleInventorySync(buf, len);
    else if (type == PKT_INTERACT_REQ) handleInteractRequest(buf, len);
    else if (type == PKT_ROAM_EVENT) handleRoamEvent(buf, len);
}

inline void NetworkManager::handleJoin(char* buf, sockaddr_in& from) {
    for (int i = 1; i < MAX_PLAYERS; i++) {
        if (players[i].active) continue;
        players[i].active = true;
        players[i].id = i;
        players[i].addr = from.sin_addr.s_addr;
        players[i].port = from.sin_port;
        players[i].hasValidPos = false;
        sanitizePlayerName(buf + 1, players[i].name);
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
        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (!players[p].active) continue;
            sendPlayerNamePacketTo(p, &from);
        }
        broadcastPlayerName(i);
        break;
    }
}

inline void NetworkManager::handlePing(char* buf, int len, sockaddr_in& from) {
    if (!isHost || len < 7) return;
    char resp[PACKET_SIZE];
    resp[0] = PKT_PONG;
    memcpy(resp + 1, buf + 1, 6);
    sendto(sock, resp, 8, 0, (sockaddr*)&from, sizeof(from));
    packetsSent++;
    bytesSent += 8;
}

inline void NetworkManager::handlePong(char* buf, int len) {
    if (len < 7 || isHost) return;
    float sentTime = 0.0f;
    memcpy(&sentTime, buf + 3, 4);
    float nowTime = (float)glfwGetTime();
    float ms = (nowTime - sentTime) * 1000.0f;
    if (ms < 0) ms = 0;
    rttMs = rttMs <= 0.0f ? ms : (rttMs * 0.8f + ms * 0.2f);
}

inline void NetworkManager::handleWelcome(char* buf) {
    myId = buf[1];
    memcpy(&worldSeed, buf + 2, 4);
    welcomeReceived = true;
    players[myId].active = true;
    players[myId].id = myId;
    players[myId].hasValidPos = false;
    sanitizePlayerName(localPlayerName, players[myId].name);
}

inline void NetworkManager::handlePlayerName(char* buf, int len) {
    if (len < 2 + PLAYER_NAME_BUF_LEN) return;
    int id = (unsigned char)buf[1];
    if (id < 0 || id >= MAX_PLAYERS) return;
    sanitizePlayerName(buf + 2, players[id].name);
    players[id].active = true;
}

inline void NetworkManager::handlePlayerState(char* buf) {
    int id = buf[1];
    if (id < 0 || id >= MAX_PLAYERS || id == myId) return;
    memcpy(&players[id].pos, buf + 2, sizeof(Vec3));
    memcpy(&players[id].yaw, buf + 14, 4);
    memcpy(&players[id].pitch, buf + 18, 4);
    players[id].flashlightOn = buf[22] != 0;
    players[id].active = true;
    players[id].hasValidPos = true;
    
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

inline void NetworkManager::handleGameStart(char* buf) {
    memcpy(&worldSeed, buf + 1, 4);
    memcpy(&spawnPos, buf + 5, sizeof(Vec3));
    gameStarted = true;
}

inline void NetworkManager::handleReshuffle(char* buf, int len) {
    ReshuffleSyncData data;
    if (!decodeReshufflePayload(buf, len, data)) return;
    reshuffleChunkX = data.chunkX;
    reshuffleChunkZ = data.chunkZ;
    reshuffleSeed = data.seed;
    memcpy(reshuffleCells, data.cells, RESHUFFLE_CELL_COUNT);
    reshuffleReceived = true;
}

inline void NetworkManager::handleScare(char* buf, int len) {
    int sourcePlayerId = -1;
    if (!decodeScarePayload(buf, len, sourcePlayerId)) return;
    
    if (isHost) {
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

inline void NetworkManager::handleNoteCollect(char* buf) {
    // Handled in story manager
}

inline void NetworkManager::handleEntitySnapshot(char* buf, int len) {
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

inline void NetworkManager::handleObjectiveState(char* buf, int len) {
    if (len < 4) return;
    objectiveSwitches[0] = buf[1] != 0;
    objectiveSwitches[1] = buf[2] != 0;
    objectiveDoorOpen = buf[3] != 0;
    objectiveStateReceived = true;
}

inline void NetworkManager::handleItemSnapshot(char* buf, int len) {
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

inline void NetworkManager::handleInventorySync(char* buf, int len) {
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

inline void NetworkManager::handleInteractRequest(char* buf, int len) {
    if (!isHost || len < 4) return;
    if (interactRequestCount >= 16) return;
    int idx = interactRequestCount++;
    interactRequests[idx].playerId = (unsigned char)buf[1];
    interactRequests[idx].requestType = (unsigned char)buf[2];
    interactRequests[idx].targetId = (unsigned char)buf[3];
    interactRequests[idx].valid = true;
}

inline void NetworkManager::handleRoamEvent(char* buf, int len) {
    if (len < 8) return;
    roamEventType = (unsigned char)buf[1];
    roamEventA = (unsigned char)buf[2];
    roamEventB = (unsigned char)buf[3];
    memcpy(&roamEventDuration, buf + 4, 4);
    roamEventReceived = true;
}

inline NetworkManager netMgr;
