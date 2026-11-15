#pragma once
// In-game guidance for regular (non-debug) play: squad status lines, interaction prompts,
// objective markers and the downed / ghost state.

#include "player_vitals.h"

struct ObjectiveMarker {
    Vec3 pos;
    const char* label;
};

inline constexpr int MAX_OBJECTIVE_MARKERS = 16;
inline constexpr float MARKER_NEAR_RANGE = 14.0f;    // always shown with line of sight
inline constexpr float MARKER_SCANNER_RANGE = 90.0f; // shown through walls while the scanner is out

inline int collectObjectiveMarkers(ObjectiveMarker* out) {
    int n = 0;
    auto add = [&](const Vec3& pos, const char* label) {
        if (n < MAX_OBJECTIVE_MARKERS) out[n++] = {pos, label};
    };

    if (multiState == MULTI_IN_GAME && !coop.doorOpen) {
        add(coop.switches[0], "DOOR SWITCH");
        add(coop.switches[1], "DOOR SWITCH");
    }
    if (npcLostSurvivorActive && !npcLostSurvivorEscorted) add(npcLostSurvivorPos, "SURVIVOR");
    if (isStoryExitReady()) {
        add(coop.doorPos, "EXIT");
        return n;
    }

    if (isLevelZero(gCurrentLevel)) {
        static const char* nodeNames[3] = {"NODE A", "NODE B", "NODE C"};
        for (int i = 0; i < 3; i++) {
            if (!level1NodeDone[i]) add(level1Nodes[i], nodeNames[i]);
        }
        if (!level1NodeDone[1] && level1NodeStage[1] > 0) {
            add(level1SyncSwitches[0], "SYNC SWITCH");
            add(level1SyncSwitches[1], "SYNC SWITCH");
        }
        if (level1HoldActive) add(level1Nodes[0], "HOLD ZONE");
        return n;
    }

    if (!level2BatteryInstalled) add(level2BatteryNode, "LIFT BATTERY");
    if (!level2FusePanelPowered) add(level2PowerNode, "POWER PANEL");
    for (int i = 0; i < 3; i++) {
        if (!level2FuseDone[i]) add(level2FuseNodes[i], "FUSE SLOT");
    }
    if (!level2AccessReady) add(level2AccessNode, "SECURITY ACCESS");
    bool liftReady = level2BatteryInstalled && level2FuseCount >= 3 && level2AccessReady;
    if (liftReady) add(level2LiftNode, level2HoldActive ? "HOLD LIFT" : "LIFT");
    return n;
}

inline void drawObjectiveMarkers() {
    ObjectiveMarker markers[MAX_OBJECTIVE_MARKERS];
    int count = collectObjectiveMarkers(markers);
    bool scannerOut = activeDeviceSlot == 2 && !scannerOverheated;

    for (int i = 0; i < count; i++) {
        Vec3 d = markers[i].pos - cam.pos;
        d.y = 0.0f;
        float dist = d.len();
        Vec3 labelPos = markers[i].pos + Vec3(0.0f, 1.3f, 0.0f);
        bool inSight = dist < MARKER_NEAR_RANGE && hasLabelLineOfSight(labelPos);
        bool scanned = scannerOut && dist < MARKER_SCANNER_RANGE;
        if (!inSight && !scanned) continue;

        float sx = 0.0f, sy = 0.0f;
        if (!projectToScreen(labelPos, sx, sy)) continue;
        char buf[48];
        snprintf(buf, sizeof(buf), "%s %.0fm", markers[i].label, dist);
        float alpha = inSight ? 0.95f : 0.62f;
        drawHudTextCentered(buf, sx, sy, 1.05f, 0.55f, 0.90f, 0.95f, alpha);
        drawHudTextCentered("v", sx, sy - 0.045f, 1.05f, 0.55f, 0.90f, 0.95f, alpha);
    }

    if (multiState != MULTI_IN_GAME) return;
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
        unsigned char flags = netMgr.players[p].flags;
        if ((flags & PLAYER_FLAG_DOWNED) == 0 || (flags & PLAYER_FLAG_DEAD) != 0) continue;
        float sx = 0.0f, sy = 0.0f;
        if (!projectToScreen(netMgr.players[p].pos + Vec3(0.0f, 1.0f, 0.0f), sx, sy)) continue;
        drawHudTextCentered("DOWNED - REVIVE", sx, sy, 1.15f, 0.98f, 0.42f, 0.36f, 0.95f);
    }
}

