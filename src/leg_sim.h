#pragma once
#include "leg_foot.h"

// ============================================================================
// LEG SIMULATION
// The body is a point that is pushed around by the input (and stopped by walls).
// Everything else follows from where the two feet are:
//   - a stride clock decides WHEN a foot leaves the floor,
//   - the swing foot aims where the hip will pass over it at mid stance,
//   - planted feet never move; they only roll heel -> flat -> ball,
//   - the pelvis rides on the supporting legs (see leg_body.h).
// One step costs a few hundred nanoseconds: no solvers, no iteration over anything
// but the two feet.
// ============================================================================

typedef bool (*LegBlockedFn)(float x, float z, float r);

inline constexpr float LEG_BODY_RADIUS = 0.30f;
inline constexpr float LEG_FOOT_RADIUS = 0.09f;
inline constexpr float LEG_CORRECTION_SWING = 0.34f;  // seconds, a step taken while standing
inline constexpr int LEG_MAX_STRIKES = 8;
// How far ahead of the hip a foot lands, as a share of the distance the hip travels during
// that foot's stance. Walking: wherever an almost straight leg reaches the floor (see
// strikeShare), which comes out at 0.43-0.47 - less than half, because the contact rolls from
// heel to toe. Runners land closer to under the body; sideways there is no roll at all.
inline constexpr float LEG_LEAD_MIN = 0.30f;
inline constexpr float LEG_LEAD_MAX = 0.50f;
inline constexpr float LEG_LEAD_RUN = 0.30f;
inline constexpr float LEG_LEAD_SIDE = 0.50f;
inline constexpr float LEG_STRIKE_REACH = 0.998f;  // leg extension at heel strike: ~7 degrees of knee
inline constexpr float LEG_SIDESTEP_GAP = 0.10f;
inline constexpr float LEG_SWING_REACH = 0.9980f;  // the swinging leg never extends past this: ~7 degrees
// The hips never come down further than this to keep a foot. A little more than the deepest
// point of a steady gait (8 cm at a 2 m/s walk, 11 cm running): past it the foot is let go.
inline constexpr float LEG_MAX_SINK = 0.095f;
inline constexpr float LEG_MAX_SINK_RUN = 0.13f;
// The trunk cannot turn faster than the feet can step around under it. A faster look (a mouse
// flick) leaves the trunk behind for a moment, the way a head turns before the hips do.
inline constexpr float LEG_TURN_MAX = 4.2f;        // rad/s
inline constexpr float LEG_TURN_TWIST = 0.90f;     // rad a planted foot is asked to take while the trunk turns over it
// Walking at an angle (W+D) the trunk turns into the direction of travel, at this rate. It
// has to be about as quick as the velocity itself comes round (a quarter of a second): until
// the trunk is there the legs are walking sideways at a forward walk's speed, which they
// cannot do - both feet end up on one side of the body, half a metre out.
inline constexpr float LEG_CRAB_RATE = 3.0f;       // rad/s
// Changing direction: m/s2 the feet give against the old velocity, and how far round the new
// direction has to be (1 - cosine of the angle: 0.3 is about 45 degrees) for all of it.
inline constexpr float LEG_CUT_ACCEL = 20.0f;
inline constexpr float LEG_CUT_FULL = 0.3f;
// m/s the landing spot of a foot in the air may be moved at when the plan changes.
inline constexpr float LEG_RETARGET_SPEED = 12.0f;
inline constexpr float LEG_LAND_REACH = 0.42f;     // m, hip to landing spot: an almost straight leg from walking height

struct LegInput {
    float moveFwd = 0.0f, moveRight = 0.0f;  // -1..1
    float yaw = 0.0f;                        // where the head looks
    // Where the eyes look, relative to `yaw`: the keys push the body that way, not where the
    // look has got to. While the look is catching up after a quick turn the body is then
    // already going the new way - sideways or backwards at first, as far as its trunk is
    // concerned, and no faster than legs go like that. (Pushed along the look instead, it ran
    // on along a wide arc for as long as the trunk took to come round: seconds, at a sprint.)
    float steer = 0.0f;
    bool run = false, crouch = false;
    float walkSpeed = 1.7f, runSpeed = 4.2f;
};

