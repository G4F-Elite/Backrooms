#pragma once
// In-game guidance for regular (non-debug) play: world markers, interaction prompts and
// the downed / ghost state. Everything is a pictogram plus, at most, a number.

#include "player_vitals.h"
#include "hud_spectator.h"
#include "hud_heist.h"

struct ObjectiveMarker {
    Vec3 pos;
    int icon;
    bool primary;   // the one the screen-edge pointer leads to
    HudColor color;
};

inline constexpr int MAX_OBJECTIVE_MARKERS = 24;
inline constexpr float MARKER_NEAR_RANGE = 14.0f; // full brightness with line of sight
inline constexpr float MARKER_EDGE_X = 0.86f;     // off-screen pointer is pinned inside this box
inline constexpr float MARKER_EDGE_Y = 0.72f;

inline bool heistAnyoneCarriesCore() {
    for (int p = 0; p < MAX_PLAYERS; p++) if (heistCarriesCore(p)) return true;
    return false;
}

inline int collectObjectiveMarkers(ObjectiveMarker* out) {
    int n = 0;
    auto add = [&](const Vec3& pos, int icon, bool primary, HudColor c) {
        if (n < MAX_OBJECTIVE_MARKERS) out[n++] = {pos, icon, primary, c};
    };
    bool carrying = carryCount(heistMyCarry()) > 0;

    if (heist.stage == HEIST_TUT_GARAGE) {
        add(heist.portalPos, HUD_ICON_PORTAL, true, HUD_COOL);
    } else if (heist.stage == HEIST_TUT_HAUL) {
        if (carrying) add(heist.padPos, HUD_ICON_PALLET, true, HUD_GOOD);
        else add(heist.lessonPos[tutorLessonForDelivery(heist.tutDelivered)], HUD_ICON_LOOT, true, HUD_PAPER);
    } else if (heist.stage == HEIST_TUT_CORE) {
        if (heistCarriesCore(heistMe())) add(heist.padPos, HUD_ICON_PALLET, true, HUD_GOOD);
        else if (!heistAnyoneCarriesCore()) add(heist.corePos, HUD_ICON_CORE, true, HUD_COOL);
    } else if (heist.stage == HEIST_RUN) {
        // The rig: the marker sits on its back door. A breakdown takes over as the main goal.
        bool ready = heistDrillReady();
        bool broken = heist.rigFault != RIG_OK;
        Vec3 door = gRigInCabin ? rigWorld(RIG_HOPPER) : rigWorld(0.0f, RIG_CABIN_BACK);
        if (broken) add(rigWorld(rigFaultPoint(heist.rigFault)), HUD_ICON_WRENCH, true, HUD_BAD);
        add(door, ready ? HUD_ICON_ARROW_DOWN : HUD_ICON_DRILL, !broken && (carrying || ready), ready ? HUD_GOOD : HUD_PAPER);
        if (heist.hasKiosk) add(heist.kioskPos, heistCanAffordExtraction() ? HUD_ICON_EXTRACT : HUD_ICON_SHOP, false, heistCanAffordExtraction() ? HUD_GOOD : HUD_DIM);
    }

    // Scanner out: loot and junk piles nearby show through walls. The radar on the desk in
    // the rig does the same for whoever is home, and reaches further.
    bool scanning = activeDeviceSlot == 2 && !scannerOverheated;
    if (scanning || gRigAtRadar) {
        float range = gRigAtRadar ? RIG_RADAR_RANGE : HEIST_SCAN_MARK_RANGE;
        for (const auto& it : worldItems) {
            if (!it.active || !nearPoint2D(cam.pos, it.pos, range)) continue;
            if (isLootItemType(it.type)) add(it.pos, lootIcon(lootFromItemType(it.type)), false, HUD_COOL);
            else if (it.type == ITEM_STASH) add(it.pos, HUD_ICON_STASH, false, HUD_COOL);
        }
    }
    return n;
}

