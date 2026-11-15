#pragma once
// Procedural poses for the jointed creature meshes. Pure math, no GL.
// `gait` is a walk-cycle phase (see creatureGaitPhase), `t` is wall-clock seconds.
#include <cmath>
#include "entity_types.h"
#include "creature_mesh.h"

inline float creaturePos(float x) { return x > 0.0f ? x : 0.0f; }

// Rare, sharp head jerk: mostly 0, snaps to 1 for a few frames.
inline float creatureTwitch(float t, float rate, float seedOffset) {
    float cyc = t * rate + seedOffset;
    float f = cyc - floorf(cyc);
    return f < 0.045f ? 1.0f : 0.0f;
}

// Stiff, heavy walk. Arms hang almost dead; when it stops, it stops mid-step.
inline CreaturePose stalkerPose(float gait, float t, EntityState state) {
    CreaturePose p;
    float s = sinf(gait), c = cosf(gait);
    p.rot[CJ_LEG_L].x = -0.34f * s;
    p.rot[CJ_LEG_R].x = 0.34f * s;
    p.rot[CJ_SHIN_L].x = 0.62f * creaturePos(c);
    p.rot[CJ_SHIN_R].x = 0.62f * creaturePos(-c);
    p.rootOffset.y = -0.035f * s * s;
    p.rot[CJ_ROOT].z = 0.025f * s;
    p.rot[CJ_SPINE].x = 0.10f + 0.015f * sinf(t * 0.9f);
    p.rot[CJ_SPINE].z = -0.035f * s;
    p.rot[CJ_ARM_L].x = 0.07f * s;
    p.rot[CJ_ARM_R].x = -0.07f * s;
    p.rot[CJ_ARM_L].z = -0.03f;
    p.rot[CJ_ARM_R].z = 0.03f;
    p.rot[CJ_FOREARM_L].x = -0.10f;
    p.rot[CJ_FOREARM_R].x = -0.16f;
    p.rot[CJ_HEAD].z = 0.30f + 0.04f * sinf(t * 0.37f) + 0.18f * creatureTwitch(t, 0.21f, 0.3f);
    p.rot[CJ_HEAD].x = 0.16f;
    p.rot[CJ_HEAD].y = 0.07f * sinf(t * 0.23f);
    if (state == ENT_ATTACKING) {
        p.rot[CJ_SPINE].x = 0.24f;
        p.rot[CJ_ARM_L].x = -1.30f;
        p.rot[CJ_ARM_R].x = -1.12f;
        p.rot[CJ_FOREARM_L].x = -0.28f;
        p.rot[CJ_FOREARM_R].x = -0.40f;
        p.rot[CJ_HEAD].z = 0.10f;
        p.rot[CJ_HEAD].x = -0.20f;
    } else if (state == ENT_FLEEING) {
        p.rot[CJ_SPINE].x = 0.20f;
        p.rot[CJ_HEAD].x = 0.30f;
    }
    return p;
}

// Diagonal scuttle: opposite limbs move together, the body snakes between them.
inline CreaturePose crawlerPose(float gait, float t, EntityState state) {
    CreaturePose p;
    const float swing = 0.36f, lift = 0.20f;
    float a = gait, b = gait + 3.14159265f;
    p.rot[CJ_ARM_L].y = swing * sinf(a);
    p.rot[CJ_ARM_L].z = -lift * creaturePos(cosf(a));
    p.rot[CJ_LEG_R].y = -swing * sinf(a);
    p.rot[CJ_LEG_R].z = lift * creaturePos(cosf(a));
    p.rot[CJ_ARM_R].y = -swing * sinf(b);
    p.rot[CJ_ARM_R].z = lift * creaturePos(cosf(b));
    p.rot[CJ_LEG_L].y = swing * sinf(b);
    p.rot[CJ_LEG_L].z = -lift * creaturePos(cosf(b));
    p.rot[CJ_ROOT].y = 0.07f * sinf(gait);
    p.rot[CJ_ROOT].z = 0.035f * cosf(gait);
    p.rootOffset.y = 0.012f * sinf(gait * 2.0f);
    p.rot[CJ_HEAD].z = 0.35f * sinf(t * 0.6f) + 0.55f * creatureTwitch(t, 0.37f, 0.6f);
    p.rot[CJ_HEAD].x = 0.10f * sinf(t * 1.3f);
    p.rot[CJ_HEAD].y = -0.12f * sinf(gait);
    if (state == ENT_IDLE) {
        // Coiled before a rush: pressed into the carpet, shaking.
        p.rootOffset.y = -0.055f;
        p.rot[CJ_ROOT].z += 0.012f * sinf(t * 41.0f);
        p.rot[CJ_HEAD].x = -0.25f;
    } else if (state == ENT_ATTACKING) {
        p.rot[CJ_HEAD].x = -0.45f;
        p.rot[CJ_ROOT].x = -0.10f;
    }
    return p;
}

