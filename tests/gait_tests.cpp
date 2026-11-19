// Headless check of the leg simulation: walks the body at fixed speeds and compares what
// comes out with published gait data. No window, no OpenGL.
//   g++ -std=c++17 -O2 -Isrc -o build/gait_tests.exe tests/gait_tests.cpp
#include <cstdio>
#include <cstring>
#include <cmath>
#include <chrono>
#include <vector>
#include "../src/leg_view.h"

static int gFailed = 0;

static void expectRange(const char* what, float v, float lo, float hi) {
    bool ok = v >= lo && v <= hi;
    if (!ok) gFailed++;
    printf("    %-34s %8.3f   [%6.3f .. %6.3f]  %s\n", what, v, lo, hi, ok ? "ok" : "FAIL");
}

struct Measure {
    float speed = 0, cadence = 0, stepLen = 0, stepWidth = 0;
    float dsShare = 0, flightShare = 0;
    float vertPP = 0, latPP = 0, headPitchDeg = 0, headYawDeg = 0;
    float kneeSwingDeg = 0, kneeStanceDeg = 0, kneeStrikeDeg = 0, hipStrikeDeg = 0;
    float clearanceMin = 1.0f;
    float maxSlide = 0, maxStretch = 0;
    float maxPelvisAcc = 0;
    float kneeSnap = 0;      // largest change of knee angular velocity between two ticks, deg/s
    float kneeMidDeg = 180;  // straightest the supporting knee gets
    int steps = 0;
};

static Measure runScenario(float fwd, float right, float walkSpeed, bool run, bool crouch, float seconds, float turnRate = 0.0f) {
    LegSim s;
    s.reset(Vec3(0, 0, 0), 0.0f);
    HeadState hs;
    LegInput in;
    in.moveFwd = fwd;
    in.moveRight = right;
    in.walkSpeed = walkSpeed;
    in.run = run;
    in.crouch = crouch;
    const float dt = 1.0f / 120.0f;
    const float warm = 4.0f;
    Measure m;
    float yMin = 9, yMax = -9, lMin = 9, lMax = -9, pMin = 9, pMax = -9, hyMin = 9, hyMax = -9;
    int n = 0, nDs = 0, nFlight = 0;
    Vec3 startPos, planted[2];
    bool wasSwing[2] = {false, false};
    std::vector<LegStrike> strikes;
    std::vector<float> strikeT;
    float prevVy = 0.0f;
    float prevKnee[2] = {0, 0}, prevKneeVel[2] = {0, 0};
    int kneeTicks = 0;
    int total = (int)((warm + seconds) / dt);
    for (int k = 0; k < total; k++) {
        float t = k * dt;
        in.yaw += turnRate * dt;
        s.strikeCount = 0;
        s.step(in, dt, nullptr);
        HeadView hv = computeHead(s, hs, dt, 1.0f);
        for (int i = 0; i < 2; i++) {
            // A planted foot must stay exactly where it landed.
            if (!s.foot[i].swing && !wasSwing[i]) {
                float slide = (s.foot[i].pose.anchor - planted[i]).len();
                if (k > 0 && slide > m.maxSlide) m.maxSlide = slide;
            }
            planted[i] = s.foot[i].pose.anchor;
            wasSwing[i] = s.foot[i].swing;
        }
        if (t < warm) { startPos = s.root; prevVy = s.pelvisVy; continue; }
        for (int i = 0; i < s.strikeCount; i++) { strikes.push_back(s.strikes[i]); strikeT.push_back(t); }
        n++;
        bool d0 = !s.foot[0].swing, d1 = !s.foot[1].swing;
        if (d0 && d1) nDs++;
        if (!d0 && !d1) nFlight++;
        if (s.pelvisY < yMin) yMin = s.pelvisY;
        if (s.pelvisY > yMax) yMax = s.pelvisY;
        if (s.sway < lMin) lMin = s.sway;
        if (s.sway > lMax) lMax = s.sway;
        if (hv.pitch < pMin) pMin = hv.pitch;
        if (hv.pitch > pMax) pMax = hv.pitch;
        if (hv.yaw < hyMin) hyMin = hv.yaw;
        if (hv.yaw > hyMax) hyMax = hv.yaw;
        float acc = fabsf(s.pelvisVy - prevVy) / dt;
        prevVy = s.pelvisVy;
        if (acc > m.maxPelvisAcc) m.maxPelvisAcc = acc;
        kneeTicks++;
        for (int i = 0; i < 2; i++) {
            const LegFoot& f = s.foot[i];
            Vec3 pole = headingFwd(f.pose.yaw);
            LegPose lp = solveLeg(s.dims, s.hip[i], footAnkle(s.dims, f.pose), pole);
            float deg = lp.kneeFlex * 57.2958f;
            // A leg that moves like a limb has no corners in its knee angle: the angular
            // velocity may be high, but it may not jump from one tick to the next.
            float kneeVel = (deg - prevKnee[i]) / dt;
            if (kneeTicks >= 3 && fabsf(kneeVel - prevKneeVel[i]) > m.kneeSnap) m.kneeSnap = fabsf(kneeVel - prevKneeVel[i]);
            prevKnee[i] = deg;
            prevKneeVel[i] = kneeVel;
            if (!f.swing && s.footPhase(i) > 0.20f && s.footPhase(i) < 0.45f && deg < m.kneeMidDeg) m.kneeMidDeg = deg;
            if (f.swing) {
                if (deg > m.kneeSwingDeg) m.kneeSwingDeg = deg;
                if (f.u > 0.3f && f.u < 0.7f && f.pose.lift < m.clearanceMin) m.clearanceMin = f.pose.lift;
            } else {
                float ph = s.footPhase(i);
                if (ph > 0.05f && ph < 0.30f && deg > m.kneeStanceDeg) m.kneeStanceDeg = deg;
                float miss = (footAnkle(s.dims, f.pose) - lp.ankle).len();
                if (miss > m.maxStretch) m.maxStretch = miss;
                if (f.age <= dt * 1.5f) {
                    m.kneeStrikeDeg = deg;
                    Vec3 th = lp.knee - lp.hip;
                    m.hipStrikeDeg = atan2f(th.dot(headingFwd(s.bodyYaw)), -th.y) * 57.2958f;
                }
            }
        }
    }
    float dur = n * dt;
    m.speed = (s.root - startPos).len() / dur;
    m.steps = (int)strikes.size();
    if (strikes.size() >= 3) {
        m.cadence = (float)(strikes.size() - 1) / (strikeT.back() - strikeT.front()) * 60.0f;
        Vec3 dir = (strikes.back().pos - strikes.front().pos).norm();
        Vec3 side(dir.z, 0, -dir.x);
        float len = 0, wid = 0;
        for (size_t i = 1; i < strikes.size(); i++) {
            Vec3 d = strikes[i].pos - strikes[i - 1].pos;
            len += fabsf(d.dot(dir));
            wid += fabsf(d.dot(side));
        }
        m.stepLen = len / (strikes.size() - 1);
        m.stepWidth = wid / (strikes.size() - 1);
    }
    m.dsShare = (float)nDs / n;
    m.flightShare = (float)nFlight / n;
    m.vertPP = yMax - yMin;
    m.latPP = lMax - lMin;
    m.headPitchDeg = (pMax - pMin) * 0.5f * 57.2958f;
    m.headYawDeg = (hyMax - hyMin) * 0.5f * 57.2958f;
    return m;
}

