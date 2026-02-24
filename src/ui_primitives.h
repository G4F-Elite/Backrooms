#pragma once

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

#include "ui_style_tokens.h"

enum UiPrimitiveTone {
    UI_PRIMITIVE_TONE_DEFAULT = 0,
    UI_PRIMITIVE_TONE_WARNING = 1,
    UI_PRIMITIVE_TONE_CRITICAL = 2,
    UI_PRIMITIVE_TONE_DISABLED = 3
};

struct UiPrimitiveTheme {
    UiColorToken text;
    UiColorToken mutedText;
    UiColorToken panelBase;
    UiColorToken panelEdge;
    UiColorToken panelAccent;
    float panelAlpha;
    float edgeAlpha;
    float accentAlpha;
};

struct UiPrimitivePerfBaseline {
    int panels = 0, buttons = 0, inputs = 0, lists = 0, modals = 0, tooltips = 0, indicators = 0;
    int rectPasses = 0, textPasses = 0;
    int atlasFallbackGlyphPasses = 0, atlasTexturePasses = 0;
    float rectAreaNdc = 0.0f, overdrawLayersEstimate = 0.0f;
    float cpuMsLast = 0.0f, cpuMsAvg = 0.0f, cpuMsPeak = 0.0f;
    int frameSamples = 0;
    bool budgetPassLast = true;
};

namespace UiPrimitivePerfBudget {
constexpr float kUiFrameCpuBudgetMs = 0.40f;
constexpr int kRectPassBudget = 48;
constexpr int kTextPassBudget = 26;
constexpr float kOverdrawLayerBudget = 2.50f;
}

struct UiFontPipelineState { bool initialized = false; UiFontChain display{}, heading{}, body{}, meta{}, iconFallback{}; };

enum UiIconGlyph { UI_ICON_GLYPH_NONE=0, UI_ICON_GLYPH_OBJECTIVE=1, UI_ICON_GLYPH_NETWORK=2, UI_ICON_GLYPH_WARNING=3, UI_ICON_GLYPH_CONFIRM=4 };
struct UiIconAtlasSpec { const char* sourcePath; int columns, rows; float paddingPx, sdfSoftness; };
struct UiIconAtlasState { bool initialized=false, enabled=false, textureReady=false; UiIconAtlasSpec spec{}; };

inline UiPrimitivePerfBaseline gUiPrimitivePerf;
inline float gUiPrimitivePerfStartSec = 0.0f;
inline float gUiPrimitivePulseFast = 0.0f;
inline float gUiPrimitivePulseSlow = 0.0f;
inline UiFontPipelineState gUiFontPipeline;
inline UiIconAtlasState gUiIconAtlas;

inline bool uiPrimitiveFrameWithinBudget() {
    return gUiPrimitivePerf.cpuMsLast <= UiPrimitivePerfBudget::kUiFrameCpuBudgetMs
        && gUiPrimitivePerf.rectPasses <= UiPrimitivePerfBudget::kRectPassBudget
        && gUiPrimitivePerf.textPasses <= UiPrimitivePerfBudget::kTextPassBudget
        && gUiPrimitivePerf.overdrawLayersEstimate <= UiPrimitivePerfBudget::kOverdrawLayerBudget;
}

inline void uiInitFontPipeline() {
    if (gUiFontPipeline.initialized) return;
    gUiFontPipeline.display = uiFontChainForRole(UI_FONT_ROLE_DISPLAY);
    gUiFontPipeline.heading = uiFontChainForRole(UI_FONT_ROLE_HEADING);
    gUiFontPipeline.body = uiFontChainForRole(UI_FONT_ROLE_BODY);
    gUiFontPipeline.meta = uiFontChainForRole(UI_FONT_ROLE_META);
    gUiFontPipeline.iconFallback = uiFontChainForRole(UI_FONT_ROLE_ICON_FALLBACK);
    gUiFontPipeline.initialized = true;
}

inline UiFontRole uiFontRoleForScale(float scale) {
    if (scale >= UiTypography::kScaleTitle) return UI_FONT_ROLE_DISPLAY;
    if (scale >= UiTypography::kScaleSubtitle) return UI_FONT_ROLE_HEADING;
    if (scale <= UiTypography::kScaleMeta) return UI_FONT_ROLE_META;
    return UI_FONT_ROLE_BODY;
}

inline UiIconAtlasSpec uiDefaultIconAtlasSpec() {
    return {
        "assets/ui/icons/icon_atlas_01.png",
        UiIconography::kAtlasColumns, UiIconography::kAtlasRows,
        UiIconography::kDefaultPaddingPx, UiIconography::kSdfSoftness
    };
}

