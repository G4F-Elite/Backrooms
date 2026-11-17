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

// Retries and level changes drop straight back into play; the briefing only runs on first deploy.
inline void reloadLevelAndResume(const char* status) {
    genWorld();
    buildGeom();
    storyMgr.introComplete = true;
    gameState = STATE_GAME;
    glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;
    if (status) setTrapStatus(status);
}

// Host only. Clients follow through the level/seed carried in the void shift state.
// restartRun: the squad was wiped. The floor starts over and half the wallet is gone.
inline void hostAdvanceLevel(bool restartRun) {
    if (restartRun) {
        heist.cash = cashAfterWipe(heist.cash);
    } else {
        gCurrentLevel++;
        gCompletedLevels++;
    }
    genWorld();
    buildGeom();
    firstMouse = true;
}

// The rivals winched the squad out: the run is over, show the report.
inline void hostFinishRun() {
    gRunComplete = true;
    playerEscaped = true;
    if (multiState == MULTI_IN_GAME) {
        for (int i = 0; i < 3; i++) hostSyncFeatureState();
    }
    glfwSetInputMode(gWin, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

#include "heist_host.h"
