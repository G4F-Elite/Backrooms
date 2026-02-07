#pragma once

#include <cmath>

struct AudioSafetyState {
    float humNoise = 0.0f;
    float staticNoise = 0.0f;
    float whisperNoise = 0.0f;
    float clickEnv = 0.0f;
    float insaneBurst = 0.0f;
    float limiterEnv = 0.0f;
};

inline float mixNoise(float prev, float target, float alpha) {
    return prev + (target - prev) * alpha;
}

inline float softClip(float x) {
    const float drive = 1.2f;
    return tanhf(x * drive);
}

inline float applyLimiter(float x, float& env) {
    float absX = fabsf(x);
    const float attack = 0.35f;
    const float release = 0.0015f;
    if (absX > env) env += (absX - env) * attack;
    else env += (absX - env) * release;
    float gain = 1.0f;
    const float ceiling = 0.92f;
    if (env > ceiling && env > 0.0001f) gain = ceiling / env;
    return x * gain;
}
