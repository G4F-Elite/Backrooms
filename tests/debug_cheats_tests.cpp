#include <cassert>
#include <iostream>

#include "../src/cheats.h"

void testNoclipCheatToggle() {
    resetDebugState();
    const char* seq = "NOCLIP";
    for (int i = 0; seq[i]; i++) {
        pushDebugCheatChar(seq[i]);
    }
    assert(gDebug.noclip == true);

    // Toggle off
    for (int i = 0; seq[i]; i++) {
        pushDebugCheatChar(seq[i]);
    }
    assert(gDebug.noclip == false);
}

void testGodModeCheatToggle() {
    resetDebugState();
    const char* seq = "GODMODE";
    for (int i = 0; seq[i]; i++) {
        pushDebugCheatChar(seq[i]);
    }
    assert(gDebug.godMode == true);
}

void testFlyModeCheatToggle() {
    resetDebugState();
    const char* seq = "FLYMODE";
    for (int i = 0; seq[i]; i++) {
        pushDebugCheatChar(seq[i]);
    }
    assert(gDebug.flyMode == true);
}

void testSpawnCheatToggle() {
    resetDebugState();
    const char* seq = "SPAWNER";
    for (int i = 0; seq[i]; i++) {
        pushDebugCheatChar(seq[i]);
    }
    assert(gDebug.spawnMode == true);
}

void testResetDebugState() {
    gDebug.noclip = true;
    gDebug.godMode = true;
    gDebug.flyMode = true;
    gDebug.spawnMode = true;
    resetDebugState();
    assert(gDebug.noclip == false);
    assert(gDebug.godMode == false);
    assert(gDebug.flyMode == false);
    assert(gDebug.spawnMode == false);
}

void testWrongCodeDoesNotToggle() {
    resetDebugState();
    const char* seq = "XYZABC";
    for (int i = 0; seq[i]; i++) {
        pushDebugCheatChar(seq[i]);
    }
    assert(gDebug.noclip == false);
    assert(gDebug.godMode == false);
    assert(gDebug.flyMode == false);
    assert(gDebug.spawnMode == false);
}

int main() {
    testNoclipCheatToggle();
    testGodModeCheatToggle();
    testFlyModeCheatToggle();
    testSpawnCheatToggle();
    testResetDebugState();
    testWrongCodeDoesNotToggle();
    std::cout << "All debug cheats tests passed.\n";
    return 0;
}