struct LegFoot {
    FootPose pose;
    float side = 1.0f;        // -1 left, +1 right
    bool swing = false;
    bool armed = true;        // has been through its stance phase, may lift again
    bool clockSwing = false;  // swing belongs to the walking rhythm, not to a standing correction
    float u = 0.0f;           // swing progress
    float dodge = 0.0f;       // how far out of its path it has swung to get round the other leg (clearSwing)
    float load = 0.5f;        // share of body weight
    float age = 10.0f;        // seconds on the floor
    float strikePitch = 0.0f;
    Vec3 from, target;
    Vec3 landRoot;            // where the body is expected to be when this foot lands
    float fromYaw = 0.0f, fromPitch = 0.0f, targetYaw = 0.0f, landPitch = 0.0f;
    float pitchRate = 0.0f;   // how fast the planted foot is turning
    float swingTime = LEG_CORRECTION_SWING;
    // Running: the push of this leg (peak force / mass) over its planned ground time.
    float contactTime = 0.5f, thrust = 0.0f, pushAge = 10.0f;
};

struct LegStrike {
    int foot;      // 0 left, 1 right
    float force;   // 0..1
    Vec3 pos;
    float yaw;
};

struct LegSim {
    BodyDims dims = makeBodyDims(1.81f);
    Gait gait = {};
    float gaitScale = 1.0f, gaitTurn = 1.0f;  // what went into it: direction of travel, turning

    // Body
    Vec3 root, vel;
    Vec3 wantVel;             // what the input asks for
    float accel = 0.0f;       // how fast vel is closing in on it right now
    float speed = 0.0f;       // smoothed horizontal speed
    float wantSpeed = 0.0f;
    float bodyYaw = 0.0f, yawRate = 0.0f;
    float headYaw = 0.0f;     // last look direction
    float headTwist = 0.0f;   // look direction relative to the trunk, not wrapped: the trunk follows the way the head went
    float headRate = 0.0f;    // how fast the look direction is turning (smoothed)
    float crab = 0.0f;        // where the trunk is asked to point relative to the look: walking at an angle (moveBody)
    bool runMode = false;
    float run = 0.0f, crouch = 0.0f, idle = 1.0f;

    // Rhythm
    float phase = 0.0f;       // stride phase: left heel strike at 0, right at 0.5
    bool walking = false;
    float settle = 1.0f;      // time since the last foot came down
    LegFoot foot[2];

    // Pelvis (leg_body.h)
    float pelvisY = 0.0f, pelvisVy = 0.0f;
    float sinkV = 0.0f;                   // how fast the hips are giving way to keep a foot (enforceReach)
    Vec3 swayVec, swayVel;                // horizontal offset of the pelvis from the root (world)
    float sway = 0.0f;                    // ... its component along the body's right axis
    Vec3 surgeVec;                        // speed ripple along the direction of travel (world)
    float heightAvg = 0.0f;
    float heightTarget = 0.0f, heightTargetV = 0.0f;
    float landY = 0.0f;                   // hip height when the last foot came down
    float runY = 0.0f, runVy = 0.0f;      // running bounce (spring-mass)
    Vec3 travelDir = Vec3(0, 0, 1);
    float rot = 0.0f, rotV = 0.0f;        // transverse rotation, + = right hip forward
    float list = 0.0f, listV = 0.0f;      // frontal tilt, + = right side down
    float lastDrop = 0.0f;
    Vec3 lastCop;
    Vec3 pelvisPos, hip[2];
    float pelvisYaw = 0.0f;

    // Output for sound, camera jolt, footprints, tests
    LegStrike strikes[LEG_MAX_STRIKES];
    int strikeCount = 0;
    float lastStepTime = 0.0f, lastStepLen = 0.0f, sinceStrike = 0.0f;
    Vec3 lastStrikePos;
    int stepsTaken = 0;
    float lastDt = 1.0f / 120.0f;  // the time step being taken

    void reset(const Vec3& pos, float yaw);
    void step(const LegInput& in, float dt, LegBlockedFn blocked);

    float footPhase(int i) const {
        float p = phase - (i == 0 ? 0.0f : 0.5f);
        return p - floorf(p);
    }
    float crouchDrop() const { return 0.22f * crouch; }
    float legLength() const { return dims.standHip - crouchDrop(); }
    float maxSink() const { return gLerp(LEG_MAX_SINK, LEG_MAX_SINK_RUN, run); }
    float stepDrop() const;
    float stanceKnee(int i) const;
    float strikeShare(float landSpeed) const;
    Vec3 homePos(int i) const {
        float w = 0.19f + 0.08f * crouch;
        return root + headingRight(bodyYaw) * (foot[i].side * w * 0.5f);
    }
    float homeYaw(int i) const { return bodyYaw - foot[i].side * 0.14f; }
    float turnHurry() const { return gSat((fabsf(yawRate) - 0.8f) / 2.2f); }

