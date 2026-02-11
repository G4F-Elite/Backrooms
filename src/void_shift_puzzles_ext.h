#pragma once

#include <cstdio>

inline void initLevel1PuzzleStages() {
    level1NodeStage[0] = level1NodeStage[1] = level1NodeStage[2] = 0;
    level1NodeGoal[0] = 3; // breaker calibration pulses
    level1NodeGoal[1] = 2; // phone sync pulses
    level1NodeGoal[2] = 3; // resonance lock pulses
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
        if (!(echoPlayback || resonatorMode == RESONATOR_PING)) {
            setTrapStatus("NODE B NEEDS PING OR ECHO");
            return true;
        }
        level1NodeStage[nodeIndex]++;
        setTrapStatus("NODE B: PHONE SYNC ACTIVE");
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
        std::snprintf(out, outSize, "[E] NODE B PHONE SYNC %d/%d", level1NodeStage[1], level1NodeGoal[1]);
        return;
    }
    std::snprintf(out, outSize, "[E] NODE C RESONANCE LOCK %d/%d", level1NodeStage[2], level1NodeGoal[2]);
}
