#pragma once
// Multiplayer screens: mode + callsign, join, host lobby, waiting room.
// Requires menu.h and net.h first. Input: input_multi.h (item order is shared through the
// enums below).

#include "lan_discovery.h"
#include "player_name.h"
#include "dedicated_client.h"
#include "ui_widgets.h"
#include "menu_screens.h"

enum MultiState {
    MULTI_NONE = 0,
    MULTI_HOST_LOBBY,
    MULTI_JOIN_MENU,
    MULTI_CONNECTING,
    MULTI_IN_GAME
};

inline MultiState multiState = MULTI_NONE;
inline char multiJoinIP[64] = "192.168.0.1";
inline char multiJoinPort[8] = "27015";
inline bool multiIPManualEdit = false;
inline bool multiMasterManualEdit = false;
inline char multiNickname[PLAYER_NAME_BUF_LEN] = "Player";
inline int multiNetworkMode = 0; // 0 LAN, 1 DEDICATED
inline char multiMasterIP[64] = "127.0.0.1";
inline char multiMasterPort[8] = "27015";
inline char multiConnectStatus[128] = "";
inline float multiWaitBeforeStart = 0.0f;

enum MultiMenuItem { MULTI_ITEM_NICK = 0, MULTI_ITEM_HOST, MULTI_ITEM_JOIN, MULTI_ITEM_COUNT };
enum JoinMenuItem { JOIN_ITEM_ADDRESS = 0, JOIN_ITEM_PORT, JOIN_ITEM_CONNECT, JOIN_ITEM_FIRST_ROOM };
enum HostMenuItem { HOST_ITEM_START = 0, HOST_ITEM_CLOSE, HOST_ITEM_COUNT };

inline constexpr int JOIN_ROOMS_SHOWN = 5;
inline constexpr float MULTI_PANEL_W = 600.0f;

inline int joinRoomsShown() {
    if (multiNetworkMode != 0) return 0;
    return lanDiscovery.roomCount < JOIN_ROOMS_SHOWN ? lanDiscovery.roomCount : JOIN_ROOMS_SHOWN;
}

inline int joinItemCount() {
    return JOIN_ITEM_FIRST_ROOM + joinRoomsShown();
}

inline void drawNetworkTabs(float l, float r, float y) {
    static const char* names[2] = {"LAN", "SERVERS"};
    uiTabs(l + 28.0f, r - 28.0f, y, 40.0f, names, 2, multiNetworkMode);
}

// Bottom strip of a panel: one line of status. `warn` paints it as a problem.
inline void drawPanelStrip(float l, float r, float b, const char* text, bool warn = false) {
    uiText(text, l + 28.0f, b + 18.0f, UI_SMALL_TEXT, warn ? UI_BAD : UI_MUTE);
}

// "LABEL   [ field ]" row. The field is item `idx`.
inline void drawFieldRow(int idx, const char* label, float l, float r, float y, const char* text) {
    uiText(label, l + 28.0f, y, UI_ROW_TEXT, menuSel == idx ? UI_ACCENT : UI_DIM);
    uiField(idx, l + 190.0f, r - 28.0f, y, 32.0f, text, menuSel == idx);
}

// The four seats of the squad: who is in, who hosts, which are still empty.
inline float drawSquadSlots(float l, float r, float y) {
    const float rowH = 30.0f;
    for (int i = 0; i < MAX_PLAYERS; i++) {
        float ry = y - rowH * (float)i;
        bool active = netMgr.players[i].active;
        char num[8];
        snprintf(num, sizeof(num), "%d", i + 1);
        uiText(num, l, ry, UI_ROW_TEXT, UI_MUTE);
        if (!active) continue;
        uiText(netMgr.players[i].name[0] ? netMgr.players[i].name : "Player", l + 30.0f, ry, UI_ROW_TEXT, UI_TEXT);
        if (i == 0) uiTextR("HOST", r, ry, UI_SMALL_TEXT, UI_DIM);
        else if (i == netMgr.myId) uiTextR("YOU", r, ry, UI_SMALL_TEXT, UI_DIM);
    }
    return y - rowH * (float)MAX_PLAYERS;
}

inline void beginMultiScreen(float tm, float& in) {
    uiBeginScreen();
    uiMenuBackdrop(tm, 0.42f);
    in = uiScreenIn(0.18f);
    ui.alpha = in;
}