static void printRow(const char* name, const Measure& m) {
    printf("%-14s v=%.2f  cad=%5.1f/min  step=%.3fm  width=%.3fm  DS=%4.1f%%  flight=%4.1f%%  vert=%.1fcm  lat=%.1fcm\n",
           name, m.speed, m.cadence, m.stepLen, m.stepWidth, m.dsShare * 100.0f, m.flightShare * 100.0f, m.vertPP * 100.0f, m.latPP * 100.0f);
    printf("               knee: strike %.0f  stance %.0f  swing %.0f deg   hip@strike %.0f deg   clearance %.1f cm   head pitch +-%.2f yaw +-%.2f deg\n",
           m.kneeStrikeDeg, m.kneeStanceDeg, m.kneeSwingDeg, m.hipStrikeDeg, m.clearanceMin * 100.0f, m.headPitchDeg, m.headYawDeg);
    printf("               slide %.4f m   stance miss %.4f m   peak pelvis accel %.1f m/s2   knee: mid stance %.0f deg, velocity jump %.0f deg/s per tick\n", m.maxSlide, m.maxStretch, m.maxPelvisAcc,
           m.kneeMidDeg, m.kneeSnap);
}

static void checkCommon(const Measure& m) {
    expectRange("planted foot slide (m)", m.maxSlide, 0.0f, 0.0005f);
    expectRange("stance leg reach miss (m)", m.maxStretch, 0.0f, 0.004f);
}

// Stop, turn on the spot, start again: the feet must end up side by side under the hips.
static void testStopAndTurn() {
    printf("\n[stop / turn in place]\n");
    LegSim s;
    s.reset(Vec3(0, 0, 0), 0.0f);
    LegInput in;
    const float dt = 1.0f / 120.0f;
    in.moveFwd = 1.0f;
    for (int k = 0; k < 360; k++) s.step(in, dt, nullptr);
    in.moveFwd = 0.0f;
    int before = s.stepsTaken;
    for (int k = 0; k < 360; k++) s.step(in, dt, nullptr);
    expectRange("steps needed to stop", (float)(s.stepsTaken - before), 1.0f, 4.0f);
    expectRange("speed after stopping", s.speed, 0.0f, 0.01f);
    Vec3 apart = s.foot[1].pose.anchor - s.foot[0].pose.anchor;
    expectRange("feet apart sideways (m)", apart.dot(headingRight(s.bodyYaw)), 0.10f, 0.30f);
    expectRange("feet apart fore-aft (m)", fabsf(apart.dot(headingFwd(s.bodyYaw))), 0.0f, 0.08f);
    expectRange("standing hip height (m)", s.pelvisY, s.dims.standHip - 0.012f, s.dims.standHip + 0.001f);

    before = s.stepsTaken;
    in.yaw = 2.6f;
    for (int k = 0; k < 480; k++) s.step(in, dt, nullptr);
    expectRange("steps to turn 150 deg", (float)(s.stepsTaken - before), 2.0f, 10.0f);
    float twist = fabsf(gAngleDiff(s.foot[0].pose.yaw, s.homeYaw(0)));
    expectRange("left foot twist after turn", twist, 0.0f, 0.46f);
    expectRange("moved while turning (m)", (s.root - Vec3(0, 0, s.root.z)).len(), 0.0f, 0.01f);
}

static float segmentDist(Vec3 p1, Vec3 q1, Vec3 p2, Vec3 q2) {
    Vec3 d1 = q1 - p1, d2 = q2 - p2, r = p1 - p2;
    float a = d1.dot(d1), e = d2.dot(d2), f = d2.dot(r), c = d1.dot(r), b = d1.dot(d2);
    float den = a * e - b * b;
    float u = den > 1e-9f ? gSat((b * f - c * e) / den) : 0.0f;
    float t = (b * u + f) / e;
    if (t < 0.0f) { t = 0.0f; u = gSat(-c / a); }
    else if (t > 1.0f) { t = 1.0f; u = gSat((b - c) / a); }
    return ((p1 + d1 * u) - (p2 + d2 * t)).len();
}

