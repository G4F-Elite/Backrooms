#pragma once

inline void settingsInput(GLFWwindow* w, bool fromPause) {
    static constexpr int SETTINGS_ITEMS = 13;
    static constexpr int SETTINGS_BINDS_INDEX = 11;
    static constexpr int SETTINGS_BACK_INDEX = 12;
    
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS;
    
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
    
    float* vals[] = {
        &settings.masterVol, &settings.musicVol, &settings.ambienceVol, 
        &settings.sfxVol, &settings.voiceVol, &settings.vhsIntensity, &settings.mouseSens
    };
    float maxV[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.006f};
    float minV[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0005f};
    float step[] = {0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.05f, 0.0003f};
    
    if (menuSel <= 6) {
        if (left && !leftPressed) { 
            *vals[menuSel] -= step[menuSel]; 
            if (*vals[menuSel] < minV[menuSel]) *vals[menuSel] = minV[menuSel]; 
            triggerMenuNavigateSound();
        }
        if (right && !rightPressed) { 
            *vals[menuSel] += step[menuSel]; 
            if (*vals[menuSel] > maxV[menuSel]) *vals[menuSel] = maxV[menuSel]; 
            triggerMenuNavigateSound();
        }
    } else if (menuSel == 7) {
        if (left && !leftPressed) {
            settings.upscalerMode = clampUpscalerMode(settings.upscalerMode - 1);
            triggerMenuNavigateSound();
        }
        if (right && !rightPressed) {
            settings.upscalerMode = clampUpscalerMode(settings.upscalerMode + 1);
            triggerMenuNavigateSound();
        }
    } else if (menuSel == 8) {
        if (left && !leftPressed) {
            settings.renderScalePreset = stepRenderScalePreset(settings.renderScalePreset, -1);
            triggerMenuNavigateSound();
        }
        if (right && !rightPressed) {
            settings.renderScalePreset = stepRenderScalePreset(settings.renderScalePreset, 1);
            triggerMenuNavigateSound();
        }
    } else if (menuSel == 9) {
        if (left && !leftPressed) {
            settings.fsrSharpness = clampFsrSharpness(settings.fsrSharpness - 0.05f);
            triggerMenuNavigateSound();
        }
        if (right && !rightPressed) {
            settings.fsrSharpness = clampFsrSharpness(settings.fsrSharpness + 0.05f);
            triggerMenuNavigateSound();
        }
    } else if (menuSel == 10) {
        if (left && !leftPressed) {
            settings.aaMode = stepAaMode(settings.aaMode, -1);
            triggerMenuNavigateSound();
        }
        if (right && !rightPressed) {
            settings.aaMode = stepAaMode(settings.aaMode, 1);
            triggerMenuNavigateSound();
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