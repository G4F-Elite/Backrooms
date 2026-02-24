#pragma once

#include <cmath>
#include <cstdlib>

// ── Aspect-ratio-aware layout ──────────────────────────────────────────────
// All UI uses a virtual 16:9 coordinate space centered on screen.
// On wider monitors the sides are letterboxed; on narrower ones the top/bottom.
// uiX()/uiY() convert from virtual coords (−1..1) to actual NDC.

inline float gUiAspect = 16.0f / 9.0f; // updated each frame from currentWinW/H

inline void uiUpdateAspect(int w, int h) {
    gUiAspect = (h > 0) ? (float)w / (float)h : (16.0f / 9.0f);
}

// Reference aspect is 16:9. UI is designed in that space.
constexpr float kUiRefAspect = 16.0f / 9.0f;

// Convert virtual X (−1..1 in 16:9 space) to actual NDC X.
inline float uiX(float vx) {
    if (gUiAspect > kUiRefAspect) {
        // Wider than 16:9 → compress X to keep content centered
        return vx * (kUiRefAspect / gUiAspect);
    }
    return vx;
}

// Convert virtual Y (−1..1) to actual NDC Y.
inline float uiY(float vy) {
    if (gUiAspect < kUiRefAspect) {
        // Taller than 16:9 → compress Y
        return vy * (gUiAspect / kUiRefAspect);
    }
    return vy;
}

// Scale a horizontal size from virtual to NDC.
inline float uiW(float vw) {
    if (gUiAspect > kUiRefAspect) return vw * (kUiRefAspect / gUiAspect);
    return vw;
}

// Scale a vertical size from virtual to NDC.
inline float uiH(float vh) {
    if (gUiAspect < kUiRefAspect) return vh * (gUiAspect / kUiRefAspect);
    return vh;
}

// ── Typography ──────────────────────────────────────────────────────────────
namespace UiTypography {
constexpr float kScaleTitle = 2.8f;
constexpr float kScaleSubtitle = 2.0f;
constexpr float kScaleMenuItem = 1.7f;
constexpr float kScaleHint = 1.4f;
constexpr float kScaleMeta = 1.1f;
}

namespace UiSpacing {
constexpr float kMenuStartY = 0.08f;
constexpr float kMenuItemStepY = 0.10f;
constexpr float kMenuChevronGapNdc = 0.06f;
constexpr float kTitleDriftNdc = 0.001f;
constexpr float kMenuAtmosphereEdgeBandNdc = 0.04f;
}

enum UiFontRole {
    UI_FONT_ROLE_DISPLAY = 0,
    UI_FONT_ROLE_HEADING = 1,
    UI_FONT_ROLE_BODY = 2,
    UI_FONT_ROLE_META = 3,
    UI_FONT_ROLE_ICON_FALLBACK = 4
};

struct UiFontChain {
    const char* primary;
    const char* fallbackLatin;
    const char* fallbackCjk;
    const char* fallbackSymbols;
    float preferredScale;
    float minScale;
    float maxScale;
};

inline UiFontChain uiFontChainForRole(UiFontRole role) {
    if (role == UI_FONT_ROLE_DISPLAY)
        return {"BR-Display-Condensed","Noto Sans","Noto Sans CJK JP","Noto Sans Symbols 2", UiTypography::kScaleTitle, 2.2f, 3.2f};
    if (role == UI_FONT_ROLE_HEADING)
        return {"BR-Display","Noto Sans","Noto Sans CJK JP","Noto Sans Symbols 2", UiTypography::kScaleSubtitle, 1.6f, 2.6f};
    if (role == UI_FONT_ROLE_META)
        return {"BR-UI-Text","Noto Sans","Noto Sans CJK JP","Noto Sans Symbols 2", UiTypography::kScaleMeta, 0.9f, 1.3f};
    if (role == UI_FONT_ROLE_ICON_FALLBACK)
        return {"BR-UI-Icons","Noto Sans Symbols 2","Noto Sans CJK JP","Noto Sans", UiTypography::kScaleHint, 1.0f, 1.8f};
    return {"BR-UI-Text","Noto Sans","Noto Sans CJK JP","Noto Sans Symbols 2", UiTypography::kScaleHint, 1.1f, 1.6f};
}

inline float uiClampTypographyScale(float requestedScale, UiFontRole role) {
    UiFontChain chain = uiFontChainForRole(role);
    float s = requestedScale;
    if (s < chain.minScale) s = chain.minScale;
    if (s > chain.maxScale) s = chain.maxScale;
    return s;
}

namespace UiIconography {
constexpr int kAtlasColumns = 8;
constexpr int kAtlasRows = 8;
constexpr float kMinReadableNdc = 0.018f;
constexpr float kMaxReadableNdc = 0.070f;
constexpr float kSdfSoftness = 0.10f;
constexpr float kDefaultPaddingPx = 2.0f;
}

