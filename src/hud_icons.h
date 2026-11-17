#pragma once
// HUD pictogram drawing: icon atlas texture, icons, key caps and small gauges.
// Included from menu_hud.h (needs the text shader and overlay helpers of menu.h).

#include "hud_icon_art.h"

inline GLuint hudIconTex = 0;

inline void initHudIcons() {
    std::vector<unsigned char> alpha;
    int w = 0, h = 0;
    buildHudIconAtlas(alpha, w, h);
    std::vector<unsigned char> rgb((size_t)w * h * 3);
    for (size_t i = 0; i < alpha.size(); i++) rgb[i * 3] = rgb[i * 3 + 1] = rgb[i * 3 + 2] = alpha[i];
    glGenTextures(1, &hudIconTex);
    glBindTexture(GL_TEXTURE_2D, hudIconTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

inline float hudAspect() {
    return (float)currentWinW / (float)currentWinH;
}

inline void drawIconRaw(int icon, float cx, float cy, float h, float r, float g, float b, float a) {
    float hh = h * 0.5f, hw = hh / hudAspect();
    float u0 = (float)(icon % HUD_ICON_COLS) / (float)HUD_ICON_COLS, u1 = u0 + 1.0f / (float)HUD_ICON_COLS;
    float v0 = (float)(icon / HUD_ICON_COLS) / (float)HUD_ICON_ROWS, v1 = v0 + 1.0f / (float)HUD_ICON_ROWS;
    float v[24] = {
        cx - hw, cy - hh, u0, v1,  cx + hw, cy - hh, u1, v1,  cx + hw, cy + hh, u1, v0,
        cx - hw, cy - hh, u0, v1,  cx + hw, cy + hh, u1, v0,  cx - hw, cy + hh, u0, v0
    };
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glUseProgram(textShader);
    glUniform3f(glGetUniformLocation(textShader, "col"), r, g, b);
    glUniform1f(glGetUniformLocation(textShader, "alpha"), a);
    glBindTexture(GL_TEXTURE_2D, hudIconTex);
    glBindVertexArray(textVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

// Icon centred at (cx, cy), `h` tall in NDC, with a soft drop shadow.
inline void drawIcon(int icon, float cx, float cy, float h, float r, float g, float b, float a = 0.95f) {
    if (icon < 0 || icon >= HUD_ICON_COUNT || !hudIconTex) return;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawIconRaw(icon, cx + 0.003f, cy - 0.004f, h, 0.0f, 0.0f, 0.0f, a * 0.65f);
    drawIconRaw(icon, cx, cy, h, r, g, b, a);
    glEnable(GL_CULL_FACE);
}

// Keyboard key: a dark cap with the key name on it.
inline void drawKeyCap(const char* label, float cx, float cy, float h, float a = 0.95f) {
    float sc = h * (float)currentWinH * 0.036f; // letter about 0.58 of the cap height
    float tw = measureTextWidthNdc(label, sc);
    float hw = tw * 0.5f + h * 0.30f / hudAspect() + 0.006f;
    float minHw = h * 0.5f / hudAspect();
    if (hw < minHw) hw = minHw;
    float hh = h * 0.5f;
    drawOverlayRectNdc(cx - hw, cy - hh, cx + hw, cy + hh, 0.93f, 0.90f, 0.80f, a);
    drawOverlayRectNdc(cx - hw + 0.004f, cy - hh + 0.012f, cx + hw - 0.004f, cy + hh - 0.006f, 0.10f, 0.10f, 0.11f, a);
    float ch = sc * 8.0f / (float)currentWinH * 2.0f;
    drawText(label, cx - tw * 0.5f, cy - ch * 0.5f + 0.003f, sc, 0.96f, 0.93f, 0.82f, a);
}

// Horizontal gauge: dark track, coloured fill.
inline void drawGauge(float x0, float y0, float x1, float y1, float frac, float r, float g, float b, float a = 0.9f) {
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    drawOverlayRectNdc(x0, y0, x1, y1, 0.02f, 0.02f, 0.03f, 0.62f * a);
    float pad = 0.004f;
    if (frac > 0.002f) drawOverlayRectNdc(x0 + pad / hudAspect(), y0 + pad, x0 + (x1 - x0) * frac - pad / hudAspect(), y1 - pad, r, g, b, a);
}

// Progress ring made of segments around (cx, cy): hold actions fill it clockwise.
inline void drawHoldRing(float cx, float cy, float radius, float frac, float r, float g, float b) {
    const int segments = 16;
    float dot = radius * 0.16f;
    for (int i = 0; i < segments; i++) {
        float ang = 6.2831853f * (float)i / (float)segments;
        float x = cx + sinf(ang) * radius / hudAspect(), y = cy + cosf(ang) * radius;
        bool lit = (float)i < frac * (float)segments;
        if (lit) drawOverlayRectNdc(x - dot / hudAspect(), y - dot, x + dot / hudAspect(), y + dot, r, g, b, 0.95f);
        else drawOverlayRectNdc(x - dot / hudAspect(), y - dot, x + dot / hudAspect(), y + dot, 0.05f, 0.05f, 0.06f, 0.55f);
    }
}

// Right-aligned / centred number next to an icon (prices, cash, distances).
inline void drawHudNumber(int value, float x, float y, float sc, float r, float g, float b, float a = 0.95f, bool sign = false) {
    char buf[16];
    snprintf(buf, sizeof(buf), sign ? "%+d" : "%d", value);
    drawText(buf, x - 0.002f, y - 0.003f, sc, 0.0f, 0.0f, 0.0f, a * 0.7f);
    drawText(buf, x, y, sc, r, g, b, a);
}
