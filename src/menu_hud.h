#pragma once

extern float vhsTime;

// === Visual HUD Bar Constants ===
static const float BAR_LEFT = -0.95f;
static const float BAR_RIGHT = -0.50f;
static const float BAR_W = BAR_RIGHT - BAR_LEFT;
static const float BAR_H = 0.032f;
static const float BAR_HALF_H = 0.016f;
static const float BAR_LABEL_X = -0.94f;
static const float BAR_PCT_X = -0.52f;
static const float BAR_TEXT_SC = 1.0f;

// Draw a rectangular border frame using 4 thin rectangles
// Thickness is in NDC units; auto-compensates for aspect ratio so lines look equal
inline void drawBorderRect(float left, float bottom, float right, float top,
    float r, float g, float b, float a, float thickness) {
    extern int currentWinW, currentWinH;
    float aspect = (float)currentWinW / (float)currentWinH;
    float vt = thickness / aspect;  // vertical-edge thickness (pixel-matched)
    float inset = thickness * 0.5f; // draw slightly inside to avoid sub-pixel gaps
    // Top edge
    drawOverlayRectNdc(left + vt, top - thickness, right - vt, top, r, g, b, a);
    // Bottom edge
    drawOverlayRectNdc(left + vt, bottom, right - vt, bottom + thickness, r, g, b, a);
    // Left edge (full height to cover corners)
    drawOverlayRectNdc(left, bottom, left + vt, top, r, g, b, a);
    // Right edge
    drawOverlayRectNdc(right - vt, bottom, right, top, r, g, b, a);
}

// Draw corner accent marks (2 short lines per corner, extending inward)
inline void drawCornerAccents(float left, float bottom, float right, float top,
    float r, float g, float b, float a, float len, float thickness) {
    extern int currentWinW, currentWinH;
    float vt = thickness / ((float)currentWinW / (float)currentWinH);
    // Top-left corner: horizontal right, vertical down
    drawOverlayRectNdc(left, top - thickness, left + len, top, r, g, b, a);
    drawOverlayRectNdc(left, top - len, left + vt, top, r, g, b, a);
    // Top-right corner: horizontal left, vertical down
    drawOverlayRectNdc(right - len, top - thickness, right, top, r, g, b, a);
    drawOverlayRectNdc(right - vt, top - len, right, top, r, g, b, a);
    // Bottom-left corner: horizontal right, vertical up
    drawOverlayRectNdc(left, bottom, left + len, bottom + thickness, r, g, b, a);
    drawOverlayRectNdc(left, bottom, left + vt, bottom + len, r, g, b, a);
    // Bottom-right corner: horizontal left, vertical up
    drawOverlayRectNdc(right - len, bottom, right, bottom + thickness, r, g, b, a);
    drawOverlayRectNdc(right - vt, bottom, right, bottom + len, r, g, b, a);
}

// Panel backdrop behind left-side bars (stamina, HP, sanity)
inline void drawLeftBarPanel() {
    float pl = -0.97f, pr = -0.48f;
    float sl = -0.94f, sr = -0.51f;
    drawOverlayRectNdc(pl, -0.97f, pr, -0.78f, 0.01f, 0.01f, 0.02f, 0.50f);
    // Panel border frame
    drawBorderRect(pl, -0.97f, pr, -0.78f, 0.30f, 0.27f, 0.24f, 0.85f, 0.004f);
    // Corner accents for horror feel
    drawCornerAccents(pl, -0.97f, pr, -0.78f, 0.50f, 0.45f, 0.36f, 0.92f, 0.015f, 0.004f);
    // Separator lines between bars
    drawOverlayRectNdc(sl, -0.853f, sr, -0.850f, 0.25f, 0.22f, 0.18f, 0.55f);
    drawOverlayRectNdc(sl, -0.908f, sr, -0.905f, 0.25f, 0.22f, 0.18f, 0.55f);
}

// Panel backdrop behind right-side flashlight bar
inline void drawRightBarPanel() {
    float pl = 0.48f, pr = 0.97f;
    drawOverlayRectNdc(pl, -0.97f, pr, -0.89f, 0.01f, 0.01f, 0.02f, 0.50f);
    // Panel border frame
    drawBorderRect(pl, -0.97f, pr, -0.89f, 0.30f, 0.27f, 0.24f, 0.85f, 0.004f);
    // Corner accents for horror feel
    drawCornerAccents(pl, -0.97f, pr, -0.89f, 0.50f, 0.45f, 0.36f, 0.92f, 0.015f, 0.004f);
}