// ── Color palette ──────────────────────────────────────────────────────────
// Cool desaturated base with warm amber accent — cinematic horror feel.
struct UiColorToken { float r, g, b; };

namespace UiColor {
// Text hierarchy — warm off-white that reads well on dark panels
constexpr UiColorToken kTextPrimary  {0.90f, 0.88f, 0.82f};
constexpr UiColorToken kTextSecondary{0.68f, 0.66f, 0.60f};
constexpr UiColorToken kTextMuted    {0.48f, 0.46f, 0.42f};
constexpr UiColorToken kTextHint     {0.42f, 0.40f, 0.36f};
// Overlay / panel background
constexpr UiColorToken kOverlayBase  {0.04f, 0.04f, 0.05f};
constexpr UiColorToken kOverlayWarm  {0.10f, 0.08f, 0.06f};
// Accent states — amber warning, red critical
constexpr UiColorToken kAccent       {0.82f, 0.62f, 0.28f}; // warm amber
constexpr UiColorToken kStateDefault {0.78f, 0.76f, 0.70f};
constexpr UiColorToken kStateWarning {0.90f, 0.68f, 0.28f};
constexpr UiColorToken kStateCritical{0.92f, 0.32f, 0.26f};
constexpr UiColorToken kStateDisabled{0.38f, 0.36f, 0.34f};
// Subtle tint for selection highlight
constexpr UiColorToken kSelectGlow   {0.60f, 0.48f, 0.22f};
}

// ── Alpha / depth ──────────────────────────────────────────────────────────
namespace UiDepth {
constexpr float kAlphaOverlayBase  = 0.72f;
constexpr float kAlphaOverlayWarm  = 0.18f;
constexpr float kAlphaTitleBase    = 0.94f;
constexpr float kAlphaTitlePulse   = 0.04f;
constexpr float kStateSelected     = 1.0f;
constexpr float kStateIdle         = 0.55f;
constexpr float kStateDefaultAlpha = 1.0f;
constexpr float kStateWarningAlpha = 0.95f;
constexpr float kStateCriticalAlpha= 0.98f;
constexpr float kStateDisabledAlpha= 0.40f;
constexpr float kMenuSubtitleAlpha = 0.70f;
constexpr float kMenuAtmosphereEdgeAlphaBase  = 0.06f;
constexpr float kMenuAtmosphereEdgeAlphaPulse = 0.02f;
constexpr float kAlphaHint = 0.55f;
constexpr float kAlphaMeta = 0.70f;
constexpr float kPanelAlpha = 0.82f;
constexpr float kPanelEdgeAlpha = 0.35f;
}

// ── Motion ──────────────────────────────────────────────────────────────────
namespace UiMotion {
constexpr float kPulseFastSec   = 0.18f;
constexpr float kPulseNormalSec = 0.30f;
constexpr float kPulseSlowSec  = 0.50f;
constexpr float kPulseTitleRate = 1.6f;
constexpr float kDriftAmbientRate = 0.5f;
}

inline bool gUiReducedMotion = false;

inline bool uiReducedMotionEnabled() {
    if (gUiReducedMotion) return true;
    const char* env = std::getenv("BR_UI_REDUCED_MOTION");
    return env && env[0] != '\0' && env[0] != '0';
}

inline float uiMotionAmplitude(float amount) {
    return uiReducedMotionEnabled() ? 0.0f : amount;
}

inline float uiMotionRate(float rate) {
    return uiReducedMotionEnabled() ? 0.0f : rate;
}

// ── WCAG contrast helpers ──────────────────────────────────────────────────
inline float uiLinearToSrgbChannel(float c) {
    if (c <= 0.03928f) return c / 12.92f;
    return powf((c + 0.055f) / 1.055f, 2.4f);
}

inline float uiRelativeLuminance(const UiColorToken& c) {
    return 0.2126f * uiLinearToSrgbChannel(c.r) + 0.7152f * uiLinearToSrgbChannel(c.g) + 0.0722f * uiLinearToSrgbChannel(c.b);
}

inline float uiContrastRatio(const UiColorToken& a, const UiColorToken& b) {
    float la = uiRelativeLuminance(a), lb = uiRelativeLuminance(b);
    float light = la > lb ? la : lb, dark = la > lb ? lb : la;
    return (light + 0.05f) / (dark + 0.05f);
}

inline bool uiContrastBaselinePassesAa() {
    return uiContrastRatio(UiColor::kTextPrimary, UiColor::kOverlayBase) >= 4.5f
        && uiContrastRatio(UiColor::kTextSecondary, UiColor::kOverlayBase) >= 4.5f
        && uiContrastRatio(UiColor::kStateWarning, UiColor::kOverlayBase) >= 3.0f;
}
