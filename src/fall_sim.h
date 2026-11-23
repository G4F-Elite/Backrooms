#pragma once
// Falling down a hole in the floor (trap holes, the abyss).
//
// The floor holds the body until its middle is a foot's width past the edge. From there it
// is a body in free fall: it keeps the speed it walked in with, gravity is the real one
// (metres and seconds), the air slows it towards a human's terminal speed, and the shaft has
// walls - the body hits them, loses most of that speed and scrapes on down.
// The body also turns: whatever stops its feet (the edge, a wall) tips it over in the
// direction it was going, and the air brings it round face down. A wall it scrapes along
// drags on one side of it and spins it round.
// The eyes are in a head on a neck, not bolted to the body: the head is late to follow when
// the body turns, is thrown towards whatever stopped the body, and swings back.
//
// Pure rules: the world is asked only "is the floor missing in this cell".

#include <cmath>

#include "math.h"

inline constexpr float FALL_GRAVITY = 9.81f;     // m/s^2
inline constexpr float FALL_TERMINAL = 54.0f;    // m/s, a person falling flat
inline constexpr float FALL_LIP = 0.25f;         // m past the edge before the floor lets go
inline constexpr float FALL_BODY_R = 0.20f;      // m, what of the body the shaft walls stop (< FALL_LIP)
inline constexpr float FALL_BOUNCE = 0.22f;      // of the speed into a wall that comes back
inline constexpr float FALL_SCRAPE = 0.82f;      // of the speed along a wall kept after hitting it
inline constexpr float FALL_TIP_ARM = 1.0f;      // m, feet to the centre of mass: v / arm is the tip-over rate
inline constexpr float FALL_TIP_MAX = 2.6f;      // rad/s
inline constexpr float FALL_FACE_DOWN = -1.12f;  // rad, the pitch the air settles a falling body at
inline constexpr float FALL_PITCH_LIMIT = 1.45f;
inline constexpr float FALL_ROLL_LIMIT = 1.1f;
inline constexpr float FALL_SPIN = 1.5f;         // rad/s of spin per m/s a wall scraped off one side
inline constexpr float FALL_SPIN_MAX = 3.0f;     // rad/s
inline constexpr float FALL_SPIN_DRAG = 0.9f;    // 1/s, the air stops the spin
// The neck: a braced spring that swings about 2 times a second and settles in a swing or two.
inline constexpr float FALL_NECK_STIFF = 200.0f; // 1/s^2
inline constexpr float FALL_NECK_DAMP = 11.0f;   // 1/s
inline constexpr float FALL_NECK_WHIP = 3.0f;    // rad/s the head is thrown per m/s the body lost
inline constexpr float FALL_NECK_LAG = 0.7f;     // of a turn of the body the head does not make at once
inline constexpr float FALL_NECK_RATE = 9.0f;    // rad/s, the fastest the neck lets the head go
inline constexpr float FALL_NECK_PITCH = 0.7f;   // rad, as far as the neck bends: nod,
inline constexpr float FALL_NECK_ROLL = 0.5f;    // ear to shoulder,
inline constexpr float FALL_NECK_YAW = 1.0f;     // chin to shoulder
inline constexpr float FALL_MAX_STEP = 1.0f / 120.0f;

struct FallState {
    Vec3 pos, vel;        // the eyes; vel.y < 0 on the way down
    float yaw = 0.0f, yawVel = 0.0f;  // the body; only a wall can spin it
    float pitch = 0.0f, pitchVel = 0.0f;
    float roll = 0.0f, rollVel = 0.0f;  // > 0: the head tips to the right
    // The head against the body, same signs.
    float headYaw = 0.0f, headYawVel = 0.0f;
    float headPitch = 0.0f, headPitchVel = 0.0f;
    float headRoll = 0.0f, headRollVel = 0.0f;
    float time = 0.0f;
    float hit = 0.0f;     // m/s, the hardest wall hit of the last fallStep (0: none)
};

inline float fallClamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// The floor is missing under the whole square of half-side `r` around (x, z).
template <class OpenFn>
inline bool fallOverHole(float x, float z, float r, float cs, OpenFn isOpen) {
    for (int i = 0; i < 4; i++) {
        float px = x + ((i & 1) ? r : -r), pz = z + ((i & 2) ? r : -r);
        if (!isOpen((int)floorf(px / cs), (int)floorf(pz / cs))) return false;
    }
    return true;
}

