#include <cassert>
#include <cstring>
#include <iostream>

#include "../src/settings_model.h"

void testEveryTabHasRowsWithLabelsAndHints() {
    for (int tab = 0; tab < SETTINGS_TAB_COUNT; tab++) {
        assert(settingsTabName(tab)[0] != 0);
        int count = settingsRowCount(tab);
        assert(count > 0);
        for (int i = 0; i < count; i++) {
            SettingRow row = settingsRowAt(tab, i);
            assert(row.label[0] != 0);
            assert(row.hint[0] != 0);
        }
    }
    assert(settingsRowCount(SETTINGS_TAB_COUNT) == 0);
    assert(settingsRowAt(SETTINGS_TAB_VIDEO, 99).id == -1);
}

void testEverySettingIsOnExactlyOneRow() {
    int seen[SET_COUNT] = {};
    for (int tab = 0; tab < SETTINGS_TAB_COUNT; tab++) {
        for (int i = 0; i < settingsRowCount(tab); i++) {
            SettingRow row = settingsRowAt(tab, i);
            if (row.kind == SETTING_BIND || row.kind == SETTING_RESET_BINDS) continue;
            assert(row.id >= 0 && row.id < SET_COUNT);
            seen[row.id]++;
        }
    }
    for (int id = 0; id < SET_COUNT; id++) assert(seen[id] == 1);
}

void testControlsTabListsEveryBind() {
    int binds = 0;
    for (int i = 0; i < settingsRowCount(SETTINGS_TAB_CONTROLS); i++) {
        SettingRow row = settingsRowAt(SETTINGS_TAB_CONTROLS, i);
        if (row.kind != SETTING_BIND) continue;
        assert(row.id == binds);
        assert(std::strcmp(row.label, gameplayBindLabel(binds)) == 0);
        binds++;
    }
    assert(binds == GAMEPLAY_BIND_COUNT);
    SettingRow last = settingsRowAt(SETTINGS_TAB_CONTROLS, settingsRowCount(SETTINGS_TAB_CONTROLS) - 1);
    assert(last.kind == SETTING_RESET_BINDS);
}

void testSelectionWraps() {
    int count = settingsRowCount(SETTINGS_TAB_AUDIO);
    assert(clampSettingsSelection(SETTINGS_TAB_AUDIO, -1) == count - 1);
    assert(clampSettingsSelection(SETTINGS_TAB_AUDIO, count) == 0);
    assert(clampSettingsSelection(SETTINGS_TAB_AUDIO, 2) == 2);
}

void testSlidersClampAndStep() {
    Settings s;
    SettingRow master = settingsRowAt(SETTINGS_TAB_AUDIO, 0);
    assert(master.kind == SETTING_SLIDER && master.id == SET_VOL_MASTER);
    setSettingSlider01(s, SET_VOL_MASTER, 2.0f);
    assert(s.masterVol == 1.0f);
    assert(!adjustSetting(s, master, 1));
    assert(adjustSetting(s, master, -1));
    assert(s.masterVol > 0.94f && s.masterVol < 0.96f);
    setSettingSlider01(s, SET_VOL_MASTER, -1.0f);
    assert(s.masterVol == 0.0f);
    assert(!adjustSetting(s, master, -1));
}

void testMouseSensitivityKeepsItsRange() {
    Settings s;
    setSettingSlider01(s, SET_MOUSE_SENS, 0.0f);
    assert(s.mouseSens == MOUSE_SENS_MIN);
    setSettingSlider01(s, SET_MOUSE_SENS, 1.0f);
    assert(s.mouseSens == MOUSE_SENS_MAX);
    assert(settingSlider01(s, SET_MOUSE_SENS) == 1.0f);
    setSettingSlider01(s, SET_MOUSE_SENS, 0.5f);
    assert(s.mouseSens > 0.0029f && s.mouseSens < 0.0031f);
}