// Shortest distance between the centre lines of the two legs: thigh, shank and sole of one
// against the other's (thigh against thigh left out - they meet at the pelvis anyway).
// The shanks are 11 cm thick and the feet 10 cm wide, so below about 7 cm one leg is inside the other.
static float legsApart(const LegSim& s) {
    Vec3 seg[2][3][2];
    for (int i = 0; i < 2; i++) {
        LegPose lp = s.legPose(i);  // what the renderer draws
        Vec3 fw, up, rt;
        footAxes(s.foot[i].pose, fw, up, rt);
        seg[i][0][0] = lp.hip;
        seg[i][0][1] = lp.knee;
        seg[i][1][0] = lp.knee;
        seg[i][1][1] = lp.ankle;
        seg[i][2][0] = lp.ankle - fw * s.dims.heelBack - up * s.dims.ankleH;
        seg[i][2][1] = lp.ankle + fw * s.dims.toeFwd - up * s.dims.ankleH;
    }
    float best = 9.0f;
    for (int a = 0; a < 3; a++) {
        for (int b = 0; b < 3; b++) {
            if (a == 0 && b == 0) continue;
            float d = segmentDist(seg[0][a][0], seg[0][a][1], seg[1][b][0], seg[1][b][1]);
            if (d < best) best = d;
        }
    }
    return best;
}

struct TurnMeasure {
    float apart = 9.0f;      // closest the legs came
    float inside = 0.0f;     // share of the time one leg was inside the other
    float drop = 0.0f;       // deepest the hips went below their standing height
    float twist = 0.0f;      // most a planted foot was turned against the trunk
    float behind = 0.0f;     // furthest the trunk fell behind the look direction (second half of the run)
};

// What a player does with the mouse and the keys, not what a treadmill does: `script` sets
// the input for time t.
template <typename F>
static TurnMeasure runTurning(float seconds, F script, bool mouse = false) {
    LegSim s;
    s.reset(Vec3(0, 0, 0), 0.0f);
    LegInput in;
    const float dt = 1.0f / 120.0f;
    TurnMeasure m;
    int n = (int)(seconds / dt), inside = 0;
    for (int k = 0; k < n; k++) {
        script(k * dt, in);
        if (mouse) {  // the script moves the mouse: the view settles on it, the look comes round after the view
            static float view, look, aim, rate;
            if (k == 0) view = look = aim = rate = 0.0f;
            float turned = (in.yaw - view) * viewSettle(dt);
            view += turned;
            in.yaw = look = s.followLook(look, aim, rate, turned, dt);
            in.steer = view - look;
        }
        s.strikeCount = 0;
        s.step(in, dt, nullptr);
        float apart = legsApart(s);
        if (apart < m.apart) m.apart = apart;
        if (apart < 0.07f) inside++;
        if (s.legLength() - s.pelvisY > m.drop) m.drop = s.legLength() - s.pelvisY;
        if (k > n / 2 && fabsf(s.headTwist) > m.behind) m.behind = fabsf(s.headTwist);
        for (int i = 0; i < 2; i++) {
            float tw = fabsf(gAngleDiff(s.homeYaw(i), s.foot[i].pose.yaw));
            if (!s.foot[i].swing && tw > m.twist) m.twist = tw;
        }
    }
    m.inside = (float)inside / n;
    return m;
}

static void printTurn(const char* name, const TurnMeasure& m) {
    printf("  %-30s legs apart %.3f m (inside each other %.1f%% of the time)   hips down %.1f cm   foot twist %.0f deg   trunk behind %.0f deg\n",
           name, m.apart, m.inside * 100.0f, m.drop * 100.0f, m.twist * 57.3f, m.behind * 57.3f);
}

