#pragma once
// Network manager core - types and basic methods
#include "net_types.h"
#include "net_sync_codec.h"
#include "player_name.h"
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
    bool welcomeReceived;
    int myId;
    NetPlayer players[MAX_PLAYERS];
    unsigned int worldSeed;
    Vec3 spawnPos;
    char hostIP[64];
    unsigned short hostPort;
    char localPlayerName[PLAYER_NAME_BUF_LEN];
    
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
    int inventoryPlush[MAX_PLAYERS];
    bool inventorySyncReceived;
    
    int roamEventType;
    int roamEventA;
    int roamEventB;
    float roamEventDuration;
    bool roamEventReceived;
    
    NetInteractRequest interactRequests[16];
    int interactRequestCount;

    // Coop ping marker (simple broadcast marker)
    bool pingMarkReceived;
    int pingMarkFrom;
    Vec3 pingMarkPos;
    float pingMarkTtl;

    inline void updatePingMarkTtl(float dt){
        if(pingMarkTtl <= 0.0f) return;
        pingMarkTtl -= dt;
        if(pingMarkTtl <= 0.0f){
            pingMarkTtl = 0.0f;
            pingMarkReceived = false;
        }
    }
    
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
        welcomeReceived = false;
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
        pingMarkReceived = false;
        pingMarkFrom = -1;
        pingMarkPos = Vec3(0,0,0);
        pingMarkTtl = 0.0f;
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
            inventoryPlush[i] = 0;
            inventoryBattery[i] = 0;
        }
        for (int i = 0; i < MAX_SYNC_ENTITIES; i++) entitySnapshot[i].active = false;
        for (int i = 0; i < MAX_SYNC_ITEMS; i++) itemSnapshot[i].active = false;
        for (int i = 0; i < 16; i++) interactRequests[i].valid = false;
        strcpy(hostIP, "192.168.0.1");
        hostPort = NET_PORT;
        sanitizePlayerName("Player", localPlayerName);
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
        welcomeReceived = true;
        lastPacketRecvTime = (float)glfwGetTime();
        myId = 0;
        worldSeed = seed;
        players[0].active = true;
        players[0].isHost = true;
        players[0].id = 0;
        players[0].hasValidPos = false;
        sanitizePlayerName("Host", players[0].name);
        return true;
    }
    
    bool joinGame(const char* ip, const char* nickname = nullptr, unsigned short port = NET_PORT) {
        strcpy(hostIP, ip);
        hostPort = port;
        if (nickname) sanitizePlayerName(nickname, localPlayerName);
        sockaddr_in local;
        local.sin_family = AF_INET;
        local.sin_addr.s_addr = INADDR_ANY;
        local.sin_port = 0;
        bind(sock, (sockaddr*)&local, sizeof(local));
        sendJoinRequest(localPlayerName);
        connected = true;
        welcomeReceived = false;
        lastPacketRecvTime = (float)glfwGetTime();
        return true;
    }
    
    void sendJoinRequest(const char* nickname) {
        char cleanName[PLAYER_NAME_BUF_LEN];
        sanitizePlayerName(nickname, cleanName);
        char buf[PACKET_SIZE] = {};
        buf[0] = PKT_JOIN;
        memcpy(buf + 1, cleanName, PLAYER_NAME_BUF_LEN);
        sockaddr_in dest;
        dest.sin_family = AF_INET;
        inet_pton(AF_INET, hostIP, &dest.sin_addr);
        dest.sin_port = htons(hostPort);
        sendto(sock, buf, 64, 0, (sockaddr*)&dest, sizeof(dest));
    }

    void setLocalPlayerName(const char* nickname) {
        sanitizePlayerName(nickname, localPlayerName);
        sanitizePlayerName(localPlayerName, players[myId].name);
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
            dest.sin_port = htons(hostPort);
            sendto(sock, buf, len, 0, (sockaddr*)&dest, sizeof(dest));
            packetsSent++;
            bytesSent += len;
        }
    }
    
    int getPlayerCount() {
        int c = 0;
        for (int i = 0; i < MAX_PLAYERS; i++) if (players[i].active) c++;
        return c;
    }

    int connectionQuality(float nowTime) const {
        if (!connected) return 0; // offline
        if (isHost) return 4;     // excellent on host local loop
        float since = nowTime - lastPacketRecvTime;
        if (since > 4.0f) return 0;
        if (rttMs <= 0.0f) return 1;
        if (rttMs < 75.0f && since < 1.0f) return 4;
        if (rttMs < 130.0f && since < 1.4f) return 3;
        if (rttMs < 210.0f && since < 2.0f) return 2;
        return 1;
    }

    const char* connectionQualityLabel(float nowTime) const {
        int q = connectionQuality(nowTime);
        if (q >= 4) return "EXCELLENT";
        if (q == 3) return "GOOD";
        if (q == 2) return "FAIR";
        if (q == 1) return "POOR";
        return "OFFLINE";
    }

    bool connectionUnstable(float nowTime) const {
        int q = connectionQuality(nowTime);
        return q <= 1;
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
    
    bool clientTimedOut(float nowTime) const {
        if (!connected || isHost) return false;
        return (nowTime - lastPacketRecvTime) > 6.0f;
    }
    
    void shutdown() {
        if (connected) sendLeave();
        if (sock != INVALID_SOCKET) closesocket(sock);
        sock = INVALID_SOCKET;
        WSACleanup();
        connected = false;
        gameStarted = false;
        welcomeReceived = false;
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
            inventoryPlush[i] = 0;
        }
        for (int i = 0; i < MAX_SYNC_ENTITIES; i++) entitySnapshot[i].active = false;
        for (int i = 0; i < MAX_SYNC_ITEMS; i++) itemSnapshot[i].active = false;
        for (int i = 0; i < 16; i++) interactRequests[i].valid = false;
    }
    
    // Declared here, defined in net_packets.h
    void sendPlayerNamePacketTo(int playerId, const sockaddr_in* target);
    void broadcastPlayerName(int playerId);
    void sendPlayerState(Vec3 pos, float yaw, float pitch, bool flashlight);
    void sendEntityState(int entityId, Vec3 pos, int state);
    void sendEntitySpawn(int entityId, int type, Vec3 pos);
    void sendEntityRemove(int entityId);
    void sendNoteSpawn(int noteId, Vec3 pos);
    void sendNoteCollect(int noteId);
    void sendReshuffle(int chunkX, int chunkZ, unsigned int seed);
    void sendScare(int sourcePlayerId);
    void sendEntitySnapshot(const NetEntitySnapshotEntry* entries, int count);
    void sendObjectiveState(bool sw0, bool sw1, bool doorOpen);
    void sendItemSnapshot(const NetWorldItemSnapshotEntry* entries, int count);
    void sendInventorySync();
    void sendInteractRequest(int requestType, int targetId);
    void sendRoamEvent(int eventType, int a, int b, float duration);
    void sendPingMark(const Vec3& pos);
    void sendGameStart(Vec3 spawn);
    void sendPing(float nowTime);
    void sendLeave();
    void handleLeave(char* buf, int len);
    void pruneStalePlayers(float nowTime);
    void update();
    void handlePacket(char* buf, int len, sockaddr_in& from);
    void handleJoin(char* buf, sockaddr_in& from);
    void handlePing(char* buf, int len, sockaddr_in& from);
    void handlePong(char* buf, int len);
    void handleWelcome(char* buf);
    void handlePlayerName(char* buf, int len);
    void handlePlayerState(char* buf);
    void handleGameStart(char* buf);
    void handleReshuffle(char* buf, int len);
    void handleScare(char* buf, int len);
    void handleNoteCollect(char* buf);
    void handleEntitySnapshot(char* buf, int len);
    void handleObjectiveState(char* buf, int len);
    void handleItemSnapshot(char* buf, int len);
    void handleInventorySync(char* buf, int len);
    void handleInteractRequest(char* buf, int len);
    void handleRoamEvent(char* buf, int len);
    void handlePingMark(char* buf, int len);
};
