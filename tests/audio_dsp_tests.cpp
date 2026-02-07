#include <cassert>
#include <cmath>
#include <iostream>

#include "../src/audio_dsp.h"

void testSoftClipBounds() {
    for (float x = -5.0f; x <= 5.0f; x += 0.1f) {
        float y = softClip(x);
        assert(y <= 1.0f + 1e-4f);
        assert(y >= -1.0f - 1e-4f);
    }
}

void testLimiterReducesPeak() {
    float env = 0.0f;
    float out = 0.0f;
    for (int i = 0; i < 128; i++) out = applyLimiter(2.0f, env);
    assert(std::fabs(out) < 1.2f);
}

void testNoiseSmoothing() {
    float prev = 0.0f;
    for (int i = 0; i < 64; i++) prev = mixNoise(prev, 1.0f, 0.1f);
    assert(prev > 0.9f && prev < 1.0f);
}

int main() {
    testSoftClipBounds();
    testLimiterReducesPeak();
    testNoiseSmoothing();
    std::cout << "All audio DSP tests passed.\n";
    return 0;
}
