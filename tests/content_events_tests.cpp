#include <cassert>
#include <iostream>

#include "../src/content_events.h"

void testEchoTypeRollRanges() {
    assert(chooseEchoTypeFromRoll(0) == ECHO_CACHE);
    assert(chooseEchoTypeFromRoll(49) == ECHO_CACHE);
    assert(chooseEchoTypeFromRoll(50) == ECHO_RESTORE);
    assert(chooseEchoTypeFromRoll(79) == ECHO_RESTORE);
    assert(chooseEchoTypeFromRoll(80) == ECHO_BREACH);
}

void testSpawnDelayRange() {
    float d1 = nextEchoSpawnDelaySeconds(0);
    float d2 = nextEchoSpawnDelaySeconds(19);
    assert(d1 == 18.0f);
    assert(d2 == 37.0f);
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

int main() {
    testEchoTypeRollRanges();
    testSpawnDelayRange();
    testCacheOutcomeAddsOneItem();
    testRestoreOutcomeClampsVitals();
    testBreachOutcome();
    testRangeCheckIgnoresHeight();
    std::cout << "All content events tests passed.\n";
    return 0;
}
