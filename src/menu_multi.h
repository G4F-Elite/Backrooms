#pragma once
// Multiplayer menu UI - requires menu.h and net.h to be included first
// Include order: glad.h -> menu.h -> net.h -> menu_multi.h
#include "lan_discovery.h"
#include "player_name.h"
#include "dedicated_client.h"

// Multiplayer state
enum MultiState {
    MULTI_NONE = 0,
    MULTI_HOST_LOBBY,
    MULTI_JOIN_MENU,
    MULTI_CONNECTING,
    MULTI_IN_GAME
};

inline MultiState multiState = MULTI_NONE;
inline int multiMenuSel = 0;
inline char multiJoinIP[64] = "192.168.0.1";
inline char multiJoinPort[8] = "27015";
inline int multiInputField = 0;  // 0 = IP, 1 = Port
inline bool multiIPManualEdit = false;
inline bool multiMasterManualEdit = false;
inline bool multiEditingNickname = false;
inline char multiNickname[PLAYER_NAME_BUF_LEN] = "Player";
inline int multiNetworkMode = 0; // 0 LAN, 1 DEDICATED
inline char multiMasterIP[64] = "127.0.0.1";
inline char multiMasterPort[8] = "27017";

// Draw multiplayer main menu
inline void drawMultiMenuScreen(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawTextCentered("MULTIPLAYER", 0.0f, 0.45f, 3.0f, 0.9f, 0.85f, 0.4f, 0.9f);
    
    const char* opts[] = {"HOST GAME", "JOIN GAME", "BACK"};
    for (int i = 0; i < 3; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = 0.1f - i * 0.12f;
        float baseX = -measureTextWidthNdc(opts[i], 2.0f) * 0.5f;
        if (menuSel == i) drawText(">", baseX - 0.08f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
        drawText(opts[i], baseX, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
    }
    
    const bool lanTab = (multiNetworkMode == 0);
    drawTextCentered(lanTab ? "[LAN]   SERVERS" : " LAN   [SERVERS]", 0.0f, -0.30f, 1.55f, 0.65f, 0.75f, 0.5f, 0.88f);
    drawTextCentered(lanTab ? "LAN MODE: USE RADMIN OR HAMACHI" : "SERVERS MODE: CENTRALIZED DIRECTORY", 0.0f, -0.40f, 1.3f, 0.55f, 0.62f, 0.46f, 0.74f);
    drawTextCentered("PORT: 27015 UDP", 0.0f, -0.48f, 1.35f, 0.5f, 0.5f, 0.4f, 0.6f);
    char nickBuf[80];
    snprintf(nickBuf, 80, "NICKNAME: [%s%s]", multiNickname, multiEditingNickname ? "_" : "");
    drawTextCentered(nickBuf, 0.0f, -0.58f, 1.4f, 0.75f, 0.8f, 0.55f, 0.85f);
    drawTextCentered("TAB: EDIT NICKNAME   LEFT/RIGHT: SWITCH TAB", 0.0f, -0.68f, 1.1f, 0.55f, 0.6f, 0.45f, 0.7f);
    lanDiscovery.ensureLocalIP();
    char ipLine[96];
    snprintf(ipLine, 96, "LOCAL IP: %s", lanDiscovery.localIP);
    drawTextCentered(ipLine, 0.0f, -0.78f, 1.2f, 0.55f, 0.65f, 0.45f, 0.75f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw join game IP entry with separate IP and Port fields
inline void drawJoinMenuScreen(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    bool dedicated = (multiNetworkMode == 1);
    drawTextCentered("JOIN GAME", 0.0f, 0.55f, 3.0f, 0.9f, 0.85f, 0.4f, 0.9f);
    drawTextCentered(dedicated ? " LAN   [SERVERS]" : "[LAN]   SERVERS", 0.0f, 0.43f, 1.45f, 0.68f, 0.78f, 0.52f, 0.90f);

    // IP Field (LAN target IP or Dedicated master IP)
    float ipSel = (multiInputField == 0) ? 1.0f : 0.5f;
    drawText(dedicated ? "MASTER IP:" : "IP ADDRESS:", -0.45f, 0.25f, 1.8f, 0.6f*ipSel, 0.6f*ipSel, 0.5f*ipSel, 0.8f);
    char ipBuf[48];
    const char* shownIP = dedicated ? multiMasterIP : multiJoinIP;
    if (multiInputField == 0) {
        snprintf(ipBuf, 48, "[%s_]", shownIP);
    } else {
        snprintf(ipBuf, 48, "[%s]", shownIP);
    }
    drawText(ipBuf, -0.45f, 0.12f, 2.0f, 0.9f*ipSel, 0.9f*ipSel, 0.6f*ipSel, 1.0f);
    
    // Port Field (LAN target port or Dedicated master port)
    float portSel = (multiInputField == 1) ? 1.0f : 0.5f;
    drawText(dedicated ? "MASTER PORT:" : "PORT:", 0.15f, 0.25f, 1.8f, 0.6f*portSel, 0.6f*portSel, 0.5f*portSel, 0.8f);
    char portBuf[24];
    const char* shownPort = dedicated ? multiMasterPort : multiJoinPort;
    if (multiInputField == 1) {
        snprintf(portBuf, 24, "[%s_]", shownPort);
    } else {
        snprintf(portBuf, 24, "[%s]", shownPort);
    }
    drawText(portBuf, 0.15f, 0.12f, 2.0f, 0.9f*portSel, 0.9f*portSel, 0.6f*portSel, 1.0f);
    
    // Menu options
    const char* opts[] = {"CONNECT", "BACK"};
    for (int i = 0; i < 2; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = -0.1f - i * 0.12f;
        if (menuSel == i) drawText(">", -0.20f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
        drawText(opts[i], -0.13f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
    }
    
    drawText("TAB TO SWITCH FIELDS    0-9 AND . FOR INPUT", -0.58f, -0.4f, 1.3f, 0.5f, 0.5f, 0.4f, 0.6f);
    drawText(dedicated ? "BACKSPACE TO DELETE     ENTER/G CONNECT" : "BACKSPACE TO DELETE     ENTER TO CONNECT", -0.58f, -0.5f, 1.3f, 0.5f, 0.5f, 0.4f, 0.6f);
    drawText("LEFT/RIGHT OR T: SWITCH LAN/SERVERS TAB", -0.58f, -0.57f, 1.1f, 0.55f, 0.62f, 0.46f, 0.72f);
    if (multiNetworkMode == 0) drawText("AUTO LAN SCAN: R REFRESH, F NEXT ROOM", -0.58f, -0.64f, 1.2f, 0.55f, 0.65f, 0.45f, 0.7f);
    else drawText("MASTER SCAN: R REFRESH, F NEXT SERVER", -0.58f, -0.64f, 1.2f, 0.55f, 0.65f, 0.45f, 0.7f);
    
    char roomHead[64];
    snprintf(roomHead, 64, multiNetworkMode == 0 ? "ROOMS FOUND: %d" : "SERVERS FOUND: %d", multiNetworkMode == 0 ? lanDiscovery.roomCount : dedicatedDirectory.roomCount);
    drawText(roomHead, -0.58f, -0.76f, 1.2f, 0.7f, 0.8f, 0.55f, 0.75f);
    if (!dedicated) {
        for (int i = 0; i < lanDiscovery.roomCount && i < 3; i++) {
            const LanRoomInfo& room = lanDiscovery.rooms[i];
            char roomLine[128];
            snprintf(roomLine, 128, "%s %d/%d %s", room.ip, (int)room.playerCount, (int)room.maxPlayers, room.gameStarted ? "IN GAME" : "LOBBY");
            float alpha = (i == lanDiscovery.selectedRoom) ? 0.95f : 0.65f;
            drawText(roomLine, -0.58f, -0.84f - i * 0.07f, 1.1f, 0.75f, 0.85f, 0.6f, alpha);
        }
    } else {
        for (int i = 0; i < dedicatedDirectory.roomCount && i < 3; i++) {
            const DedicatedRoomInfo& room = dedicatedDirectory.rooms[i];
            char roomLine[128];
            snprintf(roomLine, 128, "%s:%hu %d/%d %s", room.ip, room.gamePort, (int)room.playerCount, (int)room.maxPlayers, room.inGame ? "IN GAME" : "LOBBY");
            float alpha = (i == dedicatedDirectory.selectedRoom) ? 0.95f : 0.65f;
            drawText(roomLine, -0.58f, -0.84f - i * 0.07f, 1.1f, 0.75f, 0.85f, 0.6f, alpha);
        }
        const DedicatedRoomInfo* room = dedicatedDirectory.getSelectedRoom();
        if (room) {
            char targetLine[128];
            snprintf(targetLine, 128, "TARGET: %s:%hu", room->ip, room->gamePort);
            drawText(targetLine, -0.58f, -1.06f, 1.1f, 0.72f, 0.85f, 0.62f, 0.82f);
        }
    }
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw host lobby
inline void drawHostLobbyScreen(float tm, int playerCount) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawTextCentered("HOST LOBBY", 0.0f, 0.45f, 3.0f, 0.9f, 0.85f, 0.4f, 0.9f);
    drawTextCentered("WAITING FOR PLAYERS...", 0.0f, 0.2f, 1.8f, 0.6f, 0.7f, 0.5f, 0.7f);
    
    char buf[48];
    snprintf(buf, 48, "PLAYERS CONNECTED: %d/%d", playerCount, MAX_PLAYERS);
    drawTextCentered(buf, 0.0f, 0.05f, 1.4f, 0.5f, 0.6f, 0.4f, 0.8f);
    
    const char* opts[] = {"START GAME", "BACK"};
    for (int i = 0; i < 2; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = -0.15f - i * 0.12f;
        if (menuSel == i) drawText(">", -0.22f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
        drawText(opts[i], -0.15f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
    }
    
    float listY = -0.42f;
    drawText("PLAYERS IN LOBBY:", -0.42f, listY, 1.3f, 0.65f, 0.75f, 0.55f, 0.8f);
    listY -= 0.08f;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        char playerLine[80];
        const bool active = netMgr.players[i].active;
        const char* name = active ? netMgr.players[i].name : "(empty)";
        if (active && i == 0) snprintf(playerLine, 80, "%d. %s [HOST]", i + 1, name);
        else snprintf(playerLine, 80, "%d. %s", i + 1, name);
        drawText(playerLine, -0.42f, listY, 1.1f, active ? 0.7f : 0.45f, active ? 0.8f : 0.45f, active ? 0.55f : 0.4f, active ? 0.85f : 0.6f);
        listY -= 0.07f;
    }

    drawText("SHARE YOUR RADMIN/HAMACHI IP", -0.42f, -0.75f, 1.3f, 0.5f, 0.5f, 0.4f, 0.6f);
    lanDiscovery.ensureLocalIP();
    char ipLine[96];
    snprintf(ipLine, 96, "YOUR LAN IP: %s", lanDiscovery.localIP);
    drawText(ipLine, -0.38f, -0.84f, 1.2f, 0.6f, 0.75f, 0.5f, 0.8f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw connecting/waiting screen
inline void drawWaitingScreen(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    if(netMgr.welcomeReceived){
        drawTextCentered("CONNECTED TO HOST", 0.0f, 0.3f, 2.5f, 0.3f, 0.8f, 0.3f, 0.9f);
        drawTextCentered("WAITING FOR HOST TO START...", 0.0f, 0.1f, 2.0f, 0.9f, 0.85f, 0.6f, 0.7f+0.2f*sinf(tm*2.0f));
    }else{
        drawTextCentered("CONNECTING TO HOST...", 0.0f, 0.3f, 2.5f, 0.85f, 0.75f, 0.45f, 0.9f);
        drawTextCentered("NO HANDSHAKE YET", 0.0f, 0.1f, 1.8f, 0.8f, 0.65f, 0.45f, 0.85f);
    }
    char pc[48];
    snprintf(pc, 48, "LOBBY: %d/%d PLAYERS", netMgr.getPlayerCount(), MAX_PLAYERS);
    drawTextCentered(pc, 0.0f, -0.05f, 1.4f, 0.65f, 0.75f, 0.55f, 0.8f);
    if(netMgr.connected){
        char netLine[72];
        std::snprintf(netLine, 72, "NET: %s  PING: %.0fms", netMgr.connectionQualityLabel((float)glfwGetTime()), netMgr.rttMs);
        drawTextCentered(netLine, 0.0f, -0.12f, 1.2f, 0.68f, 0.78f, 0.58f, 0.82f);
        if(netMgr.connectionUnstable((float)glfwGetTime())){
            drawTextCentered("CONNECTION UNSTABLE. AUTO-RECOVERY ACTIVE.", 0.0f, -0.20f, 1.12f, 0.9f, 0.62f, 0.42f, 0.9f);
        }
    }
    float py = -0.30f;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!netMgr.players[i].active) continue;
        char line[72];
        const char* nm = netMgr.players[i].name[0] ? netMgr.players[i].name : "Player";
        snprintf(line, 72, "%d. %s%s", i + 1, nm, i == 0 ? " [HOST]" : "");
        drawText(line, -0.3f, py, 1.1f, 0.72f, 0.82f, 0.62f, 0.78f);
        py -= 0.07f;
    }
    
    drawTextCentered("PRESS ESC TO DISCONNECT", 0.0f, -0.7f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw multiplayer pause menu with teleport option
inline void drawMultiPause(int playerCount) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawFullscreenOverlay(0.02f,0.02f,0.03f,0.72f);
    
    drawTextCentered("PAUSED", 0.0f, 0.35f, 3.0f, 0.9f, 0.85f, 0.4f);
    
    char buf[32];
    snprintf(buf, 32, "PLAYERS: %d", playerCount);
    drawTextCentered(buf, 0.0f, 0.2f, 1.5f, 0.5f, 0.7f, 0.5f, 0.7f);
    
    const char* opts[] = {"RESUME", "TELEPORT TO PLAYER", "SETTINGS", "DISCONNECT", "QUIT"};
    for (int i = 0; i < 5; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = 0.0f - i * 0.1f;
        if (menuSel == i) drawText(">", -0.35f, y, 1.8f, 0.9f * s, 0.85f * s, 0.4f * s);
        drawText(opts[i], -0.28f, y, 1.8f, 0.9f * s, 0.85f * s, 0.4f * s);
    }
    
    drawTextCentered("ESC - RESUME", 0.0f, -0.65f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw in-game multiplayer HUD
inline void drawMultiHUD(int playerCount, bool isHost) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    char buf[32];
    snprintf(buf, 32, "PLAYERS: %d", playerCount);
    drawText(buf, 0.65f, 0.85f, 1.2f, 0.6f, 0.8f, 0.5f, 0.7f);
    
    if (isHost) {
        drawText("HOST", 0.75f, 0.78f, 1.0f, 0.2f, 0.7f, 0.3f, 0.6f);
    }
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Handle IP input for join menu
inline void handleIPInput(int key) {
    int len = (int)strlen(multiJoinIP);
    if (key >= '0' && key <= '9' && len < 15) {
        multiJoinIP[len] = (char)key;
        multiJoinIP[len + 1] = 0;
    } else if (key == '.' && len < 15 && len > 0 && multiJoinIP[len-1] != '.') {
        multiJoinIP[len] = '.';
        multiJoinIP[len + 1] = 0;
    } else if ((key == 8 || key == 127) && len > 0) { // Backspace
        multiJoinIP[len - 1] = 0;
    }
}