inline void uiInitIconAtlasScaffold() {
    if (gUiIconAtlas.initialized) return;
    gUiIconAtlas.spec = uiDefaultIconAtlasSpec();
    const char* iconEnv = std::getenv("BR_UI_ICON_ATLAS");
    if (iconEnv && std::strcmp(iconEnv, "0") != 0) gUiIconAtlas.enabled = true;
    gUiIconAtlas.textureReady = false;
    gUiIconAtlas.initialized = true;
}

inline const char* uiIconFallbackGlyph(UiIconGlyph glyph) {
    if (glyph == UI_ICON_GLYPH_OBJECTIVE) return "#";
    if (glyph == UI_ICON_GLYPH_NETWORK) return "~";
    if (glyph == UI_ICON_GLYPH_WARNING) return "!";
    if (glyph == UI_ICON_GLYPH_CONFIRM) return "+";
    return ".";
}

inline UiPrimitiveTheme makeUiPrimitiveTheme() {
    UiPrimitiveTheme theme{};
    theme.text = UiColor::kTextPrimary;
    theme.mutedText = UiColor::kTextMuted;
    theme.panelBase = UiColor::kOverlayBase;
    theme.panelEdge = UiColor::kTextSecondary;
    theme.panelAccent = UiColor::kOverlayWarm;
    theme.panelAlpha = UiDepth::kPanelAlpha;
    theme.edgeAlpha = UiDepth::kPanelEdgeAlpha;
    theme.accentAlpha = 0.20f;
    const char* themeEnv = std::getenv("BR_UI_THEME");
    if (themeEnv && uiEqualsIgnoreCase(themeEnv, "high-contrast")) {
        theme.panelBase = UiColorToken{0.01f, 0.01f, 0.01f};
        theme.panelEdge = UiColorToken{0.92f, 0.88f, 0.70f};
        theme.panelAlpha = 0.92f;
        theme.edgeAlpha = 0.70f;
    }
    if (!uiContrastBaselinePassesAa()) {
        theme.text = UiColorToken{0.98f, 0.98f, 0.95f};
        theme.mutedText = UiColorToken{0.90f, 0.90f, 0.82f};
        theme.panelBase = UiColorToken{0.01f, 0.01f, 0.01f};
        theme.panelAlpha = 0.94f;
    }
    return theme;
}

// ── Low-level draw: all coords are already in actual NDC ────────────────────
inline void uiPrimitiveRect(float left, float bottom, float right, float top, const UiColorToken& c, float alpha) {
    if (alpha <= 0.001f) return;
    float l = left < -1.0f ? -1.0f : left;
    float b = bottom < -1.0f ? -1.0f : bottom;
    float r = right > 1.0f ? 1.0f : right;
    float t = top > 1.0f ? 1.0f : top;
    if (r <= l || t <= b) return;
    gUiPrimitivePerf.rectAreaNdc += (r - l) * (t - b);
    drawOverlayRectNdc(l, b, r, t, c.r, c.g, c.b, alpha);
    gUiPrimitivePerf.rectPasses++;
}

// Virtual-coord rect: converts from 16:9 virtual space to actual NDC
inline void uiVRect(float vl, float vb, float vr, float vt, const UiColorToken& c, float alpha) {
    uiPrimitiveRect(uiX(vl), uiY(vb), uiX(vr), uiY(vt), c, alpha);
}

inline void uiPrimitiveTextCentered(const char* s, float cx, float y, float scale, const UiColorToken& c, float alpha, UiFontRole role = UI_FONT_ROLE_BODY) {
    uiInitFontPipeline();
    drawTextCentered(s, cx, y, uiClampTypographyScale(scale, role), c.r, c.g, c.b, alpha);
    gUiPrimitivePerf.textPasses++;
}

inline void uiPrimitiveText(const char* s, float x, float y, float scale, const UiColorToken& c, float alpha, UiFontRole role = UI_FONT_ROLE_BODY) {
    uiInitFontPipeline();
    drawText(s, x, y, uiClampTypographyScale(scale, role), c.r, c.g, c.b, alpha);
    gUiPrimitivePerf.textPasses++;
}

inline void uiPrimitiveIcon(UiIconGlyph glyph, float left, float bottom, float right, float top, const UiColorToken& c, float alpha) {
    uiInitIconAtlasScaffold();
    float iconW = right - left;
    if (iconW < UiIconography::kMinReadableNdc) right = left + UiIconography::kMinReadableNdc;
    if ((right - left) > UiIconography::kMaxReadableNdc) right = left + UiIconography::kMaxReadableNdc;
    uiPrimitiveRect(left, bottom, right, top, UiColor::kOverlayBase, 0.50f);
    const char* fb = uiIconFallbackGlyph(glyph);
    float sc = (gUiIconAtlas.enabled && gUiIconAtlas.textureReady) ? UiTypography::kScaleHint : UiTypography::kScaleMeta;
    if (gUiIconAtlas.enabled && gUiIconAtlas.textureReady) gUiPrimitivePerf.atlasTexturePasses++;
    else gUiPrimitivePerf.atlasFallbackGlyphPasses++;
    uiPrimitiveTextCentered(fb, (left+right)*0.5f, bottom+0.002f, sc, c, alpha, UI_FONT_ROLE_ICON_FALLBACK);
}

