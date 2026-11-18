#pragma once
// Shared by every menu input handler: one snapshot of mouse and keys per frame (written
// into `ui`, see ui_core.h) and the navigation all list-shaped screens have in common.

#include <GLFW/glfw3.h>
#include "menu.h"
#include "audio.h"
#include "net.h"
#include "ui_widgets.h"
#include "menu_screens.h"
#include "menu_settings.h"
#include "menu_multi.h"
#include "lan_discovery.h"

extern bool escPressed, upPressed, downPressed, enterPressed, leftPressed, rightPressed;
extern bool firstMouse;
extern float lastX, lastY;

inline bool isAnyKeyboardKeyDown(GLFWwindow* w) {
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_MENU; k++) {
        if (glfwGetKey(w, k) == GLFW_PRESS) return true;
    }
    return false;
}

inline int firstPressedKeyboardKey(GLFWwindow* w) {
    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_MENU; k++) {
        if (glfwGetKey(w, k) == GLFW_PRESS) return k;
    }
    return -1;
}

inline void uiPollMouse(GLFWwindow* w, bool fresh) {
    static double lastCx = 0.0, lastCy = 0.0;
    double cx = 0.0, cy = 0.0;
    int ww = 1, wh = 1;
    glfwGetCursorPos(w, &cx, &cy);
    glfwGetWindowSize(w, &ww, &wh);
    if (ww < 1) ww = 1;
    if (wh < 1) wh = 1;
    ui.mx = ((float)cx / (float)ww * 2.0f - 1.0f) * uiHalfW();
    ui.my = (1.0f - (float)cy / (float)wh * 2.0f) * uiHalfH();
    bool down = glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    // Coming back from the game the button may still be held: that is not a click.
    ui.mouseMoved = !fresh && (fabs(cx - lastCx) + fabs(cy - lastCy) > 0.5);
    ui.mousePressed = !fresh && down && !ui.mouseDown;
    ui.mouseReleased = !fresh && !down && ui.mouseDown;
    ui.mouseDown = down;
    if (!down) ui.dragItem = -1;
    if (fresh) {
        ui.pressItem = -1;
        ui.pressBack = false;
    } else if (ui.mousePressed) {
        ui.pressItem = uiHot(UI_ZONE_ITEM);
        ui.pressBack = uiHot(UI_ZONE_BACK) >= 0;
    }
    lastCx = cx;
    lastCy = cy;
}

// Reads mouse and keys once per frame. `typing`: a text field has the keyboard, so letters
// and SPACE must not navigate.
inline void uiPollInput(GLFWwindow* w, bool typing) {
    static double lastPoll = -10.0;
    static bool tabHeld = false;
    static int holdDir = 0;
    static double nextRepeat = 0.0;
    const double now = glfwGetTime();
    uiPollMouse(w, now - lastPoll > 0.25);
    lastPoll = now;

    auto key = [&](int k) { return glfwGetKey(w, k) == GLFW_PRESS; };
    bool upNow = key(GLFW_KEY_UP) || (!typing && key(GLFW_KEY_W));
    bool downNow = key(GLFW_KEY_DOWN) || (!typing && key(GLFW_KEY_S));
    bool leftNow = key(GLFW_KEY_LEFT) || (!typing && key(GLFW_KEY_A));
    bool rightNow = key(GLFW_KEY_RIGHT) || (!typing && key(GLFW_KEY_D));
    bool enterNow = key(GLFW_KEY_ENTER) || key(GLFW_KEY_KP_ENTER) || (!typing && key(GLFW_KEY_SPACE));
    bool escNow = key(GLFW_KEY_ESCAPE);
    bool tabNow = key(GLFW_KEY_TAB);

    ui.up = upNow && !upPressed;
    ui.down = downNow && !downPressed;
    ui.left = leftNow && !leftPressed;
    ui.right = rightNow && !rightPressed;
    ui.enter = enterNow && !enterPressed;
    ui.esc = escNow && !escPressed;
    ui.tab = tabNow && !tabHeld;
    ui.shift = key(GLFW_KEY_LEFT_SHIFT) || key(GLFW_KEY_RIGHT_SHIFT);

    // left / right repeat while held, so sliders can be swept with the keyboard
    int dir = rightNow ? 1 : (leftNow ? -1 : 0);
    ui.adjust = 0;
    if (dir == 0) {
        holdDir = 0;
    } else if (dir != holdDir) {
        holdDir = dir;
        ui.adjust = dir;
        nextRepeat = now + 0.28;
    } else if (now >= nextRepeat) {
        ui.adjust = dir;
        nextRepeat = now + 0.055;
    }

    upPressed = upNow;
    downPressed = downNow;
    leftPressed = leftNow;
    rightPressed = rightNow;
    enterPressed = enterNow;
    escPressed = escNow;
    tabHeld = tabNow;
}

// Up / down with wrap-around, hover-to-select, ENTER and click. Returns true when the
// selected item (menuSel) was activated this frame. A click is a press and a release on the
// same item, so nothing fires while the button that confirmed it is still going down.
inline bool uiMenuNav(int count) {
    if (count <= 0) return false;
    if (menuSel < 0 || menuSel >= count) menuSel = 0;
    if (ui.up) {
        menuSel = (menuSel + count - 1) % count;
        triggerMenuNavigateSound();
    }
    if (ui.down) {
        menuSel = (menuSel + 1) % count;
        triggerMenuNavigateSound();
    }
    int hot = uiHot(UI_ZONE_ITEM);
    if (hot >= count) hot = -1;
    if (ui.mouseMoved && ui.dragItem < 0 && hot >= 0 && hot != menuSel) {
        menuSel = hot;
        triggerMenuNavigateSound();
    }
    if (ui.mousePressed && hot >= 0) menuSel = hot;
    if (ui.mouseReleased) {
        int pressed = ui.pressItem;
        ui.pressItem = -1;
        if (pressed >= 0 && pressed == hot) {
            menuSel = hot;
            return true;
        }
    }
    return ui.enter;
}

// ESC, or a click on the ESC hint in the bottom bar.
inline bool uiBackRequested() {
    bool clicked = ui.mouseReleased && ui.pressBack && uiHot(UI_ZONE_BACK) >= 0;
    if (ui.mouseReleased) ui.pressBack = false;
    return ui.esc || clicked;
}

inline void resumeGameFromMenu(GLFWwindow* w) {
    gameState = STATE_GAME;
    glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    firstMouse = true;
}