inline void drawStatusLines() {
    if (trapStatusTimer > 0.0f && trapStatusText[0] != '\0') {
        float a = trapStatusTimer < 1.0f ? trapStatusTimer : 1.0f;
        drawHudTextCentered(trapStatusText, 0.0f, 0.66f, 1.22f, 0.92f, 0.84f, 0.58f, 0.95f * a);
    }
    if (echoStatusTimer > 0.0f && echoStatusText[0] != '\0') {
        float a = echoStatusTimer < 1.0f ? echoStatusTimer : 1.0f;
        drawHudTextCentered(echoStatusText, 0.0f, 0.59f, 1.10f, 0.70f, 0.86f, 0.90f, 0.92f * a);
    }
}

inline void drawPrompt(const char* text, float r = 0.80f, float g = 0.84f, float b = 0.62f) {
    drawHudTextCentered(text, 0.0f, -0.43f, 1.30f, r, g, b, 0.94f);
}

inline void drawDownedState() {
    char buf[64];
    snprintf(buf, sizeof(buf), "DOWNED %.0fs", playerDownedTimer);
    drawHudTextCentered(buf, 0.0f, -0.22f, 1.45f, 0.96f, 0.52f, 0.44f, 0.95f);
    if (gReviveProgress > 0.0f) {
        snprintf(buf, sizeof(buf), "TEAMMATE REVIVING %.0f%%", gReviveProgress * 100.0f);
        drawHudTextCentered(buf, 0.0f, -0.30f, 1.2f, 0.70f, 0.92f, 0.70f, 0.94f);
    } else if (invPlush > 0) {
        drawHudTextCentered("[E] USE PLUSH TO SELF-REVIVE", 0.0f, -0.30f, 1.2f, 0.90f, 0.72f, 0.62f, 0.92f);
    } else if (multiState == MULTI_IN_GAME) {
        drawHudTextCentered("A TEAMMATE CAN REVIVE YOU [HOLD E]", 0.0f, -0.30f, 1.2f, 0.90f, 0.72f, 0.62f, 0.92f);
    } else {
        drawHudTextCentered("NO PLUSH TO SELF-REVIVE", 0.0f, -0.30f, 1.2f, 0.90f, 0.72f, 0.62f, 0.92f);
    }
}

// One prompt at a time, most urgent first.
inline void drawInteractionPrompt() {
    if (gCoopDead) {
        drawHudTextCentered("YOU ARE DEAD", 0.0f, -0.22f, 1.45f, 0.96f, 0.52f, 0.44f, 0.95f);
        drawHudTextCentered("GHOST: YOU RETURN WHEN THE SQUAD REACHES THE NEXT LEVEL", 0.0f, -0.30f, 1.1f, 0.90f, 0.72f, 0.62f, 0.92f);
        return;
    }
    if (playerDowned) {
        drawDownedState();
        return;
    }
    if (gReviveCandidate >= 0) {
        const char* name = netMgr.players[gReviveCandidate].name[0] ? netMgr.players[gReviveCandidate].name : "TEAMMATE";
        char buf[80];
        snprintf(buf, sizeof(buf), netMgr.localReviveTarget ? "REVIVING %s..." : "[HOLD E] REVIVE %s", name);
        drawPrompt(buf, 0.70f, 0.92f, 0.70f);
        return;
    }
    if (nearbyWorldItemId >= 0) {
        drawPrompt(worldItemPickupPrompt(nearbyWorldItemType));
        return;
    }

    char prompt[96];
    buildVoidShiftInteractPrompt(cam.pos, prompt, 96);
    if (prompt[0] != '\0') {
        drawPrompt(prompt);
        return;
    }

    bool coopGame = multiState == MULTI_IN_GAME;
    if (coopGame && !coop.doorOpen) {
        bool onSwitch = nearPoint2D(cam.pos, coop.switches[0], 2.4f) || nearPoint2D(cam.pos, coop.switches[1], 2.4f);
        if (onSwitch) {
            drawPrompt("DOOR SWITCH HELD: A TEAMMATE MUST STAND ON THE OTHER ONE");
            return;
        }
    }
    if (!nearPoint2D(cam.pos, coop.doorPos, 2.4f)) return;
    if (coopGame && !coop.doorOpen) drawPrompt("EXIT LOCKED: STAND ON BOTH DOOR SWITCHES", 0.88f, 0.72f, 0.58f);
    else if (!isStoryExitReady()) drawPrompt("EXIT LOCKED: COMPLETE THE CONTRACT", 0.88f, 0.72f, 0.58f);
    else drawPrompt(isLevelZero(gCurrentLevel) ? "[E] DESCEND TO PARKING" : "[E] EXTRACT", 0.75f, 0.88f, 0.70f);
}

inline void drawGameplayGuidance() {
    drawObjectiveMarkers();
    drawStatusLines();
    drawInteractionPrompt();
}
