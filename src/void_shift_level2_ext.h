#pragma once

#include <cstdio>

inline bool hasLevel2BatteryAssistNearby() {
    if (multiState != MULTI_IN_GAME) return false;
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
        if (nearPoint2D(netMgr.players[p].pos, level2BatteryNode, 2.8f)) return true;
    }
    return false;
}

inline void initLevel2PuzzleStages() {
    level2BatteryStage = 0;
    level2FusePanelPowered = false;
}

inline bool processLevel2Step(const Vec3& playerPos) {
    if (!level2BatteryInstalled && nearPoint2D(playerPos, level2BatteryNode, 2.4f)) {
        if (level2BatteryStage == 0) {
            level2BatteryStage = 1;
            setTrapStatus("BATTERY: RELEASE SAFETY LOCK");
            addAttention(4.0f);
            return true;
        }
        if (level2BatteryStage == 1) {
            if (!(echoPlayback || hasLevel2BatteryAssistNearby())) {
                setTrapStatus("BATTERY: NEED SECOND HAND OR ECHO");
                return true;
            }
            level2BatteryInstalled = true;
            level2BatteryStage = 2;
            setTrapStatus("BATTERY INSTALLED");
            awardArchivePoints(20, "HEAVY OBJECTIVE COMPLETE");
            addAttention(12.0f);
            return true;
        }
    }

    if (!level2FusePanelPowered && nearPoint2D(playerPos, level2PowerNode, 2.3f)) {
        level2FusePanelPowered = true;
        setTrapStatus("FUSE PANEL POWERED");
        addAttention(6.0f);
        return true;
    }

    return false;
}

inline bool buildLevel2ActionPrompt(const Vec3& playerPos, char* out, int outSize) {
    if (!out || outSize < 2) return false;

    if (!level2BatteryInstalled && nearPoint2D(playerPos, level2BatteryNode, 2.4f)) {
        if (level2BatteryStage == 0) std::snprintf(out, outSize, "[E] BATTERY STAGE 1: RELEASE LOCK");
        else if (level2BatteryStage == 1) std::snprintf(out, outSize, "[E] BATTERY STAGE 2: LIFT WITH ECHO");
        else std::snprintf(out, outSize, "[E] BATTERY READY");
        return true;
    }
    if (!level2FusePanelPowered && nearPoint2D(playerPos, level2PowerNode, 2.3f)) {
        std::snprintf(out, outSize, "[E] POWER FUSE PANEL");
        return true;
    }
    return false;
}
