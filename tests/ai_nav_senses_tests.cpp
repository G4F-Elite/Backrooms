// Unit tests for the pure AI building blocks: grid navigation, senses, the snapshot state
// byte and the spectator camera math.
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "../src/ai_senses.h"
#include "../src/entity_types.h"
#include "../src/spectator_math.h"

static const float TCS = 5.0f;
static std::vector<const char*> gMap;

static int cellAt(int x, int z) {
    if (z < 0 || z >= (int)gMap.size()) return 1;
    int w = 0;
    while (gMap[z][w]) w++;
    if (x < 0 || x >= w) return 1;
    return gMap[z][x] == '#' ? 1 : 0;
}

static float cx(int c) { return (c + 0.5f) * TCS; }

static void uMap() {
    gMap = {
        "#########",
        "#...#...#",
        "#...#...#",
        "#.......#",
        "#########",
    };
}

void testLineOfSightStopsAtWalls() {
    uMap();
    assert(navLineOfSight(cx(1), cx(1), cx(3), cx(1), TCS, cellAt));
    assert(!navLineOfSight(cx(3), cx(1), cx(5), cx(1), TCS, cellAt)); // wall column between the rooms
    assert(navLineOfSight(cx(3), cx(3), cx(5), cx(3), TCS, cellAt));  // the gap at the bottom
    assert(!navLineOfSight(cx(1), cx(1), cx(7), cx(2), TCS, cellAt));
    // Symmetric, and a point always sees itself.
    assert(!navLineOfSight(cx(5), cx(1), cx(3), cx(1), TCS, cellAt));
    assert(navLineOfSight(cx(2), cx(2), cx(2), cx(2), TCS, cellAt));
}

void testWalkableLineNeedsRoomForShoulders() {
    uMap();
    // Cutting diagonally past the end of the dividing wall (its corner is at x=20, z=15):
    // the centre line squeezes by 20 cm below the corner, a 32 cm shoulder does not.
    float ax = 17.5f, az = 14.0f, bx = 22.5f, bz = 16.4f;
    assert(navLineOfSight(ax, az, bx, bz, TCS, cellAt));
    assert(!navWalkableLine(ax, az, bx, bz, 0.32f, TCS, cellAt));
    assert(navWalkableLine(ax, az, bx, bz, 0.05f, TCS, cellAt));
    // A little lower and the whole body fits.
    assert(navWalkableLine(ax, az + 1.0f, bx, bz + 1.0f, 0.32f, TCS, cellAt));
}

void testPathGoesAroundTheWall() {
    uMap();
    NavCell path[32];
    int n = navFindPath({3, 1}, {5, 1}, cellAt, path, 32);
    assert(n == 6); // down two, across two, up two
    assert(path[n - 1].x == 5 && path[n - 1].z == 1);
    for (int i = 0; i < n; i++) {
        assert(cellAt(path[i].x, path[i].z) == 0);
        int px = i == 0 ? 3 : path[i - 1].x, pz = i == 0 ? 1 : path[i - 1].z;
        assert(abs(path[i].x - px) + abs(path[i].z - pz) == 1); // no diagonal corner cutting
    }
    // Truncated output still starts at the first step and reports what it wrote.
    NavCell head[3];
    assert(navFindPath({3, 1}, {5, 1}, cellAt, head, 3) == 3);
    assert(head[0].x == path[0].x && head[0].z == path[0].z);
}

void testPathFailsCleanly() {
    gMap = {
        "#######",
        "#..#..#",
        "#######",
    };
    NavCell path[8];
    assert(navFindPath({1, 1}, {5, 1}, cellAt, path, 8) == 0); // sealed off
    assert(navFindPath({1, 1}, {3, 1}, cellAt, path, 8) == 0); // goal is a wall
    assert(navFindPath({1, 1}, {1, 1}, cellAt, path, 8) == 0); // already there
    assert(navFindPath({1, 1}, {500, 1}, cellAt, path, 8) == 0); // outside the search window
}

