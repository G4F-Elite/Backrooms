#pragma once

enum VhsResolution {
    VHS_RES_AUTO, VHS_RES_240, VHS_RES_480, VHS_RES_720, VHS_RES_NATIVE, VHS_RES_COUNT
};

struct VhsResolutionPreset {
    const char* label;
    int rows; // zero follows the output framebuffer
};

inline constexpr VhsResolutionPreset VHS_RESOLUTIONS[] = {
    {"AUTO", 400}, {"240 LINES", 240}, {"480 LINES", 480}, {"720 LINES", 720}, {"NATIVE", 0}
};
static_assert(sizeof(VHS_RESOLUTIONS) / sizeof(*VHS_RESOLUTIONS) == VHS_RES_COUNT);

inline int vhsTapeRows(int preset, int outputHeight) {
    if (outputHeight <= 0) return 0;
    if (preset < 0) preset = VHS_RES_AUTO;
    if (preset >= VHS_RES_COUNT) preset = VHS_RES_NATIVE;
    const int rows = VHS_RESOLUTIONS[preset].rows;
    return rows > 0 && rows < outputHeight ? rows : outputHeight;
}
