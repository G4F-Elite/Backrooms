#include <cassert>
#include <atomic>
#include <iostream>

#include "../src/audio.h"

SoundState sndState = {};
std::atomic<bool> audioRunning{false};
HANDLE hEvent = nullptr;

void testMenuNavigateTriggerSetsFlag() {
    sndState.uiMoveTrig = false;
    triggerMenuNavigateSound();
    assert(sndState.uiMoveTrig);
}

void testMenuConfirmTriggerSetsFlag() {
    sndState.uiConfirmTrig = false;
    triggerMenuConfirmSound();
    assert(sndState.uiConfirmTrig);
}

void testMenuAdjustTriggerSetsFlag() {
    sndState.uiAdjustTrig = false;
    triggerMenuAdjustSound();
    assert(sndState.uiAdjustTrig);
}

void testFillAudioConsumesUiFlags() {
    short buf[64] = {};
    sndState.uiMoveTrig = true;
    sndState.uiAdjustTrig = true;
    sndState.uiConfirmTrig = true;
    fillAudio(buf, 64);
    assert(!sndState.uiMoveTrig);
    assert(!sndState.uiAdjustTrig);
    assert(!sndState.uiConfirmTrig);
}

int main() {
    testMenuNavigateTriggerSetsFlag();
    testMenuAdjustTriggerSetsFlag();
    testMenuConfirmTriggerSetsFlag();
    testFillAudioConsumesUiFlags();
    std::cout << "All menu audio feedback tests passed.\n";
    return 0;
}
