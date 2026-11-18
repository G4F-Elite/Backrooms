#pragma once
// UI widgets built on ui_core.h: panels, list rows, buttons, sliders, switches, choice
// steppers, tabs, text fields, key hints. Canvas units everywhere (see ui_core.h).
// The look is text on darkness: no boxes, no frames, no divider lines. Selection is the
// accent colour plus one thin bar; everything else is one of three greys.
// Widgets only draw and register click zones; what a click does is up to the input code.

#include "ui_core.h"
#include "hud_icons.h"

inline constexpr float UI_ROW_TEXT = 1.5f;   // text size of rows and buttons
inline constexpr float UI_SMALL_TEXT = 1.25f;
inline constexpr float UI_HINT_BAR_H = 44.0f;

// Icon from the HUD atlas, `h` canvas pixels tall.
inline void uiIcon(int icon, float cx, float cy, float h, UiCol c, float a = 1.0f) {
    drawIcon(icon, uiNdcX(cx), uiNdcY(cy), uiNdcY(h), c.r, c.g, c.b, a * ui.alpha);
}

// Window: no box at all, only a soft patch of darkness behind the content so it reads
// over whatever is on screen.
inline void uiPanel(float l, float b, float r, float t, float bodyAlpha = 0.45f) {
    const float feather = 90.0f;
    uiRect(l, b, r, t, UI_BLACK, bodyAlpha);
    uiShadow(l, b, r, t, feather, bodyAlpha);
}

inline constexpr float UI_HEADER_H = 58.0f;

// Title of a panel. Returns the y where its content starts.
inline float uiPanelHeader(float l, float r, float t, const char* title, const char* note = nullptr) {
    float y = t - UI_HEADER_H * 0.5f;
    uiText(title, l + 28.0f, y, 2.0f, UI_TEXT);
    if (note && note[0]) uiTextR(note, r - 28.0f, y, UI_SMALL_TEXT, UI_MUTE);
    return t - UI_HEADER_H;
}

// Caption of a section inside a panel.
inline void uiSection(float l, float, float y, const char* caption) {
    uiText(caption, l, y, UI_SMALL_TEXT, UI_MUTE);
}

// A selectable row: nothing but a thin accent bar on its left edge while selected.
// Registers the row as item `idx` and returns how lit it is (0..1) so the caller can tint
// its content.
inline float uiRowBack(int idx, float l, float b, float r, float t, bool selected) {
    float glow = uiItemGlow(idx, selected);
    uiZone(UI_ZONE_ITEM, idx, l, b, r, t);
    float mid = (b + t) * 0.5f;
    if (glow > 0.01f) uiRect(l, mid - 8.0f, l + 2.0f, mid + 8.0f, UI_ACCENT, glow);
    return glow;
}

inline UiCol uiRowColor(float glow, bool enabled = true) {
    if (!enabled) return UI_MUTE;
    return uiMix(UI_DIM, UI_ACCENT, glow);
}

// A row that is only a label: main menu and pause entries.
inline void uiListItem(int idx, const char* label, float l, float r, float y, float h, bool selected, float size = 2.0f) {
    float glow = uiRowBack(idx, l, y - h * 0.5f, r, y + h * 0.5f, selected);
    uiText(label, l + 18.0f, y, size, uiMix(UI_DIM, UI_ACCENT, glow));
}

// Button: a label starting at `l`, with the same accent bar as a list row.
inline void uiButton(int idx, const char* label, float l, float b, float r, float t, bool selected, bool enabled = true) {
    float glow = uiRowBack(idx, l - 14.0f, b, r, t, selected);
    UiCol c = enabled ? uiMix(UI_TEXT, UI_ACCENT, glow) : UI_MUTE;
    uiText(label, l, (b + t) * 0.5f, UI_ROW_TEXT, c);
}

// Slider: track from l to r at height y. Registers the track as a drag zone for item `idx`.
inline void uiSlider(int idx, float l, float r, float y, float value, float glow, bool enabled = true) {
    value = uiClamp01(value);
    UiCol fill = enabled ? uiMix(UI_DIM, UI_ACCENT, glow) : UI_MUTE;
    float kx = l + (r - l) * value;
    uiRect(l, y - 0.5f, r, y + 0.5f, UI_LINE, 0.45f);
    uiRect(l, y - 0.5f, kx, y + 0.5f, fill, 0.95f);
    uiRect(kx - 1.0f, y - 5.0f, kx + 1.0f, y + 5.0f, enabled ? uiMix(UI_TEXT, UI_ACCENT, glow) : UI_MUTE, 1.0f);
    if (enabled) uiZone(UI_ZONE_SLIDER, idx, l - 6.0f, y - 14.0f, r + 6.0f, y + 14.0f);
}

// On / off switch: the word itself, right edge at `right`.
inline void uiSwitch(float right, float y, bool on, float glow, bool enabled = true) {
    UiCol lit = enabled ? uiMix(UI_TEXT, UI_ACCENT, glow) : UI_MUTE;
    uiTextR(on ? "ON" : "OFF", right, y, UI_ROW_TEXT, on ? lit : UI_MUTE);
}

