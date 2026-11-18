#pragma once
// UI core: one canvas, one palette, one input snapshot for every menu screen.
//
// Canvas: virtual pixels, origin in the middle of the window, y up. The window is always at
// least 1280x720 of them (see uiScale in menu.h), so a layout written once fits any size.
// Drawing: flat and gradient quads, pixel-snapped text. Everything goes through ui.alpha,
// which is how screens fade in.
// Input: screens register click zones while they draw; the input code reads them on the next
// frame together with the mouse, the navigation keys and the typed characters.

#include <GLFW/glfw3.h>
#include "menu.h"

struct UiCol { float r, g, b; };

inline constexpr UiCol UI_BG = {0.030f, 0.029f, 0.027f};
inline constexpr UiCol UI_BG_LIT = {0.105f, 0.098f, 0.082f};
inline constexpr UiCol UI_LINE = {0.34f, 0.31f, 0.24f};
inline constexpr UiCol UI_ACCENT = {0.95f, 0.86f, 0.42f};
inline constexpr UiCol UI_TEXT = {0.90f, 0.87f, 0.74f};
inline constexpr UiCol UI_DIM = {0.58f, 0.55f, 0.45f};
inline constexpr UiCol UI_MUTE = {0.34f, 0.33f, 0.28f};
inline constexpr UiCol UI_GOOD = {0.56f, 0.93f, 0.56f};
inline constexpr UiCol UI_BAD = {0.97f, 0.43f, 0.36f};
inline constexpr UiCol UI_BLACK = {0.0f, 0.0f, 0.0f};

inline UiCol uiMix(UiCol a, UiCol b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t};
}

enum UiZoneKind {
    UI_ZONE_ITEM = 0, // a selectable row or button; id = item index
    UI_ZONE_TAB,      // id = tab index
    UI_ZONE_SLIDER,   // slider track; id = item index
    UI_ZONE_PREV,     // "<" of a choice row; id = item index
    UI_ZONE_NEXT,     // ">" of a choice row; id = item index
    UI_ZONE_BACK      // leaves the screen, same as ESC
};

struct UiZone {
    int kind, id;
    float l, b, r, t;
};

inline constexpr int UI_MAX_ZONES = 128;
inline constexpr int UI_MAX_ITEMS = 48;
inline constexpr int UI_MAX_CHARS = 32;
inline constexpr char UI_CHAR_BACKSPACE = '\b';

struct UiState {
    // drawing
    float alpha = 1.0f;   // multiplies every primitive
    float time = 0.0f;
    float dt = 0.016f;
    int screen = -1;      // gameState the animations belong to
    float screenAge = 0.0f;
    float itemGlow[UI_MAX_ITEMS] = {};

    // zones registered by the screen drawn last frame
    UiZone zones[UI_MAX_ZONES];
    int zoneCount = 0;
    int zoneScreen = -1;

    // mouse, canvas units
    float mx = 0.0f, my = 0.0f;
    bool mouseMoved = false, mouseDown = false, mousePressed = false, mouseReleased = false;
    int dragItem = -1;    // slider being dragged
    float dragL = 0.0f, dragR = 1.0f;
    int pressItem = -1;   // item the button went down on: a click is press + release on it
    bool pressBack = false;

    // keys: true on the frame the key went down
    bool up = false, down = false, left = false, right = false;
    bool enter = false, esc = false, tab = false, shift = false;
    int adjust = 0;       // left / right with auto-repeat while held: -1, 0, +1

    // typed characters since the last frame (backspace arrives as UI_CHAR_BACKSPACE)
    char chars[UI_MAX_CHARS];
    int charCount = 0;
};

inline UiState ui;

inline float uiHalfW() { return (float)currentWinW * 0.5f / uiScale(); }
inline float uiHalfH() { return (float)currentWinH * 0.5f / uiScale(); }
inline float uiNdcX(float x) { return x * uiScale() * 2.0f / (float)currentWinW; }
inline float uiNdcY(float y) { return y * uiScale() * 2.0f / (float)currentWinH; }

inline float uiClamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline float uiEase(float t) {
    t = uiClamp01(t);
    return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);
}

// Frame-rate independent "move towards target".
inline float uiApproach(float cur, float target, float speed) {
    float k = 1.0f - expf(-speed * ui.dt);
    return cur + (target - cur) * k;
}

inline float uiPulse(float speed) {
    return 0.5f + 0.5f * sinf(ui.time * speed);
}

// ---------------------------------------------------------------------------------------
// Quads

inline GLuint uiQuadShader = 0, uiQuadVAO = 0, uiQuadVBO = 0;

