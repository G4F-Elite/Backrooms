#pragma once
// Multiplayer menu UI - requires menu.h and net.h to be included first
// Include order: glad.h -> menu.h -> net.h -> menu_multi.h

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

// Draw multiplayer main menu
inline void drawMultiMenuScreen(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawText("MULTIPLAYER", -0.32f, 0.45f, 3.0f, 0.9f, 0.85f, 0.4f, 0.9f);
    
    const char* opts[] = {"HOST GAME", "JOIN GAME", "BACK"};
    for (int i = 0; i < 3; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = 0.1f - i * 0.12f;
        if (menuSel == i) drawText(">", -0.25f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
        drawText(opts[i], -0.18f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
    }
    
    drawText("USE RADMIN VPN OR HAMACHI FOR LAN", -0.48f, -0.3f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    drawText("PORT: 27015 UDP", -0.22f, -0.42f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw join game IP entry with separate IP and Port fields
inline void drawJoinMenuScreen(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawText("JOIN GAME", -0.25f, 0.55f, 3.0f, 0.9f, 0.85f, 0.4f, 0.9f);
    
    // IP Field
    float ipSel = (multiInputField == 0) ? 1.0f : 0.5f;
    drawText("IP ADDRESS:", -0.45f, 0.25f, 1.8f, 0.6f*ipSel, 0.6f*ipSel, 0.5f*ipSel, 0.8f);
    char ipBuf[48];
    if (multiInputField == 0) {
        snprintf(ipBuf, 48, "[%s_]", multiJoinIP);
    } else {
        snprintf(ipBuf, 48, "[%s]", multiJoinIP);
    }
    drawText(ipBuf, -0.45f, 0.12f, 2.0f, 0.9f*ipSel, 0.9f*ipSel, 0.6f*ipSel, 1.0f);
    
    // Port Field
    float portSel = (multiInputField == 1) ? 1.0f : 0.5f;
    drawText("PORT:", 0.15f, 0.25f, 1.8f, 0.6f*portSel, 0.6f*portSel, 0.5f*portSel, 0.8f);
    char portBuf[24];
    if (multiInputField == 1) {
        snprintf(portBuf, 24, "[%s_]", multiJoinPort);
    } else {
        snprintf(portBuf, 24, "[%s]", multiJoinPort);
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
    drawText("BACKSPACE TO DELETE     ENTER TO CONNECT", -0.52f, -0.5f, 1.3f, 0.5f, 0.5f, 0.4f, 0.6f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw host lobby
inline void drawHostLobbyScreen(float tm, int playerCount) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawText("HOST LOBBY", -0.28f, 0.45f, 3.0f, 0.9f, 0.85f, 0.4f, 0.9f);
    drawText("WAITING FOR PLAYERS...", -0.32f, 0.2f, 1.8f, 0.6f, 0.7f, 0.5f, 0.7f);
    
    char buf[32];
    snprintf(buf, 32, "PLAYERS CONNECTED: %d", playerCount);
    drawText(buf, -0.28f, 0.05f, 1.5f, 0.5f, 0.6f, 0.4f, 0.8f);
    
    const char* opts[] = {"START GAME", "BACK"};
    for (int i = 0; i < 2; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = -0.15f - i * 0.12f;
        if (menuSel == i) drawText(">", -0.22f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
        drawText(opts[i], -0.15f, y, 2.0f, 0.9f * s, 0.85f * s, 0.4f * s);
    }
    
    drawText("SHARE YOUR RADMIN/HAMACHI IP", -0.42f, -0.5f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw connecting/waiting screen
inline void drawWaitingScreen(float tm) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawText("CONNECTED TO HOST", -0.35f, 0.3f, 2.5f, 0.3f, 0.8f, 0.3f, 0.9f);
    drawText("WAITING FOR HOST TO START...", -0.42f, 0.1f, 2.0f, 0.9f, 0.85f, 0.6f, 0.7f+0.2f*sinf(tm*2.0f));
    
    drawText("PRESS ESC TO DISCONNECT", -0.35f, -0.2f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Draw multiplayer pause menu with teleport option
inline void drawMultiPause(int playerCount) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    drawText("PAUSED", -0.2f, 0.35f, 3.0f, 0.9f, 0.85f, 0.4f);
    
    char buf[32];
    snprintf(buf, 32, "PLAYERS: %d", playerCount);
    drawText(buf, -0.15f, 0.2f, 1.5f, 0.5f, 0.7f, 0.5f, 0.7f);
    
    const char* opts[] = {"RESUME", "TELEPORT TO PLAYER", "SETTINGS", "DISCONNECT", "QUIT"};
    for (int i = 0; i < 5; i++) {
        float s = (menuSel == i) ? 1.0f : 0.5f;
        float y = 0.0f - i * 0.1f;
        if (menuSel == i) drawText(">", -0.35f, y, 1.8f, 0.9f * s, 0.85f * s, 0.4f * s);
        drawText(opts[i], -0.28f, y, 1.8f, 0.9f * s, 0.85f * s, 0.4f * s);
    }
    
    drawText("ESC - RESUME", -0.18f, -0.65f, 1.5f, 0.5f, 0.5f, 0.4f, 0.6f);
    
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