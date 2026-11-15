#pragma once
// Run flow: level exit, co-op level advance / squad wipe, leaving a run.

#include "coop.h"

inline void leaveRunToMenu() {
    if (multiState == MULTI_IN_GAME) {
        netMgr.shutdown();
        lanDiscovery.stop();
        multiState = MULTI_NONE;
    }
    gCurrentLevel = 0;
    gameState = STATE_MENU;
    menuSel = 0;
    glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

// Host only. Clients follow through the level/seed carried in the void shift state.
inline void hostAdvanceLevel(bool restartRun) {
    if (restartRun) {
        gCurrentLevel = 0;
    } else if (isLevelZero(gCurrentLevel)) {
        gCurrentLevel = 1;
        gCompletedLevels++;
    } else {
        gCompletedLevels++;
        gRunComplete = true;
        playerEscaped = true;
        for (int i = 0; i < 3; i++) hostSyncFeatureState();
        glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        return;
    }
    genWorld();
    buildGeom();
    firstMouse = true;
    setTrapStatus(restartRun ? "SQUAD LOST. SHIFT RESTARTED." : "DESCENDING: LEVEL 2 - PARKING");
}

// Called when the local player presses interact at an unlocked exit.
inline void requestLevelExit() {
    if (multiState != MULTI_IN_GAME) {
        playerEscaped = true;
        glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        return;
    }
    if (netMgr.isHost) gHostExitRequested = true;
    else netMgr.sendInteractRequest(REQ_EXIT_LEVEL, 0);
}

inline bool isRemotePlayerStanding(int p) {
    if (p == netMgr.myId || !netMgr.players[p].active) return false;
    return (netMgr.players[p].flags & (PLAYER_FLAG_DOWNED | PLAYER_FLAG_DEAD)) == 0;
}

inline void updateCoopFlowHost(float dt) {
    static float wipeTimer = 0.0f;
    if (multiState != MULTI_IN_GAME || !netMgr.isHost || gRunComplete) {
        wipeTimer = 0.0f;
        return;
    }

    if (gHostExitRequested) {
        gHostExitRequested = false;
        if (coop.doorOpen && isStoryExitReady()) {
            hostAdvanceLevel(false);
            return;
        }
    }

    bool anyStanding = !gCoopDead && !playerDowned;
    for (int p = 0; p < MAX_PLAYERS && !anyStanding; p++) {
        if (isRemotePlayerStanding(p)) anyStanding = true;
    }
    if (anyStanding) {
        wipeTimer = 0.0f;
        return;
    }

    // Everyone is downed or dead: give self-revives a moment, then restart the shift.
    wipeTimer += dt;
    if (wipeTimer >= 8.0f) {
        wipeTimer = 0.0f;
        hostAdvanceLevel(true);
    }
}
