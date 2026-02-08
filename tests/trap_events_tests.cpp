#include <cassert>
#include <iostream>

#include "../src/trap_events.h"

void testInsideTrapTrigger() {
    assert(isInsideTrapTrigger(12, 5, 10, 5, 8));
    assert(!isInsideTrapTrigger(11, 4, 10, 5, 8));
    assert(!isInsideTrapTrigger(30, 5, 10, 5, 8));
}

void testLookingAtPoint() {
    Vec3 cam(0, 0, 0);
    Vec3 target(0, 0, 5);
    assert(isLookingAtPoint(cam, 0.0f, 0.0f, target, 0.8f, 10.0f));
    assert(!isLookingAtPoint(cam, 1.57f, 0.0f, target, 0.8f, 10.0f));
}

void testStareProgressUpdate() {
    float p = 0.0f;
    p = updateTrapStareProgress(p, 1.0f, true);
    assert(p > 0.9f);
    p = updateTrapStareProgress(p, 1.0f, false);
    assert(p < 1.0f);
}

void testBlurUpdate() {
    float b = 0.0f;
    b = updateAnomalyBlur(b, 0.5f, true);
    assert(b > 0.5f);
    b = updateAnomalyBlur(b, 1.0f, false);
    assert(b < 0.5f);
}

void testFloorHoleRolls() {
    assert(floorHoleCountFromRoll(0) == 4);
    assert(floorHoleCountFromRoll(3) == 7);
    assert(floorHoleDurationFromRoll(0) == 9.0f);
    assert(floorHoleDurationFromRoll(5) == 14.0f);
}

int main() {
    testInsideTrapTrigger();
    testLookingAtPoint();
    testStareProgressUpdate();
    testBlurUpdate();
    testFloorHoleRolls();
    std::cout << "All trap events tests passed.\n";
    return 0;
}