// "<  VALUE  >" between l and r. The arrows are click zones for item `idx`.
inline void uiStepper(int idx, float l, float r, float y, const char* value, bool canPrev, bool canNext, float glow, bool enabled = true) {
    UiCol c = uiRowColor(glow, enabled);
    uiTextC(value, (l + r) * 0.5f, y, UI_ROW_TEXT, enabled ? uiMix(UI_TEXT, UI_ACCENT, glow) : UI_MUTE);
    uiText("<", l + 6.0f, y, UI_ROW_TEXT, c, canPrev ? 1.0f : 0.22f);
    uiTextR(">", r - 6.0f, y, UI_ROW_TEXT, c, canNext ? 1.0f : 0.22f);
    if (canPrev) uiZone(UI_ZONE_PREV, idx, l - 6.0f, y - 14.0f, l + 30.0f, y + 14.0f);
    if (canNext) uiZone(UI_ZONE_NEXT, idx, r - 30.0f, y - 14.0f, r + 6.0f, y + 14.0f);
}

// Row of tabs written one after another from `l`. The current one is in the accent colour.
inline void uiTabs(float l, float, float y, float h, const char* const* names, int count, int current) {
    const float gap = 32.0f;
    float x = l;
    for (int i = 0; i < count; i++) {
        float w = uiTextW(names[i], UI_ROW_TEXT);
        float x0 = x - gap * 0.5f, x1 = x + w + gap * 0.5f;
        bool active = i == current, hover = uiMouseIn(x0, y - h * 0.5f, x1, y + h * 0.5f);
        uiZone(UI_ZONE_TAB, i, x0, y - h * 0.5f, x1, y + h * 0.5f);
        UiCol c = active ? UI_ACCENT : (hover ? UI_TEXT : UI_MUTE);
        uiText(names[i], x, y, UI_ROW_TEXT, c);
        x += w + gap;
    }
}

// Text field: an underline. Focused: accent underline and a blinking caret after the text.
inline void uiField(int idx, float l, float r, float y, float h, const char* text, bool focused) {
    float b = y - h * 0.5f, t = y + h * 0.5f;
    float glow = uiItemGlow(idx, focused);
    uiZone(UI_ZONE_ITEM, idx, l, b, r, t);
    uiRect(l, b, r, b + 1.0f, uiMix(UI_LINE, UI_ACCENT, glow), 0.60f + 0.40f * glow);
    uiText(text, l + 2.0f, y, UI_ROW_TEXT, UI_TEXT);
    if (focused && fmodf(ui.time, 1.0f) < 0.55f) {
        float cx = l + 5.0f + uiTextW(text, UI_ROW_TEXT);
        uiRect(cx, y - 8.0f, cx + 2.0f, y + 8.0f, UI_ACCENT, 0.9f);
    }
}

// Keyboard key: just its name, left edge at x. Returns its width.
inline float uiKeyCap(const char* label, float x, float y, float a = 1.0f) {
    uiText(label, x, y, UI_SMALL_TEXT, UI_DIM, a);
    return uiTextW(label, UI_SMALL_TEXT);
}

struct UiHint {
    const char* key;
    const char* label;
};

// Line along the bottom edge: KEY WHAT IT DOES, left to right. Only for keys a player
// would not guess. A hint whose key is ESC is also the screen's clickable "back".
inline void uiHintBar(const UiHint* hints, int count) {
    float y = -uiHalfH() + UI_HINT_BAR_H * 0.5f;
    float x = -uiHalfW() + 40.0f;
    for (int i = 0; i < count; i++) {
        bool isBack = strcmp(hints[i].key, "ESC") == 0;
        float end = x + uiTextW(hints[i].key, UI_SMALL_TEXT) + 8.0f + uiTextW(hints[i].label, UI_SMALL_TEXT);
        bool hover = isBack && uiMouseIn(x - 8.0f, y - 16.0f, end + 8.0f, y + 16.0f);
        if (isBack) uiZone(UI_ZONE_BACK, 0, x - 8.0f, y - 16.0f, end + 8.0f, y + 16.0f);
        float labelX = x + uiKeyCap(hints[i].key, x, y) + 8.0f;
        uiText(hints[i].label, labelX, y, UI_SMALL_TEXT, hover ? UI_ACCENT : UI_MUTE);
        x = end + 28.0f;
    }
}

// Ring of dots with one bright one running around: "working on it".
inline void uiSpinner(float cx, float cy, float radius, UiCol c) {
    const int dots = 10;
    float head = fmodf(ui.time * 9.0f, (float)dots);
    for (int i = 0; i < dots; i++) {
        float ang = 6.2831853f * (float)i / (float)dots;
        float behind = fmodf(head - (float)i + (float)dots, (float)dots) / (float)dots;
        float x = cx + sinf(ang) * radius, y = cy + cosf(ang) * radius;
        uiRect(x - 1.5f, y - 1.5f, x + 1.5f, y + 1.5f, c, 0.15f + 0.85f * (1.0f - behind) * (1.0f - behind));
    }
}

// Darkens the edges of the screen so panels and text sit on a calmer picture.
inline void uiVignette(float strength) {
    float hw = uiHalfW(), hh = uiHalfH();
    uiGradV(-hw, hh - 200.0f, hw, hh, UI_BLACK, 0.0f, strength);
    uiGradV(-hw, -hh, hw, -hh + 200.0f, UI_BLACK, strength, 0.0f);
    uiGradH(-hw, -hh, -hw + 260.0f, hh, UI_BLACK, strength, 0.0f);
    uiGradH(hw - 260.0f, -hh, hw, hh, UI_BLACK, 0.0f, strength);
}

// Backdrop of every screen outside a run: the corridor, haze, and `dim` of darkness on top.
inline void uiMenuBackdrop(float tm, float dim) {
    glDisable(GL_DEPTH_TEST);
    drawMainMenuBackdrop(tm);
    drawMenuAtmosphere(tm);
    drawFullscreenOverlay(0.02f, 0.02f, 0.02f, dim);
    uiVignette(0.55f);
}