inline const char* uiQuadVS = R"(#version 330 core
layout(location=0) in vec2 p; layout(location=1) in vec4 c; out vec4 vc;
void main() { gl_Position = vec4(p, 0.0, 1.0); vc = c; })";
inline const char* uiQuadFS = R"(#version 330 core
in vec4 vc; out vec4 fc;
void main() { fc = vc; })";

inline void uiInitQuads() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &uiQuadVS, 0);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &uiQuadFS, 0);
    glCompileShader(fs);
    uiQuadShader = glCreateProgram();
    glAttachShader(uiQuadShader, vs);
    glAttachShader(uiQuadShader, fs);
    glLinkProgram(uiQuadShader);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &uiQuadVAO);
    glGenBuffers(1, &uiQuadVBO);
    glBindVertexArray(uiQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiQuadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 24, (void*)8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
}

// One colour, one alpha per corner: bottom-left, bottom-right, top-right, top-left.
inline void uiQuad(float l, float b, float r, float t, UiCol c, float aBL, float aBR, float aTR, float aTL) {
    if (r <= l || t <= b) return;
    if (!uiQuadShader) uiInitQuads();
    float x0 = uiNdcX(l), x1 = uiNdcX(r), y0 = uiNdcY(b), y1 = uiNdcY(t);
    float k = ui.alpha;
    float v[36] = {
        x0, y0, c.r, c.g, c.b, aBL * k,   x1, y0, c.r, c.g, c.b, aBR * k,   x1, y1, c.r, c.g, c.b, aTR * k,
        x0, y0, c.r, c.g, c.b, aBL * k,   x1, y1, c.r, c.g, c.b, aTR * k,   x0, y1, c.r, c.g, c.b, aTL * k
    };
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(uiQuadShader);
    glBindVertexArray(uiQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_CULL_FACE);
}

inline void uiRect(float l, float b, float r, float t, UiCol c, float a = 1.0f) {
    uiQuad(l, b, r, t, c, a, a, a, a);
}

// Horizontal gradient: alpha aL on the left edge, aR on the right.
inline void uiGradH(float l, float b, float r, float t, UiCol c, float aL, float aR) {
    uiQuad(l, b, r, t, c, aL, aR, aR, aL);
}

// Vertical gradient: alpha aB at the bottom, aT at the top.
inline void uiGradV(float l, float b, float r, float t, UiCol c, float aB, float aT) {
    uiQuad(l, b, r, t, c, aB, aB, aT, aT);
}

// 1-canvas-pixel-wide outline, drawn inside the rectangle.
inline void uiFrame(float l, float b, float r, float t, UiCol c, float a = 1.0f, float th = 1.0f) {
    uiRect(l, t - th, r, t, c, a);
    uiRect(l, b, r, b + th, c, a);
    uiRect(l, b + th, l + th, t - th, c, a);
    uiRect(r - th, b + th, r, t - th, c, a);
}

// Soft shadow around a rectangle.
inline void uiShadow(float l, float b, float r, float t, float size, float a) {
    uiGradV(l, b - size, r, b, UI_BLACK, 0.0f, a);
    uiGradV(l, t, r, t + size, UI_BLACK, a, 0.0f);
    uiGradH(l - size, b, l, t, UI_BLACK, 0.0f, a);
    uiGradH(r, b, r + size, t, UI_BLACK, a, 0.0f);
    uiQuad(l - size, b - size, l, b, UI_BLACK, 0.0f, 0.0f, a, 0.0f);
    uiQuad(r, b - size, r + size, b, UI_BLACK, 0.0f, 0.0f, 0.0f, a);
    uiQuad(r, t, r + size, t + size, UI_BLACK, a, 0.0f, 0.0f, 0.0f);
    uiQuad(l - size, t, l, t + size, UI_BLACK, 0.0f, a, 0.0f, 0.0f);
}

// ---------------------------------------------------------------------------------------
// Text. `size` 1.0 is an 8 px glyph cell; y is the vertical centre of the capitals.

inline float uiTextW(const char* s, float size) {
    return measureTextWidthNdc(s, size) / uiNdcX(1.0f);
}

inline void uiText(const char* s, float x, float y, float size, UiCol c, float a = 1.0f) {
    float cellNdc = textCellPx(size) * 2.0f / (float)currentWinH;
    drawText(s, uiNdcX(x), uiNdcY(y) - cellNdc * 0.4375f, size, c.r, c.g, c.b, a * ui.alpha);
}

inline void uiTextC(const char* s, float cx, float y, float size, UiCol c, float a = 1.0f) {
    uiText(s, cx - uiTextW(s, size) * 0.5f, y, size, c, a);
}

