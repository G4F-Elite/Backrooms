#pragma once
#include "progression.h"
inline void menuInput(GLFWwindow* w) {
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
    
    if (gameState == STATE_MENU) {
        // Main menu: START GAME, MULTIPLAYER, SETTINGS, GUIDE, QUIT (5 items: 0-4)
        if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 4; triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel++; if (menuSel > 4) menuSel = 0; triggerMenuNavigateSound(); }
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            if (menuSel == 0) { 
                // Start game - go to intro first
                gCurrentLevel = 0;
                gameState = STATE_INTRO;
            }
            else if (menuSel == 1) { 
                // Multiplayer menu
                gameState = STATE_MULTI; 
                menuSel = 0; 
            }
            else if (menuSel == 2) { 
                // Settings
                settingsTab = SETTINGS_TAB_VIDEO;
                gameState = STATE_SETTINGS; 
                menuSel = 0; 
            }
            else if (menuSel == 3) { 
                // Guide screen
                guideReturnToPause = false;
                gameState = STATE_GUIDE;
            }
            else { 
                // Quit
                glfwSetWindowShouldClose(w, 1); 
            }
        }
    } 
    else if (gameState == STATE_GUIDE) {
        if ((esc && !escPressed) || (enter && !enterPressed)) {
            triggerMenuConfirmSound();
            if(guideReturnToPause){
                gameState = STATE_PAUSE;
                menuSel = (multiState == MULTI_IN_GAME) ? 3 : 2;
            }else{
                gameState = STATE_MENU;
                menuSel = 3;
            }
        }
    }
    else if (gameState == STATE_PAUSE) {
        if (multiState == MULTI_IN_GAME) {
            if (up && !upPressed) { menuSel--; if (menuSel < 0) menuSel = 5; triggerMenuNavigateSound(); }
            if (down && !downPressed) { menuSel++; if (menuSel > 5) menuSel = 0; triggerMenuNavigateSound(); }
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
                else if (menuSel == 2) { 
                    settingsTab = SETTINGS_TAB_VIDEO;
                    gameState = STATE_SETTINGS_PAUSE; 
                    menuSel = 0; 
                }
                else if (menuSel == 3) {
                    guideReturnToPause = true;
                    gameState = STATE_GUIDE;
                }
                else if (menuSel == 4) { 
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
                    settingsTab = SETTINGS_TAB_VIDEO;
                    gameState = STATE_SETTINGS_PAUSE; 
                    menuSel = 0; 
                }
                else if (menuSel == 2) { 
                    guideReturnToPause = true;
                    gameState = STATE_GUIDE;
                }
                else if (menuSel == 3) { 
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
        static bool modeSwitchPressed = false;
        bool tabNow = glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS;
        bool leftNow = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
        bool rightNow = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
        bool modeSwitchNow = glfwGetKey(w, GLFW_KEY_T) == GLFW_PRESS || leftNow || rightNow;
        if (tabNow && !tabPressed) multiEditingNickname = !multiEditingNickname;
        if (modeSwitchNow && !modeSwitchPressed) {
            multiNetworkMode = (multiNetworkMode == 0) ? 1 : 0;
            triggerMenuAdjustSound();
        }
        tabPressed = tabNow;
        modeSwitchPressed = modeSwitchNow;

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
                multiMasterManualEdit = false;
                if (multiNetworkMode == 0) {
                    lanDiscovery.startClient();
                    lanDiscovery.requestScan();
                    dedicatedDirectory.stop();
                } else {
                    int mPort = backrooms::protocol::kDefaultMasterPort;
                    if (std::sscanf(multiMasterPort, "%d", &mPort) != 1 || mPort < 1 || mPort > 65535) mPort = backrooms::protocol::kDefaultMasterPort;
                    dedicatedDirectory.start(multiMasterIP, (unsigned short)mPort);
                    dedicatedDirectory.requestScan();
                    lanDiscovery.stop();
                }
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
            if (multiNetworkMode == 0) lanDiscovery.stop();
            else dedicatedDirectory.stop();
            gameState = STATE_MULTI; 
            menuSel = 1;
        }
        
        // TAB to switch between IP and Port fields
        static bool tabPressed = false;
        static bool modeSwitchPressed = false;
        bool tabNow = glfwGetKey(w, GLFW_KEY_TAB) == GLFW_PRESS;
        bool leftNow = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
        bool rightNow = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
        bool modeSwitchNow = glfwGetKey(w, GLFW_KEY_T) == GLFW_PRESS || leftNow || rightNow;
        if (tabNow && !tabPressed) {
            multiInputField = (multiInputField == 0) ? 1 : 0;
        }
        tabPressed = tabNow;
        if (modeSwitchNow && !modeSwitchPressed) {
            multiNetworkMode = (multiNetworkMode == 0) ? 1 : 0;
            multiIPManualEdit = false;
            multiMasterManualEdit = false;
            if (multiNetworkMode == 0) {
                dedicatedDirectory.stop();
                lanDiscovery.startClient();
                lanDiscovery.requestScan();
            } else {
                lanDiscovery.stop();
                int mPort = backrooms::protocol::kDefaultMasterPort;
                if (std::sscanf(multiMasterPort, "%d", &mPort) != 1 || mPort < 1 || mPort > 65535) mPort = backrooms::protocol::kDefaultMasterPort;
                dedicatedDirectory.stop();
                dedicatedDirectory.start(multiMasterIP, (unsigned short)mPort);
                dedicatedDirectory.requestScan();
            }
            triggerMenuAdjustSound();
        }
        modeSwitchPressed = modeSwitchNow;
        
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
                    char* targetIP = (multiNetworkMode == 0) ? multiJoinIP : multiMasterIP;
                    int len = (int)strlen(targetIP);
                    if (len < 15) {
                        if (i < 10 || (i == 10 && len > 0 && targetIP[len-1] != '.')) {
                            targetIP[len] = chars[i];
                            targetIP[len + 1] = 0;
                            if (multiNetworkMode == 0) multiIPManualEdit = true;
                            else multiMasterManualEdit = true;
                        }
                    }
                } else {
                    // Port field - numbers only
                    if (i < 10) {
                        char* targetPort = (multiNetworkMode == 0) ? multiJoinPort : multiMasterPort;
                        int len = (int)strlen(targetPort);
                        if (len < 5) {
                            targetPort[len] = chars[i];
                            targetPort[len + 1] = 0;
                            if (multiNetworkMode == 0) multiIPManualEdit = true;
                            else multiMasterManualEdit = true;
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
                char* targetIP = (multiNetworkMode == 0) ? multiJoinIP : multiMasterIP;
                int len = (int)strlen(targetIP);
                if (len > 0) {
                    targetIP[len - 1] = 0;
                    if (multiNetworkMode == 0) multiIPManualEdit = true;
                    else multiMasterManualEdit = true;
                }
            } else {
                char* targetPort = (multiNetworkMode == 0) ? multiJoinPort : multiMasterPort;
                int len = (int)strlen(targetPort);
                if (len > 0) {
                    targetPort[len - 1] = 0;
                    if (multiNetworkMode == 0) multiIPManualEdit = true;
                    else multiMasterManualEdit = true;
                }
            }
        }
        bsPressed = bsNow;
        
        static bool refreshPressed = false;
        bool refreshNow = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS;
        if (refreshNow && !refreshPressed) {
            if (multiNetworkMode == 0) lanDiscovery.requestScan();
            else {
                int mPort = backrooms::protocol::kDefaultMasterPort;
                if (std::sscanf(multiMasterPort, "%d", &mPort) != 1 || mPort < 1 || mPort > 65535) mPort = backrooms::protocol::kDefaultMasterPort;
                dedicatedDirectory.stop();
                dedicatedDirectory.start(multiMasterIP, (unsigned short)mPort);
                dedicatedDirectory.requestScan();
            }
        }
        refreshPressed = refreshNow;
        
        static bool pickRoomPressed = false;
        bool pickRoomNow = glfwGetKey(w, GLFW_KEY_F) == GLFW_PRESS;
        if (pickRoomNow && !pickRoomPressed) {
            if (multiNetworkMode == 0) {
                lanDiscovery.selectNextRoom();
                const LanRoomInfo* room = lanDiscovery.getSelectedRoom();
                if (room) {
                    snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", room->ip);
                    snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", room->gamePort);
                    multiIPManualEdit = false;
                }
            } else {
                dedicatedDirectory.selectNextRoom();
                const DedicatedRoomInfo* room = dedicatedDirectory.getSelectedRoom();
                if (room) {
                    snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", room->ip);
                    snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", room->gamePort);
                    multiIPManualEdit = false;
                }
            }
        }
        pickRoomPressed = pickRoomNow;

        static bool quickConnectPressed = false;
        bool quickConnectNow = glfwGetKey(w, GLFW_KEY_G) == GLFW_PRESS;
        if (quickConnectNow && !quickConnectPressed && multiNetworkMode == 1) {
            const DedicatedRoomInfo* room = dedicatedDirectory.getSelectedRoom();
            if (room) {
                std::snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", room->ip);
                std::snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", room->gamePort);
                char fullAddr[64];
                std::snprintf(fullAddr, 64, "%s", multiJoinIP);
                netMgr.init();
                if (netMgr.joinGame(fullAddr, multiNickname)) {
                    lanDiscovery.stop();
                    dedicatedDirectory.stop();
                    multiState = MULTI_CONNECTING;
                    gameState = STATE_MULTI_WAIT;
                    menuSel = 0;
                }
            }
        }
        quickConnectPressed = quickConnectNow;
        
        if (multiNetworkMode == 0) {
            lanDiscovery.updateClient((float)glfwGetTime());
            if (!multiIPManualEdit) {
                const LanRoomInfo* room = lanDiscovery.getSelectedRoom();
                if (room) {
                    snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", room->ip);
                    snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", room->gamePort);
                }
            }
        } else {
            dedicatedDirectory.update((float)glfwGetTime());
            if (!multiIPManualEdit) {
                const DedicatedRoomInfo* room = dedicatedDirectory.getSelectedRoom();
                if (room) {
                    snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", room->ip);
                    snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", room->gamePort);
                }
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
                    dedicatedDirectory.stop();
                    multiState = MULTI_CONNECTING;
                    gameState = STATE_MULTI_WAIT;  // Wait for host to start
                    menuSel = 0;
                }
            }
            else { 
                // Back
                lanDiscovery.stop();
                dedicatedDirectory.stop();
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
            dedicatedDirectory.stop();
            multiState = MULTI_NONE;
            gameState = STATE_MULTI;
            menuSel = 1;
        }
        // Keep updating network for game start signal
        netMgr.update();
        if (!netMgr.welcomeReceived) {
            float sincePacket = (float)glfwGetTime() - netMgr.lastPacketRecvTime;
            if (sincePacket > 6.0f) {
                netMgr.shutdown();
                lanDiscovery.stop();
                multiState = MULTI_NONE;
                gameState = STATE_MULTI_JOIN;
                menuSel = 0;
            }
        }
    }
    
    escPressed = esc;
    upPressed = up; 
    downPressed = down; 
    enterPressed = enter;
}
