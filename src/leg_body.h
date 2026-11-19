#pragma once
// Included by leg_sim.h: the pelvis and the rolling of planted feet.

// ============================================================================
// PELVIS
// Vertical: the hip rides on the supporting leg, and that leg has a length it wants to
//   be at each moment of its stance (stanceKnee): almost straight at contact, giving
//   under the weight, straightening over mid stance, unlocking before the heel comes up.
//   Hip height is then geometry - an arc over each foot - so it is highest at mid stance
//   and lowest around heel strike, 3-5 cm in all (Orendurff 2004), and the knee angle has
//   no plateaus or corners in it: it is what the height was built from.
//   LEG_VAULT_GAIN sets how far ahead the heel is put down, and with it how deep the
//   valley between two steps is.
//   Running is the opposite (lowest at mid stance): a spring-mass bounce, integrated
//   from the push of each stance leg, with free flight in between.
// Lateral: the centre of mass is a pendulum falling away from the supporting
//   foot (linear inverted pendulum, omega = sqrt(g / h)). Its periodic solution
//   gives wide slow sway when walking slowly and narrow quick sway when fast,
//   with no speed table (Zijlstra & Hof 1997).
// ============================================================================

inline constexpr float LEG_VAULT_GAIN = 0.82f;
inline constexpr float LEG_RUN_SINK = 0.070f;   // runners carry the hips lower
inline constexpr float LEG_PELVIS_ROT = 0.115f; // rad per metre of fore-aft foot separation
inline constexpr float LEG_PELVIS_LIST = 0.075f;
inline constexpr float LEG_KNEE_WALK = 0.15f;    // rad: a walking leg is never quite as straight as a standing one
inline constexpr float LEG_KNEE_GIVE = 0.15f;    // rad: how far the knee gives when the weight comes onto it
inline constexpr float LEG_KNEE_UNLOCK = 0.10f;  // rad: flexion that builds up before heel-off

// Drop of a stiff pendulum leg over one full step at the current speed.
inline float LegSim::stepDrop() const {
    float legLen = legLength();
    float stepLen = speed * gait.stepTime;
    if (stepLen > 1.2f * legLen) stepLen = 1.2f * legLen;
    return legLen - sqrtf(legLen * legLen - 0.25f * stepLen * stepLen);
}

// The hip is most of the way down its path when a foot comes down (the bottom follows a
// moment later, in double support). From that height an almost straight leg reaches the
// floor a certain distance ahead - that is where the heel goes.
inline float LegSim::strikeShare(float landSpeed) const {
    float travel = landSpeed * gait.stanceTime;
    if (travel < 0.05f) return LEG_LEAD_MAX;
    float hipY = legLength() - LEG_VAULT_GAIN * stepDrop();
    float ax, ay;
    footAnkleLocal(dims, gait.strikePitch, ax, ay);
    float reach = dims.legMax() * LEG_STRIKE_REACH;
    float h2 = reach * reach - (hipY - ay) * (hipY - ay);
    float ahead = (h2 > 0.0f ? sqrtf(h2) : 0.0f) - ax;
    return gClamp(ahead / travel, LEG_LEAD_MIN, LEG_LEAD_MAX);
}

// Knee flexion a planted leg asks for, on top of the standing 5 degrees.
inline float LegSim::stanceKnee(int i) const {
    float s = gSat(foot[i].age / (gait.stanceTime > 0.1f ? gait.stanceTime : 0.1f));
    float go = gSat(speed / 1.0f) * (1.0f - idle) * (1.0f - run);
    // Sideways the feet are far apart twice a step; the knees stay bent rather than vaulting.
    float lateral = fabsf(travelDir.dot(headingRight(bodyYaw))) * gSat(speed / 0.5f) * (1.0f - idle);
    float give = s < 0.20f ? gSmooth(s / 0.20f) : 1.0f - gSmooth((s - 0.20f) / 0.42f);
    float unlock = gSmooth((s - 0.62f) / 0.25f);
    return 0.30f * lateral + go * (LEG_KNEE_WALK + (LEG_KNEE_GIVE + 0.09f * gSat((speed - 0.7f) / 0.9f)) * give + LEG_KNEE_UNLOCK * unlock);
}

