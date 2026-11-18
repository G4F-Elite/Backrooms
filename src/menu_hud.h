#pragma once
// In-run HUD basics: vital bars, the shift clock and the full-screen hurt / hallucination
// vignettes. Built on the UI kit, so it scales with the window like the menus do.

#include "ui_widgets.h"

extern float vhsTime;

// NDC -> canvas, for HUD code that still thinks in -1..1.
inline float uiFromNdcX(float nx) { return nx * uiHalfW(); }
inline float uiFromNdcY(float ny) { return ny * uiHalfH(); }

// Backing of a HUD group, an NDC rectangle: a dark plate, no outline.
inline void drawHudPlate(float left, float bottom, float right, float top, float bodyAlpha = 0.62f) {
    float l = uiFromNdcX(left), r = uiFromNdcX(right), b = uiFromNdcY(bottom), t = uiFromNdcY(top);
    uiRect(l, b, r, t, UI_BLACK, bodyAlpha);
}

// Small square slot (carried loot): dark cell with a hairline frame.
inline void drawHudSlot(float cxNdc, float cyNdc, float hNdc, bool filled) {
    float cx = uiFromNdcX(cxNdc), cy = uiFromNdcY(cyNdc), half = uiFromNdcY(hNdc) * 0.5f;
    uiRect(cx - half, cy - half, cx + half, cy + half, UI_BLACK, 0.30f);
    uiFrame(cx - half, cy - half, cx + half, cy + half, filled ? UI_DIM : UI_LINE, filled ? 0.80f : 0.45f);
}

struct HudBarAnim {
    float shown = 0.0f; // fades the bar in while it has something to say
    float trail = 1.0f; // where the fill was a moment ago: losses leave a pale tail
};

inline constexpr float HUD_BAR_W = 180.0f;
inline constexpr float HUD_BAR_PITCH = 20.0f;

// One vital bar. `slot` counts up from the bottom edge; `rightSide` mirrors it to the right
// corner. Hidden bars fade out instead of popping.
inline void drawVitalBar(HudBarAnim& anim, int slot, bool rightSide, int icon, float frac, bool visible, UiCol fill) {
    frac = uiClamp01(frac);
    anim.shown = uiApproach(anim.shown, visible ? 1.0f : 0.0f, 9.0f);
    anim.trail = frac >= anim.trail ? frac : uiApproach(anim.trail, frac, 2.2f);
    if (anim.shown < 0.02f) return;

    float hw = uiHalfW(), y = -uiHalfH() + 30.0f + HUD_BAR_PITCH * (float)slot;
    float l = rightSide ? hw - 36.0f - HUD_BAR_W : -hw + 62.0f, r = l + HUD_BAR_W;
    float prevAlpha = ui.alpha;
    ui.alpha = prevAlpha * anim.shown;

    // a thin line: dark track, pale tail of what was just lost, coloured fill
    uiRect(l, y - 2.0f, r, y + 2.0f, UI_BLACK, 0.55f);
    if (anim.trail > frac + 0.004f) uiRect(l + HUD_BAR_W * frac, y - 2.0f, l + HUD_BAR_W * anim.trail, y + 2.0f, UI_TEXT, 0.55f);
    if (frac > 0.002f) uiRect(l, y - 2.0f, l + HUD_BAR_W * frac, y + 2.0f, fill, 0.95f);
    uiIcon(icon, l - 17.0f, y, 17.0f, frac < 0.25f ? uiMix(UI_TEXT, fill, 0.6f) : UI_DIM, 0.9f);
    ui.alpha = prevAlpha;
}

inline void drawHealthBar(float hp) {
    static HudBarAnim anim;
    UiCol fill = {0.72f, 0.16f, 0.11f};
    if (hp < 30.0f) {
        float pulse = uiPulse(5.0f);
        fill = {0.95f, 0.20f * pulse, 0.10f * pulse};
    }
    drawVitalBar(anim, 1, false, HUD_ICON_HEART, hp / 100.0f, hp < 100.0f, fill);
}

inline void drawSanityBar(float sanity) {
    static HudBarAnim anim;
    UiCol fill = {0.40f, 0.24f, 0.62f};
    if (sanity < 30.0f) {
        float flicker = 0.5f + 0.5f * sinf(vhsTime * 8.0f + sinf(vhsTime * 13.0f) * 2.0f);
        fill = {0.50f * flicker + 0.08f, 0.17f * flicker + 0.04f, 0.72f * flicker + 0.10f};
    }
    drawVitalBar(anim, 0, false, HUD_ICON_SANITY, sanity / 100.0f, sanity < 100.0f, fill);
}

inline void drawStaminaBar(float stamina) {
    static HudBarAnim anim;
    UiCol fill = {0.24f, 0.56f, 0.24f};
    if (stamina < 25.0f) {
        float pulse = uiPulse(6.0f);
        fill = {0.50f * pulse + 0.10f, 0.60f * pulse + 0.20f, 0.15f * pulse};
    }
    drawVitalBar(anim, 2, false, HUD_ICON_STAMINA, stamina / 125.0f, stamina < 124.0f, fill);
}

inline void drawFlashlightBattery(float battery, bool isOn) {
    static HudBarAnim anim;
    UiCol fill = isOn ? UiCol{0.86f, 0.78f, 0.24f} : UiCol{0.36f, 0.35f, 0.32f};
    if (isOn && battery < 20.0f) fill = uiMix(fill, UI_BAD, uiPulse(6.0f));
    drawVitalBar(anim, 0, true, HUD_ICON_FLASHLIGHT, battery / 100.0f, battery < 100.0f, fill);
}

// Shift clock, top-right.
inline void drawSurvivalTime(float t) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", (int)(t / 60.0f), (int)t % 60);
    float right = uiHalfW() - 40.0f, y = uiHalfH() - 36.0f;
    float w = uiTextW(buf, 2.0f);
    uiTextShadow(buf, right - w, y, 2.0f, UI_DIM, 0.85f);
}

// Colour bleeding in from the edges of the screen.
inline void drawEdgeVignette(UiCol c, float a, float reach) {
    float hw = uiHalfW(), hh = uiHalfH();
    uiGradV(-hw, hh - reach, hw, hh, c, 0.0f, a);
    uiGradV(-hw, -hh, hw, -hh + reach, c, a, 0.0f);
    uiGradH(-hw, -hh, -hw + reach * 1.3f, hh, c, a, 0.0f);
    uiGradH(hw - reach * 1.3f, -hh, hw, hh, c, 0.0f, a);
}

// Red at the edges: a flash on every hit, and a steady stain below half health.
inline void drawDamageOverlay(float flash, float hp) {
    if (flash < 0.01f && hp > 50.0f) return;
    float a = flash + (100.0f - hp) / 100.0f * 0.3f;
    if (a > 0.8f) a = 0.8f;
    drawEdgeVignette({0.62f, 0.0f, 0.0f}, a * 0.85f, 150.0f + 110.0f * uiClamp01(flash));
}

inline void drawHallucinationEffect(float intensity) {
    if (intensity < 0.05f) return;
    float a = intensity * 0.3f;
    if (a > 0.4f) a = 0.4f;
    drawEdgeVignette({0.30f, 0.0f, 0.42f}, a * (1.3f + 0.3f * uiPulse(1.7f)), 170.0f);
}
