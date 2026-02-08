#pragma once
#include <GLFW/glfw3.h>
#include "menu.h"
#include "audio.h"
#include "net.h"
#include "menu_multi.h"
#include "lan_discovery.h"

extern bool escPressed, upPressed, downPressed, enterPressed, leftPressed, rightPressed;
extern bool firstMouse;
extern float lastX, lastY;

inline bool isAnyKeyboardKeyDown(GLFWwindow* w) {
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_MENU; k++) {
        if (glfwGetKey(w, k) == GLFW_PRESS) return true;
    }
    return false;
}

inline int firstPressedKeyboardKey(GLFWwindow* w) {
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_MENU; k++) {
        if (glfwGetKey(w, k) == GLFW_PRESS) return k;
    }
    return -1;
}

inline void pushNicknameChar(char c) {
    char next[PLAYER_NAME_BUF_LEN + 1] = {};
    int len = (int)strlen(multiNickname);
    if (len >= PLAYER_NAME_BUF_LEN - 1) return;
    memcpy(next, multiNickname, len);
    next[len] = c;
    next[len + 1] = '\0';
    sanitizePlayerName(next, multiNickname);
}

inline void popNicknameChar() {
    int len = (int)strlen(multiNickname);
    if (len <= 0) return;
    multiNickname[len - 1] = '\0';
    sanitizePlayerName(multiNickname, multiNickname);
}

inline void handleNicknameInput(GLFWwindow* w) {
    static bool letterPressed[26] = {false};
    static bool digitPressed[10] = {false};
    static bool spacePressedNick = false, minusPressedNick = false;
    static bool underscorePressedNick = false, backspacePressedNick = false;

    for (int i = 0; i < 26; i++) {
        bool now = glfwGetKey(w, GLFW_KEY_A + i) == GLFW_PRESS;
        if (now && !letterPressed[i]) pushNicknameChar((char)('a' + i));
        letterPressed[i] = now;
    }
    for (int i = 0; i < 10; i++) {
        bool now = glfwGetKey(w, GLFW_KEY_0 + i) == GLFW_PRESS;
        if (now && !digitPressed[i]) pushNicknameChar((char)('0' + i));
        digitPressed[i] = now;
    }
    bool spaceNow = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spaceNow && !spacePressedNick) pushNicknameChar(' ');
    spacePressedNick = spaceNow;

    bool shiftHeld = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
                     glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    bool minusNow = glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS;
    if (shiftHeld && minusNow && !underscorePressedNick) pushNicknameChar('_');
    else if (minusNow && !minusPressedNick) pushNicknameChar('-');
    underscorePressedNick = shiftHeld && minusNow;
    minusPressedNick = minusNow;

    bool bsNow = glfwGetKey(w, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    if (bsNow && !backspacePressedNick) popNicknameChar();
    backspacePressedNick = bsNow;
}

#include "input_settings.h"