inline void LegSim::placePelvis() {
    Vec3 r0 = headingRight(bodyYaw);
    pelvisYaw = bodyYaw + rot;
    pelvisPos = root + swayVec + surgeVec;
    sway = swayVec.dot(r0);
    pelvisPos.y = pelvisY;
    Vec3 rp = headingRight(pelvisYaw);
    float c = cosf(list), s = sinf(list);
    for (int i = 0; i < 2; i++) {
        float side = foot[i].side * dims.hipHalf;
        hip[i] = pelvisPos + rp * (side * c);
        hip[i].y -= side * s;
    }
}

inline void LegSim::updatePelvis(float dt) {
    Vec3 f0 = headingFwd(bodyYaw), r0 = headingRight(bodyYaw);
    float legLen = legLength();

    // The hip rides on the supporting legs. Each planted leg has a length it wants to be at
    // this point of its stance (stanceKnee): nearly straight at contact, giving under the
    // weight, straightening over mid stance, unlocking before the heel comes up. Hip height is
    // then plain geometry - an arc over each foot - and the load blends one arc into the next
    // while both feet are down.
    const float heightOmega = 28.0f;
    float wsum = foot[0].load + foot[1].load;
    if (wsum > 0.05f) {
        float h = 0.0f;
        Vec3 cop;
        for (int i = 0; i < 2; i++) {
            Vec3 a = footAnkle(dims, foot[i].pose);
            if (!foot[i].swing) a.y -= foot[i].pose.lift;  // a heel still settling counts as down
            Vec3 d = a - hip[i] - vel * dt;  // the hips are last tick's
            float len = dims.legMax() * cosf(0.5f * stanceKnee(i)) - 0.001f;
            float dist2 = d.x * d.x + d.z * d.z;
            if (dist2 > 0.49f * len * len) dist2 = 0.49f * len * len;
            float hi = a.y + sqrtf(len * len - dist2) + foot[i].side * dims.hipHalf * sinf(list);
            // The leg that is being left does not get a say any more: its share is the height
            // the hips had when the other foot came down, so the valley is round, not a V.
            const LegFoot& o = foot[1 - i];
            if (!foot[i].swing && !o.swing && o.age < foot[i].age) {
                float stay = 1.0f - gSmooth(o.age / (0.05f + gait.dsTime));
                hi += (landY - hi) * stay;
            }
            h += foot[i].load * hi;
            cop += (foot[i].pose.anchor - root) * foot[i].load;
        }
        lastDrop = dims.standHip - h / wsum;
        lastCop = cop / wsum;
    }

    // Running bounce: each stance leg pushes with a half sine, gravity does the rest.
    if (run > 0.02f) {
        float acc = -GAIT_G;
        for (int i = 0; i < 2; i++) {
            const LegFoot& f = foot[i];
            if (f.pushAge < f.contactTime) acc += f.thrust * sinf(MATH_PI * f.pushAge / f.contactTime);
        }
        // A soft tether keeps the average height where it belongs without touching the bounce.
        runVy += (acc - 9.0f * runY - 3.6f * runVy) * dt;
        runY = gClamp(runY + runVy * dt, -0.16f, 0.12f);
    } else {
        runY = 0.0f;
        runVy = 0.0f;
    }

    float walkH = -lastDrop;
    float runH = -LEG_RUN_SINK + runY;
    float target = legLen + gLerp(walkH, runH, run);
    // Never ask for a hip height that a leg in front of the body cannot reach: the one that
    // carries the weight, and the one that is about to land (its limit fades in during the
    // second half of the swing, so the hips settle onto it instead of being caught by it).
    // A leg trailing behind is not held: it rolls onto its toes and leaves.
    for (int i = 0; i < 2; i++) {
        const LegFoot& f = foot[i];
        FootPose p = f.pose;
        p.lift = 0.0f;
        Vec3 hipAt = hip[i];  // last tick's, close enough
        float hold = 1.0f;
        if (f.swing) {
            if (f.u < 0.30f) continue;
            p.anchor = f.target;
            p.pitch = f.landPitch;
            p.lift = 0.0f;
            hipAt += f.landRoot - root;
            hold = gSmooth((f.u - 0.34f) / 0.60f);
        }
        Vec3 a = footAnkle(dims, p);
        Vec3 d = a - hipAt;
        if (d.dot(travelDir) < -0.10f) continue;
        float reach = dims.legMax() * 0.997f;
        float h2 = reach * reach - (d.x * d.x + d.z * d.z);
        float limit = a.y + (h2 > 0.04f ? sqrtf(h2) : 0.2f) + f.side * dims.hipHalf * sinf(list);
        if (target > limit) target -= (target - limit) * hold;
    }
    target = gClamp(target, legLen - maxSink(), legLen + 0.02f);
    // Coming out of a run the hips straighten up at a human rate, they do not pop up.
    if (!runMode && run > 0.02f && target > heightTarget + 0.45f * dt) target = heightTarget + 0.45f * dt;
    // The spring only has to swallow jumps of the target; a target that is merely moving is
    // followed without falling behind (the lag would be centimetres, and a centimetre of hip
    // is ten degrees of knee).
    float tv = gClamp((target - heightTarget) / dt, -0.5f, 0.5f);
    heightTargetV += gClamp(tv - heightTargetV, -5.0f * dt, 5.0f * dt);
    heightTarget = target;
    pelvisVy -= heightTargetV;
    gSpring(pelvisY, pelvisVy, target, heightOmega, dt);
    pelvisVy += heightTargetV;

    // Sideways the body is a pendulum falling away from the supporting foot. For alternating
    // feet that has an exact periodic solution: zero when support changes, furthest towards the
    // stance foot at mid stance, y = c * (1 - cosh(w (t - T/2)) / cosh(w T/2)).
    // Offsets are taken across the path: along it the feet simply pass under the body.
    float omega = sqrtf(GAIT_G / legLen);
    Vec3 swayTarget;
    if (walking) {
        int lead = foot[0].age < foot[1].age ? 0 : 1;
        if (foot[lead].swing) lead = 1 - lead;
        int sup = foot[lead].age >= gait.dsTime * 0.5f ? lead : 1 - lead;
        float T = gait.stepTime;
        float t = gClamp(foot[sup].age - gait.dsTime * 0.5f, 0.0f, T);
        float across = gSat((speed - 0.15f) / 0.30f);
        Vec3 pathSide(travelDir.z, 0.0f, -travelDir.x);
        Vec3 side = Vec3::lerp(r0, pathSide * (pathSide.dot(r0) >= 0.0f ? 1.0f : -1.0f), across).norm();
        float c = (foot[sup].pose.anchor - root).dot(side);
        float y = c * (1.0f - coshf(omega * (t - 0.5f * T)) / coshf(omega * 0.5f * T));
        swayTarget = side * y;
    } else {
        // Standing: the weight simply settles between the feet.
        swayTarget = r0 * lastCop.dot(r0);
    }
    float follow = gLerp(25.0f, omega, idle);
    gSpring(swayVec.x, swayVel.x, swayTarget.x, follow, dt);
    gSpring(swayVec.z, swayVel.z, swayTarget.z, follow, dt);

    // The pelvis turns to follow the swinging leg and dips on the unloaded side.
    float ahead = (foot[1].pose.anchor - foot[0].pose.anchor).dot(f0);
    float rotTarget = gClamp(LEG_PELVIS_ROT * ahead * (1.0f - 0.4f * run), -0.14f, 0.14f);
    gSpring(rot, rotV, rotTarget, 16.0f, dt);
    // The drop is deepest just after toe-off; the hip hikes back to level as its leg swings
    // through, so the foot comes down from a level pelvis (Perry: neutral again at contact).
    float listTarget = wsum > 0.05f ? LEG_PELVIS_LIST * (foot[0].load - foot[1].load) / wsum : 0.0f;
    for (int i = 0; i < 2; i++) {
        float level = 0.9f * fabsf(travelDir.dot(f0)) * gSmooth((foot[i].u - 0.20f) / 0.70f);
        if (foot[i].swing && foot[i].clockSwing) listTarget *= 1.0f - level;
    }
    if (!walking) listTarget *= 0.4f;
    gSpring(list, listV, listTarget, 20.0f, dt);

    // Height and forward speed trade energy like a pendulum: slowest on top of the vault.
    // It is a few millimetres, kept as a world vector so a change of direction cannot jerk it.
    float bob = pelvisY - legLen;
    heightAvg += (bob - heightAvg) * (1.0f - expf(-dt / 0.6f));
    float ripple = -GAIT_G * gClamp(bob - heightAvg, -0.04f, 0.04f) / (speed > 0.7f ? speed : 0.7f);
    surgeVec += travelDir * (ripple * 0.5f * (1.0f - run) * gSat(speed / 0.5f) * dt);
    surgeVec *= expf(-dt * 1.5f);
    float surgeLen = surgeVec.len();
    if (surgeLen > 0.02f) surgeVec *= 0.02f / surgeLen;

    placePelvis();
}

