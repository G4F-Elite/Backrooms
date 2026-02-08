#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/math.h"

void testFastSinAccuracy() {
    // Test at known values
    float err = fabsf(fastSinf(0.0f) - sinf(0.0f));
    assert(err < 0.01f);

    err = fabsf(fastSinf(1.5708f) - sinf(1.5708f));
    assert(err < 0.01f);

    err = fabsf(fastSinf(3.14159f) - sinf(3.14159f));
    assert(err < 0.02f);

    err = fabsf(fastSinf(-1.5708f) - sinf(-1.5708f));
    assert(err < 0.01f);
}

void testFastCosAccuracy() {
    float err = fabsf(fastCosf(0.0f) - cosf(0.0f));
    assert(err < 0.01f);

    err = fabsf(fastCosf(1.5708f) - cosf(1.5708f));
    assert(err < 0.02f);

    err = fabsf(fastCosf(3.14159f) - cosf(3.14159f));
    assert(err < 0.02f);
}

void testFastTanBasic() {
    float err = fabsf(fastTanf(0.0f) - tanf(0.0f));
    assert(err < 0.02f);

    // Fast tan has larger error due to accumulated sin/cos approx
    err = fabsf(fastTanf(0.5f) - tanf(0.5f));
    assert(err < 0.2f);
}

void testGameWrapperToggle() {
    gFastMathEnabled = false;
    float standard = gameSinf(1.0f);
    assert(standard == sinf(1.0f));

    gFastMathEnabled = true;
    float fast = gameSinf(1.0f);
    assert(fast == fastSinf(1.0f));

    gFastMathEnabled = false;
    float back = gameSinf(1.0f);
    assert(back == sinf(1.0f));
}

void testFastSinRange() {
    // Test across a wide range
    for (int i = -100; i <= 100; i++) {
        float x = (float)i * 0.1f;
        float fast = fastSinf(x);
        float std_val = sinf(x);
        float err = fabsf(fast - std_val);
        assert(err < 0.05f);
    }
}

int main() {
    testFastSinAccuracy();
    testFastCosAccuracy();
    testFastTanBasic();
    testGameWrapperToggle();
    testFastSinRange();
    std::cout << "All fast math tests passed.\n";
    return 0;
}