void testPathCostSteersAroundWatchedCells() {
    // A ring corridor: two equally long ways from the left side to the right side.
    gMap = {
        "#######",
        "#.....#",
        "#.###.#",
        "#.....#",
        "#######",
    };
    NavCell path[32];
    auto topWatched = [](int, int z) { return z == 1 ? 8 : 0; };
    int n = navFindPathCost({1, 2}, {5, 2}, cellAt, topWatched, path, 32);
    assert(n == 6);
    for (int i = 0; i < n - 1; i++) assert(path[i].z != 1); // took the bottom corridor
    auto bottomWatched = [](int, int z) { return z == 3 ? 8 : 0; };
    n = navFindPathCost({1, 2}, {5, 2}, cellAt, bottomWatched, path, 32);
    assert(n == 6);
    for (int i = 0; i < n - 1; i++) assert(path[i].z != 3);
    // A watched dead end is still entered when it is the only way.
    gMap = {
        "#####",
        "#...#",
        "#####",
    };
    auto allWatched = [](int, int) { return 8; };
    assert(navFindPathCost({1, 1}, {3, 1}, cellAt, allWatched, path, 32) == 2);
}

void testPickReachableCells() {
    uMap();
    NavCell out = {0, 0};
    NavCell threat = {1, 1};
    assert(navPickReachable({2, 2}, 2, 12, &threat, 0u, cellAt, out));
    assert(cellAt(out.x, out.z) == 0);
    assert(abs(out.x - threat.x) + abs(out.z - threat.z) >= 6); // ran to the far room
    for (unsigned roll = 0; roll < 50; roll++) {
        assert(navPickReachable({2, 2}, 2, 4, nullptr, roll * 7919u, cellAt, out));
        assert(cellAt(out.x, out.z) == 0);
    }
    gMap = {
        "###",
        "#.#",
        "###",
    };
    assert(!navPickReachable({1, 1}, 1, 5, nullptr, 3u, cellAt, out)); // nowhere to go
}

void testFootstepLoudness() {
    assert(footstepNoiseRadius(0.0f, false, false) == 0.0f);
    assert(footstepNoiseRadius(PLAYER_WALK_SPEED, false, false) == NOISE_WALK);
    assert(footstepNoiseRadius(PLAYER_SPRINT_SPEED, false, false) == NOISE_SPRINT);
    assert(footstepNoiseRadius(2.0f, true, false) == NOISE_CROUCH);
    assert(footstepNoiseRadius(PLAYER_WALK_SPEED, false, true) < NOISE_WALK); // quiet steps perk
    assert(NOISE_CROUCH < NOISE_WALK && NOISE_WALK < NOISE_SPRINT && NOISE_SPRINT < NOISE_SQUEAK);
}

void testHearingIsMuffledByWalls() {
    uMap();
    Vec3 listener(cx(3), 0, cx(1));
    Vec3 sameRoom(cx(1), 0, cx(1));  // 10 m, clear
    Vec3 nextRoom(cx(5), 0, cx(1));  // 10 m, wall in between
    assert(heardLoudness(listener, sameRoom, 12.0f, 1.0f, TCS, cellAt) > 0.0f);
    assert(heardLoudness(listener, nextRoom, 12.0f, 1.0f, TCS, cellAt) == 0.0f);
    assert(heardLoudness(listener, nextRoom, 26.0f, 1.0f, TCS, cellAt) > 0.0f);
    assert(heardLoudness(listener, sameRoom, 12.0f, 0.5f, TCS, cellAt) == 0.0f); // distracted listener
    assert(heardLoudness(listener, sameRoom, 0.0f, 1.0f, TCS, cellAt) == 0.0f);
    // Closer is louder.
    Vec3 close(cx(2), 0, cx(1));
    assert(heardLoudness(listener, close, 12.0f, 1.0f, TCS, cellAt) > heardLoudness(listener, sameRoom, 12.0f, 1.0f, TCS, cellAt));
}

void testViewConeAndLooking() {
    uMap();
    AiTarget t;
    t.pos = Vec3(cx(1), 1.7f, cx(1));
    t.yaw = 0.5f * 3.14159265f; // facing +X
    assert(targetLooksAt(t, Vec3(cx(3), 0, cx(1)), 40.0f, TCS, cellAt));
    assert(!targetLooksAt(t, Vec3(cx(5), 0, cx(1)), 40.0f, TCS, cellAt)); // in the cone, behind the wall
    assert(!targetLooksAt(t, Vec3(cx(1), 0, cx(3)), 40.0f, TCS, cellAt)); // 90 degrees off to the side
    assert(!targetLooksAt(t, Vec3(cx(3), 0, cx(1)), 5.0f, TCS, cellAt));  // too far
    t.pitch = 1.3f; // staring at the ceiling
    assert(!targetLooksAt(t, Vec3(cx(3), 0, cx(1)), 40.0f, TCS, cellAt));
    t.pitch = 0.0f;
    // Lights out: only a flashlight shows anything, and not very far.
    assert(!targetSeesInLight(t, Vec3(cx(3), 0, cx(1)), 40.0f, true, TCS, cellAt));
    t.flashlight = true;
    assert(targetSeesInLight(t, Vec3(cx(3), 0, cx(1)), 40.0f, true, TCS, cellAt));
}

