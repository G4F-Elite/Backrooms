#pragma once
#include "math.h"

// ============================================================================
// GAIT PARAMETERS
// Body proportions and the empirical relations that turn "how fast and in which
// direction is the body moving" into "how long is a step and how is it timed".
// No state here. Sources are listed in README.md.
// ============================================================================

inline constexpr float GAIT_G = 9.81f;

inline float gSat(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }
inline float gLerp(float a, float b, float t) { return a + (b - a) * t; }
inline float gSmooth(float x) { x = gSat(x); return x * x * (3.0f - 2.0f * x); }
inline float gClamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

// Segment lengths as fractions of stature (Winter, Biomechanics and Motor Control of Human Movement).
struct BodyDims {
    float stature;
    float thigh, shank;       // hip-knee, knee-ankle
    float ankleH;             // ankle joint above the sole
    float heelBack;           // heel contact point behind the ankle
    float ballFwd;            // metatarsal heads (ball) in front of the ankle
    float toeFwd;             // toe tip in front of the ankle
    float footHalfW;
    float hipHalf;            // hip joint centre from the midline
    float standHip;           // hip joint height in quiet standing
    float eyeAboveHip;
    float legMax() const { return thigh + shank; }
};

inline BodyDims makeBodyDims(float stature) {
    BodyDims b;
    b.stature = stature;
    b.thigh = 0.245f * stature;
    b.shank = 0.246f * stature;
    b.ankleH = 0.039f * stature;
    float footLen = 0.152f * stature;
    b.heelBack = 0.26f * footLen;
    b.ballFwd = 0.46f * footLen;
    b.toeFwd = 0.74f * footLen;
    b.footHalfW = 0.0275f * stature;
    b.hipHalf = 0.049f * stature;
    // Knees are never locked. Leg length is very sensitive to it: 1 mm shorter is ~5 degrees of flexion.
    b.standHip = b.thigh + b.shank + b.ankleH - 0.001f;
    b.eyeAboveHip = 0.936f * stature - b.standHip;
    return b;
}

// What one step looks like at the current speed.
struct Gait {
    float stepTime;     // heel strike to opposite heel strike
    float strideTime;   // two steps
    float duty;         // share of the stride a foot spends on the ground
    float stanceTime, swingTime;
    float dsTime;       // one double support period (0 when running)
    float stepWidth;    // lateral distance between the footprints
    float clearance;    // peak clearance of the lowest point of the swinging foot
    float toeOut;       // foot progression angle
    float strikePitch;  // foot-to-floor angle at contact for a full forward step
    float toeOffPitch;  // heel-up angle at the moment the toes leave
};

// Steps get shorter and quicker when the body does not move straight ahead.
// fwd/lat: unit direction of travel in the pelvis frame.
inline float gaitDirScale(float fwd, float lat) {
    float sf = fwd >= 0.0f ? 1.0f : 0.70f;
    const float sl = 0.45f;
    float a = fwd / sf, b = lat / sl;
    float d = sqrtf(a * a + b * b);
    return d > 1e-4f ? 1.0f / d : 1.0f;
}

// speed: horizontal speed of the body, m/s. run: 0 walking .. 1 running. crouch: 0..1.
inline Gait gaitFor(float speed, float dirScale, float run, float crouch, const BodyDims& b) {
    Gait g;
    float v = speed < 0.05f ? 0.05f : speed;
    float vg = v / dirScale;  // the forward speed that would need the same effort

    // Walking: preferred step length follows s/L = (v / sqrt(gL))^0.42 (Grieve 1968, Kuo 2001).
    // 1.25 m/s -> 0.66 m and 114 steps/min for a 1.81 m body.
    float L = b.standHip;
    float froude = vg / sqrtf(GAIT_G * L);
    float stepLen = L * powf(froude, 0.42f) * dirScale * (1.0f - 0.22f * crouch);
    float tWalk = gClamp(stepLen / v, 0.30f, 1.0f);
    // Stance takes ~62% of the stride at normal speed, more when slow (Perry; Dicharry 2010).
    float dutyWalk = gClamp(0.745f - 0.10f * vg, 0.54f, 0.74f);

    // Running: cadence barely grows with speed (160-180 steps/min), the flight phase does.
    float tRun = 1.0f / gClamp(2.55f + 0.16f * (vg - 2.2f), 2.4f, 3.4f);
    float dutyRun = gClamp(0.40f - 0.035f * (vg - 2.2f), 0.26f, 0.42f);

    g.stepTime = gLerp(tWalk, tRun, run);
    g.strideTime = g.stepTime * 2.0f;
    g.duty = gLerp(dutyWalk, dutyRun, run);
    g.stanceTime = g.duty * g.strideTime;
    g.swingTime = (1.0f - g.duty) * g.strideTime;
    g.dsTime = g.duty > 0.5f ? (g.duty - 0.5f) * g.strideTime : 0.0f;

    // Preferred step width is about 0.13 leg lengths (Donelan 2001), wider when slow, narrow when running.
    float widthWalk = gLerp(0.16f, 0.115f, gSat((vg - 0.4f) / 1.2f));
    g.stepWidth = gLerp(widthWalk, 0.05f, run) + 0.05f * crouch;
    // Minimum toe clearance in walking is only 1-2 cm; running lifts the foot far higher.
    g.clearance = gLerp(0.020f + 0.012f * gSat(vg / 1.6f), 0.13f, run);
    g.toeOut = gLerp(0.12f, 0.05f, run);
    g.strikePitch = gLerp(0.12f + 0.26f * gSat(vg / 1.5f), 0.14f, run);
    g.toeOffPitch = gLerp(0.95f + 0.30f * gSat(vg / 1.6f), 1.05f, run);
    return g;
}

// Exact step of a critically damped spring towards `target`: stable for any dt.
inline void gSpring(float& x, float& v, float target, float omega, float dt) {
    float d = x - target;
    float tmp = (v + omega * d) * dt;
    float e = expf(-omega * dt);
    v = (v - omega * tmp) * e;
    x = target + (d + tmp) * e;
}

inline float gAngleDiff(float from, float to) {
    float d = fmodf(to - from + MATH_PI, MATH_TAU);
    if (d < 0.0f) d += MATH_TAU;
    return d - MATH_PI;
}

// Engine convention: yaw 0 looks along +Z, growing yaw turns to the left (+X).
inline Vec3 headingFwd(float yaw) { return Vec3(sinf(yaw), 0.0f, cosf(yaw)); }
inline Vec3 headingRight(float yaw) { return Vec3(-cosf(yaw), 0.0f, sinf(yaw)); }
// `v` turned about the vertical the way a growing yaw turns a heading.
inline Vec3 turnedBy(const Vec3& v, float a) {
    float c = cosf(a), s = sinf(a);
    return Vec3(v.x * c + v.z * s, v.y, v.z * c - v.x * s);
}
