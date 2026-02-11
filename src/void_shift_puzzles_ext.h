#pragma once

#include <cstdio>

inline void initLevel1PuzzleStages() {
    level1NodeStage[0] = level1NodeStage[1] = level1NodeStage[2] = 0;
    level1NodeGoal[0] = 3; // breaker calibration pulses
    level1NodeGoal[1] = 2; // phone sync pulses
    level1NodeGoal[2] = 3; // resonance lock pulses
    level1SyncTimer = 0.0f;
    level1SyncSwitches[0] = Vec3(level1Nodes[1].x + CS * 0.75f, 0.0f, level1Nodes[1].z);
    level1SyncSwitches[1] = Vec3(level1Nodes[1].x - CS * 0.75f, 0.0f, level1Nodes[1].z);
}

inline bool isLevel1SwitchHeld(int idx) {
    if (idx < 0 || idx > 1) return false;
    if (nearPoint2D(cam.pos, level1SyncSwitches[idx], 1.9f)) return true;
    if (echoPlayback && nearPoint2D(echoGhostPos, level1SyncSwitches[idx], 1.9f)) return true;
    if (multiState == MULTI_IN_GAME) {
        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
            if (nearPoint2D(netMgr.players[p].pos, level1SyncSwitches[idx], 1.9f)) return true;
        }
    }
    return false;
}

inline void updateLevel1SyncSwitchProgress(float dt) {
    if (level1NodeDone[1] || level1NodeStage[1] <= 0) return;
    bool left = isLevel1SwitchHeld(0), right = isLevel1SwitchHeld(1);
    if (left && right) {
        level1SyncTimer += dt;
        if (level1SyncTimer >= 4.0f) {
            level1SyncTimer = 0.0f;
            level1NodeStage[1] = level1NodeGoal[1];
            level1NodeDone[1] = true;
            setTrapStatus("NODE B SYNC SWITCHES LOCKED");
            awardArchivePoints(20, "SYNC PUZZLE COMPLETE");
        }
    } else {
        level1SyncTimer -= dt * 1.5f;
        if (level1SyncTimer < 0.0f) level1SyncTimer = 0.0f;
    }
}

inline bool isLevel1HoldMaintained() {
    Vec3 anchor = level1Nodes[0];
    if (nearPoint2D(cam.pos, anchor, 5.5f)) return true;
    if (echoPlayback && nearPoint2D(echoGhostPos, anchor, 5.5f)) return true;
    if (multiState == MULTI_IN_GAME) {
        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
            if (nearPoint2D(netMgr.players[p].pos, anchor, 5.5f)) return true;
        }
    }
    return false;
}

inline bool processLevel1NodeStage(int nodeIndex) {
    if (nodeIndex < 0 || nodeIndex > 2 || level1NodeDone[nodeIndex]) return false;

    if (nodeIndex == 0) {
        if (resonatorMode != RESONATOR_SCAN) {
            setTrapStatus("NODE A NEEDS SCAN MODE");
            return true;
        }
        level1NodeStage[nodeIndex]++;
        setTrapStatus("NODE A: BREAKER CALIBRATING");
    } else if (nodeIndex == 1) {
        if (resonatorMode != RESONATOR_PING) {
            setTrapStatus("NODE B NEEDS PING MODE");
            return true;
        }
        level1NodeStage[nodeIndex] = 1;
        setTrapStatus("NODE B: HOLD BOTH SYNC SWITCHES");
        setEchoStatus("USE ECHO OR TEAMMATE FOR SECOND SWITCH");
    } else {
        if (attentionLevel > 70.0f) {
            setTrapStatus("NODE C UNSTABLE: LOWER ATTENTION");
            return true;
        }
        level1NodeStage[nodeIndex]++;
        setTrapStatus("NODE C: RESONANCE LOCK");
    }

    if (level1NodeStage[nodeIndex] >= level1NodeGoal[nodeIndex]) {
        level1NodeDone[nodeIndex] = true;
        awardArchivePoints(15, "NODE PHASE COMPLETE");
        addAttention(7.0f);
    }
    return true;
}

inline void buildLevel1NodeActionPrompt(int nodeIndex, char* out, int outSize) {
    if (!out || outSize < 2) return;
    if (nodeIndex < 0 || nodeIndex > 2) {
        out[0] = '\0';
        return;
    }

    if (nodeIndex == 0) {
        std::snprintf(out, outSize, "[E] NODE A BREAKER %d/%d", level1NodeStage[0], level1NodeGoal[0]);
        return;
    }
    if (nodeIndex == 1) {
        if (level1NodeStage[1] <= 0) std::snprintf(out, outSize, "[E] NODE B START PHONE SYNC");
        else std::snprintf(out, outSize, "NODE B SYNC HOLD %.1fs/4.0s", level1SyncTimer);
        return;
    }
    std::snprintf(out, outSize, "[E] NODE C RESONANCE LOCK %d/%d", level1NodeStage[2], level1NodeGoal[2]);
}
