#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/fall_sim.h"

static const float CS = 5.0f;
// One hole cell at (0, 0); and a 1x3 pit along z for the abyss-like case.
static bool oneHole(int wx, int wz) { return wx == 0 && wz == 0; }
static bool longPit(int wx, int wz) { return wx == 0 && wz >= 0 && wz <= 2; }

void testFloorHoldsUntilPastTheLip() {
    assert(fallSupported(-1.0f, 2.5f, CS, oneHole));               // on the floor
    assert(fallSupported(FALL_LIP * 0.5f, 2.5f, CS, oneHole));     // toes over the edge
    assert(!fallSupported(FALL_LIP + 0.05f, 2.5f, CS, oneHole));   // past it
    assert(!fallSupported(2.5f, 2.5f, CS, oneHole));
    assert(fallSupported(CS - 0.1f, 2.5f, CS, oneHole));           // the far lip holds too
    assert(!fallSupported(2.5f, 5.0f, CS, longPit));               // no lip between two open cells
}

void testFreeFallIsRealGravity() {
    FallState s = fallBegin(Vec3(2.5f, 1.7f, 2.5f), Vec3(), 0.0f, 0.0f);
    for (int i = 0; i < 60; i++) fallStep(s, 1.0f / 60.0f, CS, oneHole);
    assert(fabsf(s.time - 1.0f) < 1e-3f);
    float drop = 1.7f - s.pos.y;
    assert(drop > 4.6f && drop < 4.95f);          // g/2 * t^2 = 4.9 m, less a little air
    assert(-s.vel.y > 9.2f && -s.vel.y < 9.81f);
    assert(s.hit == 0.0f);
}

void testFrameRateDoesNotChangeTheFall() {
    FallState a = fallBegin(Vec3(2.5f, 1.7f, 2.5f), Vec3(1.0f, 0, 0.5f), 0.3f, 0.1f);
    FallState b = a;
    for (int i = 0; i < 30; i++) fallStep(a, 1.0f / 30.0f, CS, oneHole);
    for (int i = 0; i < 240; i++) fallStep(b, 1.0f / 240.0f, CS, oneHole);
    assert(fabsf(a.pos.y - b.pos.y) < 0.05f);
    assert(fabsf(a.pitch - b.pitch) < 0.05f);
}

void testTerminalSpeed() {
    auto open = [](int, int) { return true; };
    FallState s = fallBegin(Vec3(0, 0, 0), Vec3(), 0.0f, 0.0f);
    for (int i = 0; i < 60 * 40; i++) fallStep(s, 1.0f / 60.0f, CS, open);
    assert(fabsf(-s.vel.y - FALL_TERMINAL) < 1.0f);
}

void testMomentumCarriesAndWallsStop() {
    // Walked in along +x at 3 m/s: keeps drifting, meets the far wall, stays in the shaft.
    FallState s = fallBegin(Vec3(0.3f, 1.7f, 2.5f), Vec3(3.0f, 0, 0), 1.5708f, 0.0f);
    float hardest = 0.0f, maxX = 0.0f;
    bool drifted = false;
    for (int i = 0; i < 150; i++) {
        fallStep(s, 1.0f / 60.0f, CS, oneHole);
        if (i == 30) drifted = s.pos.x > 1.5f;
        if (s.hit > hardest) hardest = s.hit;
        if (s.pos.x > maxX) maxX = s.pos.x;
        assert(s.pos.x >= FALL_BODY_R - 1e-3f && s.pos.x <= CS - FALL_BODY_R + 1e-3f);
        assert(s.pos.z >= FALL_BODY_R - 1e-3f && s.pos.z <= CS - FALL_BODY_R + 1e-3f);
    }
    assert(drifted);
    assert(hardest > 2.0f);                 // a real hit
    assert(maxX > CS - FALL_BODY_R - 0.1f);
    assert(fabsf(s.vel.x) < 1.0f);          // most of it is gone
    assert(s.pos.y < -20.0f);
}

