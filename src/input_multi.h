#pragma once
// Multiplayer screens input: mode + callsign, join, host lobby, waiting room.
// Layout and item order: menu_multi.h.

inline bool isAddressChar(char c) {
    return (c >= '0' && c <= '9') || c == '.';
}

inline bool isPortChar(char c) {
    return c >= '0' && c <= '9';
}

inline void commitNickname() {
    char clean[PLAYER_NAME_BUF_LEN];
    sanitizePlayerName(multiNickname, clean);
    memcpy(multiNickname, clean, PLAYER_NAME_BUF_LEN);
}

inline void closeMultiSession() {
    netMgr.shutdown();
    lanDiscovery.stop();
    dedicatedDirectory.stop();
    multiState = MULTI_NONE;
}

// LAN: listen for rooms. Servers: nothing to scan, the address is typed in.
inline void applyNetworkMode(bool scanning) {
    multiIPManualEdit = false;
    multiMasterManualEdit = false;
    multiConnectStatus[0] = 0;
    dedicatedDirectory.stop();
    if (multiNetworkMode == 0 && scanning) {
        lanDiscovery.startClient();
        lanDiscovery.requestScan();
    } else {
        lanDiscovery.stop();
    }
}

// TAB, or a click on a tab. Returns true when the mode changed.
inline bool multiModeSwitchInput(bool scanning) {
    const UiZone* tabZone = ui.mousePressed ? uiHotZone(UI_ZONE_TAB) : nullptr;
    int mode = multiNetworkMode;
    if (tabZone) mode = tabZone->id;
    else if (ui.tab) mode = multiNetworkMode == 0 ? 1 : 0;
    if (mode == multiNetworkMode) return false;
    multiNetworkMode = mode;
    applyNetworkMode(scanning);
    triggerMenuAdjustSound();
    return true;
}

inline int parsePortOrDefault(const char* text) {
    int port = NET_PORT;
    if (std::sscanf(text, "%d", &port) != 1 || port < 1 || port > 65535) port = NET_PORT;
    return port;
}

inline void multiConnect() {
    bool dedicated = multiNetworkMode == 1;
    const char* address = dedicated ? multiMasterIP : multiJoinIP;
    int port = parsePortOrDefault(dedicated ? multiMasterPort : multiJoinPort);
    commitNickname();
    multiConnectStatus[0] = 0;
    netMgr.init();
    if (!netMgr.joinGame(address, multiNickname, (unsigned short)port)) {
        std::snprintf(multiConnectStatus, sizeof(multiConnectStatus), "COULD NOT OPEN A CONNECTION TO %s:%d.", address, port);
        return;
    }
    lanDiscovery.stop();
    dedicatedDirectory.stop();
    multiState = MULTI_CONNECTING;
    multiWaitBeforeStart = 0.8f;
    gameState = STATE_MULTI_WAIT;
    menuSel = 0;
    if (dedicated) std::snprintf(multiConnectStatus, sizeof(multiConnectStatus), "SERVER %s:%d", address, port);
}

inline void multiHost() {
    commitNickname();
    multiConnectStatus[0] = 0;
    netMgr.init();
    if (!netMgr.hostGame((unsigned int)time(nullptr))) {
        std::snprintf(multiConnectStatus, sizeof(multiConnectStatus), "COULD NOT OPEN PORT %d. IS ANOTHER HOST RUNNING?", (int)NET_PORT);
        return;
    }
    netMgr.setLocalPlayerName(multiNickname);
    lanDiscovery.startHost();
    multiState = MULTI_HOST_LOBBY;
    gameState = STATE_MULTI_HOST;
    menuSel = HOST_ITEM_START;
}

inline void multiMenuInput(GLFWwindow* w) {
    bool typing = menuSel == MULTI_ITEM_NICK;
    uiPollInput(w, typing);
    multiModeSwitchInput(false);
    if (typing) uiEditText(multiNickname, PLAYER_NAME_BUF_LEN, isPlayerNameChar);

    bool activate = uiMenuNav(MULTI_ITEM_COUNT);
    if (uiBackRequested()) {
        triggerMenuConfirmSound();
        commitNickname();
        multiConnectStatus[0] = 0;
        gameState = STATE_MENU;
        menuSel = MAIN_MENU_MULTIPLAYER;
        return;
    }
    if (!activate) return;
    if (menuSel == MULTI_ITEM_NICK) {
        // ENTER in the field moves on; a click only puts the caret there
        if (ui.enter) menuSel = MULTI_ITEM_HOST;
        return;
    }
    triggerMenuConfirmSound();
    if (menuSel == MULTI_ITEM_HOST) {
        multiHost();
    } else {
        commitNickname();
        gameState = STATE_MULTI_JOIN;
        menuSel = JOIN_ITEM_ADDRESS;
        applyNetworkMode(true);
    }
}

