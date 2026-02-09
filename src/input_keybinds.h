#pragma once
inline void keybindsInput(GLFWwindow* w, bool fromPause) {
    static bool waitRelease = false;
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;

    if (keybindCaptureIndex >= 0) {
        if (waitRelease) {
            if (!isAnyKeyboardKeyDown(w)) waitRelease = false;
        } else {
            int pressed = firstPressedKeyboardKey(w);
            if (pressed >= 0) {
                int* keyRef = gameplayBindByIndex(settings.binds, keybindCaptureIndex);
                if (keyRef) *keyRef = pressed;
                keybindCaptureIndex = -1;
                triggerMenuConfirmSound();
            } else if (esc && !escPressed) {
                keybindCaptureIndex = -1;
                triggerMenuConfirmSound();
            }
        }
    } else {
        if (up && !upPressed) { menuSel = clampKeybindMenuIndex(menuSel - 1); triggerMenuNavigateSound(); }
        if (down && !downPressed) { menuSel = clampKeybindMenuIndex(menuSel + 1); triggerMenuNavigateSound(); }
        if (enter && !enterPressed) {
            if (isGameplayBindIndex(menuSel)) {
                keybindCaptureIndex = menuSel;
                waitRelease = true;
                triggerMenuConfirmSound();
            } else if (menuSel == KEYBINDS_BACK_INDEX) {
                triggerMenuConfirmSound();
                gameState = fromPause ? STATE_SETTINGS_PAUSE : STATE_SETTINGS;
                settingsTab = SETTINGS_TAB_VIDEO;
                menuSel = 10;
            }
        }
        if (esc && !escPressed) {
            triggerMenuConfirmSound();
            gameState = fromPause ? STATE_SETTINGS_PAUSE : STATE_SETTINGS;
            settingsTab = SETTINGS_TAB_VIDEO;
            menuSel = 10;
        }
    }

    escPressed = esc;
    upPressed = up;
    downPressed = down;
    enterPressed = enter;
}