void testBodyTipsTheWayItWent() {
    // yaw 0 looks along +z. Forwards: the face goes down faster than from standing.
    FallState fwd = fallBegin(Vec3(2.5f, 1.7f, 0.3f), Vec3(0, 0, 2.0f), 0.0f, 0.0f);
    FallState still = fallBegin(Vec3(2.5f, 1.7f, 0.3f), Vec3(), 0.0f, 0.0f);
    assert(fwd.pitchVel < -1.0f && still.pitchVel == 0.0f);
    // Sideways to the right (+x at yaw 0): rolls right.
    FallState side = fallBegin(Vec3(0.3f, 1.7f, 2.5f), Vec3(2.0f, 0, 0), 0.0f, 0.0f);
    assert(side.rollVel > 1.0f && fabsf(side.pitchVel) < 1e-3f);
    for (int i = 0; i < 12; i++) {
        fallStep(fwd, 1.0f / 60.0f, CS, oneHole);
        fallStep(still, 1.0f / 60.0f, CS, oneHole);
        fallStep(side, 1.0f / 60.0f, CS, oneHole);
    }
    assert(fwd.pitch < still.pitch && still.pitch < 0.0f);
    assert(side.roll > 0.2f);
    // Everyone ends up face down and level, within limits all the way.
    for (int i = 0; i < 60 * 6; i++) {
        fallStep(side, 1.0f / 60.0f, CS, [](int, int) { return true; });
        assert(fabsf(side.roll) <= FALL_ROLL_LIMIT && fabsf(side.pitch) <= FALL_PITCH_LIMIT);
    }
    assert(fabsf(side.roll) < 0.05f && fabsf(side.pitch - FALL_FACE_DOWN) < 0.05f);
}

void testHeadIsThrownAtTheWallAndComesBack() {
    // Sideways into the +x wall, looking along +z: the ear goes to the right shoulder.
    FallState s = fallBegin(Vec3(4.0f, 1.7f, 2.5f), Vec3(3.0f, 0, 0), 0.0f, 0.0f);
    assert(s.headRollVel < 0.0f);  // the body tipped right off the edge, the head is late
    float thrown = 0.0f;
    bool hit = false;
    for (int i = 0; i < 90; i++) {
        fallStep(s, 1.0f / 60.0f, CS, oneHole);
        if (s.hit > 0.0f) hit = true;
        if (hit && s.headRoll > thrown) thrown = s.headRoll;
        assert(fabsf(s.headRoll) <= FALL_NECK_ROLL && fabsf(s.headPitch) <= FALL_NECK_PITCH);
        assert(fabsf(s.headYaw) <= FALL_NECK_YAW);
    }
    assert(hit && thrown > 0.2f);
    assert(fabsf(s.headRoll) < 0.05f);  // the neck brought it back
    // Face first into the +z wall: a nod.
    FallState f = fallBegin(Vec3(2.5f, 1.7f, 4.0f), Vec3(0, 0, 3.0f), 0.0f, 0.0f);
    float nod = 0.0f;
    bool faceHit = false;
    for (int i = 0; i < 60; i++) {
        fallStep(f, 1.0f / 60.0f, CS, oneHole);
        if (f.hit > 0.0f) faceHit = true;
        if (faceHit && f.headPitch < nod) nod = f.headPitch;
    }
    assert(faceHit && nod < -0.2f);
    assert(fallViewPitch(f) >= -FALL_PITCH_LIMIT && fallViewPitch(f) <= FALL_PITCH_LIMIT);
}

void testScrapingAWallSpinsTheBody() {
    // Straight down the middle: nothing turns the body round, the head stays in line.
    FallState still = fallBegin(Vec3(2.5f, 1.7f, 2.5f), Vec3(), 0.7f, 0.0f);
    // Along +z and into the +x wall: the right side is held back, the body turns right.
    FallState s = fallBegin(Vec3(4.5f, 1.7f, 1.0f), Vec3(2.0f, 0, 3.0f), 0.0f, 0.0f);
    float lag = 0.0f;
    for (int i = 0; i < 30; i++) {
        fallStep(still, 1.0f / 60.0f, CS, oneHole);
        fallStep(s, 1.0f / 60.0f, CS, oneHole);
        if (s.headYaw < lag) lag = s.headYaw;
    }
    assert(still.yaw == 0.7f && still.headYaw == 0.0f && fabsf(still.headRoll) < 1e-4f);
    assert(s.yaw > 0.1f && s.yawVel > 0.0f);
    assert(lag < -0.01f);                       // the head came round after the body
    assert(fabsf(fallViewYaw(s) - s.yaw - s.headYaw) < 1e-6f);
}

void testShakeGrowsWithSpeed() {
    FallState s = fallBegin(Vec3(2.5f, 1.7f, 2.5f), Vec3(), 0.0f, 0.0f);
    float start = fallShake(s);
    for (int i = 0; i < 120; i++) fallStep(s, 1.0f / 60.0f, CS, oneHole);
    assert(fallShake(s) > start + 0.15f && fallShake(s) <= 0.9f);
}

int main() {
    testFloorHoldsUntilPastTheLip();
    testFreeFallIsRealGravity();
    testFrameRateDoesNotChangeTheFall();
    testTerminalSpeed();
    testMomentumCarriesAndWallsStop();
    testBodyTipsTheWayItWent();
    testHeadIsThrownAtTheWallAndComesBack();
    testScrapingAWallSpinsTheBody();
    testShakeGrowsWithSpeed();
    std::cout << "fall_sim tests passed\n";
    return 0;
}