// Does the floor still hold a player standing at (x, z)?
template <class OpenFn>
inline bool fallSupported(float x, float z, float cs, OpenFn isOpen) {
    return !fallOverHole(x, z, FALL_LIP, cs, isOpen);
}

// Something stopped part of the body that was moving at (vx, vz): the rest goes on and the
// body turns over that way - forwards pitches the face down, sideways rolls.
inline void fallTip(FallState& s, float vx, float vz) {
    float fwd = vx * sinf(s.yaw) + vz * cosf(s.yaw);
    float right = vx * cosf(s.yaw) - vz * sinf(s.yaw);
    s.pitchVel = fallClamp(s.pitchVel - fwd / FALL_TIP_ARM, -FALL_TIP_MAX, FALL_TIP_MAX);
    s.rollVel = fallClamp(s.rollVel + right / FALL_TIP_ARM, -FALL_TIP_MAX, FALL_TIP_MAX);
}

// The body's turning changed by this much at once: the head stays behind.
inline void fallHeadLag(FallState& s, float dYawVel, float dPitchVel, float dRollVel) {
    s.headYawVel -= FALL_NECK_LAG * dYawVel;
    s.headPitchVel -= FALL_NECK_LAG * dPitchVel;
    s.headRollVel -= FALL_NECK_LAG * dRollVel;
}

// One axis of the neck for a substep: the spring pulls the head back in line, and the neck
// only bends so far.
inline void fallNeck(float& angle, float& vel, float limit, float h) {
    vel += (-FALL_NECK_STIFF * angle - FALL_NECK_DAMP * vel) * h;
    vel = fallClamp(vel, -FALL_NECK_RATE, FALL_NECK_RATE);
    angle += vel * h;
    if (fabsf(angle) > limit) {
        angle = angle > 0.0f ? limit : -limit;
        vel = 0.0f;
    }
}

// Where the eyes look: the body's turn and the head's on top of it.
inline float fallViewYaw(const FallState& s) { return s.yaw + s.headYaw; }
inline float fallViewPitch(const FallState& s) {
    return fallClamp(s.pitch + s.headPitch, -FALL_PITCH_LIMIT, FALL_PITCH_LIMIT);
}
inline float fallViewRoll(const FallState& s) { return s.roll + s.headRoll; }

// The floor lets go of a player whose eyes are at `eye`, moving at `vel` (horizontal).
inline FallState fallBegin(const Vec3& eye, const Vec3& vel, float yaw, float pitch) {
    FallState s;
    s.pos = eye;
    s.vel = Vec3(vel.x, 0.0f, vel.z);
    s.yaw = yaw;
    s.pitch = pitch;
    fallTip(s, vel.x, vel.z);  // the feet were on the edge a moment ago
    fallHeadLag(s, 0.0f, s.pitchVel, s.rollVel);
    return s;
}