void testAwarenessBuildsAndFades() {
    float far = sightEvidencePerSec(20.0f, 22.0f, false, false);
    float near = sightEvidencePerSec(4.0f, 22.0f, false, false);
    assert(near > far && far > 0.0f);
    assert(sightEvidencePerSec(30.0f, 22.0f, false, false) == 0.0f);
    assert(sightEvidencePerSec(10.0f, 22.0f, true, false) > sightEvidencePerSec(10.0f, 22.0f, false, false));
    assert(sightEvidencePerSec(10.0f, 22.0f, false, true) < sightEvidencePerSec(10.0f, 22.0f, false, false));

    float a = 0.0f;
    int framesToAlert = 0;
    while (a < AWARE_ALERT && framesToAlert < 10000) {
        a = stepAwareness(a, far, 1.0f / 60.0f);
        framesToAlert++;
    }
    float secondsFar = framesToAlert / 60.0f;
    assert(secondsFar > 1.0f && secondsFar < 4.0f); // a grey zone, not a switch
    for (int i = 0; i < 60 * 3; i++) a = stepAwareness(a, 0.0f, 1.0f / 60.0f);
    assert(a < AWARE_ALERT && a > AWARE_SUSPICIOUS); // still looking for you three seconds later
    for (int i = 0; i < 60 * 10; i++) a = stepAwareness(a, 0.0f, 1.0f / 60.0f);
    assert(a == 0.0f);
}

void testIsolationFindsTheLoner() {
    AiTarget t[3];
    t[0].pos = Vec3(0, 0, 0);
    t[1].pos = Vec3(3, 0, 0);
    t[2].pos = Vec3(40, 0, 0);
    computeIsolation(t, 3);
    assert(fabsf(t[0].isolation - 3.0f) < 0.01f);
    assert(fabsf(t[1].isolation - 3.0f) < 0.01f);
    assert(fabsf(t[2].isolation - 37.0f) < 0.01f);
    computeIsolation(t, 1);
    assert(t[0].isolation == 99.0f); // solo: always alone
}

void testEntityStatePacking() {
    for (int state = 0; state <= ENT_ATTACKING; state++) {
        for (int mood = 0; mood < MOOD_COUNT; mood++) {
            unsigned char packed = packEntityState(state, mood);
            assert(unpackEntityState(packed) == state);
            assert(unpackEntityMood(packed) == mood);
        }
    }
    // Old hosts send the bare state: it must read back as "wandering", not garbage.
    assert(unpackEntityMood((unsigned char)ENT_CHASING) == MOOD_WANDER);
}

void testSpectatorCameraStaysOutOfWalls() {
    uMap();
    const float ceiling = 4.5f;
    Vec3 focus(cx(3), 1.7f, cx(2));
    // Looking along +X from behind: 12.5 m of room behind the focus, so the full distance is free.
    float free = spectatorFreeDistance(focus, 0.5f * 3.14159265f, 0.0f, 3.4f, TCS, ceiling, cellAt);
    assert(free > 3.2f && free <= 3.4f + 1e-3f);
    // Looking along -X: the dividing wall is 2.5 m behind the focus.
    free = spectatorFreeDistance(focus, -0.5f * 3.14159265f, 0.0f, 3.4f, TCS, ceiling, cellAt);
    assert(free < 2.5f && free > 1.8f);
    Vec3 camPos = spectatorOrbitPos(focus, -0.5f * 3.14159265f, 0.0f, free);
    assert(cellAt((int)floorf(camPos.x / TCS), (int)floorf(camPos.z / TCS)) == 0);
    // Looking steeply down from above: the ceiling stops the camera, not a wall.
    free = spectatorFreeDistance(focus, 0.0f, -1.2f, 6.0f, TCS, ceiling, cellAt);
    Vec3 high = spectatorOrbitPos(focus, 0.0f, -1.2f, free);
    assert(high.y <= ceiling - SPECTATOR_WALL_MARGIN + 1e-3f && free < 3.2f);
    // Negative world coordinates behave the same (the real levels live there too).
    gMap = {"###", "#.#", "###"};
    auto shifted = [](int x, int z) { return cellAt(x + 40, z + 40); };
    Vec3 f2(-38.5f * TCS, 1.7f, -38.5f * TCS); // centre of cell (-39, -39)
    free = spectatorFreeDistance(f2, 0.0f, 0.0f, 3.4f, TCS, ceiling, shifted);
    assert(free > 1.9f && free < 2.4f);
}