    void moveBody(const LegInput& in, float dt, LegBlockedFn blocked);
    void updateRhythm(float dt, LegBlockedFn blocked);
    void liftFoot(int i, bool clockSwing, LegBlockedFn blocked);
    void planTarget(int i, bool snap, float dt, LegBlockedFn blocked);
    void updateSwing(int i, float dt, LegBlockedFn blocked);
    void landFoot(int i);
    void reachSwing(int i);
    void updateLoads(float dt);
    void updatePelvis(float dt);
    void placePelvis();
    void updateStancePitch(int i, float dt);
    void enforceReach(float dt, LegBlockedFn blocked);
    // leg_guard.h: the legs keep out of each other, and a foot is not kept longer than it can be.
    void turnBody(float lookYaw, float dt);
    void clearTarget(int i, Vec3& want, float wantYaw, const Vec3& base, float yawL) const;
    void clearSwing(int i);
    void hurryLanding(int i, float within);
    void guardFeet(LegBlockedFn blocked);
    Vec3 kneePole(int i) const;
    float followLook(float look, float& aim, float& rate, float moved, float dt) const;
    float turnMax() const;
    LegPose legPose(int i) const { return solveLeg(dims, hip[i], footAnkle(dims, foot[i].pose), kneePole(i)); }
};

inline void LegSim::reset(const Vec3& pos, float yaw) {
    *this = LegSim();
    root = Vec3(pos.x, 0.0f, pos.z);
    bodyYaw = headYaw = yaw;
    gait = gaitFor(0.0f, 1.0f, 0.0f, 0.0f, dims);
    for (int i = 0; i < 2; i++) {
        foot[i].side = i == 0 ? -1.0f : 1.0f;
        foot[i].pose.anchor = homePos(i);
        foot[i].pose.yaw = homeYaw(i);
        foot[i].pose.pitch = 0.0f;
        foot[i].pose.lift = 0.0f;
    }
    pelvisY = heightTarget = landY = dims.standHip;
    lastStrikePos = root;
    placePelvis();
}

// Input -> velocity -> position. People are slower backwards and sideways, and they need
// a couple of steps to get going.
// Nobody walks diagonally with the hips square to where they look: the trunk turns into the
// direction of travel and only the head stays on the view. So W+D is a plain forward walk
// with the trunk 45 degrees to the right, S+D a backward walk with it 45 degrees to the
// left, and only A/D alone is a sidestep. In between (analog sticks) the trunk angle is a
// triangle wave of the input angle, so nothing jumps. The speed limit is that of the gait
// the legs really do: forward, backward or sideways with respect to the trunk.
inline float crabAngle(float a) {  // a: input direction, 0 = forward, +pi/2 = right
    const float q = 0.25f * MATH_PI;
    if (a > 3.0f * q) return a - MATH_PI;
    if (a < -3.0f * q) return a + MATH_PI;
    if (a > q) return 2.0f * q - a;
    if (a < -q) return -2.0f * q - a;
    return a;
}

