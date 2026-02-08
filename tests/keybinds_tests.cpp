#include <cassert>
#include <iostream>
#include <string>

#include "../src/keybinds.h"

void testClampKeybindMenuIndexWraps() {
    assert(clampKeybindMenuIndex(-1) == KEYBINDS_BACK_INDEX);
    assert(clampKeybindMenuIndex(KEYBINDS_BACK_INDEX + 1) == 0);
    assert(clampKeybindMenuIndex(3) == 3);
}

void testGameplayBindLookup() {
    GameplayBinds binds = {};
    int* fwd = gameplayBindByIndex(binds, 0);
    int* pause = gameplayBindByIndex(binds, 8);
    int* invalid = gameplayBindByIndex(binds, 99);
    assert(fwd != nullptr);
    assert(pause != nullptr);
    assert(invalid == nullptr);
    *fwd = GLFW_KEY_UP;
    *pause = GLFW_KEY_P;
    assert(binds.forward == GLFW_KEY_UP);
    assert(binds.pause == GLFW_KEY_P);
}

void testKeyNameFormatting() {
    assert(std::string(keyNameForUi(GLFW_KEY_W)) == "W");
    assert(std::string(keyNameForUi(GLFW_KEY_1)) == "1");
    assert(std::string(keyNameForUi(GLFW_KEY_LEFT_SHIFT)) == "L-SHIFT");
}

int main() {
    testClampKeybindMenuIndexWraps();
    testGameplayBindLookup();
    testKeyNameFormatting();
    std::cout << "All keybind tests passed.\n";
    return 0;
}