// Mode, callsign, then host or join.
inline void drawMultiMenuScreen(float tm) {
    float in = 0.0f;
    beginMultiScreen(tm, in);
    float l = -MULTI_PANEL_W * 0.5f, r = MULTI_PANEL_W * 0.5f;
    float t = 170.0f - (1.0f - in) * 10.0f, b = t - 290.0f;
    uiPanel(l, b, r, t);
    float y = uiPanelHeader(l, r, t, "MULTIPLAYER") - 20.0f;
    drawNetworkTabs(l, r, y);
    y -= 48.0f;
    drawFieldRow(MULTI_ITEM_NICK, "CALLSIGN", l, r, y, multiNickname);
    y -= 48.0f;
    uiButton(MULTI_ITEM_HOST, "HOST A GAME", l + 28.0f, y - 17.0f, r - 28.0f, y + 17.0f, menuSel == MULTI_ITEM_HOST);
    y -= 36.0f;
    uiButton(MULTI_ITEM_JOIN, "JOIN A GAME", l + 28.0f, y - 17.0f, r - 28.0f, y + 17.0f, menuSel == MULTI_ITEM_JOIN);

    lanDiscovery.ensureLocalIP();
    char line[96];
    snprintf(line, sizeof(line), "YOUR IP  %s    PORT  %d UDP", lanDiscovery.localIP, (int)NET_PORT);
    if (multiConnectStatus[0]) drawPanelStrip(l, r, b, multiConnectStatus, true);
    else drawPanelStrip(l, r, b, line);

    ui.alpha = 1.0f;
    static const UiHint hints[] = {{"ESC", "BACK"}, {"TAB", "LAN / SERVERS"}};
    uiHintBar(hints, 2);
    uiEndScreen();
}

inline void drawLanRoomRow(int idx, const LanRoomInfo& room, float l, float r, float y) {
    float glow = uiRowBack(idx, l, y - 14.0f, r, y + 14.0f, menuSel == idx);
    UiCol c = uiMix(UI_TEXT, UI_ACCENT, glow);
    char players[16];
    snprintf(players, sizeof(players), "%d/%d", (int)room.playerCount, (int)room.maxPlayers);
    uiText(room.hostName[0] ? room.hostName : "Host", l + 14.0f, y, UI_ROW_TEXT, c);
    uiText(room.ip, l + 204.0f, y, UI_SMALL_TEXT, UI_MUTE);
    uiTextR(players, r - 100.0f, y, UI_ROW_TEXT, c);
    uiTextR(room.gameStarted ? "IN GAME" : "LOBBY", r, y, UI_SMALL_TEXT, room.gameStarted ? UI_BAD : UI_DIM);
}

// What is known about the dedicated server the fields point at.
inline void drawDedicatedInfo(float l, float y) {
    char line[128];
    float nowT = (float)glfwGetTime();
    snprintf(line, sizeof(line), "TARGET  %s:%s", multiMasterIP, multiMasterPort);
    uiText(line, l, y, UI_ROW_TEXT, UI_TEXT);
    if (dedicatedDirectory.hasRecentResponse(nowT)) {
        snprintf(line, sizeof(line), "DIRECTORY ONLINE, LAST ANSWER %.1fS AGO.", nowT - dedicatedDirectory.lastResponseAt);
    } else if (dedicatedDirectory.queriesSent > 0) {
        snprintf(line, sizeof(line), "DIRECTORY HAS NOT ANSWERED YET (%d REQUESTS).", dedicatedDirectory.queriesSent);
    } else {
        snprintf(line, sizeof(line), "DIRECT CONNECTION: NO SERVER DIRECTORY IS USED.");
    }
    uiText(line, l, y - 28.0f, UI_SMALL_TEXT, UI_DIM);
}