inline void LegSim::moveBody(const LegInput& in, float dt, LegBlockedFn blocked) {
    Vec3 f = headingFwd(in.yaw + in.steer), r = headingRight(in.yaw + in.steer);
    float mf = in.moveFwd, mr = in.moveRight;
    float ml = sqrtf(mf * mf + mr * mr);
    Vec3 want;
    float crabWant = 0.0f;
    if (ml > 0.01f) {
        mf /= ml;
        mr /= ml;
        if (ml > 1.0f) ml = 1.0f;
        float top = (in.run && !in.crouch) ? in.runSpeed : in.walkSpeed;
        if (in.crouch) top *= 0.55f;
        float dir = atan2f(mr, mf), turn = crabAngle(dir);
        crabWant = -turn;  // (a growing yaw turns left)
        float tf = cosf(dir - turn - in.steer), tr = sinf(dir - turn - in.steer);  // travel as the turned trunk sees it
        float a = tf / (tf >= 0.0f ? 1.0f : 0.60f), b = tr / 0.45f;
        want = (f * mf + r * mr) * (top * ml / sqrtf(a * a + b * b));
    }
    // (No faster than the trunk can come round at this speed: asked for more, it only falls behind.)
    float crabRate = fminf(LEG_CRAB_RATE, 0.85f * turnMax()) * dt;
    crab += gClamp(crabWant - crab, -crabRate, crabRate);
    wantVel = want;
    wantSpeed = want.len();

    Vec3 dv = want - vel;
    float dl = dv.len();
    float sp = vel.len();
    accel = wantSpeed > sp ? 3.5f + 1.2f * wantSpeed : 6.5f;
    // Going one way and asked to go another, the feet are dug in: the old speed is braked off
    // far harder than a walk is started or let run out. (Without this a turn at a sprint is a
    // skid: the body carries on for metres along the way it was going.)
    if (wantSpeed > 0.05f && sp > 0.05f) {
        float off = 1.0f - want.dot(vel) / (wantSpeed * sp);  // 0 the same way .. 2 the opposite
        accel = gLerp(accel, fmaxf(accel, LEG_CUT_ACCEL), gSmooth(off / LEG_CUT_FULL));
    }
    if (dl > accel * dt) dv = dv * (accel * dt / dl);
    vel += dv;

    float nx = root.x + vel.x * dt, nz = root.z + vel.z * dt;
    // (A wall takes the wish with it: the feet must not be sent where the body cannot follow.)
    if (blocked && blocked(nx, root.z, LEG_BODY_RADIUS)) vel.x = wantVel.x = 0.0f;
    else root.x = nx;
    if (blocked && blocked(root.x, nz, LEG_BODY_RADIUS)) vel.z = wantVel.z = 0.0f;
    else root.z = nz;

    speed += (vel.len() - speed) * (1.0f - expf(-dt / 0.10f));
    crouch += ((in.crouch ? 1.0f : 0.0f) - crouch) * (1.0f - expf(-dt * 7.0f));

    // Walk <-> run switches near Froude 0.5 (about 2.1 m/s), with hysteresis.
    if (!runMode && speed > 2.25f) runMode = true;
    if (runMode && speed < 1.95f) runMode = false;
    run += ((runMode ? 1.0f : 0.0f) - run) * (1.0f - expf(-dt * 6.0f));

    idle += ((walking ? 0.0f : 1.0f) - idle) * (1.0f - expf(-dt * 5.0f));
    turnBody(in.yaw, dt);
}

inline void LegSim::liftFoot(int i, bool clockSwing, LegBlockedFn blocked) {
    LegFoot& f = foot[i];
    f.swing = true;
    f.armed = false;
    f.clockSwing = clockSwing;
    f.u = 0.0f;
    f.dodge = 0.0f;
    f.from = f.pose.anchor;
    f.fromYaw = f.pose.yaw;
    f.fromPitch = f.pose.pitch;
    // Stepping round on the spot gets quicker the faster the trunk turns.
    f.swingTime = clockSwing ? gait.swingTime : gLerp(LEG_CORRECTION_SWING, 0.20f, turnHurry());
    planTarget(i, true, 0.0f, blocked);
}