// Rolls a planted foot: heel rocker after contact, heel rise and push-off before toe-off,
// and whatever extra heel (or toe) lift the leg needs to still reach the hip.
inline void LegSim::updateStancePitch(int i, float dt) {
    LegFoot& f = foot[i];
    float target = 0.0f;

    float flatTime = 0.10f * gait.strideTime;
    if (flatTime < 0.06f) flatTime = 0.06f;
    // The forefoot comes down fastest right after contact and settles gently.
    if (f.age < flatTime) target = f.strikePitch * (1.0f - f.age / flatTime) * (1.0f - f.age / flatTime);

    float sp = vel.len();
    if (walking && f.armed && sp > 0.15f) {
        float fwdness = gSat(vel.dot(headingFwd(f.pose.yaw)) / sp) * gSat(sp / 0.6f);
        float phi = footPhase(i);
        // Walking: heel-off in terminal stance, then the push in double support.
        // Running: one sweep over the second half of the contact. Blended where the two meet.
        float ds = gait.duty > 0.52f ? gait.duty - 0.5f : 0.02f;
        float rise = gSat((phi - 0.36f) / 0.14f);
        float x = gSat((phi - 0.5f) / ds);
        float pushWalk = 0.20f * rise * rise + (gait.toeOffPitch - 0.20f) * x * (0.4f + 0.6f * x);
        float pushRun = gait.toeOffPitch * gSmooth((phi / gait.duty - 0.5f) / 0.5f);
        target -= gLerp(pushRun, pushWalk, gSat((gait.duty - 0.47f) / 0.08f)) * fwdness;
    }

    // Reach: if the leg is too short for this pitch, roll further onto the ball
    // (hip ahead of the foot) or onto the heel (hip behind it).
    float reach = dims.legMax() * (0.9975f - 0.012f * run);  // a runner pushes off a bent knee
    FootPose probe = f.pose;
    probe.pitch = target;
    Vec3 a0 = footAnkle(dims, probe);
    if ((hip[i] - a0).len() > reach) {
        // (A hip that is off to the side counts as ahead: the heel comes up, as it does at the
        // end of a sidestep. Toes in the air gain a sideways leg next to nothing.)
        Vec3 off = hip[i] - a0;
        bool hipAhead = off.dot(headingFwd(f.pose.yaw)) >= -0.5f * sqrtf(off.x * off.x + off.z * off.z);
        float lo = target, hi = hipAhead ? -1.25f : 0.60f;
        if ((hipAhead && hi < lo) || (!hipAhead && hi > lo)) {
            for (int k = 0; k < 12; k++) {
                probe.pitch = 0.5f * (lo + hi);
                if ((hip[i] - footAnkle(dims, probe)).len() > reach) lo = probe.pitch;
                else hi = probe.pitch;
            }
            target = hi;
        }
    }

    // The foot is a mass, not a servo: its turning rate is limited, and so is how fast the
    // rate itself may change.
    float rateWant = gClamp((target - f.pose.pitch) / dt, -14.0f, 14.0f);
    f.pitchRate += gClamp(rateWant - f.pitchRate, -700.0f * dt, 700.0f * dt);
    float before = f.pose.pitch;
    f.pose.pitch += f.pitchRate * dt;
    if ((f.pose.pitch - target) * (before - target) < 0.0f) f.pose.pitch = target;  // no overshoot
    f.pitchRate = (f.pose.pitch - before) / dt;
    // A heel that was still a few millimetres up when its time came is set down, not teleported.
    f.pose.lift = f.pose.lift > 0.8f * dt ? f.pose.lift - 0.8f * dt : 0.0f;
}

