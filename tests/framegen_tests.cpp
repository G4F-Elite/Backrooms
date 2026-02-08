#include <cassert>
#include <iostream>
#include <string>

#include "../src/upscaler_settings.h"

void testFramegenModeClamp() {
    assert(clampFramegenMode(-1) == FRAMEGEN_MODE_OFF);
    assert(clampFramegenMode(0) == FRAMEGEN_MODE_OFF);
    assert(clampFramegenMode(1) == FRAMEGEN_MODE_INTERP);
    assert(clampFramegenMode(99) == FRAMEGEN_MODE_INTERP);
}

void testFramegenModeStep() {
    assert(stepFramegenMode(FRAMEGEN_MODE_OFF, 1) == FRAMEGEN_MODE_INTERP);
    assert(stepFramegenMode(FRAMEGEN_MODE_INTERP, -1) == FRAMEGEN_MODE_OFF);
    assert(stepFramegenMode(FRAMEGEN_MODE_OFF, -1) == FRAMEGEN_MODE_OFF);
    assert(stepFramegenMode(FRAMEGEN_MODE_INTERP, 1) == FRAMEGEN_MODE_INTERP);
}

void testFramegenModeLabel() {
    assert(std::string(framegenModeLabel(FRAMEGEN_MODE_OFF)) == "OFF");
    assert(std::string(framegenModeLabel(FRAMEGEN_MODE_INTERP)) == "INTERPOLATION");
}

int main() {
    testFramegenModeClamp();
    testFramegenModeStep();
    testFramegenModeLabel();
    std::cout << "All frame generation tests passed.\n";
    return 0;
}
