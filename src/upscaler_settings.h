#pragma once

inline constexpr int UPSCALER_MODE_OFF = 0;
inline constexpr int UPSCALER_MODE_FSR10 = 1;
inline constexpr int UPSCALER_MODE_FSR20 = 2;
inline constexpr int UPSCALER_MODE_COUNT = 3;

inline constexpr int AA_MODE_OFF = 0;
inline constexpr int AA_MODE_FXAA = 1;
inline constexpr int AA_MODE_TAA = 2;
inline constexpr int AA_MODE_COUNT = 3;

inline constexpr int FRAME_GEN_MODE_OFF = 0;
inline constexpr int FRAME_GEN_MODE_150 = 1;
inline constexpr int FRAME_GEN_MODE_200 = 2;
inline constexpr int FRAME_GEN_MODE_250 = 3;
inline constexpr int FRAME_GEN_MODE_COUNT = 4;

inline constexpr int RENDER_SCALE_PRESET_COUNT = 10;
inline constexpr int RENDER_SCALE_PRESET_DEFAULT = 7;
inline constexpr float RENDER_SCALE_PRESETS[RENDER_SCALE_PRESET_COUNT] = {
    0.20f, 0.25f, 0.33f, 0.42f, 0.50f, 0.59f, 0.67f, 0.75f, 0.83f, 1.00f
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

inline int clampAaMode(int mode) {
    if (mode < AA_MODE_OFF) return AA_MODE_OFF;
    if (mode >= AA_MODE_COUNT) return AA_MODE_COUNT - 1;
    return mode;
}

inline int stepAaMode(int mode, int delta) {
    return clampAaMode(mode + delta);
}

inline int stepRenderScalePreset(int preset, int delta) {
    return clampRenderScalePreset(preset + delta);
}

inline int clampFrameGenMode(int mode) {
    if (mode < FRAME_GEN_MODE_OFF) return FRAME_GEN_MODE_OFF;
    if (mode >= FRAME_GEN_MODE_COUNT) return FRAME_GEN_MODE_COUNT - 1;
    return mode;
}

inline int stepFrameGenMode(int mode, int delta) {
    return clampFrameGenMode(mode + delta);
}

inline bool isFrameGenEnabled(int mode) {
    return clampFrameGenMode(mode) != FRAME_GEN_MODE_OFF;
}

inline float frameGenMultiplier(int mode) {
    int clamped = clampFrameGenMode(mode);
    if (clamped == FRAME_GEN_MODE_150) return 1.5f;
    if (clamped == FRAME_GEN_MODE_200) return 2.0f;
    if (clamped == FRAME_GEN_MODE_250) return 2.5f;
    return 1.0f;
}

inline float frameGenBlendStrength(int mode) {
    int clamped = clampFrameGenMode(mode);
    if (clamped == FRAME_GEN_MODE_150) return 0.18f;
    if (clamped == FRAME_GEN_MODE_200) return 0.24f;
    if (clamped == FRAME_GEN_MODE_250) return 0.30f;
    return 0.0f;
}

inline int frameGenBaseFpsCap(int refreshRateHz, int mode, bool vsyncEnabled) {
    if (!vsyncEnabled) return 0;
    if (!isFrameGenEnabled(mode)) return 0;
    if (refreshRateHz <= 0) return 0;

    float base = (float)refreshRateHz / frameGenMultiplier(mode);
    int cap = (int)(base + 0.5f);
    if (cap < 30) cap = 30;
    if (cap > refreshRateHz) cap = refreshRateHz;
    return cap;
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
    int clamped = clampUpscalerMode(mode);
    if (clamped == UPSCALER_MODE_FSR10) return "FSR 1.0";
    if (clamped == UPSCALER_MODE_FSR20) return "FSR 2.0";
    return "OFF";
}

inline const char* aaModeLabel(int mode) {
    switch (clampAaMode(mode)) {
        case AA_MODE_FXAA: return "FXAA";
        case AA_MODE_TAA: return "TAA";
        default: return "OFF";
    }
}

inline const char* frameGenModeLabel(int mode) {
    switch (clampFrameGenMode(mode)) {
        case FRAME_GEN_MODE_150: return "1.5X";
        case FRAME_GEN_MODE_200: return "2.0X";
        case FRAME_GEN_MODE_250: return "2.5X";
        default: return "OFF";
    }
}

inline constexpr int SSAO_COUNT = 5;
inline constexpr int GI_COUNT = 4;
inline int clampSsao(int q){ return q<0?0:(q>=SSAO_COUNT?SSAO_COUNT-1:q); }
inline int clampGi(int q){ return q<0?0:(q>=GI_COUNT?GI_COUNT-1:q); }
inline int stepSsao(int q,int d){ return clampSsao(q+d); }
inline int stepGi(int q,int d){ return clampGi(q+d); }
inline const char* ssaoLabel(int q){
    const char* n[]={"OFF","LOW","MEDIUM","HIGH","ULTRA"};
    return n[clampSsao(q)];
}
inline const char* giLabel(int q){
    const char* n[]={"OFF","LOW","MEDIUM","HIGH"};
    return n[clampGi(q)];
}
