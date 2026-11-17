#pragma once
// Spectator camera math: an orbit camera that does not clip through walls, and target cycling.
// Pure functions so the tests can run them without a window.

#include <cmath>
#include "math.h"
#include "ai_nav.h"

enum SpectatorKind {
    SPEC_PLAYER = 0,
    SPEC_MONSTER = 1
};

struct SpectatorSlot {
    int kind;
    int id; // player slot, or the monster's uid
};

inline constexpr int SPECTATOR_MAX_SLOTS = 24;
inline constexpr float SPECTATOR_MIN_DIST = 1.6f;
inline constexpr float SPECTATOR_MAX_DIST = 6.0f;
inline constexpr float SPECTATOR_DEFAULT_DIST = 3.4f;
inline constexpr float SPECTATOR_MIN_PITCH = -1.2f; // looking down at the target
inline constexpr float SPECTATOR_MAX_PITCH = 0.45f; // looking slightly up from the floor
inline constexpr float SPECTATOR_WALL_MARGIN = 0.28f;
inline constexpr float SPECTATOR_GHOST_DROP = 0.62f; // the little ghost's feet hang this far below its position

inline Vec3 spectatorViewDir(float yaw, float pitch) {
    return Vec3(sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch));
}

inline float spectatorClampPitch(float pitch) {
    if (pitch < SPECTATOR_MIN_PITCH) return SPECTATOR_MIN_PITCH;
    if (pitch > SPECTATOR_MAX_PITCH) return SPECTATOR_MAX_PITCH;
    return pitch;
}

inline float spectatorClampDist(float dist) {
    if (dist < SPECTATOR_MIN_DIST) return SPECTATOR_MIN_DIST;
    if (dist > SPECTATOR_MAX_DIST) return SPECTATOR_MAX_DIST;
    return dist;
}

template <typename CellAt>
inline bool spectatorPointFree(const Vec3& p, float cs, float ceiling, CellAt&& cellAt) {
    if (p.y < SPECTATOR_WALL_MARGIN || p.y > ceiling - SPECTATOR_WALL_MARGIN) return false;
    const float m = SPECTATOR_WALL_MARGIN;
    if (cellAt(navCellCoord(p.x - m, cs), navCellCoord(p.z - m, cs)) != 0) return false;
    if (cellAt(navCellCoord(p.x + m, cs), navCellCoord(p.z - m, cs)) != 0) return false;
    if (cellAt(navCellCoord(p.x - m, cs), navCellCoord(p.z + m, cs)) != 0) return false;
    if (cellAt(navCellCoord(p.x + m, cs), navCellCoord(p.z + m, cs)) != 0) return false;
    return true;
}

// How far behind `focus` the camera can sit along the view direction before it would
// poke into a wall, the floor or the ceiling. Never more than `dist`.
template <typename CellAt>
inline float spectatorFreeDistance(const Vec3& focus, float yaw, float pitch, float dist, float cs, float ceiling, CellAt&& cellAt) {
    Vec3 dir = spectatorViewDir(yaw, pitch);
    const float step = 0.12f;
    float reached = 0.0f;
    for (float t = step; t <= dist + 1e-4f; t += step) {
        Vec3 p = focus - dir * t;
        if (!spectatorPointFree(p, cs, ceiling, cellAt)) break;
        reached = t;
    }
    return reached;
}

inline Vec3 spectatorOrbitPos(const Vec3& focus, float yaw, float pitch, float dist) {
    return focus - spectatorViewDir(yaw, pitch) * dist;
}

// The camera snaps in when a wall gets in the way and eases back out afterwards,
// so brushing past a doorway does not make the view pump.
inline float spectatorSmoothDist(float current, float freeDist, float dt) {
    if (freeDist < current) return freeDist;
    float k = 1.0f - expf(-5.0f * dt);
    return current + (freeDist - current) * k;
}

// Next slot in the list; `dir` is +1 / -1. Returns -1 for an empty list.
inline int spectatorCycle(int current, int count, int dir) {
    if (count <= 0) return -1;
    if (current < 0 || current >= count) return dir >= 0 ? 0 : count - 1;
    int next = (current + (dir >= 0 ? 1 : -1)) % count;
    if (next < 0) next += count;
    return next;
}

inline int spectatorFindSlot(const SpectatorSlot* slots, int count, int kind, int id) {
    for (int i = 0; i < count; i++) {
        if (slots[i].kind == kind && slots[i].id == id) return i;
    }
    return -1;
}

// Where a dead player's little ghost sits while it rides along: on the right shoulder of
// a teammate, or on top of a monster's head.
inline Vec3 spectatorPerch(const Vec3& targetEye, float targetYaw, bool monster, float monsterTop) {
    if (monster) return Vec3(targetEye.x, monsterTop + SPECTATOR_GHOST_DROP, targetEye.z);
    Vec3 right(cosf(targetYaw), 0.0f, -sinf(targetYaw));
    return Vec3(targetEye.x - right.x * 0.34f, targetEye.y + 0.08f, targetEye.z - right.z * 0.34f);
}
