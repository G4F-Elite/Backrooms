#pragma once

#include <cmath>
#include "math.h"

struct TrapCorridorState {
    bool active;
    bool triggered;
    bool locked;
    bool resolved;
    int startX;
    int startZ;
    int length;
    int gateX;
    int gateZ;
    Vec3 anomalyPos;
    float lockTimer;
    float stareProgress;
};

inline bool isInsideTrapTrigger(int wx, int wz, int startX, int startZ, int length) {
    return wz == startZ && wx >= startX + 2 && wx <= startX + length;
}

inline bool isLookingAtPoint(const Vec3& camPos, float yaw, float pitch, const Vec3& target, float minDot, float maxDist) {
    Vec3 to = target - camPos;
    float dist = to.len();
    if (dist > maxDist || dist < 0.001f) return false;
    to = to * (1.0f / dist);
    Vec3 fwd(mSin(yaw) * mCos(pitch), mSin(pitch), mCos(yaw) * mCos(pitch));
    return fwd.dot(to) >= minDot;
}

inline float updateTrapStareProgress(float current, float dt, bool progressActive) {
    if (progressActive) {
        current += dt;
        if (current > 4.0f) current = 4.0f;
    } else {
        current -= dt * 0.35f;
        if (current < 0.0f) current = 0.0f;
    }
    return current;
}

inline float updateAnomalyBlur(float current, float dt, bool looking) {
    if (looking) {
        current += dt * 1.2f;
        if (current > 1.2f) current = 1.2f;
    } else {
        current -= dt * 0.85f;
        if (current < 0.0f) current = 0.0f;
    }
    return current;
}

inline int floorHoleCountFromRoll(int roll) {
    int norm = roll % 4;
    if (norm < 0) norm += 4;
    return 4 + norm; // 4..7 holes
}

inline float floorHoleDurationFromRoll(int roll) {
    int norm = roll % 6;
    if (norm < 0) norm += 6;
    return 12.0f + (float)norm; // 12..17s
}
