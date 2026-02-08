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
    static bool spacePressedNick = false;
    static bool minusPressedNick = false;
    static bool underscorePressedNick = false;
    static bool backspacePressedNick = false;

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

    bool underNow = glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS &&
                    (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
    bool minusNow = glfwGetKey(w, GLFW_KEY_MINUS) == GLFW_PRESS;
    if (underNow && !underscorePressedNick) pushNicknameChar('_');
    else if (minusNow && !minusPressedNick) pushNicknameChar('-');
    underscorePressedNick = underNow;
    minusPressedNick = minusNow;

    bool bsNow = glfwGetKey(w, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    if (bsNow && !backspacePressedNick) popNicknameChar();
    backspacePressedNick = bsNow;
}

inline void settingsInput(GLFWwindow* w, bool fromPause) {
    static constexpr int SETTINGS_ITEMS = 13;
    static constexpr int SETTINGS_AA_INDEX = 10;
    static constexpr int SETTINGS_BINDS_INDEX = 11;
    static constexpr int SETTINGS_BACK_INDEX = 12;
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
    const double now = glfwGetTime();
    static int adjustHoldDir = 0;
    static double nextAdjustTime = 0.0;
    const double adjustFirstDelay = 0.28;
    const double adjustRepeatInterval = 0.055;

    auto applyAdjust = [&](int dir) {
        if (dir == 0) return;
        if (menuSel <= 6) {
            float* vals[] = {
                &settings.masterVol, &settings.musicVol, &settings.ambienceVol, &settings.sfxVol,
                &settings.voiceVol, &settings.vhsIntensity, &settings.mouseSens
            };
            float maxV[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.006f};
            float minV[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0005f};
            float step[] = {0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.0003f};
            *vals[menuSel] += step[menuSel] * (float)dir;
            if (*vals[menuSel] < minV[menuSel]) *vals[menuSel] = minV[menuSel];
            if (*vals[menuSel] > maxV[menuSel]) *vals[menuSel] = maxV[menuSel];
            triggerMenuNavigateSound();
        } else if (menuSel == 7) {
            settings.upscalerMode = clampUpscalerMode(settings.upscalerMode + dir);
            triggerMenuNavigateSound();
        } else if (menuSel == 8) {
            settings.renderScalePreset = stepRenderScalePreset(settings.renderScalePreset, dir);
            triggerMenuNavigateSound();
        } else if (menuSel == 9) {
            settings.fsrSharpness = clampFsrSharpness(settings.fsrSharpness + 0.05f * (float)dir);
            triggerMenuNavigateSound();
        } else if (menuSel == SETTINGS_AA_INDEX) {
            settings.aaMode = stepAaMode(settings.aaMode, dir);
            triggerMenuNavigateSound();
        }
    };
    
    if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = SETTINGS_ITEMS - 1; triggerMenuNavigateSound(); }
    if (down && !downPressed) { menuSel++; if (menuSel >= SETTINGS_ITEMS) menuSel = 0; triggerMenuNavigateSound(); }
    
    int adjustDir = right ? 1 : (left ? -1 : 0);
    if (menuSel <= SETTINGS_AA_INDEX) {
        if (adjustDir == 0) {
            adjustHoldDir = 0;
            nextAdjustTime = 0.0;
        } else if (adjustHoldDir != adjustDir) {
            adjustHoldDir = adjustDir;
            applyAdjust(adjustDir);
            nextAdjustTime = now + adjustFirstDelay;
        } else if (now >= nextAdjustTime) {
            applyAdjust(adjustDir);
            nextAdjustTime = now + adjustRepeatInterval;
        }
    } else if (menuSel == SETTINGS_BINDS_INDEX) {
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            gameState = fromPause ? STATE_KEYBINDS_PAUSE : STATE_KEYBINDS;
            menuSel = 0;
            keybindCaptureIndex = -1;
        }
    }
    
    if ((enter && !enterPressed && menuSel == SETTINGS_BACK_INDEX) || (esc && !escPressed)) { 
        triggerMenuConfirmSound();
        gameState = fromPause ? STATE_PAUSE : STATE_MENU; 
        menuSel = fromPause ? 1 : 2;  // Settings position in respective menu
    }
    
    escPressed = esc; 
    upPressed = up; 
    downPressed = down; 
    leftPressed = left; 
    rightPressed = right; 
    enterPressed = enter;
}