// Generic visual bar: background track + colored fill + label + percentage + border + ticks
inline void drawVisualBar(float yCenter, const char* label, float frac,
    float fr, float fg, float fb, float lr, float lg, float lb) {
    float left = BAR_LEFT, right = BAR_RIGHT, w = BAR_W;
    float labelX = BAR_LABEL_X, pctX = BAR_PCT_X;
    float yBot = yCenter - BAR_HALF_H;
    float yTop = yCenter + BAR_HALF_H;
    // Dark track background
    drawOverlayRectNdc(left, yBot, right, yTop, 0.02f, 0.02f, 0.03f, 0.6f);
    // Tick marks at 25%, 50%, 75%
    float tick25 = left + w * 0.25f;
    float tick50 = left + w * 0.50f;
    float tick75 = left + w * 0.75f;
    drawOverlayRectNdc(tick25, yBot, tick25 + 0.001f, yTop, 0.30f, 0.28f, 0.24f, 0.35f);
    drawOverlayRectNdc(tick50, yBot, tick50 + 0.001f, yTop, 0.30f, 0.28f, 0.24f, 0.35f);
    drawOverlayRectNdc(tick75, yBot, tick75 + 0.001f, yTop, 0.30f, 0.28f, 0.24f, 0.35f);
    // Colored fill (clamped)
    if (frac > 0.001f) {
        float fillR = left + w * (frac > 1.0f ? 1.0f : frac);
        drawOverlayRectNdc(left + 0.002f, yBot + 0.002f, fillR - 0.002f, yTop - 0.002f, fr, fg, fb, 0.85f);
    }
    // Border frame around bar track
    drawBorderRect(left, yBot, right, yTop, 0.40f, 0.36f, 0.30f, 0.85f, 0.004f);
    // Label text
    drawText(label, labelX, yCenter - 0.004f, BAR_TEXT_SC, lr, lg, lb, 0.85f);
    // Percentage text
    char pct[8];
    snprintf(pct, 8, "%d", (int)(frac * 100.0f + 0.5f));
    drawText(pct, pctX, yCenter - 0.004f, BAR_TEXT_SC, lr, lg, lb, 0.7f);
}

