#include <cassert>
#include <iostream>

#include "../src/minimap_bindings.h"

void testFirstPressToggles() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(true, false, false, false, state);
    assert(toggled);
}

void testHoldDoesNotRetoggle() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(true, false, false, false, state);
    assert(toggled);
    toggled = shouldToggleMinimapFromBindings(true, false, false, false, state);
    assert(!toggled);
}

void testReleaseAndPressTogglesAgain() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(false, false, false, false, state);
    assert(!toggled);
    toggled = shouldToggleMinimapFromBindings(true, false, false, false, state);
    assert(toggled);
    toggled = shouldToggleMinimapFromBindings(false, false, false, false, state);
    assert(!toggled);
    toggled = shouldToggleMinimapFromBindings(true, false, false, false, state);
    assert(toggled);
}

void testEachBindingCanToggle() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(false, false, false, true, state);
    assert(toggled);

    toggled = shouldToggleMinimapFromBindings(false, false, false, false, state);
    assert(!toggled);
    toggled = shouldToggleMinimapFromBindings(false, true, false, false, state);
    assert(toggled);

    toggled = shouldToggleMinimapFromBindings(false, false, false, false, state);
    assert(!toggled);
    toggled = shouldToggleMinimapFromBindings(false, false, true, false, state);
    assert(toggled);
}

int main() {
    testFirstPressToggles();
    testHoldDoesNotRetoggle();
    testReleaseAndPressTogglesAgain();
    testEachBindingCanToggle();
    std::cout << "All minimap binding tests passed.\n";
    return 0;
}
