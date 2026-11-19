#pragma once
// Included by leg_sim.h: what keeps the body in one piece when the input is not a steady walk.

// ============================================================================
// GUARDS
// A steady gait never needs any of this. Turning does, and so does every abrupt change
// (a flick of the mouse, W to S, a start in the middle of a stop):
//   - the trunk turns no faster than feet can step round under it; the head is free,
//   - a foot is never put down on the far side of the other one, or out of reach,
//   - a swinging foot goes round the standing leg, not through it,
//   - a planted foot that the trunk has turned too far over, or that has ended up on the
//     wrong side of the other, is picked up early - and if the other foot is in the air,
//     that one is brought down first. Until it can go, the foot turns on the spot.
// ============================================================================

inline constexpr float LEG_FOOT_GAP = 0.10f;    // m between the ankles when the feet are side by side
inline constexpr float LEG_SOLE_GAP = 0.085f;   // m between the centre lines of the soles, anywhere along them
inline constexpr float LEG_FOOT_PASS = 0.04f;   // ... and when one is well in front of the other
inline constexpr float LEG_TWIST_MAX = 0.58f;   // rad between a planted foot and the trunk: past it the foot steps
inline constexpr float LEG_PIVOT_MAX = 0.66f;   // ... and past this it turns on the spot, as a foot does in a sharp turn
inline constexpr float LEG_DODGE_OUT = 5.0f;    // m/s sideways, a swinging foot getting out of the other leg's way
inline constexpr float LEG_DODGE_BACK = 1.2f;   // ... and coming back onto its path
inline constexpr float LEG_HURRY_TIME = 0.07f;  // s, how soon a foot that is needed comes down
inline constexpr float LEG_HURRY_SPEED = 3.5f;  // m/s over the floor, the most it makes on the way there (mean; the peak is half as much again)
inline constexpr float LEG_HURRY_SPEED_RUN = 6.0f;
inline constexpr float LEG_NECK_MAX = 0.75f;     // rad the look may lead the trunk
inline constexpr float LEG_NECK_TIME = 0.08f;    // s, how softly the look slows towards that
inline constexpr float LEG_TURN_GRIP = 6.0f;     // m/s2 sideways that a turn at speed may ask of the feet
inline constexpr float LEG_LOOK_EASE = 0.10f;    // s to where the view stopped at which a look that is catching up starts to brake
inline constexpr float LEG_HOLD_REACH = 0.50f;  // m from hip to footprint that a walking leg can still stand on

// How fast the trunk may turn: as fast as feet can step round, and at speed no faster than
// the path can bend without the runner leaning over further than the legs here can.
inline float LegSim::turnMax() const { return fminf(LEG_TURN_MAX, LEG_TURN_GRIP / (speed + 0.01f)); }

// The trunk follows the head: at once while walking, and only past 30 degrees while standing.
// A steady turn is followed without lag (lag would make the body walk sideways into its own
// feet); a turn faster than LEG_TURN_MAX is followed at that rate, the same way round.
inline void LegSim::turnBody(float lookYaw, float dt) {
    float turned = gAngleDiff(headYaw, lookYaw);
    headYaw = lookYaw;
    headTwist += turned;
    float most = turnMax();
    headRate += (gClamp(turned / dt, -most, most) - headRate) * (1.0f - expf(-dt / 0.06f));

    // A whole turn behind is no turn at all; and once the head has stopped, the short way round
    // is the way.
    float wrap = fabsf(headRate) > 0.5f ? MATH_TAU : MATH_PI;
    if (headTwist > wrap) headTwist -= MATH_TAU;
    if (headTwist < -wrap) headTwist += MATH_TAU;
    // (Walking at an angle the trunk is wanted `crab` away from the look, not under it.)
    float dead = 0.55f * idle, off = headTwist + crab;
    float pull = fabsf(off) > dead ? off - (off > 0.0f ? dead : -dead) : 0.0f;
    float lead = headRate * gSat(fabsf(pull) / 0.10f + (1.0f - idle));
    float x = 0.0f, v = yawRate - lead;
    gSpring(x, v, pull, 9.0f, dt);
    float step = gClamp(x + lead * dt, -most * dt, most * dt);
    yawRate = gClamp(v + lead, -most, most);
    bodyYaw += step;
    headTwist -= step;
}

