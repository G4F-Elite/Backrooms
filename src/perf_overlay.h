#pragma once

#include <algorithm>
#include <vector>
#include <cstdio>
#include "upscaler_settings.h"

inline constexpr int PERF_GRAPH_SAMPLES = 64;

inline void initFrameTimeHistory(float* history, int count, float seedMs) {
    if (!history || count <= 0) return;
    if (seedMs < 0.1f) seedMs = 0.1f;
    for (int i = 0; i < count; i++) history[i] = seedMs;
}

inline void pushFrameTimeSample(float* history, int count, int& head, float frameMs) {
    if (!history || count <= 0) return;
    if (frameMs < 0.1f) frameMs = 0.1f;
    if (frameMs > 120.0f) frameMs = 120.0f;
    head = (head + 1) % count;
    history[head] = frameMs;
}

inline float averageFrameTimeMs(const float* history, int count) {
    if (!history || count <= 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < count; i++) sum += history[i];
    return sum / (float)count;
}

inline float percentileFrameTimeMs(const float* history, int count, float percentile) {
    if (!history || count <= 0) return 0.0f;
    if (percentile < 0.0f) percentile = 0.0f;
    if (percentile > 1.0f) percentile = 1.0f;
    std::vector<float> sorted;
    sorted.reserve((size_t)count);
    for (int i = 0; i < count; i++) sorted.push_back(history[i]);
    std::sort(sorted.begin(), sorted.end());
    int idx = (int)(percentile * (float)(count - 1) + 0.5f);
    if (idx < 0) idx = 0;
    if (idx >= count) idx = count - 1;
    return sorted[(size_t)idx];
}

inline char frameTimeLevelChar(float ms) {
    const char* levels = " .:-=+*#%@";
    const int last = 9;
    float t = ms / 40.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    int idx = (int)(t * (float)last + 0.5f);
    if (idx < 0) idx = 0;
    if (idx > last) idx = last;
    return levels[idx];
}

inline void buildFrameTimeGraph(
    const float* history,
    int count,
    int head,
    int width,
    char* out,
    int outSize
) {
    if (!out || outSize <= 0) return;
    out[0] = '\0';
    if (!history || count <= 0 || width <= 0) return;
    if (width >= outSize) width = outSize - 1;
    if (width <= 0) return;

    int oldest = (head + 1) % count;
    for (int i = 0; i < width; i++) {
        int src = (int)((float)i * (float)(count - 1) / (float)(width - 1 > 0 ? width - 1 : 1));
        int idx = (oldest + src) % count;
        out[i] = frameTimeLevelChar(history[idx]);
    }
    out[width] = '\0';
}

inline int frameGenOutputTargetHz(int refreshRateHz, int baseFpsCap, int frameGenMode, bool vsyncEnabled) {
    if (!isFrameGenEnabled(frameGenMode)) return 0;
    if (vsyncEnabled && refreshRateHz > 0) return refreshRateHz;
    if (baseFpsCap <= 0) return 0;
    int hz = (int)((float)baseFpsCap * frameGenMultiplier(frameGenMode) + 0.5f);
    if (hz < 1) hz = 1;
    return hz;
}

inline void formatFrameGenPipeline(
    char* out,
    int outSize,
    int refreshRateHz,
    int baseFpsCap,
    int frameGenMode,
    bool vsyncEnabled
) {
    if (!out || outSize <= 0) return;
    if (!isFrameGenEnabled(frameGenMode)) {
        snprintf(out, outSize, "FG: OFF");
        return;
    }
    int targetHz = frameGenOutputTargetHz(refreshRateHz, baseFpsCap, frameGenMode, vsyncEnabled);
    if (baseFpsCap > 0 && targetHz > 0) {
        snprintf(out, outSize, "FG %s: %d -> %d HZ", frameGenModeLabel(frameGenMode), baseFpsCap, targetHz);
    } else {
        snprintf(out, outSize, "FG %s: UNCAPPED", frameGenModeLabel(frameGenMode));
    }
}

inline void formatUpscalePipeline(
    char* out,
    int outSize,
    int upscalerMode,
    int renderW,
    int renderH,
    int winW,
    int winH
) {
    if (!out || outSize <= 0) return;
    if (clampUpscalerMode(upscalerMode) == UPSCALER_MODE_OFF) {
        snprintf(out, outSize, "UPSCALE: NATIVE %dx%d", winW, winH);
        return;
    }
    snprintf(out, outSize, "UPSCALE %s: %dx%d -> %dx%d", upscalerModeLabel(upscalerMode), renderW, renderH, winW, winH);
}