inline void menuPauseInput(GLFWwindow* w, bool esc, bool up, bool down, bool enter) {
    if (multiState == MULTI_IN_GAME) {
        if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 4; triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel++; if (menuSel > 4) menuSel = 0; triggerMenuNavigateSound(); }
        if (esc && !escPressed) { 
            triggerMenuConfirmSound();
            gameState = STATE_GAME;
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            if (menuSel == 0) { 
                gameState = STATE_GAME;
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
            else if (menuSel == 1) {
                extern void teleportToPlayer();
                teleportToPlayer();
                gameState = STATE_GAME;
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
            else if (menuSel == 2) { gameState = STATE_SETTINGS_PAUSE; menuSel = 0; }
            else if (menuSel == 3) {
                netMgr.shutdown();
                lanDiscovery.stop();
                multiState = MULTI_NONE;
                gameState = STATE_MENU;
                menuSel = 0;
            }
            else glfwSetWindowShouldClose(w, 1);
        }
    } else {
        if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 3; triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel++; if (menuSel > 3) menuSel = 0; triggerMenuNavigateSound(); }
        if (esc && !escPressed) { 
            triggerMenuConfirmSound();
            gameState = STATE_GAME;
            glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            if (menuSel == 0) {
                gameState = STATE_GAME;
                glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
            else if (menuSel == 1) { gameState = STATE_SETTINGS_PAUSE; menuSel = 0; }
            else if (menuSel == 2) {
                extern void genWorld();
                extern void buildGeom();
                gameState = STATE_MENU;
                menuSel = 0;
                genWorld();
                buildGeom();
            }
            else glfwSetWindowShouldClose(w, 1);
        }
    }
}

inline void menuMultiInput(GLFWwindow* w, bool esc, bool up, bool down, bool enter) {
    static bool tabPressed = false;
    bool tabNow = glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabNow && !tabPressed) multiEditingNickname = !multiEditingNickname;
    tabPressed = tabNow;

    if (multiEditingNickname) {
        handleNicknameInput(w);
        bool confirmNick = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
        if (confirmNick && !enterPressed) multiEditingNickname = false;
        if (esc && !escPressed) multiEditingNickname = false;
        return;
    }

    if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 2; triggerMenuNavigateSound(); }
    if (down && !downPressed) { menuSel++; if (menuSel > 2) menuSel = 0; triggerMenuNavigateSound(); }
    if (esc && !escPressed) { triggerMenuConfirmSound(); gameState = STATE_MENU; menuSel = 1; }
    if (enter && !enterPressed) {
        triggerMenuConfirmSound();
        if (menuSel == 0) { 
            netMgr.init();
            if (netMgr.hostGame((unsigned int)time(nullptr))) {
                netMgr.setLocalPlayerName(multiNickname);
                lanDiscovery.startHost();
                multiState = MULTI_HOST_LOBBY;
                gameState = STATE_MULTI_HOST;
                menuSel = 0;
            }
        }
        else if (menuSel == 1) {
            gameState = STATE_MULTI_JOIN;
            menuSel = 0;
            multiIPManualEdit = false;
            lanDiscovery.startClient();
            lanDiscovery.requestScan();
        }
        else { gameState = STATE_MENU; menuSel = 1; }
    }
}

inline void menuHostInput(GLFWwindow* w, bool esc, bool up, bool down, bool enter) {
    if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 1; triggerMenuNavigateSound(); }
    if (down && !downPressed) { menuSel++; if (menuSel > 1) menuSel = 0; triggerMenuNavigateSound(); }
    if (esc && !escPressed) { 
        triggerMenuConfirmSound();
        netMgr.shutdown();
        lanDiscovery.stop();
        multiState = MULTI_NONE;
        gameState = STATE_MULTI;
        menuSel = 0;
    }
    if (enter && !enterPressed) {
        triggerMenuConfirmSound();
        if (menuSel == 0) { 
            multiState = MULTI_IN_GAME;
            extern void genWorld();
            extern void buildGeom();
            genWorld();
            buildGeom();
            netMgr.sendGameStart(netMgr.spawnPos);
            gameState = STATE_INTRO;
        }
        else {
            netMgr.shutdown();
            lanDiscovery.stop();
            multiState = MULTI_NONE;
            gameState = STATE_MULTI;
            menuSel = 0;
        }
    }
    netMgr.update();
    lanDiscovery.updateHost(netMgr.getPlayerCount(), netMgr.gameStarted, (float)glfwGetTime());
}