// Where should this foot come down? The classic answer: where the hip will be half a
// stance later, so the body passes over the foot exactly at mid stance.
inline void LegSim::planTarget(int i, bool snap, float dt, LegBlockedFn blocked) {
    LegFoot& f = foot[i];
    Vec3 want;
    float wantYaw, wantPitch = 0.0f;
    Vec3 base = root;
    float yawL = bodyYaw;
    if (f.clockSwing) {
        // Look ahead the way the body will really move: its velocity closes in on the input
        // at a known rate, so both the place and the speed at landing can be predicted.
        // (Plain "position + velocity * time" puts the last step of a stop far out in front.)
        float tRem = (1.0f - f.u) * f.swingTime;
        Vec3 dv = wantVel - vel;
        float dvLen = dv.len();
        Vec3 velLand = vel;
        // Twice: the stride gets shorter as the body speeds up, so the foot lands sooner than
        // today's clock says. (Without this a start from rest throws the foot a metre ahead.)
        for (int pass = 0; pass < 2; pass++) {
            velLand = vel;
            base = root + vel * tRem;
            if (dvLen > 1e-4f && accel > 0.0f) {
                float tc = dvLen / accel;
                float gained = tRem < tc ? accel * tRem : dvLen;
                float moved = tRem < tc ? 0.5f * accel * tRem * tRem : dvLen * (tRem - 0.5f * tc);
                velLand = vel + dv * (gained / dvLen);
                base = base + dv * (moved / dvLen);
            }
            if (pass > 0 || velLand.len() <= speed + 0.1f) break;
            float later = gaitFor(velLand.len(), gaitScale, run, crouch, dims).strideTime * gaitTurn;
            if (later >= gait.strideTime) break;
            tRem *= 0.5f * (1.0f + later / gait.strideTime);
        }
        // The path bends with the look direction: by then the body has gone round an arc and
        // moves along its far end.
        float bend = gClamp(headRate * tRem, -0.8f, 0.8f);
        velLand = turnedBy(velLand, bend);
        base = root + turnedBy(base - root, 0.5f * bend);
        float landSpeed = velLand.len();
        yawL = bodyYaw + gClamp(yawRate * tRem, -0.8f, 0.8f);
        Vec3 rL = headingRight(yawL);
        float along = landSpeed > 0.05f ? fabsf(velLand.dot(headingFwd(yawL))) / landSpeed : 1.0f;
        float share = gLerp(LEG_LEAD_SIDE, gLerp(strikeShare(landSpeed), LEG_LEAD_RUN, run), along);
        // From landing to mid stance the path goes on bending; the foot is put beside the
        // point of the arc where the hip will pass it, turned the way the trunk will be there.
        float over = gait.stanceTime * share;
        float yawOver = gClamp(yawRate * over, -0.45f, 0.45f);
        Vec3 lead = turnedBy(velLand * over, 0.5f * gClamp(headRate * over, -0.45f, 0.45f));
        float leadLen = lead.len(), leadMax = 0.52f * dims.standHip;
        if (leadLen > leadMax) lead = lead * (leadMax / leadLen);
        // Coming to rest the feet go to their standing width. Sidestepping, the track is
        // widened so the feet close up instead of crossing.
        float width = gLerp(0.19f, gait.stepWidth, gSat(landSpeed / 0.5f));
        float latStep = fabsf(velLand.dot(rL)) * gait.stepTime;
        if (width < latStep + LEG_SIDESTEP_GAP) width = latStep + LEG_SIDESTEP_GAP;
        want = base + lead + headingRight(yawL + yawOver) * (f.side * width * 0.5f);
        wantYaw = yawL + yawOver - f.side * gait.toeOut;
        // Heel first going forward, toes first going backward, flat when shuffling.
        float x = gClamp(lead.dot(headingFwd(wantYaw)) / 0.30f, -1.0f, 1.0f);
        wantPitch = gait.strikePitch * (x >= 0.0f ? x : 0.7f * x);
        clearTarget(i, want, wantYaw, base, yawL);
    } else {
        // Turning on the spot the foot goes where its place will be half way through its time
        // on the floor, so it is neither behind the trunk nor ahead of it for long.
        float tRem = (1.0f - f.u) * f.swingTime;
        yawL = bodyYaw + gClamp(yawRate * (tRem + 0.5f * f.swingTime), -0.7f, 0.7f);
        want = root + headingRight(yawL) * (f.side * (0.19f + 0.08f * crouch + 0.05f * turnHurry()) * 0.5f);
        wantYaw = yawL - f.side * 0.14f;
    }
    want.y = 0.0f;
    f.landRoot = base;
    if (blocked && blocked(want.x, want.z, LEG_FOOT_RADIUS)) {
        // Never step into a wall: pull the landing spot back towards the body - no further than
        // it has to come, or it ends up under the other foot.
        for (int k = 1; k <= 8; k++) {
            Vec3 p = want + (base - want) * (k * 0.125f);
            if (!blocked(p.x, p.z, LEG_FOOT_RADIUS) || k == 8) { want = p; break; }
        }
    }
    if (snap) {
        f.target = want;
        f.targetYaw = wantYaw;
        f.landPitch = wantPitch;
        return;
    }
    float a = 1.0f - expf(-dt * 18.0f);
    // (A change of mind moves the spot by most of a metre at once - the more so the harder the
    // body brakes. The leg takes a moment to redirect: the foot is not thrown after the spot.)
    Vec3 was = f.target;
    f.target += (want - f.target) * a;
    f.targetYaw += gAngleDiff(f.targetYaw, wantYaw) * a;
    f.landPitch += (wantPitch - f.landPitch) * a;
    if (f.clockSwing) {
        // The spot follows the plan with a little lag; the other foot does not wait for it.
        Vec3 clear = f.target;
        clearTarget(i, clear, f.targetYaw, base, yawL);
        if (!blocked || !blocked(clear.x, clear.z, LEG_FOOT_RADIUS)) f.target = clear;
    }
    Vec3 shift = f.target - was;
    float shiftLen = shift.len(), shiftMax = LEG_RETARGET_SPEED * dt;
    if (shiftLen > shiftMax) {
        Vec3 part = was + shift * (shiftMax / shiftLen);
        f.target = blocked && blocked(part.x, part.z, LEG_FOOT_RADIUS) ? was : part;
    }
}