void testChoicesStepClampAndCycle() {
    Settings s;
    SettingRow aa = {SETTING_CHOICE, SET_AA, "", ""};
    s.aaMode = AA_MODE_OFF;
    assert(!settingCanStep(s, SET_AA, -1));
    assert(settingCanStep(s, SET_AA, 1));
    assert(!adjustSetting(s, aa, -1));
    assert(adjustSetting(s, aa, 1) && s.aaMode == AA_MODE_FXAA);
    s.aaMode = AA_MODE_COUNT - 1;
    assert(!adjustSetting(s, aa, 1));
    assert(cycleSetting(s, aa) && s.aaMode == AA_MODE_OFF);
}

void testTogglesFlip() {
    Settings s;
    SettingRow vsync = {SETTING_TOGGLE, SET_VSYNC, "", ""};
    bool before = s.vsync;
    assert(adjustSetting(s, vsync, 1) && s.vsync == !before);
    assert(cycleSetting(s, vsync) && s.vsync == before);
    assert(adjustSetting(s, vsync, -1) && s.vsync == !before);
}

void testRenderScaleLockedWithoutUpscaler() {
    Settings s;
    SettingRow scale = {SETTING_CHOICE, SET_RENDER_SCALE, "", ""};
    char text[24];
    s.upscalerMode = UPSCALER_MODE_OFF;
    int preset = s.renderScalePreset;
    assert(!settingEnabled(s, SET_RENDER_SCALE));
    assert(!settingEnabled(s, SET_SHARPNESS));
    assert(!adjustSetting(s, scale, -1) && !cycleSetting(s, scale));
    assert(s.renderScalePreset == preset);
    settingValueText(s, scale, text, sizeof(text));
    assert(std::strcmp(text, "NATIVE") == 0);

    s.upscalerMode = UPSCALER_MODE_FSR10;
    assert(settingEnabled(s, SET_RENDER_SCALE));
    assert(adjustSetting(s, scale, -1) && s.renderScalePreset == preset - 1);
    settingValueText(s, scale, text, sizeof(text));
    assert(std::strcmp(text, "67%") == 0);
}

void testValueText() {
    Settings s;
    char text[24];
    s.masterVol = 0.45f;
    settingValueText(s, {SETTING_SLIDER, SET_VOL_MASTER, "", ""}, text, sizeof(text));
    assert(std::strcmp(text, "45%") == 0);
    s.bloom = true;
    settingValueText(s, {SETTING_TOGGLE, SET_BLOOM, "", ""}, text, sizeof(text));
    assert(std::strcmp(text, "ON") == 0);
    s.ssaoQuality = 2;
    settingValueText(s, {SETTING_CHOICE, SET_SSAO, "", ""}, text, sizeof(text));
    assert(std::strcmp(text, "MEDIUM") == 0);
    settingValueText(s, {SETTING_BIND, 6, "", ""}, text, sizeof(text));
    assert(std::strcmp(text, "E") == 0);
}

void testRebindingSwapsOnConflict() {
    GameplayBinds binds;
    assert(bindIndexUsingKey(binds, GLFW_KEY_E, -1) == 6);
    assert(bindIndexUsingKey(binds, GLFW_KEY_E, 6) == -1);
    assignBind(binds, 7, GLFW_KEY_E); // flashlight takes E: interact gets F
    assert(binds.flashlight == GLFW_KEY_E);
    assert(binds.interact == GLFW_KEY_F);
    assignBind(binds, 0, GLFW_KEY_UP); // free key: nothing else moves
    assert(binds.forward == GLFW_KEY_UP);
    assert(binds.back == GLFW_KEY_S);
    for (int i = 0; i < GAMEPLAY_BIND_COUNT; i++) {
        assert(bindIndexUsingKey(binds, *gameplayBindByIndex(binds, i), i) == -1);
    }
}

int main() {
    testEveryTabHasRowsWithLabelsAndHints();
    testEverySettingIsOnExactlyOneRow();
    testControlsTabListsEveryBind();
    testSelectionWraps();
    testSlidersClampAndStep();
    testMouseSensitivityKeepsItsRange();
    testChoicesStepClampAndCycle();
    testTogglesFlip();
    testRenderScaleLockedWithoutUpscaler();
    testValueText();
    testRebindingSwapsOnConflict();
    std::cout << "All settings model tests passed.\n";
    return 0;
}
