#pragma once

#include "math.h"

// ============================================================================
// INTERPOLATION UTILITIES
// Uses fast math functions when g_fastMathEnabled is true
// ============================================================================

// ============================================================================
// ANGLE UTILITIES
// ============================================================================

inline float normalizeAngle(float a) {
    return g_fastMathEnabled ? fastNormalizeAngle(a) : fmodf(a + MATH_PI, MATH_TAU) - MATH_PI + (a < -MATH_PI ? MATH_TAU : 0.0f);
}

inline float lerpAngle(float from, float to, float alpha) {
    float delta = g_fastMathEnabled ? fastAngleDiff(from, to) : normalizeAngle(to - from);
    return normalizeAngle(from + delta * alpha);
}

inline float clamp01(float v) {
    return fastClamp01(v);
}

// ============================================================================
// BASIC INTERPOLATION
// ============================================================================

// Linear interpolation
inline float lerp(float a, float b, float t) {
    return fastLerp(a, b, t);
}

// Bilinear interpolation (for 2D grids)
inline float bilerp(float v00, float v10, float v01, float v11, float tx, float ty) {
    float x0 = fastLerp(v00, v10, tx);
    float x1 = fastLerp(v01, v11, tx);
    return fastLerp(x0, x1, ty);
}

// Inverse lerp - find t given value between a and b
inline float invLerp(float a, float b, float value) {
    if (fabsf(b - a) < 1e-10f) return 0.0f;
    return (value - a) / (b - a);
}

// Remap value from one range to another
inline float remap(float value, float fromMin, float fromMax, float toMin, float toMax) {
    float t = invLerp(fromMin, fromMax, value);
    return fastLerp(toMin, toMax, t);
}

// ============================================================================
// EASING FUNCTIONS
// All use fast math approximations when enabled
// ============================================================================

// Smoothstep - S-curve, zero derivative at endpoints
inline float smoothstep(float edge0, float edge1, float x) {
    return fastSmoothstep(edge0, edge1, x);
}

// Smootherstep - smoother S-curve (Ken Perlin's version)
inline float smootherstep(float edge0, float edge1, float x) {
    return fastSmootherStep(edge0, edge1, x);
}

// Quadratic easing
inline float easeInQuad(float t) {
    return t * t;
}

inline float easeOutQuad(float t) {
    return t * (2.0f - t);
}

inline float easeInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - fastPowInt(-2.0f * t + 2.0f, 2) * 0.5f;
}

// Cubic easing
inline float easeInCubic(float t) {
    return t * t * t;
}

inline float easeOutCubic(float t) {
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

inline float easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - fastPowInt(-2.0f * t + 2.0f, 3) * 0.5f;
}

// Sine easing (uses fast sin/cos)
inline float easeInSine(float t) {
    return 1.0f - mCos(t * MATH_HALF_PI);
}

inline float easeOutSine(float t) {
    return mSin(t * MATH_HALF_PI);
}

inline float easeInOutSine(float t) {
    return -(mCos(MATH_PI * t) - 1.0f) * 0.5f;
}

// Exponential easing (uses fast exp)
inline float easeInExpo(float t) {
    return t <= 0.0f ? 0.0f : mPow(2.0f, 10.0f * t - 10.0f);
}

inline float easeOutExpo(float t) {
    return t >= 1.0f ? 1.0f : 1.0f - mPow(2.0f, -10.0f * t);
}

inline float easeInOutExpo(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t < 0.5f
        ? mPow(2.0f, 20.0f * t - 10.0f) * 0.5f
        : (2.0f - mPow(2.0f, -20.0f * t + 10.0f)) * 0.5f;
}

// Circular easing (uses fast sqrt)
inline float easeInCirc(float t) {
    return 1.0f - mSqrt(1.0f - t * t);
}

inline float easeOutCirc(float t) {
    return mSqrt(1.0f - (t - 1.0f) * (t - 1.0f));
}

inline float easeInOutCirc(float t) {
    return t < 0.5f
        ? (1.0f - mSqrt(1.0f - 4.0f * t * t)) * 0.5f
        : (mSqrt(1.0f - fastPowInt(-2.0f * t + 2.0f, 2)) + 1.0f) * 0.5f;
}

// Elastic easing (uses fast sin)
inline float easeInElastic(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    const float c4 = MATH_TAU / 3.0f;
    return -mPow(2.0f, 10.0f * t - 10.0f) * mSin((t * 10.0f - 10.75f) * c4);
}

inline float easeOutElastic(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    const float c4 = MATH_TAU / 3.0f;
    return mPow(2.0f, -10.0f * t) * mSin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

// Bounce easing
inline float easeOutBounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

inline float easeInBounce(float t) {
    return 1.0f - easeOutBounce(1.0f - t);
}

inline float easeInOutBounce(float t) {
    return t < 0.5f
        ? (1.0f - easeOutBounce(1.0f - 2.0f * t)) * 0.5f
        : (1.0f + easeOutBounce(2.0f * t - 1.0f)) * 0.5f;
}

// Back easing (overshoots)
inline float easeInBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

inline float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float tm1 = t - 1.0f;
    return 1.0f + c3 * tm1 * tm1 * tm1 + c1 * tm1 * tm1;
}

