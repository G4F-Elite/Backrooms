#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/interpolation.h"

void testNormalizeAngleRange() {
    float a = normalizeAngle(7.0f);
    assert(a >= -3.1415927f && a <= 3.1415927f);
    float b = normalizeAngle(-7.0f);
    assert(b >= -3.1415927f && b <= 3.1415927f);
}

void testLerpAngleShortestPath() {
    float from = 3.10f;
    float to = -3.10f;
    float mid = lerpAngle(from, to, 0.5f);
    assert(std::fabs(mid) > 3.0f);
}

void testLerpAngleIdentity() {
    float a = 1.25f;
    float b = lerpAngle(a, a, 0.5f);
    assert(std::fabs(b - a) < 0.0001f);
}

void testClamp01() {
    assert(clamp01(-1.0f) == 0.0f);
    assert(clamp01(2.0f) == 1.0f);
    assert(clamp01(0.5f) == 0.5f);
}

int main() {
    testNormalizeAngleRange();
    testLerpAngleShortestPath();
    testLerpAngleIdentity();
    testClamp01();
    std::cout << "All interpolation tests passed.\n";
    return 0;
}