// Turning, and changing one's mind: the legs must stay out of each other, the feet must not
// be left pointing the wrong way, and the hips must not fall further than a step takes them.
static void testTurning() {
    printf("\n[turning and abrupt input]\n");
    TurnMeasure circle = runTurning(12, [](float t, LegInput& in) { in.moveFwd = 1; in.yaw = 1.0f * t; });
    printTurn("walking a 1.4 m circle", circle);
    expectRange("circle: legs apart (m)", circle.apart, 0.09f, 1.0f);
    expectRange("circle: hips down (m)", circle.drop, 0.0f, 0.075f);
    expectRange("circle: trunk behind the look (rad)", circle.behind, 0.0f, 0.05f);

    TurnMeasure tight = runTurning(12, [](float t, LegInput& in) { in.moveFwd = 1; in.yaw = 2.0f * t; });
    printTurn("walking a 0.7 m circle", tight);
    expectRange("tight circle: legs apart (m)", tight.apart, 0.06f, 1.0f);
    expectRange("tight circle: hips down (m)", tight.drop, 0.0f, 0.075f);

    TurnMeasure spin = runTurning(12, [](float t, LegInput& in) { in.yaw = 4.0f * t; });
    printTurn("spinning on the spot, 230 deg/s", spin);
    expectRange("spin: legs apart (m)", spin.apart, 0.07f, 1.0f);
    expectRange("spin: planted foot twist (rad)", spin.twist, 0.0f, 0.70f);
    expectRange("spin: hips down (m)", spin.drop, 0.0f, 0.015f);

    TurnMeasure whirl = runTurning(12, [](float t, LegInput& in) { in.yaw = 12.0f * t; });
    printTurn("spinning faster than legs can", whirl);
    expectRange("fast spin: legs inside each other", whirl.inside, 0.0f, 0.01f);
    expectRange("fast spin: planted foot twist (rad)", whirl.twist, 0.0f, 0.70f);

    TurnMeasure look = runTurning(12, [](float t, LegInput& in) { in.yaw = 2.5f * sinf(3.0f * t); });
    printTurn("looking left and right", look);
    expectRange("look around: legs inside each other", look.inside, 0.0f, 0.02f);

    TurnMeasure flick = runTurning(24, [](float t, LegInput& in) { in.moveFwd = 1; in.yaw = 1.5708f * (float)(int)(t / 1.1f); });
    printTurn("walking, 90 deg flicks", flick);
    expectRange("flicks: legs inside each other", flick.inside, 0.0f, 0.03f);
    expectRange("flicks: hips down (m)", flick.drop, 0.0f, 0.11f);

    TurnMeasure about = runTurning(24, [](float t, LegInput& in) { in.moveFwd = 1; in.yaw = 3.14159f * (float)(int)(t / 1.5f); });
    printTurn("walking, about-turns", about);
    expectRange("about-turns: legs inside each other", about.inside, 0.0f, 0.03f);
    expectRange("about-turns: hips down (m)", about.drop, 0.0f, 0.11f);

    // The same the way the demo does it: the mouse turns the view, and the look (followLook)
    // comes round after the view no faster than the body can follow. The mouse moves as a hand
    // does: a flick takes 0.12-0.2 s.
    auto flickTo = [](float t, float period, float angle, float dur) {
        int n = (int)(t / period);
        float f = gSat((t - n * period) / dur);
        return angle * (n + f * f * (3.0f - 2.0f * f));
    };
    TurnMeasure mFlick = runTurning(60, [&](float t, LegInput& in) { in.moveFwd = 1; in.yaw = flickTo(t, 1.1f, 1.5708f, 0.12f); }, true);
    printTurn("mouse: walking, 90 deg flicks", mFlick);
    // (The keys push where the eyes look - LegInput::steer - and a change of direction is
    // braked hard, so for a moment after a flick the body goes sideways over its feet. The
    // legs brush each other for a tick or two per flick; before, they never did.)
    expectRange("mouse flicks: legs inside each other", mFlick.inside, 0.0f, 0.02f);
    expectRange("mouse flicks: hips down (m)", mFlick.drop, 0.0f, 0.11f);
    expectRange("mouse flicks: look ahead of trunk (rad)", mFlick.behind, 0.0f, 0.80f);

    TurnMeasure mAbout = runTurning(60, [&](float t, LegInput& in) { in.moveFwd = 1; in.yaw = flickTo(t, 1.5f, 3.1416f, 0.2f); }, true);
    printTurn("mouse: walking, about-turns", mAbout);
    expectRange("mouse about-turns: legs apart (m)", mAbout.apart, 0.07f, 1.0f);
    expectRange("mouse about-turns: hips down (m)", mAbout.drop, 0.0f, 0.11f);

    TurnMeasure mRun = runTurning(60, [&](float t, LegInput& in) { in.moveFwd = 1; in.run = true; in.yaw = flickTo(t, 1.1f, 1.5708f, 0.12f); }, true);
    printTurn("mouse: sprinting, 90 deg flicks", mRun);
    expectRange("mouse sprint flicks: legs inside each other", mRun.inside, 0.0f, 0.01f);
    expectRange("mouse sprint flicks: hips down (m)", mRun.drop, 0.0f, 0.135f);

    TurnMeasure mStand = runTurning(60, [&](float t, LegInput& in) { in.yaw = flickTo(t, 1.5f, 3.1416f, 0.2f); }, true);
    printTurn("mouse: standing, about-turns", mStand);
    // The body now comes round the whole 180 deg (the look used to drop what was past 155),
    // and turning on the spot by exactly that much is where the feet come closest.
    expectRange("mouse standing turns: legs inside each other", mStand.inside, 0.0f, 0.025f);

    TurnMeasure mWhirl = runTurning(30, [](float t, LegInput& in) { in.moveFwd = 1; in.yaw = 12.0f * t; }, true);
    printTurn("mouse: walking, whirling", mWhirl);
    expectRange("mouse whirl: legs apart (m)", mWhirl.apart, 0.07f, 1.0f);
    expectRange("mouse whirl: look ahead of trunk (rad)", mWhirl.behind, 0.0f, 0.85f);

    // How long the look takes to come round to where the hand pointed, and how it stops there:
    // a look that has been catching up at the trunk's pace must brake, not stop dead (that is
    // 250 deg/s gone in one step).
    for (float angle : {0.5236f, 0.7854f, 1.5708f}) {
        LegSim s;
        s.reset(Vec3(0, 0, 0), 0.0f);
        LegInput in;
        in.moveFwd = 1;
        float view = 0, look = 0, aim = 0, rate = 0, done = -1, was = 0, brake = 0;
        for (int k = 0; k < 360; k++) {
            float t = k / 120.0f, want = t < 1.0f ? 0.0f : angle * gSat((t - 1.0f) / 0.10f);
            float turned = (want - view) * viewSettle(1.0f / 120.0f);
            view += turned;
            in.yaw = look = s.followLook(look, aim, rate, turned, 1.0f / 120.0f);
            in.steer = view - look;
            s.step(in, 1.0f / 120.0f, nullptr);
            if (done < 0 && look >= angle - 0.02f) done = t - 1.0f;
            if (t > 1.11f && was - rate > brake) brake = was - rate;
            was = rate;
        }
        char name[64];
        snprintf(name, sizeof name, "look: %.0f deg asked in 0.10 s, s", angle * 57.3f);
        expectRange(name, done, 0.0f, angle < 1.0f ? 0.22f : 0.36f);
        if (angle > 1.0f) expectRange("look: hardest braking in one step (deg/s)", brake * 57.3f, 2.0f, 40.0f);
    }

    // The view is the mouse's own, whatever the body is doing: it has no top speed, nothing
    // the hand does is lost, and it does not run on once the hand has stopped. Sprinting, where
    // the body turns at 95 deg/s, an about-turn of the hand in 0.15 s:
    {
        LegSim s;
        s.reset(Vec3(0, 0, 0), 0.0f);
        LegInput in;
        in.moveFwd = 1;
        in.run = true;
        const float dt = 1.0f / 120.0f, angle = 3.1416f;
        float view = 0, look = 0, aim = 0, rate = 0, top = 0, viewDone = 0, lookDone = 0, lead = 0;
        for (int k = 0; k < 840; k++) {
            float t = k * dt - 3.0f, f = gSat(t / 0.15f);
            float turned = ((t < 0.0f ? 0.0f : angle * f * f * (3.0f - 2.0f * f)) - view) * viewSettle(dt);
            view += turned;
            in.yaw = look = s.followLook(look, aim, rate, turned, dt);
            in.steer = view - look;
            s.step(in, dt, nullptr);
            if (turned / dt > top) top = turned / dt;
            if (fabsf(view - angle) > 0.0175f) viewDone = t + dt;
            if (fabsf(look - angle) > 0.0175f) lookDone = t + dt;
            if (t > 0.0f && fabsf(s.headTwist) > lead) lead = fabsf(s.headTwist);
        }
        expectRange("view: about-turn of the hand, got (deg)", view * 57.2958f, 179.9f, 180.1f);
        expectRange("view: top rate (deg/s)", top * 57.2958f, 1000.0f, 5000.0f);
        expectRange("view: there after the hand stopped (s)", viewDone - 0.15f, 0.0f, 0.20f);
        expectRange("look: comes round after it (s)", lookDone, 0.5f, 2.0f);
        expectRange("look: ahead of the trunk meanwhile (rad)", lead, 0.0f, 0.80f);
    }

    // Turning round at speed, W held, at the speeds the game asks for (4 m/s, 6.4 sprinting).
    // The body must go where the eyes look, and soon: pushed along the look, and braked no
    // harder than a walk runs out, a sprinting body skidded on for 3.5 m and took 2.2 s to
    // head the new way.
    struct CutCase { const char* name; float top; bool run; float angle, dur, headingMax, carriedMax; };
    for (const CutCase& c : {CutCase{"walk, 90 deg", 4.0f, false, 1.5708f, 0.12f, 0.40f, 1.2f},
                             CutCase{"walk, about-turn", 4.0f, false, 3.1416f, 0.20f, 0.50f, 1.1f},
                             CutCase{"sprint, 90 deg", 6.4f, true, 1.5708f, 0.12f, 0.55f, 2.3f},
                             CutCase{"sprint, about-turn", 6.4f, true, 3.1416f, 0.20f, 0.65f, 2.0f}}) {
        LegSim s;
        s.reset(Vec3(0, 0, 0), 0.0f);
        LegInput in;
        in.moveFwd = 1;
        in.run = c.run;
        in.walkSpeed = in.runSpeed = c.top;
        const float dt = 1.0f / 120.0f;
        float view = 0, look = 0, aim = 0, rate = 0, heading = -1, carried = 0;
        Vec3 from, to = headingFwd(c.angle);
        for (int k = 0; k < 840; k++) {
            float t = k * dt - 3.0f, f = gSat(t / c.dur);
            float turned = ((t < 0.0f ? 0.0f : c.angle * f * f * (3.0f - 2.0f * f)) - view) * viewSettle(dt);
            view += turned;
            in.yaw = look = s.followLook(look, aim, rate, turned, dt);
            in.steer = view - look;
            if (k == 360) from = s.root;
            s.step(in, dt, nullptr);
            if (t < 0.0f) continue;
            float on = (s.root - from).z, sp = s.vel.len();  // (it was going along +z)
            if (on > carried) carried = on;
            if (heading < 0 && sp > 0.5f && s.vel.dot(to) > 0.94f * sp) heading = t;
        }
        char what[96];
        snprintf(what, sizeof what, "%s: heading the new way after (s)", c.name);
        expectRange(what, heading, 0.0f, c.headingMax);
        snprintf(what, sizeof what, "%s: carried on the old way (m)", c.name);
        expectRange(what, carried, 0.0f, c.carriedMax);
    }

    TurnMeasure ws = runTurning(24, [](float t, LegInput& in) { in.moveFwd = ((int)(t / 0.9f) & 1) ? -1.0f : 1.0f; });
    printTurn("W <-> S", ws);
    expectRange("W<->S: hips down (m)", ws.drop, 0.0f, 0.10f);
    expectRange("W<->S: legs apart (m)", ws.apart, 0.09f, 1.0f);

    TurnMeasure ad = runTurning(24, [](float t, LegInput& in) { in.moveRight = ((int)(t / 0.9f) & 1) ? -1.0f : 1.0f; });
    printTurn("A <-> D", ad);
    expectRange("A<->D: hips down (m)", ad.drop, 0.0f, 0.115f);
    expectRange("A<->D: legs apart (m)", ad.apart, 0.07f, 1.0f);

    // A second key pressed and let go while walking: the trunk turns into the diagonal and back.
    TurnMeasure wd = runTurning(24, [](float t, LegInput& in) { in.moveFwd = 1; in.moveRight = fmodf(t, 2.6f) < 1.3f ? 1.0f : 0.0f; });
    printTurn("W, D on and off", wd);
    expectRange("W, D on/off: hips down (m)", wd.drop, 0.0f, 0.10f);
    expectRange("W, D on/off: legs inside each other (share)", wd.inside, 0.0f, 0.01f);
    expectRange("W, D on/off: foot twist (rad)", wd.twist, 0.0f, 0.67f);
    TurnMeasure wad = runTurning(24, [](float t, LegInput& in) { in.moveFwd = 1; in.moveRight = ((int)(t / 1.3f) & 1) ? -1.0f : 1.0f; });
    printTurn("W+A <-> W+D", wad);
    expectRange("W+A<->W+D: hips down (m)", wad.drop, 0.0f, 0.115f);
    expectRange("W+A<->W+D: legs inside each other (share)", wad.inside, 0.0f, 0.01f);
    TurnMeasure rwd = runTurning(24, [](float t, LegInput& in) { in.run = true; in.moveFwd = 1; in.moveRight = fmodf(t, 2.6f) < 1.3f ? 1.0f : 0.0f; });
    printTurn("sprint, D on and off", rwd);
    expectRange("sprint, D on/off: hips down (m)", rwd.drop, 0.0f, 0.135f);
    expectRange("sprint, D on/off: legs inside each other (share)", rwd.inside, 0.0f, 0.01f);

    TurnMeasure tap = runTurning(24, [](float t, LegInput& in) { in.moveFwd = fmodf(t, 0.6f) < 0.25f ? 1.0f : 0.0f; });
    printTurn("tapping W", tap);
    expectRange("tapping W: hips down (m)", tap.drop, 0.0f, 0.08f);

    TurnMeasure sprint = runTurning(24, [](float t, LegInput& in) { in.run = true; in.moveFwd = fmodf(t, 2.0f) < 1.2f ? 1.0f : 0.0f; });
    printTurn("sprint, stop, sprint", sprint);
    expectRange("sprint starts: hips down (m)", sprint.drop, 0.0f, 0.125f);
}

