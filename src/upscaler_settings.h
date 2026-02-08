#pragma once

inline constexpr int UPSCALER_MODE_OFF = 0;
inline constexpr int UPSCALER_MODE_FSR10 = 1;
inline constexpr int UPSCALER_MODE_COUNT = 2;

inline constexpr int RENDER_SCALE_PRESET_COUNT = 6;
inline constexpr int RENDER_SCALE_PRESET_DEFAULT = 3;
inline constexpr float RENDER_SCALE_PRESETS[RENDER_SCALE_PRESET_COUNT] = {
    0.50f, 0.59f, 0.67f, 0.75f, 0.83f, 1.00f
};

inline int clampUpscalerMode(int mode) {
    if (mode < UPSCALER_MODE_OFF) return UPSCALER_MODE_OFF;
    if (mode >= UPSCALER_MODE_COUNT) return UPSCALER_MODE_COUNT - 1;
    return mode;
}

inline int clampRenderScalePreset(int preset) {
    if (preset < 0) return 0;
    if (preset >= RENDER_SCALE_PRESET_COUNT) return RENDER_SCALE_PRESET_COUNT - 1;
    return preset;
}

inline int stepRenderScalePreset(int preset, int delta) {
    return clampRenderScalePreset(preset + delta);
}

inline float renderScaleFromPreset(int preset) {
    return RENDER_SCALE_PRESETS[clampRenderScalePreset(preset)];
}

inline int renderScalePercentFromPreset(int preset) {
    float scale = renderScaleFromPreset(preset);
    return (int)(scale * 100.0f + 0.5f);
}

inline float effectiveRenderScale(int upscalerMode, int preset) {
    if (clampUpscalerMode(upscalerMode) == UPSCALER_MODE_OFF) return 1.0f;
    return renderScaleFromPreset(preset);
}

inline float clampFsrSharpness(float sharpness) {
    if (sharpness < 0.0f) return 0.0f;
    if (sharpness > 1.0f) return 1.0f;
    return sharpness;
}

inline const char* upscalerModeLabel(int mode) {
    return clampUpscalerMode(mode) == UPSCALER_MODE_FSR10 ? "FSR 1.0" : "OFF";
}
