#pragma once
inline void settingsInput(GLFWwindow* w, bool fromPause) {
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
    auto switchTab = [&]() {
        settingsTab = (settingsTab == SETTINGS_TAB_AUDIO) ? SETTINGS_TAB_VIDEO : SETTINGS_TAB_AUDIO;
        menuSel = clampSettingsSelection(settingsTab, menuSel);
    };

    auto audioAdjust = [&](int idx, int dir) {
        float* vals[] = {
            &settings.masterVol,
            &settings.musicVol,
            &settings.ambienceVol,
            &settings.sfxVol,
            &settings.voiceVol
        };
        float maxV[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
        float minV[] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float step[] = {0.05f, 0.05f, 0.05f, 0.05f, 0.05f};
        int ai = idx - 1;
        if (ai < 0 || ai >= 5) return false;
        *vals[ai] += step[ai] * (float)dir;
        if (*vals[ai] < minV[ai]) *vals[ai] = minV[ai];
        if (*vals[ai] > maxV[ai]) *vals[ai] = maxV[ai];
        return true;
    };

    auto videoAdjust = [&](int idx, int dir) {
        int vi = idx - 1;
        if (vi == 0) {
            settings.vhsIntensity += 0.05f * (float)dir;
            if (settings.vhsIntensity < 0.0f) settings.vhsIntensity = 0.0f;
            if (settings.vhsIntensity > 1.0f) settings.vhsIntensity = 1.0f;
            return true;
        }
        if (vi == 1) {
            settings.mouseSens += 0.0003f * (float)dir;
            if (settings.mouseSens < 0.0005f) settings.mouseSens = 0.0005f;
            if (settings.mouseSens > 0.006f) settings.mouseSens = 0.006f;
            return true;
        }
        if (vi == 2) {
            settings.upscalerMode = clampUpscalerMode(settings.upscalerMode + dir);
            return true;
        }
        if (vi == 3) {
            settings.renderScalePreset = stepRenderScalePreset(settings.renderScalePreset, dir);
            return true;
        }
        if (vi == 4) {
            settings.fsrSharpness = clampFsrSharpness(settings.fsrSharpness + 0.05f * (float)dir);
            return true;
        }
        if (vi == 5) {
            settings.aaMode = stepAaMode(settings.aaMode, dir);
            return true;
        }
        if (vi == 6) {
            settings.rtxEnabled = !settings.rtxEnabled;
            return true;
        }
        if (vi == 7) {
            settings.fastMath = !settings.fastMath;
            return true;
        }
        if (vi == 8) {
            settings.frameGenMode = stepFrameGenMode(settings.frameGenMode, dir);
            return true;
        }
        if (vi == 9) {
            settings.vsync = !settings.vsync;
            return true;
        }
        return false;
    };

    auto applyAdjust = [&](int dir) {
        if (dir == 0) return;
        bool changed = false;
        if (menuSel == 0) {
            switchTab();
            changed = true;
        } else if (settingsTab == SETTINGS_TAB_AUDIO) {
            changed = audioAdjust(menuSel, dir);
        } else {
            changed = videoAdjust(menuSel, dir);
        }
        if (changed) triggerMenuAdjustSound();
    };
    
    if (up && !upPressed) {
        menuSel = clampSettingsSelection(settingsTab, menuSel - 1);
        triggerMenuNavigateSound();
    }
    if (down && !downPressed) {
        menuSel = clampSettingsSelection(settingsTab, menuSel + 1);
        triggerMenuNavigateSound();
    }
    
    int adjustDir = right ? 1 : (left ? -1 : 0);
    int bindsIndex = settingsBindsIndexForTab(settingsTab);
    int backIndex = settingsBackIndexForTab(settingsTab);
    bool canAdjust = menuSel == 0 || (menuSel != bindsIndex && menuSel != backIndex);
    if (canAdjust) {
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
    } else if (menuSel == bindsIndex) {
        if (enter && !enterPressed) {
            triggerMenuConfirmSound();
            settingsTab = SETTINGS_TAB_VIDEO;
            gameState = fromPause ? STATE_KEYBINDS_PAUSE : STATE_KEYBINDS;
            menuSel = 0;
            keybindCaptureIndex = -1;
        }
    }

    if (enter && !enterPressed) {
        if (menuSel == backIndex) {
            triggerMenuConfirmSound();
            gameState = fromPause ? STATE_PAUSE : STATE_MENU;
            menuSel = fromPause ? 1 : 2;
        } else if (menuSel != bindsIndex) {
            if (menuSel == 0 || settingsTab == SETTINGS_TAB_VIDEO) {
                applyAdjust(1);
            }
        }
    }
    
    if (esc && !escPressed) { 
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