inline UiColorToken uiPrimitiveToneColor(UiPrimitiveTone tone) {
    if (tone == UI_PRIMITIVE_TONE_WARNING) return UiColor::kStateWarning;
    if (tone == UI_PRIMITIVE_TONE_CRITICAL) return UiColor::kStateCritical;
    if (tone == UI_PRIMITIVE_TONE_DISABLED) return UiColor::kStateDisabled;
    return UiColor::kStateDefault;
}

inline float uiPrimitiveToneAlpha(UiPrimitiveTone tone) {
    if (tone == UI_PRIMITIVE_TONE_WARNING) return UiDepth::kStateWarningAlpha;
    if (tone == UI_PRIMITIVE_TONE_CRITICAL) return UiDepth::kStateCriticalAlpha;
    if (tone == UI_PRIMITIVE_TONE_DISABLED) return UiDepth::kStateDisabledAlpha;
    return UiDepth::kStateDefaultAlpha;
}

inline void uiPrimitiveBeginFrame(float tm) {
    uiInitFontPipeline();
    uiUpdateAspect(currentWinW, currentWinH);
    gUiPrimitivePerf = {};
    gUiPrimitivePerfStartSec = (float)glfwGetTime();
    if (uiReducedMotionEnabled()) {
        gUiPrimitivePulseFast = 0.0f;
        gUiPrimitivePulseSlow = 0.0f;
    } else {
        gUiPrimitivePulseFast = 0.5f + 0.5f * sinf(tm * (1.0f / UiMotion::kPulseFastSec));
        gUiPrimitivePulseSlow = 0.5f + 0.5f * sinf(tm * (1.0f / UiMotion::kPulseSlowSec));
    }
}

inline void uiPrimitiveEndFrame() {
    float dtMs = ((float)glfwGetTime() - gUiPrimitivePerfStartSec) * 1000.0f;
    gUiPrimitivePerf.overdrawLayersEstimate = gUiPrimitivePerf.rectAreaNdc / 4.0f;
    gUiPrimitivePerf.cpuMsLast = dtMs;
    gUiPrimitivePerf.frameSamples++;
    float n = (float)gUiPrimitivePerf.frameSamples;
    gUiPrimitivePerf.cpuMsAvg = (gUiPrimitivePerf.cpuMsAvg * (n - 1.0f) + dtMs) / n;
    if (dtMs > gUiPrimitivePerf.cpuMsPeak) gUiPrimitivePerf.cpuMsPeak = dtMs;
    gUiPrimitivePerf.budgetPassLast = uiPrimitiveFrameWithinBudget();
}

// ── Widget primitives: all coordinates are VIRTUAL (16:9 space) ─────────────
// Minimal drawing — text-driven with subtle accent marks only.
// The uiX()/uiY() mapping happens inside each primitive.

inline void drawUiPanelPrimitive(const UiPrimitiveTheme& theme, float vl, float vb, float vr, float vt, const char* title, UiPrimitiveTone tone) {
    float l = uiX(vl), b = uiY(vb), r = uiX(vr), t = uiY(vt);
    UiColorToken state = uiPrimitiveToneColor(tone);
    uiPrimitiveRect(l, b, r, t, theme.panelBase, theme.panelAlpha * 0.55f);
    uiPrimitiveRect(l, t - uiH(0.002f), r, t, state, 0.30f);
    if (title && title[0])
        uiPrimitiveText(title, l + uiW(0.010f), t - uiH(0.024f), UiTypography::kScaleMeta, theme.mutedText, 0.55f, UI_FONT_ROLE_META);
    gUiPrimitivePerf.panels++;
}

inline void drawUiButtonPrimitive(const UiPrimitiveTheme& theme, float vl, float vb, float vr, float vt, const char* label, bool focused, bool enabled) {
    float l = uiX(vl), b = uiY(vb), r = uiX(vr), t = uiY(vt);
    if (focused) {
        uiPrimitiveRect(l, b, r, t, UiColor::kSelectGlow, 0.10f);
        uiPrimitiveRect(l, b, r, b + uiH(0.002f), UiColor::kAccent, 0.50f);
    }
    float alpha = enabled ? (focused ? 0.92f : 0.55f) : 0.32f;
    uiPrimitiveTextCentered(label, (l+r)*0.5f, (b+t)*0.5f - uiH(0.008f), UiTypography::kScaleHint, theme.text, alpha, UI_FONT_ROLE_BODY);
    gUiPrimitivePerf.buttons++;
}

