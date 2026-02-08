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
    assert(sndState.uiMovePitch > 0.8f);
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
    assert(sndState.uiAdjustPitch > 0.8f);
}

void testMenuPitchVariesAcrossTriggers() {
    triggerMenuNavigateSound();
    float p1 = sndState.uiMovePitch;
    triggerMenuNavigateSound();
    float p2 = sndState.uiMovePitch;
    assert(p1 != p2);

    triggerMenuAdjustSound();
    float a1 = sndState.uiAdjustPitch;
    triggerMenuAdjustSound();
    float a2 = sndState.uiAdjustPitch;
    assert(a1 != a2);
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
    testMenuPitchVariesAcrossTriggers();
    testMenuConfirmTriggerSetsFlag();
    testFillAudioConsumesUiFlags();
    std::cout << "All menu audio feedback tests passed.\n";
    return 0;
}