// Address + port, connect, and the rooms found on the local network.
inline void drawJoinMenuScreen(float tm) {
    float in = 0.0f;
    beginMultiScreen(tm, in);
    bool dedicated = multiNetworkMode == 1;
    float l = -MULTI_PANEL_W * 0.5f - 20.0f, r = MULTI_PANEL_W * 0.5f + 20.0f;
    float t = 236.0f - (1.0f - in) * 10.0f, b = t - 452.0f;
    uiPanel(l, b, r, t);
    float y = uiPanelHeader(l, r, t, "JOIN") - 20.0f;
    drawNetworkTabs(l, r, y);
    y -= 48.0f;
    drawFieldRow(JOIN_ITEM_ADDRESS, dedicated ? "SERVER IP" : "HOST IP", l, r, y, dedicated ? multiMasterIP : multiJoinIP);
    y -= 40.0f;
    drawFieldRow(JOIN_ITEM_PORT, "PORT", l, r, y, dedicated ? multiMasterPort : multiJoinPort);
    y -= 48.0f;
    uiButton(JOIN_ITEM_CONNECT, "CONNECT", l + 28.0f, y - 17.0f, r - 28.0f, y + 17.0f, menuSel == JOIN_ITEM_CONNECT);
    y -= 52.0f;

    if (dedicated) {
        uiSection(l + 28.0f, r - 28.0f, y, "DEDICATED SERVER");
        drawDedicatedInfo(l + 28.0f, y - 34.0f);
    } else {
        uiSection(l + 28.0f, r - 28.0f, y, "ROOMS");
        int shown = joinRoomsShown();
        for (int i = 0; i < shown; i++) {
            drawLanRoomRow(JOIN_ITEM_FIRST_ROOM + i, lanDiscovery.rooms[i], l + 14.0f, r - 28.0f, y - 30.0f - 30.0f * (float)i);
        }
        if (shown == 0) {
            uiSpinner(l + 37.0f, y - 32.0f, 7.0f, UI_DIM);
            uiText("SEARCHING", l + 56.0f, y - 32.0f, UI_SMALL_TEXT, UI_MUTE);
        }
    }
    if (multiConnectStatus[0]) drawPanelStrip(l, r, b, multiConnectStatus, true);

    ui.alpha = 1.0f;
    static const UiHint hints[] = {{"ESC", "BACK"}, {"TAB", "LAN / SERVERS"}};
    uiHintBar(hints, 2);
    uiEndScreen();
}

// Host: the seats fill up, then the host starts the shift.
inline void drawHostLobbyScreen(float tm, int playerCount) {
    float in = 0.0f;
    beginMultiScreen(tm, in);
    float l = -MULTI_PANEL_W * 0.5f, r = MULTI_PANEL_W * 0.5f;
    float t = 180.0f - (1.0f - in) * 10.0f, b = t - 330.0f;
    uiPanel(l, b, r, t);
    char note[32];
    snprintf(note, sizeof(note), "%d/%d", playerCount, MAX_PLAYERS);
    float y = uiPanelHeader(l, r, t, "LOBBY", note) - 26.0f;
    y = drawSquadSlots(l + 28.0f, r - 28.0f, y) - 18.0f;
    uiButton(HOST_ITEM_START, "START THE SHIFT", l + 28.0f, y - 17.0f, r - 28.0f, y + 17.0f, menuSel == HOST_ITEM_START);
    y -= 36.0f;
    uiButton(HOST_ITEM_CLOSE, "CLOSE LOBBY", l + 28.0f, y - 17.0f, r - 28.0f, y + 17.0f, menuSel == HOST_ITEM_CLOSE);

    lanDiscovery.ensureLocalIP();
    char line[96];
    snprintf(line, sizeof(line), "%s:%d", lanDiscovery.localIP, (int)NET_PORT);
    drawPanelStrip(l, r, b, line);

    ui.alpha = 1.0f;
    static const UiHint hints[] = {{"ESC", "CLOSE LOBBY"}};
    uiHintBar(hints, 1);
    uiEndScreen();
}

// Client: handshake, then the lobby until the host starts.
inline void drawWaitingScreen(float tm) {
    float in = 0.0f;
    beginMultiScreen(tm, in);
    float l = -MULTI_PANEL_W * 0.5f, r = MULTI_PANEL_W * 0.5f;
    float t = 150.0f - (1.0f - in) * 10.0f, b = t - 270.0f;
    uiPanel(l, b, r, t);
    bool inLobby = netMgr.welcomeReceived;
    char note[48] = "";
    float nowT = (float)glfwGetTime();
    if (netMgr.connected) snprintf(note, sizeof(note), "%s  %.0f MS", netMgr.connectionQualityLabel(nowT), netMgr.rttMs);
    float y = uiPanelHeader(l, r, t, inLobby ? "LOBBY" : "CONNECTING", note) - 26.0f;
    drawSquadSlots(l + 28.0f, r - 28.0f, y);
    uiSpinner(l + 37.0f, b + 54.0f, 7.0f, UI_DIM);
    uiText(inLobby ? "WAITING FOR THE HOST" : "NO ANSWER YET", l + 56.0f, b + 54.0f, UI_SMALL_TEXT, UI_MUTE);

    bool unstable = netMgr.connected && netMgr.connectionUnstable(nowT);
    if (unstable) drawPanelStrip(l, r, b, "CONNECTION UNSTABLE. RECOVERING ON ITS OWN.", true);
    else if (multiConnectStatus[0]) drawPanelStrip(l, r, b, multiConnectStatus);

    ui.alpha = 1.0f;
    static const UiHint hints[] = {{"ESC", "DISCONNECT"}};
    uiHintBar(hints, 1);
    uiEndScreen();
}