void testSpectatorDistanceSmoothing() {
    // In: instantly. Out: eased.
    assert(spectatorSmoothDist(3.4f, 1.0f, 0.016f) == 1.0f);
    float d = spectatorSmoothDist(1.0f, 3.4f, 0.016f);
    assert(d > 1.0f && d < 1.5f);
    for (int i = 0; i < 300; i++) d = spectatorSmoothDist(d, 3.4f, 0.016f);
    assert(fabsf(d - 3.4f) < 0.01f);
    assert(spectatorClampDist(0.1f) == SPECTATOR_MIN_DIST);
    assert(spectatorClampDist(99.0f) == SPECTATOR_MAX_DIST);
    assert(spectatorClampPitch(-9.0f) == SPECTATOR_MIN_PITCH);
    assert(spectatorClampPitch(9.0f) == SPECTATOR_MAX_PITCH);
}

void testSpectatorCycling() {
    assert(spectatorCycle(0, 3, 1) == 1);
    assert(spectatorCycle(2, 3, 1) == 0);
    assert(spectatorCycle(0, 3, -1) == 2);
    assert(spectatorCycle(-1, 3, 1) == 0);  // nothing selected yet
    assert(spectatorCycle(-1, 3, -1) == 2);
    assert(spectatorCycle(7, 3, 1) == 0);   // the old target vanished from the list
    assert(spectatorCycle(0, 1, 1) == 0);
    assert(spectatorCycle(0, 0, 1) == -1);

    SpectatorSlot slots[3] = {{SPEC_PLAYER, 0}, {SPEC_PLAYER, 2}, {SPEC_MONSTER, 2}};
    assert(spectatorFindSlot(slots, 3, SPEC_PLAYER, 2) == 1);
    assert(spectatorFindSlot(slots, 3, SPEC_MONSTER, 2) == 2); // same id, different kind
    assert(spectatorFindSlot(slots, 3, SPEC_MONSTER, 9) == -1);
}

void testSpectatorPerch() {
    Vec3 eye(10.0f, 1.7f, 10.0f);
    Vec3 shoulder = spectatorPerch(eye, 0.0f, false, 0.0f);
    float side = sqrtf((shoulder.x - eye.x) * (shoulder.x - eye.x) + (shoulder.z - eye.z) * (shoulder.z - eye.z));
    assert(side > 0.2f && side < 0.5f);          // next to the head, not inside it
    assert(fabsf(shoulder.z - eye.z) < 0.01f);   // beside, not in front of the face
    assert(shoulder.y - SPECTATOR_GHOST_DROP > 1.1f && shoulder.y - SPECTATOR_GHOST_DROP < 1.5f); // feet at shoulder height
    // Turning the teammate swings the perch around with them.
    Vec3 turned = spectatorPerch(eye, 3.14159265f, false, 0.0f);
    assert(fabsf((turned.x - eye.x) + (shoulder.x - eye.x)) < 0.01f);
    Vec3 onMonster = spectatorPerch(Vec3(4.0f, 0.8f, 6.0f), 1.0f, true, 1.05f);
    assert(onMonster.x == 4.0f && onMonster.z == 6.0f);
    assert(fabsf(onMonster.y - SPECTATOR_GHOST_DROP - 1.05f) < 0.01f); // standing on its back
}

int main() {
    testLineOfSightStopsAtWalls();
    testWalkableLineNeedsRoomForShoulders();
    testPathGoesAroundTheWall();
    testPathFailsCleanly();
    testPathCostSteersAroundWatchedCells();
    testPickReachableCells();
    testFootstepLoudness();
    testHearingIsMuffledByWalls();
    testViewConeAndLooking();
    testAwarenessBuildsAndFades();
    testIsolationFindsTheLoner();
    testEntityStatePacking();
    testSpectatorCameraStaysOutOfWalls();
    testSpectatorDistanceSmoothing();
    testSpectatorCycling();
    testSpectatorPerch();
    std::cout << "All AI nav / senses / spectator math tests passed.\n";
    return 0;
}
