#pragma once

inline void drawHudText(const char* s, float x, float y, float sc, float r, float g, float b, float a = 0.95f) {
    drawText(s, x - 0.002f, y - 0.002f, sc, 0.0f, 0.0f, 0.0f, a * 0.72f);
    drawText(s, x, y, sc, r, g, b, a);
}

inline void drawHudTextCentered(const char* s, float x, float y, float sc, float r, float g, float b, float a = 0.95f) {
    drawTextCentered(s, x - 0.002f, y - 0.002f, sc, 0.0f, 0.0f, 0.0f, a * 0.72f);
    drawTextCentered(s, x, y, sc, r, g, b, a);
}

inline void drawEyeMarker(float sx, float sy, float scale, float alpha) {
    float w = 0.050f * scale;
    float h = 0.022f * scale;
    drawOverlayRectNdc(sx - w, sy - h, sx + w, sy + h, 0.46f, 0.08f, 0.08f, alpha * 0.62f);
    drawOverlayRectNdc(sx - w * 0.86f, sy - h * 0.68f, sx + w * 0.86f, sy + h * 0.68f, 0.84f, 0.16f, 0.14f, alpha * 0.92f);
    drawOverlayRectNdc(sx - w * 0.28f, sy - h * 0.58f, sx + w * 0.28f, sy + h * 0.58f, 0.05f, 0.02f, 0.02f, alpha);
    drawOverlayRectNdc(sx - w * 0.10f, sy - h * 0.20f, sx + w * 0.10f, sy + h * 0.20f, 0.90f, 0.30f, 0.24f, alpha);
}

inline float uiHudClamp01(float v){
    if(v < 0.0f) return 0.0f;
    if(v > 1.0f) return 1.0f;
    return v;
}

inline UiPrimitiveTone uiHudToneForRatio(float ratio){
    if(ratio < 0.25f) return UI_PRIMITIVE_TONE_CRITICAL;
    if(ratio < 0.60f) return UI_PRIMITIVE_TONE_WARNING;
    return UI_PRIMITIVE_TONE_DEFAULT;
}
