#pragma once

#include <cmath>

inline float normalizeAngle(float a) {
    const float twoPi = 6.2831853071795864769f;
    while (a > 3.14159265358979323846f) a -= twoPi;
    while (a < -3.14159265358979323846f) a += twoPi;
    return a;
}

inline float lerpAngle(float from, float to, float alpha) {
    float delta = normalizeAngle(to - from);
    return normalizeAngle(from + delta * alpha);
}

inline float clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}
