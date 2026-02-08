#include <cassert>
#include <iostream>

#include "../src/flashlight_behavior.h"

void testBlinkStartThreshold() {
    assert(!shouldStartFlashlightShutdownBlink(2.01f));
    assert(shouldStartFlashlightShutdownBlink(2.0f));
    assert(shouldStartFlashlightShutdownBlink(0.5f));
}

void testBlinkDuration() {
    assert(!isFlashlightShutdownBlinkFinished(0.0f));
    assert(!isFlashlightShutdownBlinkFinished(0.54f));
    assert(isFlashlightShutdownBlinkFinished(0.55f));
    assert(isFlashlightShutdownBlinkFinished(1.2f));
}

void testBlinkPatternWindows() {
    assert(isFlashlightOnDuringShutdownBlink(0.00f));
    assert(isFlashlightOnDuringShutdownBlink(0.07f));
    assert(!isFlashlightOnDuringShutdownBlink(0.08f));
    assert(!isFlashlightOnDuringShutdownBlink(0.13f));
    assert(isFlashlightOnDuringShutdownBlink(0.14f));
    assert(isFlashlightOnDuringShutdownBlink(0.23f));
    assert(!isFlashlightOnDuringShutdownBlink(0.24f));
    assert(!isFlashlightOnDuringShutdownBlink(0.29f));
    assert(isFlashlightOnDuringShutdownBlink(0.30f));
    assert(isFlashlightOnDuringShutdownBlink(0.41f));
    assert(!isFlashlightOnDuringShutdownBlink(0.42f));
    assert(!isFlashlightOnDuringShutdownBlink(0.48f));
    assert(isFlashlightOnDuringShutdownBlink(0.49f));
    assert(isFlashlightOnDuringShutdownBlink(0.70f));
}

void testBlinkNegativeTime() {
    assert(isFlashlightOnDuringShutdownBlink(-0.01f));
}

int main() {
    testBlinkStartThreshold();
    testBlinkDuration();
    testBlinkPatternWindows();
    testBlinkNegativeTime();
    std::cout << "All flashlight behavior tests passed.\n";
    return 0;
}