inline void keybindsInput(GLFWwindow* w, bool fromPause) {
    static bool waitRelease = false;
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;

    if (keybindCaptureIndex >= 0) {
        if (waitRelease) {
            if (!isAnyKeyboardKeyDown(w)) waitRelease = false;
        } else {
            int pressed = firstPressedKeyboardKey(w);
            if (pressed >= 0) {
                int* keyRef = gameplayBindByIndex(settings.binds, keybindCaptureIndex);
                if (keyRef) *keyRef = pressed;
                keybindCaptureIndex = -1;
                triggerMenuConfirmSound();
            } else if (esc && !escPressed) {
                keybindCaptureIndex = -1;
                triggerMenuConfirmSound();
            }
        }
    } else {
        if (up && !upPressed) { menuSel = clampKeybindMenuIndex(menuSel - 1); triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel = clampKeybindMenuIndex(menuSel + 1); triggerMenuNavigateSound(); }
        if (enter && !enterPressed) {
            if (isGameplayBindIndex(menuSel)) {
                keybindCaptureIndex = menuSel;
                waitRelease = true;
                triggerMenuConfirmSound();
            } else if (menuSel == KEYBINDS_BACK_INDEX) {
                triggerMenuConfirmSound();
                gameState = fromPause ? STATE_SETTINGS_PAUSE : STATE_SETTINGS;
                menuSel = 11;
            }
        }
        if (esc && !escPressed) {
            triggerMenuConfirmSound();
            gameState = fromPause ? STATE_SETTINGS_PAUSE : STATE_SETTINGS;
            menuSel = 11;
        }
    }

    escPressed = esc;
    upPressed = up;
    downPressed = down;
    enterPressed = enter;
}

