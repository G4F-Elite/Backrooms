#pragma once

// Check if current settings item is a slider that supports numeric input
inline bool isSliderItem(int tab, int sel) {
    if(sel <= 0) return false;
    if(tab == SETTINGS_TAB_AUDIO) {
        int ai = sel - 1;
        return ai >= 0 && ai < 5; // all 5 volume sliders
    }
    if(tab == SETTINGS_TAB_VIDEO) {
        int vi = sel - 1;
        return vi == 0 || vi == 1 || vi == 4; // VHS, mouse sens, FSR sharp
    }
    if(tab == SETTINGS_TAB_EFFECTS) {
        int vi = sel - 1;
        return vi == 5; // denoiser strength
    }
    return false;
}

// Apply typed numeric value to the correct slider
inline void applySliderInputValue() {
    sliderInputBuf[sliderInputLen] = 0;
    int val = 0;
    for(int i = 0; i < sliderInputLen; i++) {
        val = val * 10 + (sliderInputBuf[i] - '0');
    }
    if(val < 0) val = 0;
    if(val > 100) val = 100;
    float nv = (float)val / 100.0f;
    if(sliderInputTab == SETTINGS_TAB_AUDIO) {
        float* vols[] = {&settings.masterVol, &settings.musicVol, &settings.ambienceVol, &settings.sfxVol, &settings.voiceVol};
        int ai = sliderInputItem - 1;
        if(ai >= 0 && ai < 5) *vols[ai] = nv;
    } else if(sliderInputTab == SETTINGS_TAB_VIDEO) {
        int vi = sliderInputItem - 1;
        if(vi == 0) settings.vhsIntensity = nv;
        else if(vi == 1) settings.mouseSens = nv * 0.006f;
        else if(vi == 4) settings.fsrSharpness = nv;
    } else if(sliderInputTab == SETTINGS_TAB_EFFECTS) {
        int vi = sliderInputItem - 1;
        if(vi == 5) settings.rtxDenoiseStrength = nv;
    }
    sliderInputActive = false;
    sliderInputLen = 0;
}

inline void settingsInput(GLFWwindow* w, bool fromPause) {
    bool esc = glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    bool up = glfwGetKey(w, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    bool down = glfwGetKey(w, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
    bool left = glfwGetKey(w, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
    bool right = glfwGetKey(w, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
    bool enter = glfwGetKey(w, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
    const double now = glfwGetTime();
    static int adjustHoldDir = 0;
    static double nextAdjustTime = 0.0;
    const double adjustFirstDelay = 0.28;
    const double adjustRepeatInterval = 0.055;

    // Handle numeric input mode for sliders
    if(sliderInputActive) {
        // Check digit keys 0-9
        for(int k = GLFW_KEY_0; k <= GLFW_KEY_9; k++) {
            static bool numPressed[10] = {};
            int digit = k - GLFW_KEY_0;
            bool pressed = glfwGetKey(w, k) == GLFW_PRESS;
            if(pressed && !numPressed[digit] && sliderInputLen < 3) {
                sliderInputBuf[sliderInputLen++] = '0' + digit;
            }
            numPressed[digit] = pressed;
        }
        // Numpad digits
        for(int k = GLFW_KEY_KP_0; k <= GLFW_KEY_KP_9; k++) {
            static bool kpPressed[10] = {};
            int digit = k - GLFW_KEY_KP_0;
            bool pressed = glfwGetKey(w, k) == GLFW_PRESS;
            if(pressed && !kpPressed[digit] && sliderInputLen < 3) {
                sliderInputBuf[sliderInputLen++] = '0' + digit;
            }
            kpPressed[digit] = pressed;
        }
        // Backspace
        bool bksp = glfwGetKey(w, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
        static bool bkspPressed = false;
        if(bksp && !bkspPressed && sliderInputLen > 0) sliderInputLen--;
        bkspPressed = bksp;
        // Enter confirms
        if(enter && !enterPressed) {
            if(sliderInputLen > 0) applySliderInputValue();
            else sliderInputActive = false;
            triggerMenuConfirmSound();
        }
        // Esc cancels
        if(esc && !escPressed) {
            sliderInputActive = false;
            sliderInputLen = 0;
        }
        escPressed = esc; enterPressed = enter;
        upPressed = up; downPressed = down;
        leftPressed = left; rightPressed = right;
        return;
    }

    auto switchTab = [&](int dir) {
        settingsTab = (settingsTab + (dir > 0 ? 1 : 2)) % 3;
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
        if(vi==0){ settings.vhsIntensity+=0.05f*(float)dir; if(settings.vhsIntensity<0)settings.vhsIntensity=0; if(settings.vhsIntensity>1)settings.vhsIntensity=1; return true; }
        if(vi==1){ settings.mouseSens+=0.0003f*(float)dir; if(settings.mouseSens<0.0005f)settings.mouseSens=0.0005f; if(settings.mouseSens>0.006f)settings.mouseSens=0.006f; return true; }
        if(vi==2){ settings.upscalerMode=clampUpscalerMode(settings.upscalerMode+dir); return true; }
        if(vi==3){ settings.renderScalePreset=stepRenderScalePreset(settings.renderScalePreset,dir); return true; }
        if(vi==4){ settings.fsrSharpness=clampFsrSharpness(settings.fsrSharpness+0.05f*(float)dir); return true; }
        if(vi==5){ settings.aaMode=stepAaMode(settings.aaMode,dir); return true; }
        if(vi==6){ settings.fastMath=!settings.fastMath; return true; }
        if(vi==7){ settings.frameGenMode=stepFrameGenMode(settings.frameGenMode,dir); return true; }
        if(vi==8){ settings.vsync=!settings.vsync; return true; }
        if(vi==9){ settings.debugMode=!settings.debugMode; return true; }
        return false;
    };
    auto effectsAdjust = [&](int idx, int dir) {
        int vi = idx - 1;
        if(vi==0){ settings.ssaoQuality=stepSsao(settings.ssaoQuality,dir); return true; }
        if(vi==1){ settings.giQuality=stepGi(settings.giQuality,dir); return true; }
        if(vi==2){ settings.godRays=!settings.godRays; return true; }
        if(vi==3){ settings.bloom=!settings.bloom; return true; }
        if(vi==4){ settings.rtxDenoise=!settings.rtxDenoise; return true; }
        if(vi==5){ settings.rtxDenoiseStrength += 0.05f*(float)dir; if(settings.rtxDenoiseStrength<0.0f)settings.rtxDenoiseStrength=0.0f; if(settings.rtxDenoiseStrength>1.0f)settings.rtxDenoiseStrength=1.0f; return true; }
        return false;
    };

    auto applyAdjust = [&](int dir) {
        if (dir == 0) return;
        bool changed = false;
        if (menuSel == 0) { switchTab(dir); changed = true; }
        else if (settingsTab == SETTINGS_TAB_AUDIO) changed = audioAdjust(menuSel, dir);
        else if (settingsTab == SETTINGS_TAB_EFFECTS) changed = effectsAdjust(menuSel, dir);
        else changed = videoAdjust(menuSel, dir);
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
            // Check if this is a slider - open numeric input
            if (isSliderItem(settingsTab, menuSel)) {
                sliderInputActive = true;
                sliderInputTab = settingsTab;
                sliderInputItem = menuSel;
                sliderInputLen = 0;
                triggerMenuConfirmSound();
            } else if (menuSel == 0 || settingsTab == SETTINGS_TAB_VIDEO) {
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