// The look direction (LegInput::yaw: where the head points and the body is heading) for this
// frame. It follows the view - the eyes, which the mouse turns freely and which the caller
// owns; `moved` is how far the view has turned since the last call. `aim` (where the look is
// going) and `rate` (how fast it is turning) are kept here, the caller only stores them.
// A head cannot be turned further from the trunk than a neck goes, and the trunk turns no
// faster than the feet can step round. Inside the neck's range the look is as quick as the
// view; near its end it slows to the trunk's pace. Nothing the view asks is dropped: the
// eyes are not held back, the body comes round after them.
// What the view does in this frame is followed at once. What the look still owes from before
// is paid off as fast as the body allows, and from LEG_LOOK_EASE seconds before it would get
// there it brakes evenly: a look that has been catching up runs out, it does not stop dead.
inline float LegSim::followLook(float look, float& aim, float& rate, float moved, float dt) const {
    float behind = aim - look;
    aim += moved;
    // A whole turn behind is no turn at all. Less than that, the look goes the way the view went.
    float off = aim - look > MATH_TAU ? -MATH_TAU : (aim - look < -MATH_TAU ? MATH_TAU : 0.0f);
    aim += off;
    behind += off;
    float err = aim - look, dir = err < 0.0f ? -1.0f : 1.0f;
    float fresh = gClamp((err - behind) * dir, 0.0f, fabsf(err));
    float rest = fabsf(err) - fresh;
    float v = rate * dir - fresh / dt;  // how fast the old debt was being paid
    float pay = fminf(rest / LEG_LOOK_EASE, rest / dt), end = pay;
    if (v > pay) {
        float stop = 2.0f * rest / v;   // s it takes to stop exactly there
        end = stop > dt ? v * (1.0f - dt / stop) : 0.0f;
        pay = stop > dt ? 0.5f * (v + end) : rest / dt;
    }

    float room = gClamp(LEG_NECK_MAX - (headTwist + crab) * dir, 0.0f, 2.0f * LEG_NECK_MAX);
    float most = (turnMax() + room / LEG_NECK_TIME) * dt;
    float step = fminf(fresh + pay * dt, fabsf(err));
    rate = dir * (step > most ? most / dt : fresh / dt + end);
    return look + dir * fminf(step, most);
}

// Distance between two segments on the floor (the centre lines of two feet).
inline float floorSegDist(const Vec3& p1, const Vec3& q1, const Vec3& p2, const Vec3& q2) {
    Vec3 d1 = q1 - p1, d2 = q2 - p2, w = p1 - p2;
    float a = d1.x * d1.x + d1.z * d1.z, e = d2.x * d2.x + d2.z * d2.z;
    float b = d1.x * d2.x + d1.z * d2.z, c = d1.x * w.x + d1.z * w.z, f = d2.x * w.x + d2.z * w.z;
    float den = a * e - b * b;
    float s = den > 1e-9f ? gSat((b * f - c * e) / den) : 0.0f;
    float t = e > 1e-9f ? (b * s + f) / e : 0.0f;
    if (t < 0.0f) { t = 0.0f; s = a > 1e-9f ? gSat(-c / a) : 0.0f; }
    else if (t > 1.0f) { t = 1.0f; s = a > 1e-9f ? gSat((b - c) / a) : 0.0f; }
    float dx = w.x + d1.x * s - d2.x * t, dz = w.z + d1.z * s - d2.z * t;
    return sqrtf(dx * dx + dz * dz);
}