// Where to pin the pointer for a target that is off screen (or behind the camera).
// Returns false while the target is comfortably in view.
inline bool objectiveEdgePointer(const Vec3& worldPos, float& ex, float& ey) {
    Vec3 d = worldPos - cam.pos;
    float cy = mCos(gRenderViewYaw), sy = mSin(gRenderViewYaw);
    float cp = mCos(gRenderViewPitch), sp = mSin(gRenderViewPitch);
    float cx = d.z * sy - d.x * cy; // same screen-right axis as projectToScreen
    float cz0 = d.x * sy + d.z * cy;
    float cyv = d.y * cp - cz0 * sp;
    float cz = d.y * sp + cz0 * cp;

    float t = tanf(1.2f * 0.5f);
    float asp = (float)winW / (float)winH;
    if (cz <= 0.05f) {
        // Behind the player: point to the side that is the shorter turn.
        ex = cx >= 0.0f ? MARKER_EDGE_X : -MARKER_EDGE_X;
        ey = 0.0f;
        return true;
    }
    float nx = cx / (cz * asp * t);
    float ny = cyv / (cz * t);
    if (fabsf(nx) <= MARKER_EDGE_X && fabsf(ny) <= MARKER_EDGE_Y) return false;
    float scale = 1.0f;
    if (fabsf(nx) > MARKER_EDGE_X) scale = MARKER_EDGE_X / fabsf(nx);
    if (fabsf(ny) * scale > MARKER_EDGE_Y) scale = MARKER_EDGE_Y / fabsf(ny);
    ex = nx * scale;
    ey = ny * scale;
    return true;
}

inline void drawMarkerDistance(float dist, float x, float y, HudColor c, float a) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", dist);
    drawHudTextCentered(buf, x, y, 1.05f, c.r, c.g, c.b, a);
}

inline void drawSquadMarkers() {
    if (multiState != MULTI_IN_GAME) return;
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (p == netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
        unsigned char flags = netMgr.players[p].flags;
        if ((flags & PLAYER_FLAG_DOWNED) == 0 || (flags & PLAYER_FLAG_DEAD) != 0) continue;
        float sx = 0.0f, sy = 0.0f;
        if (!projectToScreen(netMgr.players[p].pos + Vec3(0.0f, 1.0f, 0.0f), sx, sy)) continue;
        drawIcon(HUD_ICON_REVIVE, sx, sy, 0.08f, HUD_BAD.r, HUD_BAD.g, HUD_BAD.b, 0.65f + 0.3f * hudPulse(5.0f));
    }
    if (!netMgr.pingMarkReceived || netMgr.pingMarkTtl <= 0.0f) return;
    float psx = 0.0f, psy = 0.0f;
    if (!projectToScreen(netMgr.pingMarkPos + Vec3(0.0f, 1.5f, 0.0f), psx, psy)) return;
    float alpha = 0.35f + 0.6f * (netMgr.pingMarkTtl / 6.0f);
    drawIcon(HUD_ICON_PING, psx, psy, 0.09f, HUD_PAPER.r, HUD_PAPER.g, HUD_PAPER.b, alpha > 0.95f ? 0.95f : alpha);
}

inline void drawObjectiveMarkers() {
    ObjectiveMarker markers[MAX_OBJECTIVE_MARKERS];
    int count = collectObjectiveMarkers(markers);
    int primary = -1;
    float primaryDist = 0.0f;
    for (int i = 0; i < count; i++) {
        Vec3 d = markers[i].pos - cam.pos;
        d.y = 0.0f;
        float dist = d.len();
        if (markers[i].primary && primary < 0) {
            primary = i;
            primaryDist = dist;
        }
        Vec3 labelPos = markers[i].pos + Vec3(0.0f, markers[i].primary ? 2.0f : 1.0f, 0.0f);
        float sx = 0.0f, sy = 0.0f;
        if (dist < 2.0f || !projectToScreen(labelPos, sx, sy)) continue;
        bool inSight = dist < MARKER_NEAR_RANGE && hasLabelLineOfSight(labelPos);
        float alpha = inSight ? 0.95f : (markers[i].primary ? 0.70f : 0.50f);
        float size = markers[i].primary ? 0.085f : 0.06f;
        drawIcon(markers[i].icon, sx, sy, size, markers[i].color.r, markers[i].color.g, markers[i].color.b, alpha);
        if (markers[i].primary) drawMarkerDistance(dist, sx, sy - size * 0.5f - 0.035f, markers[i].color, alpha);
    }

    // The main goal also gets a pointer on the screen edge while it is out of view.
    float ex = 0.0f, ey = 0.0f;
    if (primary >= 0 && objectiveEdgePointer(markers[primary].pos + Vec3(0.0f, 2.0f, 0.0f), ex, ey)) {
        const ObjectiveMarker& m = markers[primary];
        float pulse = 0.75f + 0.2f * hudPulse(4.0f);
        drawIcon(m.icon, ex, ey, 0.09f, m.color.r, m.color.g, m.color.b, pulse);
        drawHudTextCentered(ex < 0.0f ? "<" : ">", ex + (ex < 0.0f ? -0.05f : 0.05f), ey - 0.015f, 1.5f, m.color.r, m.color.g, m.color.b, pulse);
        drawMarkerDistance(primaryDist, ex, ey - 0.085f, m.color, pulse);
    }
    drawSquadMarkers();
}

