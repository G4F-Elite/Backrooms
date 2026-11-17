// Rig rules: heading math, the hull as an obstacle, the lane, breakdowns and the haven.
// g++ -std=c++17 tests/rig_rules_tests.cpp -o build/rig_rules_tests.exe
#include <cassert>
#include <cstdio>
#include "../src/rig_rules.h"

static bool near(float a, float b) { return fabsf(a - b) < 1e-3f; }

static void testHeading() {
    for (int dir = 0; dir < 4; dir++) {
        assert(abs(rigDirX(dir)) + abs(rigDirZ(dir)) == 1);
        RigFrame f;
        f.x = 12.5f;
        f.z = -7.5f;
        f.dir = dir;
        // forward is the heading, and the renderer's yaw points the same way
        float wx = 0.0f, wz = 0.0f;
        rigToWorld(f, 0.0f, 2.0f, wx, wz);
        assert(near(wx - f.x, 2.0f * (float)rigDirX(dir)) && near(wz - f.z, 2.0f * (float)rigDirZ(dir)));
        assert(near(sinf(rigYaw(dir)), (float)rigDirX(dir)) && near(cosf(rigYaw(dir)), (float)rigDirZ(dir)));
        // right = (cos yaw, -sin yaw), like composeModelMatrix and DecorBuilder
        rigToWorld(f, 1.0f, 0.0f, wx, wz);
        assert(near(wx - f.x, cosf(rigYaw(dir))) && near(wz - f.z, -sinf(rigYaw(dir))));
        // round trip
        float lx = 0.0f, lz = 0.0f;
        rigToWorld(f, -0.7f, 1.9f, wx, wz);
        rigToLocal(f, wx, wz, lx, lz);
        assert(near(lx, -0.7f) && near(lz, 1.9f));
    }
}

static void testHull() {
    const float r = 0.3f; // player radius
    // the aisle, the doorway and every arrival seat are free
    assert(rigHitBox(0.0f, -0.8f, r) < 0);
    assert(rigHitBox(0.0f, RIG_CABIN_BACK, r) < 0);
    assert(rigHitBox(0.0f, RIG_CABIN_BACK - 1.0f, r) < 0);
    for (int slot = 0; slot < 4; slot++) {
        RigPoint s = rigSeat(slot);
        assert(rigInCabin(s.x, s.z) && rigHitBox(s.x, s.z, r) < 0);
        for (int other = 0; other < slot; other++) assert(rigPointDist(s.x, s.z, rigSeat(other)) > 0.5f);
    }
    // walls, engine and auger are solid
    assert(rigHitBox(RIG_HALF_W, -2.0f, r) >= 0);
    assert(rigHitBox(-1.0f, RIG_CABIN_BACK, r) >= 0);
    assert(rigHitBox(0.0f, 0.9f, r) >= 0);
    assert(rigHitBox(0.0f, RIG_NOSE - 0.2f, r) == RIG_BOX_AUGER);
    // the tracks stick out past the hull and are just as solid, alongside the room and the engine
    const float side = RIG_HALF_W + RIG_TRACK_W;
    assert(rigHitBox(side + r - 0.05f, -2.0f, r) >= 0 && rigHitBox(-side - r + 0.05f, 0.9f, r) >= 0);
    assert(RIG_TRACK_BACK - RIG_TRACK_R > RIG_CABIN_BACK && RIG_TRACK_FRONT + RIG_TRACK_R < RIG_ENGINE_FRONT);
    // a 5 m cell leaves room to squeeze past on both sides
    assert(rigHitBox(side + r + 0.05f, 0.0f, r) < 0 && side + 2.0f * r < 2.5f);

    // whatever the hull runs into ends up outside it, and the auger says it was the auger
    for (float lx = -1.6f; lx <= 1.6f; lx += 0.2f) {
        for (float lz = RIG_CABIN_BACK - 0.5f; lz <= RIG_NOSE + 0.5f; lz += 0.2f) {
            float px = lx, pz = lz;
            rigPushOut(px, pz, r);
            assert(rigHitBox(px, pz, r) < 0);
        }
    }
    float px = 0.0f, pz = RIG_NOSE - 0.1f;
    assert(rigPushOut(px, pz, r) == RIG_BOX_AUGER && pz > RIG_NOSE);

    assert(rigInCabin(0.0f, -1.0f) && !rigInCabin(0.0f, 0.9f) && !rigInCabin(0.0f, RIG_CABIN_BACK - 0.2f));
    assert(rigOnBoard(0.0f, RIG_CABIN_BACK - 0.2f)); // standing in the doorway still rides along
    assert(!rigOnBoard(RIG_HALF_W + 0.4f, -1.0f));
}

