#pragma once
// Monster senses: who can be hunted, who is looking, what can be heard.
// The monsters never read player positions directly; everything they know comes from here
// (sight with line of sight, noise with a radius) or from a director hint.

#include <cmath>
#include "math.h"
#include "ai_nav.h"

// One huntable squad member, filled by the host every frame.
struct AiTarget {
    Vec3 pos;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float noiseRadius = 0.0f; // how far the footsteps carry right now, meters
    float isolation = 99.0f;  // distance to the nearest standing teammate (99 when alone)
    bool flashlight = false;
    bool crouched = false;
    int id = 0;
};

inline constexpr int AI_MAX_TARGETS = 8;
inline constexpr int AI_MAX_NOISES = 12;

// A one-shot sound: squeak, callout, shove, ping, a crawler bonking a wall.
struct AiNoise {
    Vec3 pos;
    float radius = 0.0f;
    float ttl = 0.0f;
    int sourceId = -1; // player id, -1 for the world
};

// How far a sound carries, meters. Tuned against the 5 m cell: walking is heard two cells away,
// sprinting across a whole hall, crouching only at arm's length.
inline constexpr float NOISE_CROUCH = 2.5f;
inline constexpr float NOISE_WALK = 9.0f;
inline constexpr float NOISE_SPRINT = 21.0f;
inline constexpr float NOISE_SQUEAK = 26.0f;
inline constexpr float NOISE_CALLOUT = 16.0f;
inline constexpr float NOISE_SHOVE = 12.0f;
inline constexpr float NOISE_PING = 14.0f;
inline constexpr float NOISE_WALL_MUFFLE = 0.6f; // radius multiplier without line of sight

inline constexpr float PLAYER_WALK_SPEED = 4.0f;
inline constexpr float PLAYER_SPRINT_SPEED = 6.4f;

// Footstep loudness from how fast somebody actually moves. Works for remote players too:
// the host only knows their positions, and that is enough.
inline float footstepNoiseRadius(float speed, bool crouched, bool quietSteps) {
    float radius = 0.0f;
    if (speed < 0.6f) radius = 0.0f;
    else if (crouched) radius = NOISE_CROUCH;
    else if (speed < PLAYER_WALK_SPEED * 1.25f) radius = NOISE_WALK;
    else radius = NOISE_SPRINT;
    if (quietSteps) radius *= 0.75f;
    return radius;
}

// The on-screen test the "do not look away" rules use. Half angle is a little wider than
// the real horizontal FOV, so a monster at the edge of the screen still counts as watched.
inline constexpr float LOOK_CONE_DOT = 0.60f;
inline constexpr float LOOK_MAX_PITCH = 1.05f;

inline bool isInViewCone(const Vec3& eye, float yaw, float pitch, const Vec3& point, float maxDist) {
    float dx = point.x - eye.x, dz = point.z - eye.z;
    float distSq = dx * dx + dz * dz;
    if (distSq > maxDist * maxDist) return false;
    if (distSq < 0.25f) return true;
    if (fabsf(pitch) > LOOK_MAX_PITCH) return false; // staring at the floor or the ceiling
    float inv = 1.0f / sqrtf(distSq);
    float facing = (dx * sinf(yaw) + dz * cosf(yaw)) * inv;
    return facing >= LOOK_CONE_DOT;
}

template <typename CellAt>
inline bool targetLooksAt(const AiTarget& t, const Vec3& point, float maxDist, float cs, CellAt&& cellAt) {
    if (!isInViewCone(t.pos, t.yaw, t.pitch, point, maxDist)) return false;
    return navLineOfSight(t.pos.x, t.pos.z, point.x, point.z, cs, cellAt);
}

// In the dark you only see what your flashlight shows.
template <typename CellAt>
inline bool targetSeesInLight(const AiTarget& t, const Vec3& point, float maxDist, bool lightsOut, float cs, CellAt&& cellAt) {
    if (lightsOut && !t.flashlight) return false;
    float reach = lightsOut ? (maxDist < 14.0f ? maxDist : 14.0f) : maxDist;
    return targetLooksAt(t, point, reach, cs, cellAt);
}

// Radius at which the listener still hears the sound, 0 when it does not.
template <typename CellAt>
inline float heardLoudness(const Vec3& listener, const Vec3& source, float radius, float hearingMul, float cs, CellAt&& cellAt) {
    if (radius <= 0.0f) return 0.0f;
    float dx = source.x - listener.x, dz = source.z - listener.z;
    float dist = sqrtf(dx * dx + dz * dz);
    float reach = radius * hearingMul;
    if (dist > reach) return 0.0f;
    if (!navLineOfSight(listener.x, listener.z, source.x, source.z, cs, cellAt)) {
        reach *= NOISE_WALL_MUFFLE;
        if (dist > reach) return 0.0f;
    }
    return 1.0f - dist / reach;
}

// Thief-style awareness: evidence accumulates and fades instead of flipping a switch,
// which leaves a grey zone where the monster is suspicious but not yet on you.
inline constexpr float AWARE_SUSPICIOUS = 0.35f;
inline constexpr float AWARE_ALERT = 1.0f;
inline constexpr float AWARE_DECAY_PER_SEC = 0.10f;

// Evidence per second from seeing a target: close, lit and upright targets are noticed faster.
inline float sightEvidencePerSec(float dist, float range, bool flashlight, bool crouched) {
    if (dist >= range) return 0.0f;
    float closeness = 1.0f - dist / range;
    float gain = 0.45f + 2.4f * closeness * closeness;
    if (flashlight) gain *= 1.6f;
    if (crouched) gain *= 0.6f;
    return gain;
}

inline float stepAwareness(float awareness, float evidencePerSec, float dt) {
    if (evidencePerSec > 0.0f) awareness += evidencePerSec * dt;
    else awareness -= AWARE_DECAY_PER_SEC * dt;
    if (awareness < 0.0f) awareness = 0.0f;
    if (awareness > AWARE_ALERT) awareness = AWARE_ALERT;
    return awareness;
}

// Distance from each target to its nearest squadmate. Loners are what the Shadow picks.
inline void computeIsolation(AiTarget* targets, int count) {
    for (int i = 0; i < count; i++) {
        float best = 99.0f;
        for (int j = 0; j < count; j++) {
            if (i == j) continue;
            float dx = targets[i].pos.x - targets[j].pos.x;
            float dz = targets[i].pos.z - targets[j].pos.z;
            float d = sqrtf(dx * dx + dz * dz);
            if (d < best) best = d;
        }
        targets[i].isolation = best;
    }
}