inline void menuInput(GLFWwindow* w) {
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    
    if (gameState == STATE_MENU) {
        // Main menu: START GAME, MULTIPLAYER, SETTINGS, QUIT (4 items: 0-3)
        if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 3; triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel++; if (menuSel > 3) menuSel = 0; triggerMenuNavigateSound(); }
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            if (menuSel == 0) { 
                // Start game - go to intro first
                gameState = STATE_INTRO;
            }
            else if (menuSel == 1) { 
                // Multiplayer menu
                gameState = STATE_MULTI; 
                menuSel = 0; 
            }
            else if (menuSel == 2) { 
                // Settings
                gameState = STATE_SETTINGS; 
                menuSel = 0; 
            }
            else { 
                // Quit
                glfwSetWindowShouldClose(w, 1); 
            }
        }
    } 
    else if (gameState == STATE_PAUSE) {
        if (multiState == MULTI_IN_GAME) {
            // Multiplayer pause: RESUME, TELEPORT, SETTINGS, DISCONNECT, QUIT (5 items)
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
                    // Teleport to player - defined in game_loop.h
                    extern void teleportToPlayer();
                    teleportToPlayer();
                    gameState = STATE_GAME;
                    glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    firstMouse = true;
                }
                else if (menuSel == 2) { 
                    gameState = STATE_SETTINGS_PAUSE; 
                    menuSel = 0; 
                }
                else if (menuSel == 3) { 
                    // Disconnect
                    netMgr.shutdown();
                    lanDiscovery.stop();
                    multiState = MULTI_NONE;
                    gameState = STATE_MENU;
                    menuSel = 0;
                }
                else { 
                    glfwSetWindowShouldClose(w, 1); 
                }
            }
        } else {
            // Single player pause: RESUME, SETTINGS, MAIN MENU, QUIT (4 items)
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
                else if (menuSel == 1) { 
                    gameState = STATE_SETTINGS_PAUSE; 
                    menuSel = 0; 
                }
                else if (menuSel == 2) { 
                    // Main menu
                    extern void genWorld();
                    extern void buildGeom();
                    gameState = STATE_MENU;
                    menuSel = 0;
                    genWorld();
                    buildGeom();
                }
                else { 
                    glfwSetWindowShouldClose(w, 1); 
                }
            }
        }
    }
    else if (gameState == STATE_MULTI) {
        static bool tabPressed = false;
        bool tabNow = glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabNow && !tabPressed) multiEditingNickname = !multiEditingNickname;
        tabPressed = tabNow;

        if (multiEditingNickname) {
            handleNicknameInput(w);
            bool confirmNick = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
            if (confirmNick && !enterPressed) multiEditingNickname = false;
            if (esc && !escPressed) multiEditingNickname = false;
            escPressed = esc;
            upPressed = up;
            downPressed = down;
            enterPressed = enter;
            return;
        }

        // Multiplayer menu: HOST GAME, JOIN GAME, BACK (3 items: 0-2)
        if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 2; triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel++; if (menuSel > 2) menuSel = 0; triggerMenuNavigateSound(); }
        if (esc && !escPressed) { 
            triggerMenuConfirmSound();
            gameState = STATE_MENU; 
            menuSel = 1;  // Back to Multiplayer option
        }
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            if (menuSel == 0) { 
                // Host game - initialize network and start hosting
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
                // Join game
                gameState = STATE_MULTI_JOIN; 
                menuSel = 0; 
                multiIPManualEdit = false;
                lanDiscovery.startClient();
                lanDiscovery.requestScan();
            }
            else { 
                // Back
                gameState = STATE_MENU;
                menuSel = 1;
            }
        }
    }
    else if (gameState == STATE_MULTI_HOST) {
        // Host lobby: START GAME, BACK (2 items: 0-1)
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
                // Start multiplayer game with all connected players
                // First set multiState so genWorld knows we're in multiplayer
                multiState = MULTI_IN_GAME;
                // Generate world and get spawn position
                extern void genWorld();
                extern void buildGeom();
                genWorld();
                buildGeom();
                // Now send game start with correct spawn position
                netMgr.sendGameStart(netMgr.spawnPos);
                gameState = STATE_INTRO;
            }
            else { 
                // Back - shutdown hosting
                netMgr.shutdown();
                lanDiscovery.stop();
                multiState = MULTI_NONE;
                gameState = STATE_MULTI;
                menuSel = 0;
            }
        }
        // Update network to check for new connections
        netMgr.update();
        lanDiscovery.updateHost(netMgr.getPlayerCount(), netMgr.gameStarted, (float)glfwGetTime());
    }
    else if (gameState == STATE_MULTI_JOIN) {
        // Join menu: CONNECT, BACK (2 items: 0-1)
        if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 1; triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel++; if (menuSel > 1) menuSel = 0; triggerMenuNavigateSound(); }
        if (esc && !escPressed) { 
            triggerMenuConfirmSound();
            lanDiscovery.stop();
            gameState = STATE_MULTI; 
            menuSel = 1;
        }
        
        // TAB to switch between IP and Port fields
        static bool tabPressed = false;
        bool tabNow = glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tabNow && !tabPressed) {
            multiInputField = (multiInputField == 0) ? 1 : 0;
        }
        tabPressed = tabNow;
        
        // Number input for IP/Port
        static bool numPressed[11] = {false};
        int keys[] = {GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4,
                      GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7, GLFW_KEY_8, GLFW_KEY_9,
                      GLFW_KEY_PERIOD};
        char chars[] = {'0','1','2','3','4','5','6','7','8','9','.'};
        
        for (int i = 0; i < 11; i++) {
            bool pressed = glfwGetKey(w, keys[i]) == GLFW_PRESS;
            if (pressed && !numPressed[i]) {
                if (multiInputField == 0) {
                    // IP field - allow numbers and dots
                    int len = (int)strlen(multiJoinIP);
                    if (len < 15) {
                        if (i < 10 || (i == 10 && len > 0 && multiJoinIP[len-1] != '.')) {
                            multiJoinIP[len] = chars[i];
                            multiJoinIP[len + 1] = 0;
                            multiIPManualEdit = true;
                        }
                    }
                } else {
                    // Port field - numbers only
                    if (i < 10) {
                        int len = (int)strlen(multiJoinPort);
                        if (len < 5) {
                            multiJoinPort[len] = chars[i];
                            multiJoinPort[len + 1] = 0;
                        }
                    }
                }
            }
            numPressed[i] = pressed;
        }
        
        // Backspace handling
        static bool bsPressed = false;
        bool bsNow = glfwGetKey(w, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
        if (bsNow && !bsPressed) {
            if (multiInputField == 0) {
                int len = (int)strlen(multiJoinIP);
                if (len > 0) {
                    multiJoinIP[len - 1] = 0;
                    multiIPManualEdit = true;
                }
            } else {
                int len = (int)strlen(multiJoinPort);
                if (len > 0) multiJoinPort[len - 1] = 0;
            }
        }
        bsPressed = bsNow;
        
        static bool refreshPressed = false;
        bool refreshNow = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
        if (refreshNow && !refreshPressed) {
            lanDiscovery.requestScan();
        }
        refreshPressed = refreshNow;
        
        static bool pickRoomPressed = false;
        bool pickRoomNow = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
        if (pickRoomNow && !pickRoomPressed) {
            lanDiscovery.selectNextRoom();
            const LanRoomInfo* room = lanDiscovery.getSelectedRoom();
            if (room) {
                snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", room->ip);
                snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", room->gamePort);
                multiIPManualEdit = false;
            }
        }
        pickRoomPressed = pickRoomNow;
        
        lanDiscovery.updateClient((float)glfwGetTime());
        if (!multiIPManualEdit) {
            const LanRoomInfo* room = lanDiscovery.getSelectedRoom();
            if (room) {
                snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", room->ip);
                snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", room->gamePort);
            }
        }
        
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            if (menuSel == 0) { 
                // Connect to host - go to waiting lobby
                char fullAddr[64];
                snprintf(fullAddr, 64, "%s", multiJoinIP);
                netMgr.init();
                if (netMgr.joinGame(fullAddr, multiNickname)) {
                    lanDiscovery.stop();
                    multiState = MULTI_CONNECTING;
                    gameState = STATE_MULTI_WAIT;  // Wait for host to start
                    menuSel = 0;
                }
            }
            else { 
                // Back
                lanDiscovery.stop();
                gameState = STATE_MULTI;
                menuSel = 1;
            }
        }
    }
    else if (gameState == STATE_MULTI_WAIT) {
        // Waiting for host to start
        if (esc && !escPressed) {
            triggerMenuConfirmSound();
            netMgr.shutdown();
            lanDiscovery.stop();
            multiState = MULTI_NONE;
            gameState = STATE_MULTI;
            menuSel = 1;
        }
        // Keep updating network for game start signal
        netMgr.update();
    }
    
    escPressed = esc;
    upPressed = up; 
    downPressed = down; 
    enterPressed = enter;
}