inline void joinMenuFieldInput() {
    bool dedicated = multiNetworkMode == 1;
    bool changed = false;
    if (menuSel == JOIN_ITEM_ADDRESS) changed = uiEditText(dedicated ? multiMasterIP : multiJoinIP, 16, isAddressChar);
    else if (menuSel == JOIN_ITEM_PORT) changed = uiEditText(dedicated ? multiMasterPort : multiJoinPort, 6, isPortChar);
    if (!changed) return;
    if (dedicated) multiMasterManualEdit = true;
    else multiIPManualEdit = true;
}

// The room rows follow discovery; until the player types an address, the fields do too.
inline void joinMenuSyncRooms() {
    if (multiNetworkMode != 0) return;
    lanDiscovery.updateClient((float)glfwGetTime());
    int room = menuSel - JOIN_ITEM_FIRST_ROOM;
    if (room >= 0 && room < lanDiscovery.roomCount) {
        lanDiscovery.selectedRoom = room;
        multiIPManualEdit = false;
    }
    if (multiIPManualEdit) return;
    const LanRoomInfo* info = lanDiscovery.getSelectedRoom();
    if (!info) return;
    snprintf(multiJoinIP, sizeof(multiJoinIP), "%s", info->ip);
    snprintf(multiJoinPort, sizeof(multiJoinPort), "%hu", info->gamePort);
}

inline void joinMenuInput(GLFWwindow* w) {
    bool typing = menuSel == JOIN_ITEM_ADDRESS || menuSel == JOIN_ITEM_PORT;
    uiPollInput(w, typing);
    if (multiModeSwitchInput(true) && menuSel >= JOIN_ITEM_FIRST_ROOM) menuSel = JOIN_ITEM_CONNECT;
    if (typing) joinMenuFieldInput();

    bool activate = uiMenuNav(joinItemCount());
    joinMenuSyncRooms();
    if (uiBackRequested()) {
        triggerMenuConfirmSound();
        lanDiscovery.stop();
        dedicatedDirectory.stop();
        multiConnectStatus[0] = 0;
        gameState = STATE_MULTI;
        menuSel = MULTI_ITEM_JOIN;
        return;
    }
    if (!activate) return;
    if (typing) {
        if (ui.enter) menuSel++; // address -> port -> connect
        return;
    }
    triggerMenuConfirmSound();
    multiConnect();
}

inline void hostLobbyInput(GLFWwindow* w) {
    uiPollInput(w, false);
    bool activate = uiMenuNav(HOST_ITEM_COUNT);
    bool close = uiBackRequested() || (activate && menuSel == HOST_ITEM_CLOSE);
    if (close) {
        triggerMenuConfirmSound();
        closeMultiSession();
        gameState = STATE_MULTI;
        menuSel = MULTI_ITEM_HOST;
        return;
    }
    if (activate) {
        triggerMenuConfirmSound();
        // multiState first: genWorld has to know this is a co-op world, and the spawn it
        // picks goes out with the start packet.
        extern void genWorld();
        extern void buildGeom();
        multiState = MULTI_IN_GAME;
        gCurrentLevel = 0;
        gHeistSkipTutorial = gHeistTutorialSeen;
        genWorld();
        buildGeom();
        netMgr.sendGameStart(netMgr.spawnPos);
        gameState = STATE_INTRO;
        return;
    }
    netMgr.update();
    lanDiscovery.updateHost(netMgr.getPlayerCount(), netMgr.gameStarted, (float)glfwGetTime());
}

inline void waitingRoomInput(GLFWwindow* w) {
    uiPollInput(w, false);
    if (uiBackRequested()) {
        triggerMenuConfirmSound();
        closeMultiSession();
        multiConnectStatus[0] = 0;
        gameState = STATE_MULTI;
        menuSel = MULTI_ITEM_JOIN;
        return;
    }
    netMgr.update();
    if (netMgr.welcomeReceived) return;
    float sincePacket = (float)glfwGetTime() - netMgr.lastPacketRecvTime;
    if (sincePacket <= 6.0f) return;
    closeMultiSession();
    gameState = STATE_MULTI_JOIN;
    menuSel = JOIN_ITEM_CONNECT;
    applyNetworkMode(true);
    if (multiNetworkMode == 0) snprintf(multiConnectStatus, sizeof(multiConnectStatus), "NO ANSWER. THE HOST IS UNREACHABLE OR A FIREWALL BLOCKS IT.");
    else snprintf(multiConnectStatus, sizeof(multiConnectStatus), "NO ANSWER FROM THE SERVER. CHECK THE ADDRESS AND THE PORT.");
}