inline float easeInOutBack(float t) {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;
    
    return t < 0.5f
        ? (4.0f * t * t * ((c2 + 1.0f) * 2.0f * t - c2)) * 0.5f
        : ((2.0f * t - 2.0f) * (2.0f * t - 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) * 0.5f;
}

// ============================================================================
// SPRING / DAMPED OSCILLATION
// For physics-based interpolation
// ============================================================================

// Simple spring damper (critically damped)
inline float springDamp(float current, float target, float& velocity, float smoothTime, float dt) {
    float omega = 2.0f / smoothTime;
    float x = omega * dt;
    // Fast approximation of exp(-x)
    float expTerm = g_fastMathEnabled ? (1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x)) : expf(-x);
    float change = current - target;
    float temp = (velocity + omega * change) * dt;
    velocity = (velocity - omega * temp) * expTerm;
    return target + (change + temp) * expTerm;
}

// Underdamped spring (bouncy)
inline float springUnderdamped(float current, float target, float& velocity, 
                                float stiffness, float damping, float mass, float dt) {
    float displacement = current - target;
    float springForce = -stiffness * displacement;
    float dampingForce = -damping * velocity;
    float acceleration = (springForce + dampingForce) / mass;
    velocity += acceleration * dt;
    return current + velocity * dt;
}

// ============================================================================
// BEZIER CURVES
// ============================================================================

// Quadratic bezier
inline float bezierQuadratic(float p0, float p1, float p2, float t) {
    float inv = 1.0f - t;
    return inv * inv * p0 + 2.0f * inv * t * p1 + t * t * p2;
}

// Cubic bezier
inline float bezierCubic(float p0, float p1, float p2, float p3, float t) {
    float inv = 1.0f - t;
    float inv2 = inv * inv;
    float t2 = t * t;
    return inv2 * inv * p0 + 3.0f * inv2 * t * p1 + 3.0f * inv * t2 * p2 + t2 * t * p3;
}

// Cubic bezier for Vec3
inline Vec3 bezierCubicVec3(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) {
    float inv = 1.0f - t;
    float inv2 = inv * inv;
    float t2 = t * t;
    float b0 = inv2 * inv;
    float b1 = 3.0f * inv2 * t;
    float b2 = 3.0f * inv * t2;
    float b3 = t2 * t;
    return p0 * b0 + p1 * b1 + p2 * b2 + p3 * b3;
}

// ============================================================================
// CATMULL-ROM SPLINE
// Smooth interpolation through control points
// ============================================================================

inline float catmullRom(float p0, float p1, float p2, float p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * ((2.0f * p1) +
                   (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

inline Vec3 catmullRomVec3(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t) {
    return Vec3(
        catmullRom(p0.x, p1.x, p2.x, p3.x, t),
        catmullRom(p0.y, p1.y, p2.y, p3.y, t),
        catmullRom(p0.z, p1.z, p2.z, p3.z, t)
    );
}

// ============================================================================
// NOISE FUNCTIONS
// Fast noise for procedural effects
// ============================================================================

// Fast hash for noise generation
inline uint32_t noiseHash(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

// 1D value noise [0, 1]
inline float valueNoise1D(float x) {
    int xi = fastFloor(x);
    float xf = x - (float)xi;
    
    float n0 = (float)noiseHash(xi) / 4294967296.0f;
    float n1 = (float)noiseHash(xi + 1) / 4294967296.0f;
    
    // Smoothstep interpolation
    float t = xf * xf * (3.0f - 2.0f * xf);
    return fastLerp(n0, n1, t);
}

// 2D value noise [0, 1]
inline float valueNoise2D(float x, float y) {
    int xi = fastFloor(x);
    int yi = fastFloor(y);
    float xf = x - (float)xi;
    float yf = y - (float)yi;
    
    float n00 = (float)noiseHash(xi + yi * 57) / 4294967296.0f;
    float n10 = (float)noiseHash(xi + 1 + yi * 57) / 4294967296.0f;
    float n01 = (float)noiseHash(xi + (yi + 1) * 57) / 4294967296.0f;
    float n11 = (float)noiseHash(xi + 1 + (yi + 1) * 57) / 4294967296.0f;
    
    float tx = xf * xf * (3.0f - 2.0f * xf);
    float ty = yf * yf * (3.0f - 2.0f * yf);
    
    return bilerp(n00, n10, n01, n11, tx, ty);
}

// Fractal brownian motion (fbm) - layered noise
inline float fbm(float x, float y, int octaves, float persistence, float lacunarity) {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxValue = 0.0f;
    
    for (int i = 0; i < octaves; i++) {
        total += valueNoise2D(x * frequency, y * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return total / maxValue;
}