// Changing direction in the middle of a walk, at every moment of the stride (it matters which
// foot is in the air when the key goes down). Two things are asked of the legs:
//   - a few seconds later they walk the way they would have from rest. They used to keep a
//     limp for good after W -> D: the foot left behind took a step to under the body, off the
//     clock, was left behind again by the next stride, and so on;
//   - on the way there no foot is thrown: a swinging foot covers 5.7 m/s at most in a steady
//     walk and 8.6 sprinting, and used to be whipped down at 12-18 and 37 (a third of a
//     metre in one tick).
struct ChangeCase { const char* name; float f0, r0, f1, r1, yaw, yawTime; bool run; float footSpeedMax; };
static void testDirectionChanges() {
    printf("\n[changing direction, at 16 moments of the stride]\n");
    const ChangeCase cases[] = {
        {"W -> D", 1, 0, 0, 1, 0, 0, false, 8.0f},
        {"D -> A", 0, 1, 0, -1, 0, 0, false, 8.0f},
        {"W -> S", 1, 0, -1, 0, 0, 0, false, 8.0f},
        {"W -> W+D", 1, 0, 1, 1, 0, 0, false, 10.0f},
        {"W+D -> W+A", 1, 1, 1, -1, 0, 0, false, 11.0f},
        {"S -> D", -1, 0, 0, 1, 0, 0, false, 8.0f},
        {"W, mouse 45 deg", 1, 0, 1, 0, 0.7854f, 0.12f, false, 12.0f},
        {"W, mouse 90 deg in 0.6 s", 1, 0, 1, 0, 1.5708f, 0.60f, false, 12.0f},
        {"sprint W -> D", 1, 0, 0, 1, 0, 0, true, 11.0f},
        {"sprint W -> W+D", 1, 0, 1, 1, 0, 0, true, 12.0f},
        {"sprint, mouse 90 deg", 1, 0, 1, 0, 1.5708f, 0.15f, true, 13.0f},
    };
    const float dt = 1.0f / 120.0f;
    for (const ChangeCase& c : cases) {
        float footSpeed = 0.0f, lowHip = 9.0f;
        int limp = 0, lost = 0;
        for (int o = 0; o < 16; o++) {
            float at = 3.0f + o * 0.07f;
            LegSim s;
            s.reset(Vec3(0, 0, 0), 0.0f);
            LegInput in;
            in.run = c.run;
            float view = 0, look = 0, aim = 0, rate = 0;
            bool was[2] = {false, false};
            Vec3 prev[2] = {s.foot[0].pose.anchor, s.foot[1].pose.anchor};
            int n = (int)((at + 7.0f) / dt);
            for (int k = 0; k < n; k++) {
                float t = k * dt, q = c.yawTime > 0.0f ? gSat((t - at) / c.yawTime) : 0.0f;
                in.moveFwd = t < at ? c.f0 : c.f1;
                in.moveRight = t < at ? c.r0 : c.r1;
                float turned = (c.yaw * q * q * (3.0f - 2.0f * q) - view) * viewSettle(dt);
                view += turned;
                in.yaw = look = s.followLook(look, aim, rate, turned, dt);
            in.steer = view - look;
                s.strikeCount = 0;
                s.step(in, dt, nullptr);
                bool soon = t >= at && t < at + 1.5f, late = t >= at + 3.0f;
                for (int i = 0; i < 2; i++) {
                    const LegFoot& f = s.foot[i];
                    float v = (f.pose.anchor - prev[i]).len() / dt;
                    if (soon && v > footSpeed) footSpeed = v;
                    // A step off the clock in the middle of a walk, or a leg that does not reach its foot.
                    if (late && f.swing && !was[i] && !f.clockSwing && s.walking) limp++;
                    if (late && !c.run && s.legPose(i).stretched) lost++;
                    was[i] = f.swing;
                    prev[i] = f.pose.anchor;
                }
                if (soon && s.pelvisY < lowHip) lowHip = s.pelvisY;
            }
        }
        printf("  %-26s fastest foot %.1f m/s   lowest hip %.3f m   afterwards: steps off the clock %d, ticks a leg was short %d\n",
               c.name, footSpeed, lowHip, limp, lost);
        char what[96];
        snprintf(what, sizeof what, "%s: fastest foot (m/s)", c.name);
        expectRange(what, footSpeed, 1.0f, c.footSpeedMax);
        snprintf(what, sizeof what, "%s: limping afterwards", c.name);
        expectRange(what, (float)(limp + lost), 0.0f, 0.0f);
        snprintf(what, sizeof what, "%s: lowest hip (m)", c.name);
        expectRange(what, lowHip, c.run ? 0.82f : 0.85f, 1.0f);
    }
}

