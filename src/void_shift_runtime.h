#pragma once
// Attention: how much noise and light the squad has been making. The louder the crew,
// the sooner the floor reacts (blackouts, shifting corridors, something sent to look).

#include <cstdio>

inline void clampAttention() {
    if (attentionLevel < 0.0f) attentionLevel = 0.0f;
    if (attentionLevel > 100.0f) attentionLevel = 100.0f;
}

inline void addAttention(float amount) {
    attentionLevel += amount;
    clampAttention();
}

inline void resetVoidShiftState() {
    attentionLevel = 0.0f;
    attentionEventCooldown = 0.0f;
    initVoidShiftSetpieces();
}

inline void updateAttentionEvents() {
    if (attentionEventCooldown > 0.0f || !heistDangerActive()) return;
    if (rigPosInQuiet(cam.pos)) return; // the engine drowns the crew out: nothing answers the noise here
    bool firstFloor = isLevelZero(gCurrentLevel);
    if (attentionLevel >= 90.0f) {
        if (multiState != MULTI_IN_GAME) entityMgr.spawnEntity(ENTITY_SHADOW, findSpawnPos(cam.pos, 12.0f), nullptr, 0, 0);
        attentionEventCooldown = 12.0f;
    } else if (attentionLevel >= 75.0f) {
        if (firstFloor) {
            lightsOutTimer = 18.0f;
            triggerBlackoutSetpiece(10.0f);
        } else {
            falseDoorTimer = 14.0f;
            triggerCorridorShiftSetpiece(12.0f);
        }
        if (multiState != MULTI_IN_GAME) entityMgr.spawnEntity(ENTITY_CRAWLER, findSpawnPos(cam.pos, 10.0f), nullptr, 0, 0);
        attentionEventCooldown = 10.0f;
    } else if (attentionLevel >= 50.0f) {
        if (firstFloor) {
            falseDoorTimer = 10.0f;
            triggerConferenceCallSetpiece(14.0f);
        }
        attentionEventCooldown = 8.0f;
    }
}

inline void updateVoidShiftSystems(float dt, bool sprinting, bool flashlightActive) {
    if (attentionEventCooldown > 0.0f) {
        attentionEventCooldown -= dt;
        if (attentionEventCooldown < 0.0f) attentionEventCooldown = 0.0f;
    }
    if (sprinting) addAttention(dt * (perkQuietSteps ? 0.75f : 1.1f));
    if (flashlightActive) addAttention(dt * 0.35f);
    addAttention(-dt * 1.4f);
    if (!heistDangerActive()) attentionLevel = 0.0f;

    updateVoidShiftSetpieces(dt);
    updateAttentionEvents();
    clampAttention();
}
