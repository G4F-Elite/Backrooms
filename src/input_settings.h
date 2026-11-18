#pragma once
// Settings screen input: tabs, rows, slider dragging, typed percentages, key capture.
// Rows and their behaviour come from settings_model.h; the layout is menu_settings.h.

inline void leaveSettings(bool fromPause) {
    triggerMenuConfirmSound();
    sliderInputActive = false;
    keybindCaptureIndex = -1;
    if (fromPause) {
        bool multiplayer = multiState == MULTI_IN_GAME;
        gameState = STATE_PAUSE;
        menuSel = pauseRowOf(multiplayer, PAUSE_SETTINGS);
    } else {
        gameState = STATE_MENU;
        menuSel = MAIN_MENU_SETTINGS;
    }
}

inline void switchSettingsTab(int tab) {
    tab = (tab % SETTINGS_TAB_COUNT + SETTINGS_TAB_COUNT) % SETTINGS_TAB_COUNT;
    if (tab == settingsTab) return;
    settingsTab = tab;
    menuSel = 0;
    triggerMenuNavigateSound();
}

// Waiting for the key that replaces a bind. ESC cancels, except on the pause bind itself,
// where ESC is the usual answer. A click anywhere cancels too.
inline void settingsCaptureInput(GLFWwindow* w, bool& waitRelease) {
    if (waitRelease) {
        if (!isAnyKeyboardKeyDown(w)) waitRelease = false;
        return;
    }
    int* pauseBind = &settings.binds.pause;
    bool escAssignable = gameplayBindByIndex(settings.binds, keybindCaptureIndex) == pauseBind;
    if (ui.mousePressed || (ui.esc && !escAssignable)) {
        keybindCaptureIndex = -1;
        triggerMenuNavigateSound();
        return;
    }
    int pressed = firstPressedKeyboardKey(w);
    if (pressed < 0) return;
    assignBind(settings.binds, keybindCaptureIndex, pressed);
    keybindCaptureIndex = -1;
    triggerMenuConfirmSound();
}

// Typing a percentage into the selected slider.
inline void settingsTypeInput(const SettingRow& row) {
    for (int i = 0; i < ui.charCount; i++) {
        char c = ui.chars[i];
        int len = (int)strlen(sliderInputBuf);
        if (c == UI_CHAR_BACKSPACE && len > 0) sliderInputBuf[len - 1] = '\0';
        else if (c >= '0' && c <= '9' && len < 3) {
            sliderInputBuf[len] = c;
            sliderInputBuf[len + 1] = '\0';
        }
    }
    ui.charCount = 0;
    if (ui.enter) {
        if (sliderInputBuf[0]) {
            int percent = atoi(sliderInputBuf);
            setSettingSlider01(settings, row.id, (float)(percent > 100 ? 100 : percent) / 100.0f);
        }
        sliderInputActive = false;
        triggerMenuConfirmSound();
    } else if (ui.esc || ui.mousePressed) {
        sliderInputActive = false;
    }
}

// Mouse on the controls inside a row: slider track, "<" and ">". They act on press, and a
// press they used must not turn into a click on the row when the button comes up.
inline void settingsMouseControls() {
    if (ui.mousePressed) {
        const UiZone* slider = uiHotZone(UI_ZONE_SLIDER);
        const UiZone* prev = uiHotZone(UI_ZONE_PREV);
        const UiZone* next = uiHotZone(UI_ZONE_NEXT);
        if (slider) {
            menuSel = slider->id;
            ui.dragItem = slider->id;
            ui.dragL = slider->l + 6.0f;
            ui.dragR = slider->r - 6.0f;
            ui.pressItem = -1;
        } else if (prev || next) {
            menuSel = prev ? prev->id : next->id;
            if (adjustSetting(settings, settingsRowAt(settingsTab, menuSel), prev ? -1 : 1)) triggerMenuAdjustSound();
            ui.pressItem = -1;
        }
    }
    if (ui.dragItem < 0 || !ui.mouseDown) return;
    SettingRow row = settingsRowAt(settingsTab, ui.dragItem);
    if (row.kind != SETTING_SLIDER || ui.dragR <= ui.dragL) return;
    float before = settingSlider01(settings, row.id);
    float v = uiClamp01((ui.mx - ui.dragL) / (ui.dragR - ui.dragL));
    setSettingSlider01(settings, row.id, floorf(v * 100.0f + 0.5f) / 100.0f);
    float after = settingSlider01(settings, row.id);
    if ((int)(before * 20.0f) != (int)(after * 20.0f)) triggerMenuAdjustSound();
}

inline void settingsActivateRow(const SettingRow& row, bool byKeyboard, bool& waitRelease) {
    if (row.kind == SETTING_SLIDER) {
        if (!byKeyboard || !settingEnabled(settings, row.id)) return;
        sliderInputActive = true;
        sliderInputBuf[0] = '\0';
        triggerMenuConfirmSound();
    } else if (row.kind == SETTING_BIND) {
        keybindCaptureIndex = row.id;
        waitRelease = true;
        triggerMenuConfirmSound();
    } else if (row.kind == SETTING_RESET_BINDS) {
        settings.binds = GameplayBinds();
        triggerMenuConfirmSound();
    } else if (cycleSetting(settings, row)) {
        triggerMenuAdjustSound();
    }
}

inline void settingsInput(GLFWwindow* w, bool fromPause) {
    static bool waitRelease = false;
    uiPollInput(w, sliderInputActive);
    if (keybindCaptureIndex >= 0) {
        settingsCaptureInput(w, waitRelease);
        return;
    }
    if (sliderInputActive) {
        settingsTypeInput(settingsRowAt(settingsTab, menuSel));
        return;
    }

    const UiZone* tabZone = ui.mousePressed ? uiHotZone(UI_ZONE_TAB) : nullptr;
    if (tabZone) switchSettingsTab(tabZone->id);
    else if (ui.tab) switchSettingsTab(settingsTab + (ui.shift ? -1 : 1));

    settingsMouseControls();
    bool activate = uiMenuNav(settingsRowCount(settingsTab));
    menuSel = clampSettingsSelection(settingsTab, menuSel);
    SettingRow row = settingsRowAt(settingsTab, menuSel);

    if (ui.adjust != 0 && adjustSetting(settings, row, ui.adjust)) triggerMenuAdjustSound();
    if (activate) settingsActivateRow(row, ui.enter, waitRelease);
    if (uiBackRequested()) leaveSettings(fromPause);
}