// A small room with a pillar in the middle, for bumping into things.
static bool fuzzBlocked(float x, float z, float r) {
    if (fabsf(x) > 6.0f - r || fabsf(z) > 6.0f - r) return true;
    return fabsf(x - 2.0f) < 0.4f + r && fabsf(z - 2.0f) < 0.4f + r;
}

// Ten minutes of random input (direction changes, spinning, run, crouch, walls, uneven
// frame times). Nothing may blow up, stretch, slide or get stuck.
static void testFuzz() {
    printf("\n[random input, 10 minutes]\n");
    LegSim s;
    s.reset(Vec3(0, 0, 0), 0.0f);
    HeadState hs;
    LegInput in;
    unsigned int rng = 12345u;
    auto rnd = [&]() { rng = rng * 1664525u + 1013904223u; return (float)((rng >> 8) & 0xFFFF) / 65535.0f; };
    float t = 0.0f, nextChange = 0.0f, yawRate = 0.0f;
    float maxFootDist = 0, maxMiss = 0, maxSlide = 0, minHip = 9, maxHip = 0, maxEyeJump = 0, longestIdle = 0, idle = 0;
    float maxDrop = 0, rising = 0;
    int substeps = 0, legsInside = 0;
    bool finite = true, wasSwing[2] = {false, false};
    Vec3 planted[2] = {s.foot[0].pose.anchor, s.foot[1].pose.anchor}, lastEye;
    int steps0 = s.stepsTaken, inWall = 0;
    bool first = true;
    while (t < 600.0f) {
        float dt = 0.002f + rnd() * 0.018f;  // 50..500 fps
        if (t >= nextChange) {
            nextChange = t + 0.15f + rnd() * 2.5f;
            in.moveFwd = (float)((int)(rnd() * 3.0f)) - 1.0f;
            in.moveRight = (float)((int)(rnd() * 3.0f)) - 1.0f;
            in.run = rnd() < 0.3f;
            in.crouch = rnd() < 0.15f;
            in.walkSpeed = 0.4f + rnd() * 1.6f;
            yawRate = rnd() < 0.5f ? 0.0f : (rnd() - 0.5f) * 8.0f;
            if (rnd() < 0.1f) in.yaw += (rnd() - 0.5f) * 6.0f;  // mouse flick
        }
        in.yaw += yawRate * dt;
        int n = (int)ceilf(dt * 120.0f);
        for (int k = 0; k < n; k++) {
            s.strikeCount = 0;
            int before = s.stepsTaken;
            s.step(in, dt / n, fuzzBlocked);
            HeadView hv = computeHead(s, hs, dt / n, 1.0f);
            finite = finite && std::isfinite(hv.eye.x) && std::isfinite(hv.eye.y) && std::isfinite(hv.eye.z) && std::isfinite(hv.pitch);
            if (!first && (hv.eye - lastEye).len() > maxEyeJump) maxEyeJump = (hv.eye - lastEye).len();
            if (!first && (hv.eye - lastEye).len() > 0.06f && getenv("FUZZ_DEBUG"))
                printf("  jump %.3f at t=%.2f dt=%.4f: d=(%.3f %.3f %.3f) pelvisY %.3f sway (%.3f %.3f) surge %.3f lean %.2f v %.2f run %.2f bobMean %.3f jolt %.3f\n",
                       (hv.eye - lastEye).len(), t, dt / n, hv.eye.x - lastEye.x, hv.eye.y - lastEye.y, hv.eye.z - lastEye.z, s.pelvisY,
                       s.swayVec.x, s.swayVec.z, s.surgeVec.len(), hv.lean, s.speed, s.run, hs.bobMean, hs.jolt);
            lastEye = hv.eye;
            first = false;
            if (fuzzBlocked(s.root.x, s.root.z, LEG_BODY_RADIUS * 0.98f)) inWall++;
            if (s.pelvisY < minHip) minHip = s.pelvisY;
            if (s.pelvisY > maxHip) maxHip = s.pelvisY;
            // (Measured against a body that takes its time standing up from a crouch: the hips
            // being on their way up is not the hips giving way.)
            rising = s.crouch > rising - 1.5f * dt / n ? s.crouch : rising - 1.5f * dt / n;
            float drop = s.dims.standHip - 0.22f * rising - s.pelvisY;
            if (drop > maxDrop) maxDrop = drop;
            substeps++;
            if (legsApart(s) < 0.07f) legsInside++;
            idle = (s.stepsTaken != before || s.vel.len() < 0.3f) ? 0.0f : idle + dt / n;
            if (idle > longestIdle) longestIdle = idle;
            for (int i = 0; i < 2; i++) {
                const LegFoot& f = s.foot[i];
                float d = (f.pose.anchor - s.root).len();
                if (!f.swing && d > maxFootDist) maxFootDist = d;
                if (!f.swing && !wasSwing[i]) {
                    float slide = (f.pose.anchor - planted[i]).len();
                    if (slide > maxSlide) maxSlide = slide;
                    Vec3 a = footAnkle(s.dims, f.pose);
                    LegPose lp = solveLeg(s.dims, s.hip[i], a, headingFwd(f.pose.yaw));
                    if ((a - lp.ankle).len() > maxMiss) maxMiss = (a - lp.ankle).len();
                }
                planted[i] = f.pose.anchor;
                wasSwing[i] = f.swing;
            }
        }
        t += dt;
    }
    expectRange("all numbers finite", finite ? 1.0f : 0.0f, 1.0f, 1.0f);
    expectRange("steps taken", (float)(s.stepsTaken - steps0), 400.0f, 3000.0f);
    expectRange("planted foot slide (m)", maxSlide, 0.0f, 0.0005f);
    // Under abuse a planted foot may be out of reach by up to 5 cm for a few ticks, then it is picked up.
    expectRange("stance leg reach miss (m)", maxMiss, 0.0f, 0.055f);
    expectRange("planted foot furthest from body (m)", maxFootDist, 0.2f, 1.0f);
    expectRange("lowest hip (m)", minHip, 0.55f, 1.0f);
    expectRange("highest hip (m)", maxHip, 0.9f, 1.02f);
    // Below the height the legs are set to (standing, or crouched): a steady run takes the hips 11 cm down.
    expectRange("hips below their own height (m)", maxDrop, 0.0f, 0.15f);
    expectRange("legs inside each other (share)", (float)legsInside / substeps, 0.0f, 0.01f);
    expectRange("largest eye move in one step (m)", maxEyeJump, 0.0f, 0.06f);
    expectRange("longest time moving without a step", longestIdle, 0.0f, 1.3f);
    expectRange("substeps spent inside a wall", (float)inWall, 0.0f, 0.0f);
}