template <class OpenFn>
inline void fallSubstep(FallState& s, float h, float cs, OpenFn isOpen) {
    const Vec3 vel0 = s.vel;
    const float yawVel0 = s.yawVel, pitchVel0 = s.pitchVel, rollVel0 = s.rollVel;

    // Gravity, and drag that grows with the square of the speed (equal to gravity at terminal).
    float drag = FALL_GRAVITY * sqrtf(s.vel.lenSq()) / (FALL_TERMINAL * FALL_TERMINAL);
    s.vel.x -= s.vel.x * drag * h;
    s.vel.z -= s.vel.z * drag * h;
    s.vel.y -= (FALL_GRAVITY + s.vel.y * drag) * h;

    // The shaft: one axis at a time, so a body slides down along the wall it met.
    float nx = s.pos.x + s.vel.x * h;
    if (fallOverHole(nx, s.pos.z, FALL_BODY_R, cs, isOpen)) {
        s.pos.x = nx;
    } else {
        float into = fabsf(s.vel.x);
        fallTip(s, s.vel.x * 0.6f, 0.0f);
        // The wall drags on the side that touches it; the rest of the body goes on round it.
        s.yawVel += (s.vel.x > 0.0f ? 1.0f : -1.0f) * s.vel.z * (1.0f - FALL_SCRAPE) * FALL_SPIN;
        s.vel.x *= -FALL_BOUNCE;
        s.vel.z *= FALL_SCRAPE;
        if (into > s.hit) s.hit = into;
    }
    float nz = s.pos.z + s.vel.z * h;
    if (fallOverHole(s.pos.x, nz, FALL_BODY_R, cs, isOpen)) {
        s.pos.z = nz;
    } else {
        float into = fabsf(s.vel.z);
        fallTip(s, 0.0f, s.vel.z * 0.6f);
        s.yawVel -= (s.vel.z > 0.0f ? 1.0f : -1.0f) * s.vel.x * (1.0f - FALL_SCRAPE) * FALL_SPIN;
        s.vel.z *= -FALL_BOUNCE;
        s.vel.x *= FALL_SCRAPE;
        if (into > s.hit) s.hit = into;
    }
    s.pos.y += s.vel.y * h;

    // The air turns the body face down and level, loosely: it swings past and comes back.
    s.pitchVel += (-6.0f * (s.pitch - FALL_FACE_DOWN) - 2.2f * s.pitchVel) * h;
    s.rollVel += (-5.0f * s.roll - 1.6f * s.rollVel) * h;
    s.yawVel = fallClamp(s.yawVel, -FALL_SPIN_MAX, FALL_SPIN_MAX);
    s.yawVel -= s.yawVel * FALL_SPIN_DRAG * h;
    s.yaw += s.yawVel * h;
    s.pitch += s.pitchVel * h;
    s.roll += s.rollVel * h;
    if (fabsf(s.pitch) > FALL_PITCH_LIMIT) {
        s.pitch = s.pitch > 0.0f ? FALL_PITCH_LIMIT : -FALL_PITCH_LIMIT;
        s.pitchVel = 0.0f;
    }
    if (fabsf(s.roll) > FALL_ROLL_LIMIT) {
        s.roll = s.roll > 0.0f ? FALL_ROLL_LIMIT : -FALL_ROLL_LIMIT;
        s.rollVel = 0.0f;
    }

    // The head. It does not feel gravity (it falls with the body), only what pushed the body
    // in this substep: the air, a wall. It keeps going the way the body was stopped from -
    // forwards is a nod, sideways an ear to the shoulder - and is late for the body's turns.
    Vec3 push = s.vel - vel0;
    push.y += FALL_GRAVITY * h;
    float cy = cosf(s.yaw), sy = sinf(s.yaw), cp = cosf(s.pitch), sp = sinf(s.pitch);
    float fwd = push.x * sy * cp + push.y * sp + push.z * cy * cp;  // along the face
    float right = push.x * cy - push.z * sy;
    s.headPitchVel += FALL_NECK_WHIP * fwd;
    s.headRollVel -= FALL_NECK_WHIP * right;
    fallHeadLag(s, s.yawVel - yawVel0, s.pitchVel - pitchVel0, s.rollVel - rollVel0);
    fallNeck(s.headYaw, s.headYawVel, FALL_NECK_YAW, h);
    fallNeck(s.headPitch, s.headPitchVel, FALL_NECK_PITCH, h);
    fallNeck(s.headRoll, s.headRollVel, FALL_NECK_ROLL, h);
}

// One frame of falling. isOpen(wx, wz): the floor is missing in that cell.
template <class OpenFn>
inline void fallStep(FallState& s, float dt, float cs, OpenFn isOpen) {
    s.hit = 0.0f;
    if (dt <= 0.0f) return;
    if (dt > 0.1f) dt = 0.1f;
    int n = (int)ceilf(dt / FALL_MAX_STEP);
    if (n < 1) n = 1;
    float h = dt / n;
    for (int k = 0; k < n; k++) fallSubstep(s, h, cs, isOpen);
    s.time += dt;
}

// How hard the camera is buffeted: the rush of air, and a jolt off a wall.
inline float fallShake(const FallState& s) {
    float shake = 0.02f + sqrtf(s.vel.lenSq()) * 0.011f + s.hit * 0.22f;
    return shake > 0.9f ? 0.9f : shake;
}