inline void LegSim::landFoot(int i) {
    LegFoot& f = foot[i];
    f.swing = false;
    f.u = 0.0f;
    f.age = 0.0f;
    f.pose.anchor = f.target;
    f.pose.yaw = f.targetYaw;
    f.pose.pitch = f.landPitch;
    if (f.pose.lift > 0.05f) f.pose.lift = 0.05f;
    f.strikePitch = f.landPitch;
    f.pitchRate = 0.0f;
    landY = heightTarget + crouchDrop();
    settle = 0.0f;
    // Running: the leg pushes with a half-sine force whose impulse carries the body for one step.
    f.contactTime = gait.stanceTime;
    f.pushAge = 0.0f;
    f.thrust = f.clockSwing ? GAIT_G * MATH_PI * gait.stepTime / (2.0f * f.contactTime) : 0.0f;

    float force = f.clockSwing ? 0.40f + 0.60f * gSat(speed / 4.2f) : 0.22f;
    if (strikeCount < LEG_MAX_STRIKES) strikes[strikeCount++] = {i, force, f.pose.anchor, f.pose.yaw};
    lastStepTime = sinceStrike;
    lastStepLen = (f.pose.anchor - lastStrikePos).len();
    lastStrikePos = f.pose.anchor;
    sinceStrike = 0.0f;
    stepsTaken++;
}

inline void LegSim::updateSwing(int i, float dt, LegBlockedFn blocked) {
    LegFoot& f = foot[i];
    if (f.clockSwing) {
        // Locked to the stride clock: the foot comes down exactly at its heel strike phase,
        // however early or late it left the floor.
        float left = (1.0f - footPhase(i)) * gait.strideTime;
        if (footPhase(i) < gait.duty * 0.5f) left = 0.0f;
        f.swingTime = f.u < 0.999f ? left / (1.0f - f.u) : dt;
        if (f.swingTime < 0.12f) f.swingTime = 0.12f;
    }
    f.u += dt / f.swingTime;
    if (f.u >= 1.0f) {
        landFoot(i);
        return;
    }
    // Late in the swing the leg is committed: the landing spot no longer moves.
    if (f.u < 0.80f) planTarget(i, false, dt, blocked);
    else f.landRoot = root + vel * ((1.0f - f.u) * f.swingTime);  // keeps closing in on the truth

    // Horizontal travel: the foot leaves at the speed the body has (it was rolling over the
    // toes), flies past it and arrives with no speed left. A cubic with a start slope only.
    float e = gSmooth(f.u);
    float span = (f.target - f.from).len();
    float m0 = f.clockSwing && span > 0.05f ? gClamp(vel.len() * gLerp(1.0f, 0.55f, run) * f.swingTime / span, 0.0f, 1.5f) : 0.0f;
    float u = f.u, u2 = u * u, u3 = u2 * u;
    float travel = m0 * (u3 - 2.0f * u2 + u) + (3.0f * u2 - 2.0f * u3);
    f.pose.anchor = f.from + (f.target - f.from) * travel;
    // Walking round a bend the body goes along an arc, and the foot with it: the straight line
    // from footprint to footprint would cut across the inside of the bend, through the other leg.
    float sp = vel.len();
    if (f.clockSwing && sp > 0.3f && span > 0.05f) {
        float sag = gClamp(headRate * span * span / (8.0f * sp), -0.20f, 0.20f) * 4.0f * travel * (1.0f - gSat(travel));
        Vec3 c = f.target - f.from;
        f.pose.anchor += Vec3(-c.z, 0.0f, c.x) * (sag / span);
    }
    f.pose.yaw = f.fromYaw + gAngleDiff(f.fromYaw, f.targetYaw) * e;
    clearSwing(i);
    // The foot leaves heel-up, levels out by mid swing and arrives toes-up.
    f.pose.pitch = gLerp(f.fromPitch, f.landPitch, gSmooth((f.u - 0.05f) / 0.80f));
    // Clearance is what the lowest point of the foot has. While the heel is up that point is
    // the toe tip, so the ball has to come up by the hanging toes as well.
    float pitch = f.pose.pitch;
    float toeHang = pitch < 0.0f ? (dims.toeFwd - dims.ballFwd) * sinf(-pitch) : 0.0f;
    f.pose.lift = swingLift(f.u, f.clockSwing ? gait.clearance : 0.035f, f.clockSwing ? run : 0.0f)
                + toeHang * gSmooth(f.u / 0.35f);
    // In the air the foot turns about its ankle, not about the toe tip and then the heel:
    // round off the corner the ankle's path would have where the pitch changes sign.
    const float round = 0.10f;
    f.pose.lift += 0.5f * (dims.toeFwd + dims.heelBack) * (sqrtf(pitch * pitch + round * round) - fabsf(pitch))
                 * gSmooth(f.u / 0.15f) * gSmooth((1.0f - f.u) / 0.10f);
}

