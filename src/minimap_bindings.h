#pragma once

struct MinimapBindingState {
    bool mPressed = false;
};

inline bool consumeTogglePress(bool nowPressed, bool& wasPressed) {
    const bool toggled = nowPressed && !wasPressed;
    wasPressed = nowPressed;
    return toggled;
}

inline bool shouldToggleMinimapFromBindings(
    bool mNow,
    MinimapBindingState& state
) {
    return consumeTogglePress(mNow, state.mPressed);
}
