#pragma once

struct MinimapBindingState {
    bool mPressed = false;
    bool f8Pressed = false;
};

inline bool consumeTogglePress(bool nowPressed, bool& wasPressed) {
    const bool toggled = nowPressed && !wasPressed;
    wasPressed = nowPressed;
    return toggled;
}

inline bool shouldToggleMinimapFromBindings(
    bool mNow,
    bool f8Now,
    MinimapBindingState& state
) {
    bool toggled = false;
    if (consumeTogglePress(mNow, state.mPressed)) toggled = true;
    if (consumeTogglePress(f8Now, state.f8Pressed)) toggled = true;
    return toggled;
}