// The swinging leg unfolds towards the floor but is never yanked straight: while the hip is
// still too far back and too high for the heel to reach its spot, the foot waits just above
// it and comes down as the body arrives.
inline void LegSim::reachSwing(int i) {
    LegFoot& f = foot[i];
    Vec3 d = hip[i] - footAnkle(dims, f.pose);
    float maxLen = dims.legMax() * LEG_SWING_REACH;
    float dh2 = d.x * d.x + d.z * d.z;
    if (d.y <= 0.0f || dh2 >= maxLen * maxLen) return;
    // Soft stop: the last centimetre of extension is approached, never hit.
    const float soft = 0.012f;
    float len = sqrtf(dh2 + d.y * d.y);
    float start = maxLen - soft;
    if (len <= start) return;
    float want = maxLen - soft * expf(-(len - start) / soft);
    // (Fades in after toe-off: a leg that leaves the floor straight is not snatched up.)
    // (And bounded: a foot that is simply too far away is not kicked up to hip height.)
    float up = d.y - sqrtf(want * want - dh2);
    f.pose.lift += (up < 0.12f ? up : 0.12f) * gSmooth(f.u / 0.25f);
}

// Decides when feet leave the floor.
inline void LegSim::updateRhythm(float dt, LegBlockedFn blocked) {
    bool moving = vel.len() > 0.08f || speed > 0.12f;
    bool anySwing = foot[0].swing || foot[1].swing;

    if (moving && !walking) {
        // Gait initiation. A corrective step that is already in the air simply becomes the
        // first step; otherwise the leading foot goes first sideways and the rear one else.
        int first = foot[0].swing ? 0 : (foot[1].swing ? 1 : -1);
        float p = gait.duty;
        if (first >= 0) {
            foot[first].clockSwing = true;
            p += foot[first].u * (1.0f - gait.duty);
        } else {
            Vec3 dir = vel.norm();
            float lat = dir.dot(headingRight(bodyYaw));
            if (fabsf(lat) > 0.70f) first = lat > 0.0f ? 1 : 0;
            else first = (foot[0].pose.anchor - root).dot(dir) < (foot[1].pose.anchor - root).dot(dir) ? 0 : 1;
            foot[first].armed = true;
        }
        p += first == 0 ? 0.0f : 0.5f;
        phase = p - floorf(p);
        foot[1 - first].armed = true;
        walking = true;
    }

    if (walking) {
        phase += dt / gait.strideTime;
        phase -= floorf(phase);
        if (!moving && !anySwing) walking = false;
    }

    for (int i = 0; i < 2; i++) {
        LegFoot& f = foot[i];
        LegFoot& o = foot[1 - i];
        if (f.swing) continue;
        if (walking && moving) {
            if (footPhase(i) < gait.duty) {
                f.armed = true;
            } else if (f.armed) {
                // Walking always keeps one foot down; running may leave the floor.
                bool otherDown = !o.swing && o.age > 0.03f;
                // Sideways one foot steps out and the other closes up. When it is the closing
                // foot's turn and there is nothing to close - the feet are together, because
                // the body has just turned back (A after D) - the other one goes first.
                // (Stepping "after" the body with the feet together, it could only be put down
                // behind the foot that leads, and both were left a leg's length behind.)
                Vec3 dir = vel.norm();
                float lat = dir.dot(headingRight(bodyYaw));
                if (otherDown && fabsf(lat) > 0.70f && f.side * lat < 0.0f && o.armed
                    && (o.pose.anchor - f.pose.anchor).dot(dir) < 0.19f + 0.3f * speed * gait.stepTime) {
                    phase += 0.5f;
                    phase -= floorf(phase);
                    continue;
                }
                if (run > 0.5f || otherDown) liftFoot(i, true, blocked);
            }
        }
    }

    if (!walking && !foot[0].swing && !foot[1].swing && settle > gLerp(0.10f, 0.02f, turnHurry())) {
        // Standing: a foot that ended up far from under its hip, or twisted, takes a small
        // corrective step (stopping, turning on the spot).
        int worst = -1;
        float worstScore = 1.0f;
        for (int i = 0; i < 2; i++) {
            Vec3 d = foot[i].pose.anchor - homePos(i);
            float score = sqrtf(d.x * d.x + d.z * d.z) / 0.07f;
            // (Measured against where the trunk is about to be: the foot it is leaving behind goes first.)
            float twist = fabsf(gAngleDiff(foot[i].pose.yaw, homeYaw(i) + 0.12f * yawRate)) / 0.45f;
            if (twist > score) score = twist;
            if (score > worstScore) { worstScore = score; worst = i; }
        }
        if (worst >= 0) liftFoot(worst, false, blocked);
    }
}