inline void uiTextR(const char* s, float right, float y, float size, UiCol c, float a = 1.0f) {
    uiText(s, right - uiTextW(s, size), y, size, c, a);
}

// Text over the 3D scene or the backdrop: a dark copy underneath keeps it readable.
inline void uiTextShadow(const char* s, float x, float y, float size, UiCol c, float a = 1.0f) {
    uiText(s, x + size, y - size, size, UI_BLACK, a * 0.75f);
    uiText(s, x, y, size, c, a);
}

// ---------------------------------------------------------------------------------------
// Frame, animation and zones

// Call once per frame before any screen is drawn.
inline void uiBeginFrame(float now, float dt) {
    ui.time = now;
    ui.dt = dt;
    ui.alpha = 1.0f;
    ui.zoneCount = 0;
    ui.zoneScreen = (int)gameState;
    ui.charCount = 0; // whatever the input code did not consume this frame is dropped
    if (ui.screen != (int)gameState) {
        ui.screen = (int)gameState;
        ui.screenAge = 0.0f;
        ui.dragItem = -1;
        ui.pressItem = -1;
        ui.pressBack = false;
        for (int i = 0; i < UI_MAX_ITEMS; i++) ui.itemGlow[i] = 0.0f;
    } else {
        ui.screenAge += dt;
    }
}

// 0 -> 1 over the first moments of a screen: panels fade and settle with it.
inline float uiScreenIn(float seconds = 0.22f) {
    return uiEase(ui.screenAge / seconds);
}

// Selection highlight of item `idx`, eased towards lit / unlit.
inline float uiItemGlow(int idx, bool lit) {
    if (idx < 0 || idx >= UI_MAX_ITEMS) return lit ? 1.0f : 0.0f;
    ui.itemGlow[idx] = uiApproach(ui.itemGlow[idx], lit ? 1.0f : 0.0f, lit ? 22.0f : 12.0f);
    return ui.itemGlow[idx];
}

inline void uiZone(int kind, int id, float l, float b, float r, float t) {
    if (ui.zoneCount >= UI_MAX_ZONES) return;
    ui.zones[ui.zoneCount++] = {kind, id, l, b, r, t};
}

// Is the mouse over this rectangle? For hover looks while a screen is being drawn; the
// zones of the current frame are not complete until the screen is.
inline bool uiMouseIn(float l, float b, float r, float t) {
    return ui.mx >= l && ui.mx <= r && ui.my >= b && ui.my <= t;
}

// The zone of `kind` under the mouse, or nullptr. For input code: it sees the zones of the
// screen drawn last frame. Zones of another screen never match.
inline const UiZone* uiHotZone(int kind) {
    if (ui.zoneScreen != (int)gameState) return nullptr;
    for (int i = ui.zoneCount - 1; i >= 0; i--) {
        const UiZone& z = ui.zones[i];
        if (z.kind == kind && ui.mx >= z.l && ui.mx <= z.r && ui.my >= z.b && ui.my <= z.t) return &z;
    }
    return nullptr;
}

inline int uiHot(int kind) {
    const UiZone* z = uiHotZone(kind);
    return z ? z->id : -1;
}

// ---------------------------------------------------------------------------------------
// Input

inline void uiPushChar(char c) {
    if (ui.charCount < UI_MAX_CHARS) ui.chars[ui.charCount++] = c;
}

inline void uiCharCallback(GLFWwindow*, unsigned int codepoint) {
    if (codepoint >= 32 && codepoint < 127) uiPushChar((char)codepoint);
}

inline void uiKeyCallback(GLFWwindow*, int key, int, int action, int) {
    if (key == GLFW_KEY_BACKSPACE && action != GLFW_RELEASE) uiPushChar(UI_CHAR_BACKSPACE);
}

// Text typed into fields comes through these (layout-aware, with key repeat).
inline void uiInstallCallbacks(GLFWwindow* w) {
    glfwSetCharCallback(w, uiCharCallback);
    glfwSetKeyCallback(w, uiKeyCallback);
}

// Applies the typed characters to `buf`. Returns true when the text changed.
inline bool uiEditText(char* buf, int cap, bool (*accept)(char)) {
    bool changed = false;
    int len = (int)strlen(buf);
    for (int i = 0; i < ui.charCount; i++) {
        char c = ui.chars[i];
        if (c == UI_CHAR_BACKSPACE) {
            if (len > 0) { buf[--len] = '\0'; changed = true; }
        } else if (len < cap - 1 && (!accept || accept(c))) {
            buf[len++] = c;
            buf[len] = '\0';
            changed = true;
        }
    }
    ui.charCount = 0;
    return changed;
}
