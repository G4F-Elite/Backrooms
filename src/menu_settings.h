#pragma once
// Settings screen. Rows come from settings_model.h; this file only lays them out.
// Input: input_settings.h.

#include "ui_widgets.h"

// Typing an exact percentage into the selected slider.
inline bool sliderInputActive = false;
inline char sliderInputBuf[4] = "";

inline constexpr float SETTINGS_PANEL_W = 760.0f;
inline constexpr float SETTINGS_PANEL_TOP = 326.0f;
inline constexpr float SETTINGS_TABS_H = 40.0f;
inline constexpr float SETTINGS_ROW_H = 30.0f;

inline void drawSettingsRowControl(const SettingRow& row, int idx, float l, float r, float y, float glow, bool selected) {
    bool enabled = settingEnabled(settings, row.id);
    char value[24];
    settingValueText(settings, row, value, sizeof(value));

    if (row.kind == SETTING_SLIDER) {
        uiSlider(idx, l, r - 74.0f, y, settingSlider01(settings, row.id), glow, enabled);
        if (selected && sliderInputActive) {
            char typed[12];
            snprintf(typed, sizeof(typed), "%s%s%%", sliderInputBuf, fmodf(ui.time, 1.0f) < 0.55f ? "_" : " ");
            uiTextR(typed, r, y, UI_ROW_TEXT, UI_ACCENT);
        } else {
            uiTextR(value, r, y, UI_ROW_TEXT, enabled ? uiMix(UI_TEXT, UI_ACCENT, glow) : UI_MUTE);
        }
    } else if (row.kind == SETTING_TOGGLE) {
        uiSwitch(r, y, settingToggleValue(settings, row.id), glow, enabled);
    } else if (row.kind == SETTING_CHOICE) {
        uiStepper(idx, l + 90.0f, r, y, value, settingCanStep(settings, row.id, -1), settingCanStep(settings, row.id, 1), glow, enabled);
    } else if (row.kind == SETTING_BIND) {
        if (keybindCaptureIndex == row.id) {
            uiTextR("PRESS A KEY", r, y, UI_ROW_TEXT, UI_ACCENT, 0.55f + 0.45f * uiPulse(7.0f));
        } else {
            uiTextR(value, r, y, UI_ROW_TEXT, uiMix(UI_TEXT, UI_ACCENT, glow));
        }
    }
}

inline void drawSettingsHints(const SettingRow& row) {
    if (keybindCaptureIndex >= 0) {
        static const UiHint hints[] = {{"ESC", "CANCEL"}, {"ANY KEY", "ASSIGN"}};
        uiHintBar(hints, 2);
    } else if (sliderInputActive) {
        static const UiHint hints[] = {{"ESC", "CANCEL"}, {"0-9", "TYPE PERCENT"}, {"ENTER", "APPLY"}};
        uiHintBar(hints, 3);
    } else {
        // only what Enter does when it is not the obvious thing
        const char* enterDoes = nullptr;
        if (row.kind == SETTING_SLIDER) enterDoes = "TYPE VALUE";
        else if (row.kind == SETTING_BIND) enterDoes = "REBIND";
        else if (row.kind == SETTING_RESET_BINDS) enterDoes = "RESET";
        UiHint hints[3];
        int n = 0;
        hints[n++] = {"ESC", "BACK"};
        hints[n++] = {"TAB", "NEXT TAB"};
        if (enterDoes) hints[n++] = {"ENTER", enterDoes};
        uiHintBar(hints, n);
    }
}

inline void drawSettings(bool fromPause, float tm) {
    uiBeginScreen();
    float in = uiScreenIn(0.18f);
    if (fromPause) {
        drawFullscreenOverlay(0.01f, 0.01f, 0.015f, 0.62f);
        uiVignette(0.5f);
    } else {
        uiMenuBackdrop(tm, 0.42f);
    }
    ui.alpha = in;

    float l = -SETTINGS_PANEL_W * 0.5f, r = SETTINGS_PANEL_W * 0.5f;
    // The panel is as tall as the tab needs; switching tabs slides its bottom edge.
    static float shownRows = 0.0f;
    int count = settingsRowCount(settingsTab);
    shownRows = ui.screenAge < 0.05f ? (float)count : uiApproach(shownRows, (float)count, 18.0f);
    float rowsH = SETTINGS_ROW_H * (shownRows < 6.0f ? 6.0f : shownRows);
    float t = SETTINGS_PANEL_TOP - (1.0f - in) * 10.0f;
    float b = t - (UI_HEADER_H + SETTINGS_TABS_H + 12.0f + rowsH + 12.0f + 37.0f);
    uiPanel(l, b, r, t);
    float y = uiPanelHeader(l, r, t, "SETTINGS", fromPause ? "APPLIES TO THE CURRENT RUN" : nullptr);

    const char* tabs[SETTINGS_TAB_COUNT];
    for (int i = 0; i < SETTINGS_TAB_COUNT; i++) tabs[i] = settingsTabName(i);
    y -= SETTINGS_TABS_H * 0.5f;
    uiTabs(l + 28.0f, r - 28.0f, y, SETTINGS_TABS_H, tabs, SETTINGS_TAB_COUNT, settingsTab);
    y -= SETTINGS_TABS_H * 0.5f + 12.0f + SETTINGS_ROW_H * 0.5f;

    SettingRow current = settingsRowAt(settingsTab, menuSel);
    for (int i = 0; i < count; i++) {
        SettingRow row = settingsRowAt(settingsTab, i);
        float ry = y - SETTINGS_ROW_H * (float)i;
        bool selected = menuSel == i;
        float glow = uiRowBack(i, l + 14.0f, ry - SETTINGS_ROW_H * 0.5f + 1.0f, r - 14.0f, ry + SETTINGS_ROW_H * 0.5f - 1.0f, selected);
        bool enabled = settingEnabled(settings, row.id);
        uiText(row.label, l + 28.0f, ry, UI_ROW_TEXT, enabled ? uiMix(UI_TEXT, UI_ACCENT, glow) : UI_MUTE);
        drawSettingsRowControl(row, i, 40.0f, r - 28.0f, ry, glow, selected);
    }

    // what the selected row does
    uiText(current.hint, l + 28.0f, b + 18.0f, UI_SMALL_TEXT, UI_MUTE);

    ui.alpha = 1.0f;
    drawSettingsHints(current);
    uiEndScreen();
}
