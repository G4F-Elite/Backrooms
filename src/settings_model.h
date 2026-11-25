#pragma once
// Settings data and the table the settings screen is built from. No drawing, no input:
// every row is described here once and both the screen and the input code read it.
// Tests: tests/settings_model_tests.cpp

#include <cstdio>
#include "upscaler_settings.h"
#include "vhs_settings.h"
#include "keybinds.h"

struct Settings {
    float masterVol = 0.7f;
    float musicVol = 0.55f;
    float ambienceVol = 0.75f;
    float sfxVol = 0.7f;
    float voiceVol = 0.65f;
    float vhsIntensity = 0.58f;
    int vhsResolution = VHS_RES_AUTO;
    float mouseSens = 0.002f;
    int upscalerMode = UPSCALER_MODE_FSR10;
    int renderScalePreset = RENDER_SCALE_PRESET_DEFAULT;
    float fsrSharpness = 0.35f;
    int aaMode = AA_MODE_TAA;
    bool fastMath = false;
    int ssaoQuality = 0;
    int giQuality = 0;
    bool godRays = false;
    bool bloom = false;
    bool rtxDenoise = true;
    float rtxDenoiseStrength = 0.65f;
    bool vsync = true;
    bool debugMode = false;
    GameplayBinds binds = {};
};

inline constexpr float MOUSE_SENS_MIN = 0.0005f;
inline constexpr float MOUSE_SENS_MAX = 0.006f;

enum SettingsTab {
    SETTINGS_TAB_VIDEO = 0,
    SETTINGS_TAB_EFFECTS,
    SETTINGS_TAB_AUDIO,
    SETTINGS_TAB_CONTROLS,
    SETTINGS_TAB_COUNT
};

enum SettingId {
    SET_VHS = 0,
    SET_VHS_RESOLUTION,
    SET_UPSCALER,
    SET_RENDER_SCALE,
    SET_SHARPNESS,
    SET_AA,
    SET_VSYNC,
    SET_FAST_MATH,
    SET_DEBUG,
    SET_SSAO,
    SET_GI,
    SET_GOD_RAYS,
    SET_BLOOM,
    SET_DENOISE,
    SET_DENOISE_STRENGTH,
    SET_VOL_MASTER,
    SET_VOL_MUSIC,
    SET_VOL_AMBIENCE,
    SET_VOL_SFX,
    SET_VOL_VOICE,
    SET_MOUSE_SENS,
    SET_COUNT
};

enum SettingKind {
    SETTING_SLIDER = 0, // 0..1 value, shown as a percentage
    SETTING_CHOICE,     // one of a few named values
    SETTING_TOGGLE,     // on / off
    SETTING_BIND,       // a key; `id` is the bind index
    SETTING_RESET_BINDS // puts every key back
};

struct SettingRow {
    SettingKind kind;
    int id;
    const char* label;
    const char* hint;
};

inline const char* settingsTabName(int tab) {
    static const char* names[SETTINGS_TAB_COUNT] = {"VIDEO", "EFFECTS", "AUDIO", "CONTROLS"};
    return (tab >= 0 && tab < SETTINGS_TAB_COUNT) ? names[tab] : "";
}

inline constexpr SettingRow SETTINGS_VIDEO_ROWS[] = {
    {SETTING_SLIDER, SET_VHS, "VHS EFFECT", "TAPE NOISE AND DISTORTION OVER THE PICTURE."},
    {SETTING_CHOICE, SET_VHS_RESOLUTION, "VHS RESOLUTION", "VERTICAL DETAIL OF THE TAPE IMAGE. AUTO USES UP TO 400 LINES."},
    {SETTING_CHOICE, SET_UPSCALER, "UPSCALER", "RENDERS BELOW WINDOW SIZE AND SCALES THE PICTURE UP."},
    {SETTING_CHOICE, SET_RENDER_SCALE, "RENDER SCALE", "INTERNAL RESOLUTION. LOWER IS FASTER AND SOFTER."},
    {SETTING_SLIDER, SET_SHARPNESS, "SHARPNESS", "SHARPENING APPLIED AFTER UPSCALING."},
    {SETTING_CHOICE, SET_AA, "ANTI-ALIASING", "SMOOTHS JAGGED EDGES."},
    {SETTING_TOGGLE, SET_VSYNC, "V-SYNC", "LOCKS THE FRAME RATE TO THE DISPLAY. REMOVES TEARING."},
    {SETTING_TOGGLE, SET_FAST_MATH, "FAST MATH", "CHEAPER APPROXIMATE MATH. SMALL SPEED-UP."},
    {SETTING_TOGGLE, SET_DEBUG, "DEBUG MODE", "FPS READOUT, OBJECTIVE TEXT AND THE F10 TOOLS PANEL."},
};