// Old status lines are sentences: they only show up with debug mode on.
inline void drawStatusLines() {
    if (!settings.debugMode) return;
    if (trapStatusTimer > 0.0f && trapStatusText[0] != '\0') {
        float a = trapStatusTimer < 1.0f ? trapStatusTimer : 1.0f;
        drawHudTextCentered(trapStatusText, 0.0f, 0.66f, 1.0f, 0.92f, 0.84f, 0.58f, 0.8f * a);
    }
    if (echoStatusTimer > 0.0f && echoStatusText[0] != '\0') {
        float a = echoStatusTimer < 1.0f ? echoStatusTimer : 1.0f;
        drawHudTextCentered(echoStatusText, 0.0f, 0.61f, 0.95f, 0.70f, 0.86f, 0.90f, 0.8f * a);
    }
}

inline void drawDownedState() {
    const float y = -0.24f;
    drawIconC(HUD_ICON_DOWNED, -0.05f, y, 0.11f, HUD_BAD, 0.7f + 0.25f * hudPulse(5.0f));
    drawHudNumber((int)playerDownedTimer, 0.01f, y - 0.03f, 2.2f, HUD_BAD.r, HUD_BAD.g, HUD_BAD.b);
    if (gReviveProgress > 0.0f) {
        drawIconC(HUD_ICON_REVIVE, -0.14f, y - 0.13f, 0.08f, HUD_GOOD);
        drawGauge(-0.09f, y - 0.15f, 0.16f, y - 0.11f, gReviveProgress, HUD_GOOD.r, HUD_GOOD.g, HUD_GOOD.b);
    } else if (invPlush > 0) {
        drawKeyCap(keyNameForUi(settings.binds.interact), -0.06f, y - 0.13f, 0.07f);
        drawIconC(HUD_ICON_PLUSH, 0.03f, y - 0.13f, 0.09f, HUD_PAPER);
    } else if (multiState == MULTI_IN_GAME) {
        drawIconC(HUD_ICON_GROUP, -0.04f, y - 0.13f, 0.08f, HUD_DIM);
        drawIconC(HUD_ICON_REVIVE, 0.04f, y - 0.13f, 0.08f, HUD_DIM);
    }
}

// One prompt at a time, most urgent first.
inline void drawInteractionPrompt() {
    if (gCoopDead) {
        drawSpectatorHud();
        return;
    }
    if (playerDowned) {
        drawDownedState();
        return;
    }
    if (gReviveCandidate >= 0) {
        const char* name = netMgr.players[gReviveCandidate].name[0] ? netMgr.players[gReviveCandidate].name : "";
        drawPromptRow(keyNameForUi(settings.binds.interact), HUD_ICON_REVIVE, -1, 0, HUD_GOOD, false);
        drawHudTextCentered(name, 0.0f, -0.52f, 1.1f, HUD_GOOD.r, HUD_GOOD.g, HUD_GOOD.b, 0.9f);
        return;
    }
    drawHeistPrompt();
}

inline void drawGameplayGuidance() {
    drawObjectiveMarkers();
    drawStatusLines();
    drawInteractionPrompt();
    drawHeistHud();
}
