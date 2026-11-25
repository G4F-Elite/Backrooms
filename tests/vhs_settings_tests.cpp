#include <cassert>
#include <cstring>
#include "../src/settings_model.h"

static void testTapeRowsFollowOutputSize() {
    assert(vhsTapeRows(VHS_RES_AUTO, 1080) == 400);
    assert(vhsTapeRows(VHS_RES_AUTO, 360) == 360);
    assert(vhsTapeRows(VHS_RES_240, 1080) == 240);
    assert(vhsTapeRows(VHS_RES_480, 1080) == 480);
    assert(vhsTapeRows(VHS_RES_720, 1080) == 720);
    assert(vhsTapeRows(VHS_RES_NATIVE, 2160) == 2160);
    assert(vhsTapeRows(-1, 1080) == 400);
    assert(vhsTapeRows(VHS_RES_COUNT, 1080) == 1080);
    for (int i = 0; i < VHS_RES_COUNT; ++i) {
        assert(vhsTapeRows(i, 0) == 0);
        assert(vhsTapeRows(i, -1) == 0);
        assert(vhsTapeRows(i, 137) == 137);
    }
}

static void testVideoMenuAndEffectToggle() {
    Settings s;
    int idx = 0;
    while (idx < settingsRowCount(SETTINGS_TAB_VIDEO) && settingsRowAt(SETTINGS_TAB_VIDEO, idx).id != SET_VHS_RESOLUTION)
        ++idx;
    assert(idx < settingsRowCount(SETTINGS_TAB_VIDEO));
    const auto row = settingsRowAt(SETTINGS_TAB_VIDEO, idx);
    assert(row.kind == SETTING_CHOICE);
    assert(s.vhsResolution == VHS_RES_AUTO);
    const char* labels[] = {"AUTO", "240 LINES", "480 LINES", "720 LINES", "NATIVE"};
    char value[24];
    for (const char* label : labels) {
        settingValueText(s, row, value, sizeof(value));
        assert(std::strcmp(value, label) == 0);
        assert(cycleSetting(s, row));
    }
    assert(s.vhsResolution == VHS_RES_AUTO);
    assert(adjustSetting(s, row, -1) && s.vhsResolution == VHS_RES_NATIVE);
    s.vhsIntensity = 0.0f;
    assert(!settingEnabled(s, SET_VHS_RESOLUTION) && !cycleSetting(s, row));
    assert(s.vhsResolution == VHS_RES_NATIVE);
    s.vhsIntensity = 0.5f;
    s.upscalerMode = UPSCALER_MODE_OFF;
    assert(settingEnabled(s, SET_VHS_RESOLUTION));
    assert(cycleSetting(s, row) && s.vhsResolution == VHS_RES_AUTO);
}

int main() {
    testTapeRowsFollowOutputSize();
    testVideoMenuAndEffectToggle();
}