// Does not walk. Stands, leans, and slowly opens its arms when it has you.
inline CreaturePose shadowPose(float t, EntityState state) {
    CreaturePose p;
    p.rot[CJ_ROOT].z = 0.022f * sinf(t * 0.5f);
    p.rot[CJ_ROOT].x = 0.03f + 0.012f * sinf(t * 0.31f);
    p.rot[CJ_HEAD].z = 0.26f + 0.05f * sinf(t * 0.4f);
    p.rot[CJ_HEAD].x = 0.10f;
    float open = (state == ENT_ATTACKING) ? 0.62f + 0.05f * sinf(t * 1.7f) : 0.035f;
    p.rot[CJ_ARM_L].z = -open;
    p.rot[CJ_ARM_R].z = open;
    return p;
}

inline CreaturePose entityPose(const Entity& e, float t) {
    if (e.type == ENTITY_CRAWLER) return crawlerPose(creatureGaitPhase(e.pos, e.yaw, 0.46f), t, e.state);
    if (e.type == ENTITY_SHADOW) return shadowPose(t, e.state);
    return stalkerPose(creatureGaitPhase(e.pos, e.yaw, 0.76f), t, e.state);
}

// Co-op teammate. `walk` is the travelled-distance phase, `moving` 0..1 blends the stride in,
// `crouch` 0..1 folds the legs, `pitch` tilts the head, the right arm aims the flashlight.
inline CreaturePose playerPose(float walk, float moving, float crouch, float pitch, bool flashlight, float t) {
    CreaturePose p;
    float s = sinf(walk) * moving, c = cosf(walk) * moving;
    float stride = 0.48f * (1.0f - 0.45f * crouch);
    p.rot[CJ_LEG_L].x = -stride * s - 1.20f * crouch;
    p.rot[CJ_LEG_R].x = stride * s - 1.20f * crouch;
    p.rot[CJ_SHIN_L].x = 0.75f * creaturePos(c) + 2.05f * crouch;
    p.rot[CJ_SHIN_R].x = 0.75f * creaturePos(-c) + 2.05f * crouch;
    p.rootOffset.y = -0.03f * s * s - 0.43f * crouch;
    p.rot[CJ_SPINE].x = 0.04f + 0.42f * crouch + 0.012f * sinf(t * 1.4f);
    p.rot[CJ_SPINE].y = 0.06f * s;
    p.rot[CJ_ARM_L].x = 0.42f * s - 0.25f * crouch;
    p.rot[CJ_FOREARM_L].x = -0.18f - 0.22f * creaturePos(-s);
    p.rot[CJ_ARM_L].z = -0.05f;
    p.rot[CJ_ARM_R].z = 0.05f;
    if (flashlight) {
        float aim = pitch;
        if (aim > 0.9f) aim = 0.9f;
        if (aim < -0.9f) aim = -0.9f;
        p.rot[CJ_ARM_R].x = -0.42f - 0.35f * crouch;
        p.rot[CJ_FOREARM_R].x = -1.10f - aim * 0.8f + 0.03f * c;
    } else {
        p.rot[CJ_ARM_R].x = -0.42f * s - 0.25f * crouch;
        p.rot[CJ_FOREARM_R].x = -0.18f - 0.22f * creaturePos(s);
    }
    float look = pitch;
    if (look > 1.0f) look = 1.0f;
    if (look < -1.0f) look = -1.0f;
    p.rot[CJ_HEAD].x = -look * 0.75f - 0.38f * crouch;
    return p;
}
