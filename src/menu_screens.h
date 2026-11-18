#pragma once
// Main menu and pause menu. Input lives in input_menu.h; the item order here and there
// must match (MAIN_MENU_* / PAUSE_* below are the single source for both).

#include "ui_widgets.h"

enum MainMenuItem {
    MAIN_MENU_START = 0,
    MAIN_MENU_MULTIPLAYER,
    MAIN_MENU_SETTINGS,
    MAIN_MENU_TUTORIAL,
    MAIN_MENU_QUIT,
    MAIN_MENU_COUNT
};

// Pause entries, solo and co-op. Co-op adds "teleport" and turns "main menu" into "disconnect".
enum PauseItem {
    PAUSE_RESUME = 0,
    PAUSE_TELEPORT,
    PAUSE_SETTINGS,
    PAUSE_GUIDE,
    PAUSE_LEAVE,
    PAUSE_QUIT,
    PAUSE_ITEM_KINDS
};

inline int pauseItemCount(bool multiplayer) {
    return multiplayer ? 6 : 5;
}

// Which entry sits on row `row` of the pause menu.
inline int pauseItemAt(bool multiplayer, int row) {
    static const int solo[5] = {PAUSE_RESUME, PAUSE_SETTINGS, PAUSE_GUIDE, PAUSE_LEAVE, PAUSE_QUIT};
    if (row < 0 || row >= pauseItemCount(multiplayer)) return PAUSE_RESUME;
    return multiplayer ? row : solo[row];
}

inline int pauseRowOf(bool multiplayer, int item) {
    for (int i = 0; i < pauseItemCount(multiplayer); i++) {
        if (pauseItemAt(multiplayer, i) == item) return i;
    }
    return 0;
}

inline const char* pauseItemLabel(bool multiplayer, int item) {
    switch (item) {
        case PAUSE_RESUME: return "RESUME";
        case PAUSE_TELEPORT: return "TELEPORT TO SQUAD";
        case PAUSE_SETTINGS: return "SETTINGS";
        case PAUSE_GUIDE: return "KEY SHEET";
        case PAUSE_LEAVE: return multiplayer ? "DISCONNECT" : "MAIN MENU";
        default: return "QUIT TO DESKTOP";
    }
}

inline void uiBeginScreen() {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

inline void uiEndScreen() {
    ui.alpha = 1.0f;
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

inline void drawMenu(float tm) {
    static const char* labels[MAIN_MENU_COUNT] = {"START SHIFT", "MULTIPLAYER", "SETTINGS", "TUTORIAL", "QUIT"};
    uiBeginScreen();
    uiMenuBackdrop(tm, 0.10f);
    float hw = uiHalfW(), hh = uiHalfH();
    uiGradH(-hw, -hh, -hw + 660.0f, hh, UI_BLACK, 0.70f, 0.0f);

    float in = uiScreenIn(0.35f);
    float x = -hw + 92.0f - (1.0f - in) * 26.0f;
    ui.alpha = in;

    // title block: the lamp behind it flickers now and then
    float flicker = 0.90f + 0.10f * sinf(tm * 2.1f);
    if (fmodf(tm, 7.3f) < 0.09f) flicker *= 0.55f;
    uiText("BACKROOMS", x, 120.0f, 4.0f, UI_TEXT, flicker);

    const float top = 40.0f, step = 36.0f;
    for (int i = 0; i < MAIN_MENU_COUNT; i++) {
        uiListItem(i, labels[i], x - 18.0f, x + 340.0f, top - step * (float)i, 32.0f, menuSel == i, UI_ROW_TEXT);
    }

    ui.alpha = 1.0f;
    uiTextR("v1.0.0", hw - 40.0f, -hh + UI_HINT_BAR_H * 0.5f, UI_SMALL_TEXT, UI_MUTE);
    uiEndScreen();
}

// `note`: small line in the panel header (squad size in co-op).
inline void drawPause(bool multiplayer, const char* note) {
    uiBeginScreen();
    float in = uiScreenIn(0.16f);
    drawFullscreenOverlay(0.01f, 0.01f, 0.015f, 0.62f * in);
    uiVignette(0.5f * in);
    ui.alpha = in;

    const float w = 360.0f, rowH = 36.0f;
    int count = pauseItemCount(multiplayer);
    float height = UI_HEADER_H + 20.0f + rowH * (float)count + 18.0f;
    float t = height * 0.5f + 24.0f - (1.0f - in) * 10.0f, b = t - height;
    float l = -w * 0.5f, r = w * 0.5f;
    uiPanel(l, b, r, t);
    float y = uiPanelHeader(l, r, t, "PAUSED", note) - 20.0f - rowH * 0.5f;
    for (int i = 0; i < count; i++) {
        const char* label = pauseItemLabel(multiplayer, pauseItemAt(multiplayer, i));
        uiListItem(i, label, l + 10.0f, r - 14.0f, y - rowH * (float)i, rowH - 4.0f, menuSel == i, UI_ROW_TEXT);
    }

    ui.alpha = 1.0f;
    static const UiHint hints[] = {{"ESC", "RESUME"}};
    uiHintBar(hints, 1);
    uiEndScreen();
}