inline void drawUiInputPrimitive(const UiPrimitiveTheme& theme, float vl, float vb, float vr, float vt, const char* label, const char* value, bool focused) {
    float l = uiX(vl), b = uiY(vb), r = uiX(vr), t = uiY(vt);
    uiPrimitiveRect(l, b, r, t, theme.panelBase, 0.38f);
    if (focused) uiPrimitiveRect(l, b, r, b + uiH(0.002f), UiColor::kAccent, 0.55f);
    uiPrimitiveText(label, l + uiW(0.008f), t - uiH(0.020f), UiTypography::kScaleMeta, theme.mutedText, 0.50f, UI_FONT_ROLE_META);
    uiPrimitiveText(value, l + uiW(0.008f), b + uiH(0.008f), UiTypography::kScaleHint, theme.text, focused ? 0.88f : 0.62f, UI_FONT_ROLE_BODY);
    gUiPrimitivePerf.inputs++;
}

inline void drawUiListPrimitive(const UiPrimitiveTheme& theme, float vl, float vb, float vr, float vt, const char* title, const char* const* items, int count, int selected) {
    if (count < 0) count = 0;
    if (title && title[0])
        uiPrimitiveText(title, uiX(vl + 0.008f), uiY(vt - 0.016f), UiTypography::kScaleMeta, theme.mutedText, 0.40f, UI_FONT_ROLE_META);
    float l = uiX(vl), r = uiX(vr);
    float rowTop = uiY(vt) - uiH(title && title[0] ? 0.040f : 0.010f);
    float rowH = uiH(0.052f);
    for (int i = 0; i < count; ++i) {
        float y1 = rowTop - i * rowH;
        float y0 = y1 - rowH + uiH(0.006f);
        bool active = i == selected;
        if (active) {
            uiPrimitiveRect(l + uiW(0.006f), y0, r - uiW(0.006f), y1, UiColor::kSelectGlow, 0.12f);
            uiPrimitiveRect(l + uiW(0.018f), y0, r - uiW(0.018f), y0 + uiH(0.002f), UiColor::kAccent, 0.48f);
        }
        uiPrimitiveText(items[i], l + uiW(0.018f), y0 + uiH(0.010f), UiTypography::kScaleHint, theme.text, active ? 0.94f : 0.52f, UI_FONT_ROLE_BODY);
    }
    gUiPrimitivePerf.lists++;
}

inline void drawUiModalPrimitive(const UiPrimitiveTheme& theme, const char* title, const char* body, UiPrimitiveTone tone) {
    uiPrimitiveRect(-1.0f, -1.0f, 1.0f, 1.0f, UiColor::kOverlayBase, 0.30f);
    float l = uiX(-0.34f), r = uiX(0.34f), b = uiY(-0.06f), t = uiY(0.16f);
    UiColorToken state = uiPrimitiveToneColor(tone);
    uiPrimitiveRect(l, b, r, t, theme.panelBase, 0.70f);
    uiPrimitiveRect(l, t - uiH(0.002f), r, t, state, 0.35f);
    uiPrimitiveTextCentered(title, (l+r)*0.5f, t - uiH(0.030f), UiTypography::kScaleSubtitle, state, 0.85f, UI_FONT_ROLE_HEADING);
    uiPrimitiveTextCentered(body, (l+r)*0.5f, b + uiH(0.012f), UiTypography::kScaleMeta, theme.text, 0.76f, UI_FONT_ROLE_BODY);
    gUiPrimitivePerf.modals++;
}

inline void drawUiTooltipPrimitive(const UiPrimitiveTheme& theme, float vx, float vy, const char* text) {
    uiPrimitiveTextCentered(text, uiX(vx), uiY(vy), UiTypography::kScaleMeta, theme.mutedText, 0.50f, UI_FONT_ROLE_META);
    gUiPrimitivePerf.tooltips++;
}

inline void drawUiStatusIndicatorPrimitive(const UiPrimitiveTheme& theme, float vl, float vb, float vr, float vt, const char* label, UiPrimitiveTone tone) {
    UiColorToken c = uiPrimitiveToneColor(tone);
    float a = uiPrimitiveToneAlpha(tone);
    float cy = (uiY(vb) + uiY(vt)) * 0.5f;
    float pl = uiX(vl);
    uiPrimitiveRect(pl, cy - uiH(0.005f), pl + uiW(0.003f), cy + uiH(0.005f), c, 0.45f * a);
    uiPrimitiveText(label, pl + uiW(0.010f), cy - uiH(0.007f), UiTypography::kScaleMeta, theme.text, 0.72f * a, UI_FONT_ROLE_META);
    gUiPrimitivePerf.indicators++;
}