// Landing spot of foot i: on its own side of the other foot, and within reach of the hip
// it will hang from.
inline void LegSim::clearTarget(int i, Vec3& want, float wantYaw, const Vec3& base, float yawL) const {
    const LegFoot& f = foot[i];
    const LegFoot& o = foot[1 - i];
    Vec3 r = headingRight(yawL), fw = headingFwd(yawL);
    // Both in the air (running, a recovery step): the one that comes down first goes where it
    // was going, and the other finds its place beside that. (Giving way to a foot that has only
    // just left the floor threw this one half a metre sideways at the end of its swing.)
    bool first = o.swing && (1.0f - o.u) * o.swingTime > (1.0f - f.u) * f.swingTime;
    Vec3 d = want - (o.swing ? o.target : o.pose.anchor);
    float lat = d.dot(r) * f.side;
    float need = gLerp(LEG_FOOT_GAP, LEG_FOOT_PASS, gSmooth((fabsf(d.dot(fw)) - 0.18f) / 0.17f));
    if (lat < need && !first) want += r * (f.side * (need - lat));
    // Turned towards each other, the toes of one would still land on the heel of the other:
    // the soles themselves have to be (the narrow end of) a foot's width apart.
    Vec3 oa = o.swing ? o.target : o.pose.anchor;
    Vec3 of = headingFwd(o.swing ? o.targetYaw : o.pose.yaw), wf = headingFwd(wantYaw);
    for (int k = 0; k < 3 && !first; k++) {
        float gap = floorSegDist(want - wf * dims.heelBack, want + wf * dims.toeFwd, oa - of * dims.heelBack, oa + of * dims.toeFwd);
        if (gap >= LEG_SOLE_GAP) break;
        want += r * (f.side * (LEG_SOLE_GAP - gap + 0.005f));
    }
    Vec3 hipL = base + r * (f.side * dims.hipHalf);
    Vec3 h = want - hipL;
    h.y = 0.0f;
    float hl = h.len();
    if (hl > LEG_LAND_REACH) want = hipL + h * (LEG_LAND_REACH / hl);
}

// A swinging foot that would pass through the other leg swings out round it instead.
inline void LegSim::clearSwing(int i) {
    LegFoot& f = foot[i];
    const LegFoot& o = foot[1 - i];
    Vec3 r = headingRight(bodyYaw), fw = headingFwd(bodyYaw);
    Vec3 d = f.pose.anchor - o.pose.anchor;
    float lat = d.dot(r) * f.side;
    // Only while the feet overlap fore and aft, and are at the same height (a runner's foot
    // passes the other shin well above the floor).
    float beside = 1.0f - gSmooth((fabsf(d.dot(fw)) - 0.20f) / 0.14f);
    float level = 1.0f - gSmooth((fabsf(f.pose.lift - o.pose.lift) - 0.10f) / 0.10f);
    float need = LEG_FOOT_GAP * beside;
    float push = lat < need ? need - lat : 0.0f;
    // Past each other, heel and toes still have to miss.
    Vec3 at = f.pose.anchor + r * (f.side * push);
    Vec3 sf = headingFwd(f.pose.yaw), of = headingFwd(o.pose.yaw);
    float gap = floorSegDist(at - sf * dims.heelBack, at + sf * dims.toeFwd,
                             o.pose.anchor - of * dims.heelBack, o.pose.anchor + of * dims.toeFwd);
    if (gap < LEG_SOLE_GAP) push += LEG_SOLE_GAP - gap;
    // The way round is a movement of the leg, not a property of the place: the feet pass each
    // other at several metres a second, and what they need changes by a hand's width from one
    // tick to the next. The foot goes out quickly and comes back in its own time...
    // (Height only excuses a foot that is on its own side: one that would cross the standing
    // leg's line takes its shin through the other shin however high the foot is.)
    push *= gLerp(level, 1.0f, gSmooth((0.05f - lat) / 0.05f)) * gSmooth(f.u / 0.25f);
    f.dodge += gClamp(push - f.dodge, -LEG_DODGE_BACK * lastDt, LEG_DODGE_OUT * lastDt);
    // ... and is back by the time it lands, so it arrives where it was sent.
    f.pose.anchor += r * (f.side * f.dodge * gSmooth((1.0f - f.u) / 0.12f));
}