inline constexpr SettingRow SETTINGS_EFFECTS_ROWS[] = {
    {SETTING_CHOICE, SET_SSAO, "AMBIENT OCCLUSION", "CONTACT SHADOWS IN CORNERS AND UNDER PROPS."},
    {SETTING_CHOICE, SET_GI, "GLOBAL ILLUMINATION", "LIGHT BOUNCING OFF WALLS AND FLOORS."},
    {SETTING_TOGGLE, SET_GOD_RAYS, "GOD RAYS", "VISIBLE SHAFTS OF LIGHT IN THE HAZE."},
    {SETTING_TOGGLE, SET_BLOOM, "BLOOM", "GLOW AROUND LAMPS AND BRIGHT SURFACES."},
    {SETTING_TOGGLE, SET_DENOISE, "DENOISER", "CLEANS UP THE GRAIN LEFT BY THE LIGHTING EFFECTS."},
    {SETTING_SLIDER, SET_DENOISE_STRENGTH, "DENOISE STRENGTH", "HOW HARD THE DENOISER SMOOTHS THE PICTURE."},
};

inline constexpr SettingRow SETTINGS_AUDIO_ROWS[] = {
    {SETTING_SLIDER, SET_VOL_MASTER, "MASTER", "OVERALL VOLUME."},
    {SETTING_SLIDER, SET_VOL_MUSIC, "MUSIC", "SCORE AND TENSION LAYERS."},
    {SETTING_SLIDER, SET_VOL_AMBIENCE, "AMBIENCE", "LAMP HUM, ROOM TONE, DISTANT NOISES."},
    {SETTING_SLIDER, SET_VOL_SFX, "EFFECTS", "STEPS, TOOLS, THE DRILL AND THE THINGS THAT HUNT YOU."},
    {SETTING_SLIDER, SET_VOL_VOICE, "VOICE", "SQUAD CALLOUTS."},
};

inline constexpr int SETTINGS_CONTROLS_EXTRA_ROWS = 2; // mouse sensitivity above the binds, reset below

inline int settingsRowCount(int tab) {
    if (tab == SETTINGS_TAB_VIDEO) return (int)(sizeof(SETTINGS_VIDEO_ROWS) / sizeof(SettingRow));
    if (tab == SETTINGS_TAB_EFFECTS) return (int)(sizeof(SETTINGS_EFFECTS_ROWS) / sizeof(SettingRow));
    if (tab == SETTINGS_TAB_AUDIO) return (int)(sizeof(SETTINGS_AUDIO_ROWS) / sizeof(SettingRow));
    if (tab == SETTINGS_TAB_CONTROLS) return GAMEPLAY_BIND_COUNT + SETTINGS_CONTROLS_EXTRA_ROWS;
    return 0;
}

inline SettingRow settingsRowAt(int tab, int row) {
    static constexpr SettingRow none = {SETTING_TOGGLE, -1, "", ""};
    if (row < 0 || row >= settingsRowCount(tab)) return none;
    if (tab == SETTINGS_TAB_VIDEO) return SETTINGS_VIDEO_ROWS[row];
    if (tab == SETTINGS_TAB_EFFECTS) return SETTINGS_EFFECTS_ROWS[row];
    if (tab == SETTINGS_TAB_AUDIO) return SETTINGS_AUDIO_ROWS[row];
    if (row == 0) return {SETTING_SLIDER, SET_MOUSE_SENS, "MOUSE SENSITIVITY", "HOW FAST THE CAMERA TURNS."};
    if (row <= GAMEPLAY_BIND_COUNT) {
        return {SETTING_BIND, row - 1, gameplayBindLabel(row - 1), "PRESS ENTER, THEN THE NEW KEY."};
    }
    return {SETTING_RESET_BINDS, -1, "RESET KEYS", "PUTS EVERY KEY BACK TO ITS DEFAULT."};
}

