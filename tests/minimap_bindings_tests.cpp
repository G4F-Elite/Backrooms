#include <cassert>
#include <iostream>

#include "../src/minimap_bindings.h"

void testFirstPressToggles() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(true, state);
    assert(toggled);
}

void testHoldDoesNotRetoggle() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(true, state);
    assert(toggled);
    toggled = shouldToggleMinimapFromBindings(true, state);
    assert(!toggled);
}

void testReleaseAndPressTogglesAgain() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(false, state);
    assert(!toggled);
    toggled = shouldToggleMinimapFromBindings(true, state);
    assert(toggled);
    toggled = shouldToggleMinimapFromBindings(false, state);
    assert(!toggled);
    toggled = shouldToggleMinimapFromBindings(true, state);
    assert(toggled);
}

void testOnlyMCanToggle() {
    MinimapBindingState state = {};
    bool toggled = shouldToggleMinimapFromBindings(false, state);
    assert(!toggled);

    toggled = shouldToggleMinimapFromBindings(true, state);
    assert(toggled);

    toggled = shouldToggleMinimapFromBindings(false, state);
    assert(!toggled);
    toggled = shouldToggleMinimapFromBindings(true, state);
    assert(toggled);
}

int main() {
    testFirstPressToggles();
    testHoldDoesNotRetoggle();
    testReleaseAndPressTogglesAgain();
    testOnlyMCanToggle();
    std::cout << "All minimap binding tests passed.\n";
    return 0;
}
