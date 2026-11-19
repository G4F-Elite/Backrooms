#pragma once
#include "leg_sim.h"

// ============================================================================
// HEAD / CAMERA
// The eyes ride on the simulated pelvis, so every bit of camera motion comes from
// the legs: vertical bob at step frequency, lateral sway at stride frequency, the
// small fore-aft surge, and a soft thud at foot contact.
// The head is not a rigid passenger. It counter-rotates so the gaze stays on a
// point about a metre ahead (Pozzo 1990, Hirasaki 1999, Moore 2001): up -> pitch
// down, right -> yaw left. That compensation is weak below ~1.2 m/s.
// `scale` multiplies everything that oscillates (0 = steady camera, 1 = real).
// ============================================================================

inline constexpr float HEAD_FIXATION_DIST = 1.0f;

// Weight of the camera: the time (s) in which the view settles on where the mouse points.
// It has no top speed and loses nothing of what the hand does. 0 = the raw mouse.
// The view is the eyes and is not held back by the body; what the body follows is the look
// (LegSim::followLook), which comes round after the view as fast as the legs can.
// The keys push the body where the view points all the same (LegInput::steer).
inline constexpr float VIEW_EASE = 0.04f;

// Share of the way to the mouse that the view covers in a frame of `dt` seconds.
inline float viewSettle(float dt) { return VIEW_EASE > 0.0f ? 1.0f - expf(-dt / VIEW_EASE) : 1.0f; }

struct HeadState {
    float bobMean = 0.0f;      // slow average of the pelvis height deviation
    float jolt = 0.0f, joltV = 0.0f;
    bool init = false;
};

struct HeadView {
    Vec3 eye;                                     // centre of the head, on the axis of the trunk
    float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;  // added to the look direction
    float bobY = 0.0f, bobSide = 0.0f;            // current deviations, metres (for the HUD)
    float lean = 0.0f;                            // forward lean of the trunk
};

inline HeadView computeHead(const LegSim& s, HeadState& h, float dt, float scale) {
    HeadView v;
    float legLen = s.dims.standHip - s.crouchDrop();
    float dyn = s.pelvisY - legLen;
    if (!h.init) { h.bobMean = dyn; h.init = true; }
    h.bobMean += (dyn - h.bobMean) * (1.0f - expf(-dt / 0.45f));
    float bob = dyn - h.bobMean;

    // Heel strike shock: mostly absorbed by the legs and spine, a millimetre or two is left.
    for (int i = 0; i < s.strikeCount; i++) h.joltV -= s.strikes[i].force * gLerp(0.035f, 0.11f, s.run);
    float k = 38.0f, c = 2.0f * 0.45f * k;
    h.joltV += (-k * k * h.jolt - c * h.joltV) * dt;
    h.jolt += h.joltV * dt;
    h.jolt = gClamp(h.jolt, -0.02f, 0.02f);

    // The trunk leans into the movement: a little when walking, more when running, a lot
    // when crouching. The eyes swing forward and down with it.
    v.lean = 0.03f + 0.02f * gSat(s.speed / 1.5f) + 0.09f * s.run + 0.30f * s.crouch;
    float eyeUp = s.dims.eyeAboveHip * cosf(v.lean);
    Vec3 f0 = headingFwd(s.bodyYaw);
    float upDown = (bob + h.jolt) * scale;
    float side = s.sway * scale;
    v.eye = s.root + (s.swayVec + s.surgeVec) * scale + f0 * (s.dims.eyeAboveHip * sinf(v.lean));
    v.eye.y = legLen + h.bobMean + eyeUp + upDown;

    float comp = gSmooth((s.speed - 0.5f) / 0.8f);
    v.pitch = -atan2f(upDown, HEAD_FIXATION_DIST) * comp;
    v.yaw = atan2f(side, HEAD_FIXATION_DIST) * comp;
    v.roll = s.list * 0.22f * scale;
    v.bobY = bob;
    v.bobSide = s.sway;
    return v;
}