inline int clampSettingsSelection(int tab, int idx) {
    int count = settingsRowCount(tab);
    if (count <= 0) return 0;
    if (idx < 0) return count - 1;
    if (idx >= count) return 0;
    return idx;
}

inline float settingClamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

inline float* settingSliderRef(Settings& s, int id) {
    switch (id) {
        case SET_VHS: return &s.vhsIntensity;
        case SET_SHARPNESS: return &s.fsrSharpness;
        case SET_DENOISE_STRENGTH: return &s.rtxDenoiseStrength;
        case SET_VOL_MASTER: return &s.masterVol;
        case SET_VOL_MUSIC: return &s.musicVol;
        case SET_VOL_AMBIENCE: return &s.ambienceVol;
        case SET_VOL_SFX: return &s.sfxVol;
        case SET_VOL_VOICE: return &s.voiceVol;
        case SET_MOUSE_SENS: return &s.mouseSens;
        default: return nullptr;
    }
}

// Slider position, 0..1.
inline float settingSlider01(const Settings& s, int id) {
    const float* ref = settingSliderRef(const_cast<Settings&>(s), id);
    if (!ref) return 0.0f;
    if (id == SET_MOUSE_SENS) return settingClamp01(*ref / MOUSE_SENS_MAX);
    return settingClamp01(*ref);
}

inline void setSettingSlider01(Settings& s, int id, float v) {
    float* ref = settingSliderRef(s, id);
    if (!ref) return;
    v = settingClamp01(v);
    if (id == SET_MOUSE_SENS) {
        float sens = v * MOUSE_SENS_MAX;
        *ref = sens < MOUSE_SENS_MIN ? MOUSE_SENS_MIN : sens;
        return;
    }
    *ref = v;
}

inline bool* settingToggleRef(Settings& s, int id) {
    switch (id) {
        case SET_VSYNC: return &s.vsync;
        case SET_FAST_MATH: return &s.fastMath;
        case SET_DEBUG: return &s.debugMode;
        case SET_GOD_RAYS: return &s.godRays;
        case SET_BLOOM: return &s.bloom;
        case SET_DENOISE: return &s.rtxDenoise;
        default: return nullptr;
    }
}

inline bool settingToggleValue(const Settings& s, int id) {
    const bool* ref = settingToggleRef(const_cast<Settings&>(s), id);
    return ref ? *ref : false;
}

inline int* settingChoiceRef(Settings& s, int id) {
    switch (id) {
        case SET_VHS_RESOLUTION: return &s.vhsResolution;
        case SET_UPSCALER: return &s.upscalerMode;
        case SET_RENDER_SCALE: return &s.renderScalePreset;
        case SET_AA: return &s.aaMode;
        case SET_SSAO: return &s.ssaoQuality;
        case SET_GI: return &s.giQuality;
        default: return nullptr;
    }
}

inline int settingChoiceCount(int id) {
    switch (id) {
        case SET_VHS_RESOLUTION: return VHS_RES_COUNT;
        case SET_UPSCALER: return UPSCALER_MODE_COUNT;
        case SET_RENDER_SCALE: return RENDER_SCALE_PRESET_COUNT;
        case SET_AA: return AA_MODE_COUNT;
        case SET_SSAO: return SSAO_COUNT;
        case SET_GI: return GI_COUNT;
        default: return 0;
    }
}

inline int settingChoiceValue(const Settings& s, int id) {
    const int* ref = settingChoiceRef(const_cast<Settings&>(s), id);
    int count = settingChoiceCount(id);
    if (!ref || count <= 0) return 0;
    return *ref < 0 ? 0 : (*ref >= count ? count - 1 : *ref);
}