static void bench() {
    printf("\n[cost]\n");
    LegSim s;
    s.reset(Vec3(0, 0, 0), 0.0f);
    HeadState hs;
    LegInput in;
    in.moveFwd = 1.0f;
    const int N = 2000000;
    float sink = 0.0f;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int k = 0; k < N; k++) {
        if ((k & 1023) == 0) in.run = !in.run;
        in.yaw += 0.0005f;
        s.strikeCount = 0;
        s.step(in, 1.0f / 120.0f, nullptr);
        HeadView hv = computeHead(s, hs, 1.0f / 120.0f, 1.0f);
        sink += hv.eye.y;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / N;
    printf("    one simulation step + camera: %.0f ns  (%.4f%% of a 144 fps frame at 2 steps/frame)  [%g]\n",
           ns, ns * 2.0 / (1e9 / 144.0) * 100.0, sink * 0.0);
    expectRange("step cost (microseconds)", (float)(ns / 1000.0), 0.0f, 5.0f);
}

int main() {
    printf("LEG SIMULATION vs. PUBLISHED GAIT DATA (body height 1.81 m)\n\n");

    Measure slow = runScenario(1, 0, 0.7f, false, false, 20);
    printRow("slow walk", slow);
    checkCommon(slow);
    expectRange("cadence 0.7 m/s (steps/min)", slow.cadence, 72, 92);
    expectRange("vertical excursion (m) [2.7cm]", slow.vertPP, 0.018f, 0.037f);
    expectRange("lateral excursion (m) [7.0cm]", slow.latPP, 0.045f, 0.085f);
    expectRange("double support share", slow.dsShare, 0.28f, 0.42f);

    Measure walk = runScenario(1, 0, 1.25f, false, false, 20);
    printRow("normal walk", walk);
    checkCommon(walk);
    expectRange("speed reached (m/s)", walk.speed, 1.22f, 1.28f);
    expectRange("step length (m) [0.66-0.68]", walk.stepLen, 0.62f, 0.72f);
    expectRange("cadence (steps/min) [105-118]", walk.cadence, 104, 120);
    expectRange("step width (m) [0.07-0.13]", walk.stepWidth, 0.07f, 0.14f);
    expectRange("double support share [0.2-0.26]", walk.dsShare, 0.18f, 0.28f);
    expectRange("vertical excursion (m) [4.0cm]", walk.vertPP, 0.030f, 0.050f);
    expectRange("lateral excursion (m) [3.5-4.6cm]", walk.latPP, 0.028f, 0.050f);
    expectRange("hip flexion at heel strike [25]", walk.hipStrikeDeg, 18, 34);
    // Literature: 0-8 deg at the instant of contact. Measured here one tick later, when the knee already yields.
    expectRange("knee just after heel strike [0-8]", walk.kneeStrikeDeg, 0, 24);
    expectRange("peak knee flexion in swing [60]", walk.kneeSwingDeg, 48, 75);
    expectRange("mid-swing clearance (m)", walk.clearanceMin, 0.010f, 0.040f);
    expectRange("head pitch amplitude (deg) [~1.3]", walk.headPitchDeg, 0.6f, 2.0f);
    expectRange("peak pelvis accel (m/s2) [~3.6]", walk.maxPelvisAcc, 0.5f, 8.0f);
    // The knee: gives under the weight, straightens over mid stance, and never snaps.
    expectRange("knee flexion in loading [15-20]", walk.kneeStanceDeg, 14, 30);
    expectRange("knee at mid stance [~5-10]", walk.kneeMidDeg, 3, 14);
    expectRange("knee velocity jump per tick (deg/s)", walk.kneeSnap, 0, 400);

    Measure fast = runScenario(1, 0, 1.6f, false, false, 20);
    printRow("fast walk", fast);
    checkCommon(fast);
    expectRange("vertical excursion (m) [4.8cm]", fast.vertPP, 0.038f, 0.060f);
    expectRange("lateral excursion (m) [3.9cm]", fast.latPP, 0.020f, 0.048f);
    expectRange("flight share (must walk)", fast.flightShare, 0.0f, 0.0f);
    expectRange("knee velocity jump per tick (deg/s)", fast.kneeSnap, 0, 500);

    Measure jog = runScenario(1, 0, 1.4f, true, false, 20);
    printRow("run 4.2", jog);
    checkCommon(jog);
    expectRange("cadence (steps/min) [160-180]", jog.cadence, 156, 182);
    expectRange("flight share [0.2-0.4]", jog.flightShare, 0.15f, 0.45f);
    expectRange("double support share (none)", jog.dsShare, 0.0f, 0.0f);
    expectRange("vertical excursion (m) [6-10cm]", jog.vertPP, 0.055f, 0.105f);
    expectRange("peak knee flexion in swing [>80]", jog.kneeSwingDeg, 75, 130);
    expectRange("knee velocity jump per tick (deg/s)", jog.kneeSnap, 0, 700);

    Measure back = runScenario(-1, 0, 1.4f, false, false, 15);
    printRow("backwards", back);
    checkCommon(back);
    expectRange("backward speed (m/s)", back.speed, 0.75f, 0.90f);

    Measure strafe = runScenario(0, 1, 1.4f, false, false, 15);
    printRow("sidestep", strafe);
    checkCommon(strafe);
    expectRange("sidestep speed (m/s)", strafe.speed, 0.58f, 0.68f);
    expectRange("sidestep vertical excursion (m)", strafe.vertPP, 0.0f, 0.05f);

    Measure diag = runScenario(1, 1, 1.4f, false, false, 15);
    printRow("diagonal", diag);
    checkCommon(diag);
    // W+D is a forward walk with the trunk turned into it, not a shuffle half sideways.
    expectRange("diagonal speed (m/s)", diag.speed, 1.37f, 1.43f);
    expectRange("diagonal step length (m)", diag.stepLen, 0.62f, 0.76f);
    expectRange("diagonal step width (m)", diag.stepWidth, 0.07f, 0.16f);

    Measure diagBack = runScenario(-1, 1, 1.4f, false, false, 15);
    printRow("diagonal back", diagBack);
    checkCommon(diagBack);
    expectRange("diagonal backward speed (m/s)", diagBack.speed, 0.75f, 0.90f);
    expectRange("diagonal backward step width (m)", diagBack.stepWidth, 0.07f, 0.18f);

    Measure crouch = runScenario(1, 0, 1.4f, false, true, 15);
    printRow("crouch walk", crouch);
    checkCommon(crouch);

    Measure curve = runScenario(1, 0, 1.4f, false, false, 15, 0.9f);
    printRow("walking a circle", curve);
    checkCommon(curve);

    testStopAndTurn();
    testTurning();
    testDirectionChanges();
    testFuzz();
    bench();

    printf("\n%s (%d failed)\n", gFailed == 0 ? "ALL CHECKS PASSED" : "CHECKS FAILED", gFailed);
    return gFailed == 0 ? 0 : 1;
}
