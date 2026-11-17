#pragma once
// HUD for dead co-op players: who the spectator camera is on, what that monster is thinking,
// and the controls. Included from hud_gameplay.h.

#include "spectator_cam.h"
#include "hud_heist.h"

// Closest monster to a point, with its distance (nullptr when the level is empty).
inline const Entity* nearestMonsterTo(const Vec3& pos, float& outDist) {
    const Entity* best = nullptr;
    outDist = 1e9f;
    for (const auto& e : entityMgr.entities) {
        if (!e.active) continue;
        float d = brainDist(e.pos, pos);
        if (d < outDist) {
            outDist = d;
            best = &e;
        }
    }
    return best;
}

inline void drawSpectatorTargetLines() {
    char buf[112];
    if (gSpectator.kind == SPEC_MONSTER) {
        const Entity* e = spectatorFindMonster(gSpectator.id);
        if (!e) return;
        snprintf(buf, sizeof(buf), "%s", entityTypeName(e->type));
        drawHudTextCentered(buf, 0.0f, 0.76f, 1.45f, 0.96f, 0.52f, 0.44f, 0.95f);
        drawHudTextCentered(entityMoodLine(e->type, e->mood), 0.0f, 0.69f, 1.12f, 0.92f, 0.80f, 0.62f, 0.92f);
        return;
    }

    int p = gSpectator.id;
    const char* name = netMgr.players[p].name[0] ? netMgr.players[p].name : "TEAMMATE";
    snprintf(buf, sizeof(buf), "%s", name);
    drawHudTextCentered(buf, 0.0f, 0.76f, 1.45f, 0.70f, 0.90f, 0.95f, 0.95f);

    // The dead see what the living do not: what is coming for their friend.
    if ((netMgr.players[p].flags & PLAYER_FLAG_DOWNED) != 0) {
        drawIconC(HUD_ICON_REVIVE, 0.0f, 0.68f, 0.08f, HUD_BAD, 0.6f + 0.35f * hudPulse(5.0f));
        return;
    }
    float dist = 0.0f;
    const Entity* threat = nearestMonsterTo(netMgr.players[p].pos, dist);
    if (!threat || dist > 30.0f) return;
    snprintf(buf, sizeof(buf), "%s %.0fm - %s", entityTypeName(threat->type), dist, entityMoodLine(threat->type, threat->mood));
    float urgent = dist < 10.0f ? 1.0f : 0.0f;
    drawHudTextCentered(buf, 0.0f, 0.69f, 1.08f, 0.92f, 0.80f - 0.30f * urgent, 0.62f - 0.24f * urgent, 0.92f);
}

// Controls for the dead, as key caps: next target, zoom, bodycam, walk, squeak.
inline void drawSpectatorKeys(bool riding, bool onPlayer) {
    const float y = -0.82f, h = 0.06f;
    float x = riding ? -0.30f : -0.10f;
    auto cap = [&](const char* key, int icon) {
        drawKeyCap(key, x, y, h, 0.85f);
        drawIconC(icon, x + 0.065f, y, h * 1.1f, HUD_DIM, 0.85f);
        x += 0.17f;
    };
    cap("SPACE", riding ? HUD_ICON_GHOST : HUD_ICON_EYE);
    cap("Q", HUD_ICON_NOISE);
    if (!riding) return;
    cap("A/D", HUD_ICON_GROUP);
    cap("W/S", HUD_ICON_SEARCH);
    if (onPlayer) cap("V", HUD_ICON_PERSON);
}

inline void drawSpectatorHud() {
    // Dead: a ghost until the drill gets its next lamp.
    float pulse = 0.6f + 0.3f * hudPulse(3.0f);
    drawIconC(HUD_ICON_GHOST, -0.08f, -0.62f, 0.09f, HUD_COOL, pulse);
    drawIconC(HUD_ICON_ARROW_RIGHT, 0.0f, -0.62f, 0.045f, HUD_DIM, 0.8f);
    drawIconC(HUD_ICON_DRILL, 0.08f, -0.62f, 0.09f, HUD_PAPER, pulse);
    drawSpectatorKeys(gSpectator.riding, gSpectator.kind == SPEC_PLAYER);
    if (gSpectator.riding) drawSpectatorTargetLines();
}
