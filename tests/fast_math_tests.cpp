#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/math.h"

void testDefaultMatchesStdTrig() {
    g_fastMathEnabled = false;
    float a = 1.2345f;
    assert(std::fabs(mSin(a) - sinf(a)) < 1e-6f);
    assert(std::fabs(mCos(a) - cosf(a)) < 1e-6f);
}

void testApproximationRangeAndError() {
    g_fastMathEnabled = true;
    for (float a = -3.14f; a <= 3.14f; a += 0.15f) {
        float s = mSin(a);
        float c = mCos(a);
        assert(s >= -1.05f && s <= 1.05f);
        assert(c >= -1.05f && c <= 1.05f);
        assert(std::fabs(s - sinf(a)) < 0.06f);
        assert(std::fabs(c - cosf(a)) < 0.06f);
    }
}

int main() {
    testDefaultMatchesStdTrig();
    testApproximationRangeAndError();
    std::cout << "All fast math tests passed.\n";
    return 0;
}