// Weight moves from the trailing to the leading foot during double support.
inline void LegSim::updateLoads(float dt) {
    bool d0 = !foot[0].swing, d1 = !foot[1].swing;
    float t[2] = {0.0f, 0.0f};
    if (d0 && d1) {
        t[0] = t[1] = 0.5f;
        if (walking && gait.dsTime > 0.005f) {
            int lead = foot[0].age < foot[1].age ? 0 : 1;
            float w = gSmooth(foot[lead].age / gait.dsTime);
            t[lead] = w;
            t[1 - lead] = 1.0f - w;
        }
    } else if (d0) {
        t[0] = 1.0f;
    } else if (d1) {
        t[1] = 1.0f;
    }
    float a = 1.0f - expf(-dt / 0.03f);
    for (int i = 0; i < 2; i++) foot[i].load += (t[i] - foot[i].load) * a;
}

#include "leg_body.h"
#include "leg_guard.h"

inline void LegSim::step(const LegInput& in, float dt, LegBlockedFn blocked) {
    if (dt <= 0.0f) return;
    lastDt = dt;
    moveBody(in, dt, blocked);

    if (vel.lenSq() > 0.01f) travelDir = vel.norm();
    float dirScale = gaitDirScale(travelDir.dot(headingFwd(bodyYaw)), travelDir.dot(headingRight(bodyYaw)));
    gait = gaitFor(speed, dirScale, run, crouch, dims);
    // Turning, the steps get quicker and shorter, so that no foot stays planted while the trunk
    // turns further over it than an ankle and a hip can take.
    gaitScale = dirScale;
    gaitTurn = gClamp(LEG_TURN_TWIST / (fabsf(yawRate) * gait.stanceTime + 1e-4f), 0.45f, 1.0f);
    gait.stepTime *= gaitTurn;
    gait.strideTime *= gaitTurn;
    gait.stanceTime *= gaitTurn;
    gait.swingTime *= gaitTurn;
    gait.dsTime *= gaitTurn;

    settle += dt;
    sinceStrike += dt;
    for (int i = 0; i < 2; i++) {
        if (!foot[i].swing) foot[i].age += dt;
        foot[i].pushAge += dt;
    }
    updateRhythm(dt, blocked);
    for (int i = 0; i < 2; i++) {
        if (foot[i].swing) updateSwing(i, dt, blocked);
    }
    updateLoads(dt);
    updatePelvis(dt);
    for (int i = 0; i < 2; i++) {
        if (foot[i].swing) reachSwing(i);
        else updateStancePitch(i, dt);
    }
    enforceReach(dt, blocked);
    guardFeet(blocked);
}
