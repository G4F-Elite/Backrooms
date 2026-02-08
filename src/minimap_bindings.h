#pragma once

struct MinimapBindingState {
    bool f6Pressed = false;
    bool f7Pressed = false;
    bool homePressed = false;
    bool mPressed = false;
};

inline bool consumeTogglePress(bool nowPressed, bool& wasPressed) {
    const bool toggled = nowPressed && !wasPressed;
    wasPressed = nowPressed;
    return toggled;
}

inline bool shouldToggleMinimapFromBindings(
    bool f6Now,
    bool f7Now,
    bool homeNow,
    bool mNow,
    MinimapBindingState& state
) {
    bool toggled = false;
    toggled = consumeTogglePress(f6Now, state.f6Pressed) || toggled;
    toggled = consumeTogglePress(f7Now, state.f7Pressed) || toggled;
    toggled = consumeTogglePress(homeNow, state.homePressed) || toggled;
    toggled = consumeTogglePress(mNow, state.mPressed) || toggled;
    return toggled;
}