static void testSpots() {
    // hopper and charger are used from the aisle, each at its own end
    assert(rigSpotAt(-0.4f, -0.5f, RIG_OK, true) == RIG_SPOT_HOPPER);
    assert(rigSpotAt(0.6f, -0.6f, RIG_OK, true) == RIG_SPOT_CHARGER);
    assert(rigSpotAt(0.6f, -0.6f, RIG_OK, false) != RIG_SPOT_CHARGER);
    assert(rigSpotAt(0.0f, -3.0f, RIG_OK, true) == RIG_SPOT_NONE);
    assert(rigSpotAt(-2.0f, 0.0f, RIG_OK, true) == RIG_SPOT_NONE); // not through the wall
    // every fault has a panel a body can stand at, on the right side of the wall
    for (int fault = RIG_FAULT_FUSE; fault < RIG_FAULT_COUNT; fault++) {
        RigPoint p = rigFaultPoint(fault);
        bool outside = rigFaultIsOutside(fault);
        float sx = outside ? p.x + (p.x > 0.0f ? 0.55f : -0.55f) : p.x - 0.55f; // outside: past the track
        assert(rigHitBox(sx, p.z, 0.3f) < 0);
        assert(rigInCabin(sx, p.z) == !outside);
        assert(rigSpotAt(sx, p.z, fault, true) == RIG_SPOT_FAULT);
        assert(rigSpotAt(sx, p.z, RIG_OK, false) != RIG_SPOT_FAULT);
    }
    // an engine cover cannot be fixed from the couch
    assert(rigSpotAt(-0.5f, -0.2f, RIG_FAULT_BELT, false) != RIG_SPOT_FAULT);
    for (unsigned roll = 0; roll < 50; roll++) {
        int fault = rigRollFault(roll);
        assert(fault > RIG_OK && fault < RIG_FAULT_COUNT);
    }
}

static void testLane() {
    const float cs = 5.0f;
    // parked: the break room sits in the cell behind, the auger pokes into the cell ahead
    assert(rigStepAt(0.0f, RIG_CABIN_BACK, cs) == RIG_LANE_BACK_STEP);
    assert(rigStepAt(0.0f, 0.0f, cs) == 0);
    assert(rigFrontStep(0.0f, cs) == 1);
    // walls always give way before the tip gets there, one cell at a time
    int last = rigFrontStep(0.0f, cs);
    for (float t = 0.0f; t < 60.0f; t += 0.05f) {
        int front = rigFrontStep(t, cs);
        assert(front == last || front == last + 1);
        assert((float)front * cs - cs * 0.5f <= t + RIG_NOSE + RIG_CARVE_LEAD);
        assert(((float)front + 0.5f) * cs > t + RIG_NOSE);
        last = front;
    }
    assert(RIG_SPEED < 2.0f); // slower than a crouch walk: nobody is left behind by accident
}

static void testBreakdowns() {
    for (int level = 0; level < 12; level++) {
        assert(rigFaultDelay(level, 0.0f) < rigFaultDelay(level, 1.0f));
        assert(rigFaultDelay(level, 0.0f) > RIG_RESERVE_SECONDS); // always more up-time than panic
        assert(rigFaultDelay(level + 1, 0.5f) <= rigFaultDelay(level, 0.5f));
    }
    assert(rigFaultDelay(8, 0.5f) < rigFaultDelay(0, 0.5f));

    // running: the whole rig and a few steps around it; broken: the room, then nothing
    assert(rigHavenRadius(RIG_OK, 0.0f) == RIG_HAVEN_RADIUS);
    assert(rigHavenRadius(RIG_FAULT_BELT, 1.0f) == RIG_HAVEN_RESERVE_RADIUS);
    assert(rigHavenRadius(RIG_FAULT_BELT, RIG_RESERVE_SECONDS + 0.1f) == 0.0f);
    assert(near(rigReserveLeft(RIG_FAULT_FUSE, RIG_RESERVE_SECONDS * 0.5f), 0.5f));
    assert(rigReserveLeft(RIG_FAULT_FUSE, 999.0f) == 0.0f && rigReserveLeft(RIG_OK, 999.0f) == 1.0f);

    // the reserve haven covers every seat in the room; the full one reaches past the auger
    for (float lx = -1.1f; lx <= 1.1f; lx += 0.1f) {
        for (float lz = RIG_CABIN_BACK + 0.1f; lz < RIG_CABIN_FRONT - 0.2f; lz += 0.1f) {
            assert(sqrtf(lx * lx + (lz - RIG_HAVEN_Z) * (lz - RIG_HAVEN_Z)) < RIG_HAVEN_RESERVE_RADIUS);
        }
    }
    assert(RIG_NOSE - RIG_HAVEN_Z < RIG_HAVEN_RADIUS && RIG_QUIET_RADIUS > RIG_HAVEN_RADIUS + 10.0f && rigQuietRadius(RIG_FAULT_FUSE) == 0.0f);
    // the outside covers are not under the reserve haven: fixing them is a risk
    for (int fault = RIG_FAULT_BELT; fault <= RIG_FAULT_VALVE; fault++) {
        RigPoint p = rigFaultPoint(fault);
        float sx = p.x + (p.x > 0.0f ? 0.55f : -0.55f);
        float fromRoom = sqrtf(sx * sx + (p.z - RIG_HAVEN_Z) * (p.z - RIG_HAVEN_Z));
        assert(fromRoom < RIG_HAVEN_RADIUS && fromRoom > RIG_HAVEN_RESERVE_RADIUS);
    }
    RigPoint fuse = rigFaultPoint(RIG_FAULT_FUSE);
    assert(rigPointDist(fuse.x - 0.55f, fuse.z, {0.0f, RIG_HAVEN_Z}) < RIG_HAVEN_RESERVE_RADIUS);
    assert(rigCanFix(-RIG_HALF_W - 0.45f - 1.2f, 0.85f, RIG_FAULT_BELT, 0.4f) && !rigCanFix(-RIG_HALF_W - 0.45f - 1.2f, 0.85f, RIG_FAULT_BELT, 0.0f));
}

int main() {
    testHeading();
    testHull();
    testSpots();
    testLane();
    testBreakdowns();
    printf("rig rules tests passed\n");
    return 0;
}
