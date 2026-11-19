#pragma once
#include "gait_params.h"

// ============================================================================
// FOOT AND LEG GEOMETRY
// A foot is a rigid wedge that rolls over three rockers, like the real one:
//   heel rocker   - lands on the heel with the toes up, then lowers the forefoot
//   ankle rocker  - flat on the floor while the shank rotates over it
//   forefoot rocker - heel rises, the foot pivots on the ball until toe-off
// The pose keeps a ground reference ("anchor": the point under the ankle when the
// foot is flat), so a planted foot can never slide however it rolls.
// ============================================================================

struct FootPose {
    Vec3 anchor;   // world, y = floor height
    float yaw;     // heading of the foot
    float pitch;   // > 0 toes up (pivot on the heel), < 0 heel up (pivot on the ball)
    float lift;    // clearance of the pivot point above the floor
};

// Ankle joint relative to the anchor, in the foot's own sagittal plane.
inline void footAnkleLocal(const BodyDims& b, float pitch, float& ax, float& ay) {
    float c = cosf(pitch), s = sinf(pitch);
    if (pitch >= 0.0f) {
        ax = -b.heelBack + b.heelBack * c - b.ankleH * s;
        ay = b.heelBack * s + b.ankleH * c;
    } else {
        ax = b.ballFwd - b.ballFwd * c - b.ankleH * s;
        ay = -b.ballFwd * s + b.ankleH * c;
    }
}

inline Vec3 footAnkle(const BodyDims& b, const FootPose& f) {
    float ax, ay;
    footAnkleLocal(b, f.pitch, ax, ay);
    Vec3 fw = headingFwd(f.yaw);
    return Vec3(f.anchor.x + fw.x * ax, f.anchor.y + ay + f.lift, f.anchor.z + fw.z * ax);
}

// Sole axes of the pitched foot.
inline void footAxes(const FootPose& f, Vec3& fwd, Vec3& up, Vec3& right) {
    Vec3 h = headingFwd(f.yaw);
    float c = cosf(f.pitch), s = sinf(f.pitch);
    fwd = Vec3(h.x * c, s, h.z * c);
    up = Vec3(-h.x * s, c, -h.z * s);
    right = headingRight(f.yaw);
}

// Toes are a separate hinge: on the floor they stay flat while the heel rises.
// Returns the pitch of the toe segment in world terms.
inline float footToePitch(const BodyDims& b, const FootPose& f) {
    if (f.pitch >= 0.0f) return f.pitch;
    float toeLen = b.toeFwd - b.ballFwd;
    float ratio = gSat(f.lift / toeLen);
    float lowest = -asinf(ratio);
    return f.pitch > lowest ? f.pitch : lowest;
}

struct LegPose {
    Vec3 hip, knee, ankle;
    float kneeFlex;    // radians, 0 = straight
    bool stretched;    // the ankle was out of reach and had to be pulled in
};

// Analytic two-bone IK. `pole` is where the knee should point (roughly the foot's heading).
inline LegPose solveLeg(const BodyDims& b, const Vec3& hip, const Vec3& ankleWanted, const Vec3& pole) {
    LegPose p;
    p.hip = hip;
    p.stretched = false;
    float a = b.thigh, c = b.shank;
    Vec3 d = ankleWanted - hip;
    float len = d.len();
    float maxLen = (a + c) * 0.9995f;
    float minLen = fabsf(a - c) + 0.02f;
    Vec3 dir = len > 1e-5f ? d / len : Vec3(0, -1, 0);
    if (len > maxLen) { len = maxLen; p.stretched = true; }
    if (len < minLen) len = minLen;
    p.ankle = hip + dir * len;

    float x = (a * a - c * c + len * len) / (2.0f * len);
    float h2 = a * a - x * x;
    float h = h2 > 0.0f ? sqrtf(h2) : 0.0f;
    Vec3 side = pole - dir * pole.dot(dir);
    float sl = side.len();
    side = sl > 1e-4f ? side / sl : Vec3(0, 0, 1);
    p.knee = hip + dir * x + side * h;

    float cosK = (a * a + c * c - len * len) / (2.0f * a * c);
    p.kneeFlex = MATH_PI - acosf(gClamp(cosK, -1.0f, 1.0f));
    return p;
}

// Height of the swinging foot's lowest point: zero at both ends, peak a little before the middle.
inline float swingLift(float u, float clearance, float run) {
    float w = gLerp(u, powf(gSat(u), 0.8f), run);  // runners pick the foot up early
    float s = sinf(MATH_PI * gSat(w));
    return clearance * s * sqrtf(s);
}