inline void drawHealthBar(float hp) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float frac = hp / 100.0f;
    float fr, fg, fb;
    if (hp < 30.0f) {
        float pulse = 0.5f + 0.5f * sinf(vhsTime * 5.0f);
        fr = 0.95f; fg = 0.2f * pulse; fb = 0.1f * pulse;
    } else {
        fr = 0.7f; fg = 0.15f; fb = 0.1f;
    }
    drawVisualBar(-0.88f, "HP", frac, fr, fg, fb, 0.65f, 0.6f, 0.5f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawSanityBar(float sn) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float frac = sn / 100.0f;
    float fr, fg, fb;
    if (sn < 30.0f) {
        float flicker = 0.5f + 0.5f * sinf(vhsTime * 8.0f + sinf(vhsTime * 13.0f) * 2.0f);
        fr = 0.45f * flicker; fg = 0.15f * flicker; fb = 0.65f * flicker;
    } else {
        fr = 0.35f; fg = 0.2f; fb = 0.55f;
    }
    drawVisualBar(-0.93f, "SN", frac, fr, fg, fb, 0.65f, 0.6f, 0.5f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawDamageOverlay(float fl, float hp) {
    if (fl < 0.01f && hp > 50.0f) return;
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float lo = (100.0f - hp) / 100.0f * 0.3f;
    float a = fl + lo;
    if (a > 0.8f) a = 0.8f;
    // Red vignette edges
    drawOverlayRectNdc(-1.0f, 0.85f, 1.0f, 1.0f, 0.6f, 0.0f, 0.0f, a * 0.5f);
    drawOverlayRectNdc(-1.0f, -1.0f, 1.0f, -0.85f, 0.6f, 0.0f, 0.0f, a * 0.5f);
    drawOverlayRectNdc(-1.0f, -1.0f, -0.85f, 1.0f, 0.6f, 0.0f, 0.0f, a * 0.4f);
    drawOverlayRectNdc(0.85f, -1.0f, 1.0f, 1.0f, 0.6f, 0.0f, 0.0f, a * 0.4f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawStaminaBar(float st) {
    if (st >= 124.0f) return;
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float frac = st / 125.0f;
    float fr, fg, fb;
    if (st < 25.0f) {
        float pulse = 0.5f + 0.5f * sinf(vhsTime * 6.0f);
        fr = 0.5f * pulse + 0.1f; fg = 0.6f * pulse + 0.2f; fb = 0.15f * pulse;
    } else {
        fr = 0.2f; fg = 0.5f; fb = 0.2f;
    }
    drawVisualBar(-0.82f, "ST", frac, fr, fg, fb, 0.65f, 0.6f, 0.5f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawFlashlightBattery(float battery, bool isOn) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawRightBarPanel();
    float frac = battery / 100.0f;
    float fr, fg, fb;
    if (isOn) { fr = 0.8f; fg = 0.75f; fb = 0.2f; }
    else { fr = 0.3f; fg = 0.3f; fb = 0.3f; }
    float fLeft = 0.50f, fRight = 0.95f;
    float fW = fRight - fLeft;
    float yBot = -0.93f - BAR_HALF_H;
    float yTop = -0.93f + BAR_HALF_H;
    // Track
    drawOverlayRectNdc(fLeft, yBot, fRight, yTop, 0.02f, 0.02f, 0.03f, 0.6f);
    // Tick marks at 25%, 50%, 75%
    float ftick25 = fLeft + fW * 0.25f;
    float ftick50 = fLeft + fW * 0.50f;
    float ftick75 = fLeft + fW * 0.75f;
    drawOverlayRectNdc(ftick25, yBot, ftick25 + 0.001f, yTop, 0.30f, 0.28f, 0.24f, 0.35f);
    drawOverlayRectNdc(ftick50, yBot, ftick50 + 0.001f, yTop, 0.30f, 0.28f, 0.24f, 0.35f);
    drawOverlayRectNdc(ftick75, yBot, ftick75 + 0.001f, yTop, 0.30f, 0.28f, 0.24f, 0.35f);
    // Fill
    if (frac > 0.001f) {
        float fillR = fLeft + fW * (frac > 1.0f ? 1.0f : frac);
        drawOverlayRectNdc(fLeft + 0.002f, yBot + 0.002f, fillR - 0.002f, yTop - 0.002f, fr, fg, fb, 0.85f);
    }
    // Border frame around bar track
    drawBorderRect(fLeft, yBot, fRight, yTop, 0.40f, 0.36f, 0.30f, 0.85f, 0.004f);
    // Label
    drawText("FL", 0.51f, -0.934f, BAR_TEXT_SC, 0.65f, 0.6f, 0.5f, 0.85f);
    // Percentage
    char pct[8];
    snprintf(pct, 8, "%d", (int)(frac * 100.0f + 0.5f));
    drawText(pct, 0.93f, -0.934f, BAR_TEXT_SC, 0.65f, 0.6f, 0.5f, 0.7f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawContractCounter(int count) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    char b[20]; snprintf(b, 20, "CONTRACT: %d/3", count);
    drawText(b, -0.95f, -0.66f, 1.5f, 0.72f, 0.68f, 0.46f, 0.84f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawPhaseIndicator(int phase) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const char* pn[] = { "EXPLORATION","TENSION","PURSUIT","ESCAPE" };
    if (phase >= 0 && phase < 4) {
        drawText("PHASE:", 0.49f, 0.85f, 1.25f, 0.80f, 0.75f, 0.52f, 0.88f);
        drawText(pn[phase], 0.60f, 0.82f, 1.35f, 0.86f, 0.80f, 0.56f, 0.92f);
    }
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawInteractPrompt() {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawText("[E] INTERACT", -0.15f, -0.4f, 1.8f, 0.8f, 0.75f, 0.5f, 0.8f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

inline void drawHallucinationEffect(float intensity) {
    if (intensity < 0.05f) return;
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float a = intensity * 0.3f;
    if (a > 0.4f) a = 0.4f;
    // Purple vignette edges
    drawOverlayRectNdc(-1.0f, 0.88f, 1.0f, 1.0f, 0.3f, 0.0f, 0.4f, a);
    drawOverlayRectNdc(-1.0f, -1.0f, 1.0f, -0.88f, 0.3f, 0.0f, 0.4f, a);
    drawOverlayRectNdc(-1.0f, -1.0f, -0.88f, 1.0f, 0.3f, 0.0f, 0.4f, a * 0.8f);
    drawOverlayRectNdc(0.88f, -1.0f, 1.0f, 1.0f, 0.3f, 0.0f, 0.4f, a * 0.8f);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}