// Render scale means nothing while the upscaler is off: the row is shown, but locked.
inline bool settingEnabled(const Settings& s, int id) {
    if (id == SET_VHS_RESOLUTION) return s.vhsIntensity > 0.0f;
    bool upscaling = clampUpscalerMode(s.upscalerMode) != UPSCALER_MODE_OFF;
    if (id == SET_RENDER_SCALE || id == SET_SHARPNESS) return upscaling;
    if (id == SET_DENOISE_STRENGTH) return s.rtxDenoise;
    return true;
}

inline bool settingCanStep(const Settings& s, int id, int dir) {
    return dir != 0 && settingChoiceCount(id) > 1 && settingEnabled(s, id);
}

// Left / right on a row. Returns true when something changed.
inline bool adjustSetting(Settings& s, const SettingRow& row, int dir) {
    if (dir == 0 || !settingEnabled(s, row.id)) return false;
    dir = dir > 0 ? 1 : -1;
    if (row.kind == SETTING_SLIDER) {
        float before = settingSlider01(s, row.id);
        setSettingSlider01(s, row.id, before + 0.05f * (float)dir);
        return settingSlider01(s, row.id) != before;
    }
    if (row.kind == SETTING_TOGGLE) {
        bool* ref = settingToggleRef(s, row.id);
        if (!ref) return false;
        *ref = !*ref;
        return true;
    }
    if (row.kind == SETTING_CHOICE) {
        if (!settingCanStep(s, row.id, dir)) return false;
        const int count = settingChoiceCount(row.id);
        *settingChoiceRef(s, row.id) = (settingChoiceValue(s, row.id) + dir + count) % count;
        return true;
    }
    return false;
}

// ENTER / click on a choice: next value, and back to the first one after the last.
inline bool cycleSetting(Settings& s, const SettingRow& row) { return adjustSetting(s, row, 1); }

inline void settingValueText(const Settings& s, const SettingRow& row, char* out, int cap) {
    if (row.kind == SETTING_SLIDER) {
        std::snprintf(out, cap, "%d%%", (int)(settingSlider01(s, row.id) * 100.0f + 0.5f));
    } else if (row.kind == SETTING_TOGGLE) {
        std::snprintf(out, cap, "%s", settingToggleValue(s, row.id) ? "ON" : "OFF");
    } else if (row.kind == SETTING_CHOICE) {
        int v = settingChoiceValue(s, row.id);
        if (row.id == SET_VHS_RESOLUTION) std::snprintf(out, cap, "%s", VHS_RESOLUTIONS[v].label);
        else if (row.id == SET_UPSCALER) std::snprintf(out, cap, "%s", upscalerModeLabel(v));
        else if (row.id == SET_AA) std::snprintf(out, cap, "%s", aaModeLabel(v));
        else if (row.id == SET_SSAO) std::snprintf(out, cap, "%s", ssaoLabel(v));
        else if (row.id == SET_GI) std::snprintf(out, cap, "%s", giLabel(v));
        else if (!settingEnabled(s, row.id)) std::snprintf(out, cap, "NATIVE");
        else std::snprintf(out, cap, "%d%%", renderScalePercentFromPreset(v));
    } else if (row.kind == SETTING_BIND) {
        int* key = gameplayBindByIndex(const_cast<Settings&>(s).binds, row.id);
        std::snprintf(out, cap, "%s", key ? keyNameForUi(*key) : "");
    } else {
        out[0] = '\0';
    }
}

// The bind that already uses `key`, other than `exceptIndex` (-1 when the key is free).
inline int bindIndexUsingKey(const GameplayBinds& binds, int key, int exceptIndex) {
    GameplayBinds copy = binds;
    for (int i = 0; i < GAMEPLAY_BIND_COUNT; i++) {
        if (i != exceptIndex && *gameplayBindByIndex(copy, i) == key) return i;
    }
    return -1;
}

// Rebinding onto a taken key swaps the two, so no action is ever left without a key.
inline void assignBind(GameplayBinds& binds, int index, int key) {
    int* target = gameplayBindByIndex(binds, index);
    if (!target) return;
    int other = bindIndexUsingKey(binds, key, index);
    if (other >= 0) *gameplayBindByIndex(binds, other) = *target;
    *target = key;
}