// A planted leg must be able to reach its foot. If it cannot: a foot trailing behind is let
// go (an early toe-off), a foot in front pulls the hips down - quickly, but never in one
// jerk - and a foot that is simply out of range (a sudden reversal, a shove against a wall)
// is picked up for a quick recovery step, even if that leaves the body in the air for a moment.
inline void LegSim::enforceReach(float dt, LegBlockedFn blocked) {
    float maxLen = dims.legMax() * 0.999f;
    float lowest = legLength() - maxSink() - 0.01f;
    float sinkNow = 0.0f;
    for (int i = 0; i < 2; i++) {
        LegFoot& f = foot[i];
        if (f.swing) continue;
        Vec3 a = footAnkle(dims, f.pose);
        Vec3 d = hip[i] - a;
        float len = d.len();
        if (len <= dims.legMax() * 0.978f) continue;
        float dh2 = d.x * d.x + d.z * d.z;
        bool reachable = dh2 < maxLen * maxLen;
        float sink = reachable ? d.y - sqrtf(maxLen * maxLen - dh2) : 0.0f;
        bool tooFar = !reachable || pelvisY - sink < lowest || len > maxLen + 0.05f;
        bool trailing = d.dot(travelDir) > 0.15f && f.age > 0.12f;
        bool supported = !foot[1 - i].swing || run > 0.5f;
        // A runner's leg leaves the floor still bent; it is not dragged until it is straight.
        bool spent = run > 0.5f && trailing && f.pose.pitch < -0.6f && len > dims.legMax() * 0.978f;
        if (spent) { liftFoot(i, walking, blocked); continue; }
        if (len <= maxLen) continue;
        if (tooFar || (trailing && supported)) {
            // While walking this is a step like any other, only early: it goes where the gait
            // would put it, and the rhythm carries on from it. (A step to under the body, off
            // the clock, comes down behind a body that is moving: it would be lost again at
            // once, and again every stride after that - a limp that never ends.)
            bool alone = walking && !supported;
            if (alone) hurryLanding(1 - i, 0.0f);
            liftFoot(i, walking, blocked);
            // The clock goes on from this toe-off. (With the other foot still in the air it is
            // only moved as far as this swing needs to be a swing at all: any further and the
            // foot that has just been asked to hurry is thrown down.)
            float from = alone ? 0.5f * gait.duty + 0.02f : gait.duty;
            if (walking && footPhase(i) < from) {
                phase += from - footPhase(i);
                phase -= floorf(phase);
            }
            continue;
        }
        // The hips go down for it - and the foot that is in the air comes down to take over.
        if (!supported) hurryLanding(1 - i, 0.0f);
        if (sink > sinkNow) sinkNow = sink;
    }
    if (sinkNow <= 0.0f) sinkV = 0.0f;
    if (sinkNow > 0.0f) {
        // Up to 1.2 m/s, and that speed is not there at once: the hips give, they are not dropped.
        const float rate = 1.2f;
        float going = -pelvisVy > sinkV ? -pelvisVy : sinkV;
        sinkV = going + 25.0f * dt < rate ? going + 25.0f * dt : rate;
        if (sinkNow > sinkV * dt) sinkNow = sinkV * dt;
        pelvisY -= sinkNow;
        if (pelvisVy > -0.5f * rate) pelvisVy = pelvisVy > 0.0f ? 0.0f : pelvisVy;
        placePelvis();
    }
}
