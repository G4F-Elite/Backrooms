#include <cassert>
#include <iostream>

#include "../src/content_events.h"

void testEchoTypeRollRanges() {
    assert(chooseEchoTypeFromRoll(0) == ECHO_CACHE);
    assert(chooseEchoTypeFromRoll(39) == ECHO_CACHE);
    assert(chooseEchoTypeFromRoll(40) == ECHO_RESTORE);
    assert(chooseEchoTypeFromRoll(64) == ECHO_RESTORE);
    assert(chooseEchoTypeFromRoll(65) == ECHO_BREACH);
    assert(chooseEchoTypeFromRoll(84) == ECHO_BREACH);
    assert(chooseEchoTypeFromRoll(85) == ECHO_FLOOR_HOLE);
}

void testSpawnDelayRange() {
    float d1 = nextEchoSpawnDelaySeconds(0);
    float d2 = nextEchoSpawnDelaySeconds(19);
    assert(d1 == 12.0f);
    assert(d2 == 31.0f);
}

void testCacheOutcomeAddsOneItem() {
    int b = 0, m = 0, t = 0;
    float hp = 80.0f, sn = 60.0f, st = 40.0f;
    bool breach = false;
    applyEchoOutcome(ECHO_CACHE, 1, b, m, t, hp, sn, st, breach);
    assert(!breach);
    assert(b == 0);
    assert(m == 1);
    assert(t == 0);
}

void testRestoreOutcomeClampsVitals() {
    int b = 0, m = 0, t = 0;
    float hp = 95.0f, sn = 90.0f, st = 75.0f;
    bool breach = false;
    applyEchoOutcome(ECHO_RESTORE, 0, b, m, t, hp, sn, st, breach);
    assert(!breach);
    assert(hp == 100.0f);
    assert(sn == 100.0f);
    assert(st == 100.0f);
}

void testBreachOutcome() {
    int b = 0, m = 0, t = 0;
    float hp = 50.0f, sn = 10.0f, st = 50.0f;
    bool breach = false;
    applyEchoOutcome(ECHO_BREACH, 0, b, m, t, hp, sn, st, breach);
    assert(breach);
    assert(sn == 0.0f);
}

void testRangeCheckIgnoresHeight() {
    Vec3 p(0, 1, 0);
    Vec3 e(1, 100, 1);
    assert(isEchoInRange(p, e, 2.0f));
}

void testFloorHoleOutcome() {
    int b = 0, m = 0, t = 0;
    float hp = 80.0f, sn = 60.0f, st = 40.0f;
    bool breach = false;
    applyEchoOutcome(ECHO_FLOOR_HOLE, 0, b, m, t, hp, sn, st, breach);
    assert(!breach);
    assert(hp == 0.0f);
    assert(sn == 0.0f);
}

void testIsOnFloorHole() {
    Vec3 player(5.0f, 0.0f, 5.0f);
    Vec3 hole(5.5f, 0.0f, 5.0f);
    assert(isOnFloorHole(player, hole, 1.0f));
    assert(!isOnFloorHole(player, hole, 0.3f));
}

int main() {
    testEchoTypeRollRanges();
    testSpawnDelayRange();
    testCacheOutcomeAddsOneItem();
    testRestoreOutcomeClampsVitals();
    testBreachOutcome();
    testRangeCheckIgnoresHeight();
    testFloorHoleOutcome();
    testIsOnFloorHole();
    std::cout << "All content events tests passed.\n";
    return 0;
}