inline void menuJoinInput(GLFWwindow* w, bool esc, bool up, bool down, bool enter) {
    if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 1; triggerMenuNavigateSound(); }
    if (down && !downPressed) { menuSel++; if (menuSel > 1) menuSel = 0; triggerMenuNavigateSound(); }
    if (esc && !escPressed) { triggerMenuConfirmSound(); lanDiscovery.stop(); gameState = STATE_MULTI; menuSel = 1; }
    
    static bool tabPressedJoin = false;
    bool tabNow = glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabNow && !tabPressedJoin) multiInputField = (multiInputField == 0) ? 1 : 0;
    tabPressedJoin = tabNow;
    
    static bool numPressed[11] = {false};
    int keys[] = {GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
                  GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_PERIOD};
    char chars[] = {'0','1','2','3','4','5','6','7','8','9','.'};
    
    for (int i = 0; i < 11; i++) {
        bool p = glfwGetKey(w, keys[i]) == GLFW_PRESS;
        if (p && !numPressed[i]) {
            int ipLen = (int)strlen(multiJoinIP);
            int portLen = (int)strlen(multiJoinPort);
            if (multiInputField == 0 && ipLen < 15) {
                bool canAdd = (i < 10) || (i == 10 && ipLen > 0 && multiJoinIP[ipLen - 1] != '.');
                if (canAdd) { multiJoinIP[ipLen] = chars[i]; multiJoinIP[ipLen + 1] = 0; multiIPManualEdit = true; }
            }
            else if (multiInputField == 1 && i < 10 && portLen < 5) {
                multiJoinPort[portLen] = chars[i];
                multiJoinPort[portLen + 1] = 0;
            }
        }
        numPressed[i] = p;
    }
    
    static bool bsJoin = false;
    bool bsNow = glfwGetKey(w, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    if (bsNow && !bsJoin) {
        int ipLen = (int)strlen(multiJoinIP);
        int portLen = (int)strlen(multiJoinPort);
        if (multiInputField == 0 && ipLen > 0) { multiJoinIP[ipLen - 1] = 0; multiIPManualEdit = true; }
        else if (multiInputField == 1 && portLen > 0) multiJoinPort[portLen - 1] = 0;
    }
    bsJoin = bsNow;
    
    static bool refreshP = false;
    if (glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS && !refreshP) lanDiscovery.requestScan();
    refreshP = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
    
    static bool pickP = false;
    if (glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS && !pickP) {
        lanDiscovery.selectNextRoom();
        const LanRoomInfo* r = lanDiscovery.getSelectedRoom();
        if (r) {
            snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", r->ip);
            snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", r->gamePort);
            multiIPManualEdit = false;
        }
    }
    pickP = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
    
    lanDiscovery.updateClient((float)glfwGetTime());
    if (!multiIPManualEdit) {
        const LanRoomInfo* r = lanDiscovery.getSelectedRoom();
        if (r) {
            snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", r->ip);
            snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", r->gamePort);
        }
    }
    
    if (enter && !enterPressed) {
        triggerMenuConfirmSound();
        if (menuSel == 0) {
            netMgr.init();
            if (netMgr.joinGame(multiJoinIP, multiNickname)) {
                lanDiscovery.stop();
                multiState = MULTI_CONNECTING;
                gameState = STATE_MULTI_WAIT;
                menuSel = 0;
            }
        }
        else { lanDiscovery.stop(); gameState = STATE_MULTI; menuSel = 1; }
    }
}

inline void menuInput(GLFWwindow* w) {
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    
    if (gameState == STATE_MENU) {
        if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 3; triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel++; if (menuSel > 3) menuSel = 0; triggerMenuNavigateSound(); }
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            if (menuSel == 0) gameState = STATE_INTRO;
            else if (menuSel == 1) { gameState = STATE_MULTI; menuSel = 0; }
            else if (menuSel == 2) { gameState = STATE_SETTINGS; menuSel = 0; }
            else glfwSetWindowShouldClose(w, 1);
        }
    } 
    else if (gameState == STATE_PAUSE) menuPauseInput(w, esc, up, down, enter);
    else if (gameState == STATE_MULTI) menuMultiInput(w, esc, up, down, enter);
    else if (gameState == STATE_MULTI_HOST) menuHostInput(w, esc, up, down, enter);
    else if (gameState == STATE_MULTI_JOIN) menuJoinInput(w, esc, up, down, enter);
    else if (gameState == STATE_MULTI_WAIT) {
        if (esc && !escPressed) {
            triggerMenuConfirmSound();
            netMgr.shutdown();
            lanDiscovery.stop();
            multiState = MULTI_NONE;
            gameState = STATE_MULTI;
            menuSel = 1;
        }
        netMgr.update();
    }
    
    escPressed = esc;
    upPressed = up;
    downPressed = down;
    enterPressed = enter;
}