#pragma once

inline void settingsInput(GLFWwindow* w, bool fromPause) {
    static constexpr int SETTINGS_ITEMS = 13;
    static constexpr int SETTINGS_AA_INDEX = 10;
    static constexpr int SETTINGS_BINDS_INDEX = 11;
    static constexpr int SETTINGS_BACK_INDEX = 12;
    
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
    
    const double now = glfwGetTime();
    static int adjustHoldDir = 0;
    static double nextAdjustTime = 0.0;
    const double adjustFirstDelay = 0.28;
    const double adjustRepeatInterval = 0.055;

    auto applyAdjust = [&](int dir) {
        if (dir == 0) return;
        if (menuSel <= 6) {
            float* vals[] = {
                &settings.masterVol, &settings.musicVol, &settings.ambienceVol, &settings.sfxVol,
                &settings.voiceVol, &settings.vhsIntensity, &settings.mouseSens
            };
            float maxV[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.006f};
            float minV[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0005f};
            float step[] = {0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.0003f};
            *vals[menuSel] += step[menuSel] * (float)dir;
            if (*vals[menuSel] < minV[menuSel]) *vals[menuSel] = minV[menuSel];
            if (*vals[menuSel] > maxV[menuSel]) *vals[menuSel] = maxV[menuSel];
            triggerMenuNavigateSound();
        } else if (menuSel == 7) {
            settings.upscalerMode = clampUpscalerMode(settings.upscalerMode + dir);
            triggerMenuNavigateSound();
        } else if (menuSel == 8) {
            settings.renderScalePreset = stepRenderScalePreset(settings.renderScalePreset, dir);
            triggerMenuNavigateSound();
        } else if (menuSel == 9) {
            settings.fsrSharpness = clampFsrSharpness(settings.fsrSharpness + 0.05f * (float)dir);
            triggerMenuNavigateSound();
        } else if (menuSel == SETTINGS_AA_INDEX) {
            settings.aaMode = stepAaMode(settings.aaMode, dir);
            triggerMenuNavigateSound();
        }
    };
    
    if (up && !upPressed) {
        menuSel--;
        if (menuSel < 0) menuSel = SETTINGS_ITEMS - 1;
        triggerMenuNavigateSound();
    }
    if (down && !downPressed) {
        menuSel++;
        if (menuSel >= SETTINGS_ITEMS) menuSel = 0;
        triggerMenuNavigateSound();
    }
    
    int adjustDir = right ? 1 : (left ? -1 : 0);
    if (menuSel <= SETTINGS_AA_INDEX) {
        if (adjustDir == 0) {
            adjustHoldDir = 0;
            nextAdjustTime = 0.0;
        } else if (adjustHoldDir != adjustDir) {
            adjustHoldDir = adjustDir;
            applyAdjust(adjustDir);
            nextAdjustTime = now + adjustFirstDelay;
        } else if (now >= nextAdjustTime) {
            applyAdjust(adjustDir);
            nextAdjustTime = now + adjustRepeatInterval;
        }
    } else if (menuSel == SETTINGS_BINDS_INDEX && enter && !enterPressed) {
        triggerMenuConfirmSound();
        gameState = fromPause ? STATE_KEYBINDS_PAUSE : STATE_KEYBINDS;
        menuSel = 0;
        keybindCaptureIndex = -1;
    }
    
    if ((enter && !enterPressed && menuSel == SETTINGS_BACK_INDEX) || (esc && !escPressed)) { 
        triggerMenuConfirmSound();
        gameState = fromPause ? STATE_PAUSE : STATE_MENU; 
        menuSel = fromPause ? 1 : 2;
    }
    
    escPressed = esc;
    upPressed = up;
    downPressed = down;
    leftPressed = left;
    rightPressed = right;
    enterPressed = enter;
}

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
        if (up && !upPressed) {
            menuSel = clampKeybindMenuIndex(menuSel - 1);
            triggerMenuNavigateSound();
        }
        if (down && !downPressed) {
            menuSel = clampKeybindMenuIndex(menuSel + 1);
            triggerMenuNavigateSound();
        }
        if (enter && !enterPressed) {
            if (isGameplayBindIndex(menuSel)) {
                keybindCaptureIndex = menuSel;
                waitRelease = true;
                triggerMenuConfirmSound();
            } else if (menuSel == KEYBINDS_BACK_INDEX) {
                triggerMenuConfirmSound();
                gameState = fromPause ? STATE_SETTINGS_PAUSE : STATE_SETTINGS;
                menuSel = 11;
            }
        }
        if (esc && !escPressed) {
            triggerMenuConfirmSound();
            gameState = fromPause ? STATE_SETTINGS_PAUSE : STATE_SETTINGS;
            menuSel = 11;
        }
    }

    escPressed = esc;
    upPressed = up;
    downPressed = down;
    enterPressed = enter;
}