// Foot i is in the air and the other leg cannot wait for it: it comes down now. The stride
// clock is moved forward with it, so the rhythm carries on from the new footfall.
inline void LegSim::hurryLanding(int i, float within) {
    LegFoot& f = foot[i];
    if (!f.swing) return;
    if (within < LEG_HURRY_TIME) within = LEG_HURRY_TIME;
    // However badly it is needed, a foot is not thrown at the floor: it gets the time the rest
    // of its way takes at the pace of a quick step.
    Vec3 togo = f.target - f.pose.anchor;
    float need = sqrtf(togo.x * togo.x + togo.z * togo.z) / gLerp(LEG_HURRY_SPEED, LEG_HURRY_SPEED_RUN, run);
    if (within < need) within = need;
    if (!f.clockSwing) {
        float t = within / (1.0f - f.u + 1e-3f);
        if (t < f.swingTime) f.swingTime = t > 0.12f ? t : 0.12f;
        return;
    }
    float p = footPhase(i);
    if (p < gait.duty * 0.5f) return;  // already due
    float early = (1.0f - p) - within / gait.strideTime;
    if (early <= 0.0f) return;
    phase += early;
    phase -= floorf(phase);
}

// Where the knee points (the pole of the leg IK): mostly where the foot does, a little where
// the pelvis does - and never across the other leg. A foot turned inwards under a crouching
// body would otherwise put its knee through the other knee.
inline Vec3 LegSim::kneePole(int i) const {
    float turn = 0.7f * gAngleDiff(pelvisYaw, foot[i].pose.yaw);
    const float inwards = 0.12f;
    if (turn * foot[i].side > inwards) turn = foot[i].side * inwards;
    return headingFwd(pelvisYaw + turn);
}

inline void LegSim::guardFeet(LegBlockedFn blocked) {
    Vec3 r = headingRight(bodyYaw), fw = headingFwd(bodyYaw);
    // (A foot in the air is judged by where it is going: on its way there it passes the other
    // one closely, and round a bend on what looks like the wrong side of it.)
    Vec3 apart = (foot[1].swing ? foot[1].target : foot[1].pose.anchor) - (foot[0].swing ? foot[0].target : foot[0].pose.anchor);
    bool crossed = apart.dot(r) < 0.0f && fabsf(apart.dot(fw)) < 0.25f;
    for (int i = 0; i < 2; i++) {
        LegFoot& f = foot[i];
        LegFoot& o = foot[1 - i];
        if (f.swing) continue;
        // The body is leaving its only support behind (a hard start, a shove of speed): the
        // other foot is put down by the time this one cannot be stood on any more, not after.
        float sp = vel.len();
        if (o.swing && run < 0.5f && sp > 0.2f) {
            Vec3 d = hip[i] - f.pose.anchor;
            d.y = 0.0f;
            Vec3 then = d + vel * ((1.0f - o.u) * o.swingTime);
            if (d.dot(vel) > 0.0f && then.len() > LEG_HOLD_REACH) hurryLanding(1 - i, (LEG_HOLD_REACH - d.len()) / sp);
        }
        float twist = gAngleDiff(homeYaw(i), f.pose.yaw);
        if (fabsf(twist) > LEG_PIVOT_MAX) f.pose.yaw = homeYaw(i) + (twist > 0.0f ? LEG_PIVOT_MAX : -LEG_PIVOT_MAX);
        bool twisted = fabsf(twist) > LEG_TWIST_MAX;
        // Of two crossed feet the one that has stood longer goes.
        bool wrongSide = crossed && (o.swing || f.age >= o.age);
        if (!twisted && !wrongSide) continue;
        if (o.swing) {
            // (A foot that is only turned waits for a step that is on its way anyway, turning
            // on the spot if it has to. Whipping the other leg down for it looks far worse.)
            if (wrongSide || !o.clockSwing) hurryLanding(1 - i, 0.0f);
            continue;
        }
        if (o.age < 0.05f) continue;
        liftFoot(i, walking, blocked);
        if (walking && footPhase(i) < gait.duty) {
            phase += gait.duty - footPhase(i);
            phase -= floorf(phase);
        }
        break;
    }
}
