// Drive the real settings input handler, including GLFW edge detection and click
// consumption. Substitute only the physical device and window queries.
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cassert>

static int heldKey = -1, mouseButton = GLFW_RELEASE;
static double cursorX = 640, cursorY = 360, now = 1.0;

static void windowSize(GLFWwindow*, int* width, int* height) {
    *width = 1280;
    *height = 720;
}
static void cursorPosition(GLFWwindow*, double* x, double* y) {
    *x = cursorX;
    *y = cursorY;
}
#define glfwGetWindowSize windowSize
#define glfwGetCursorPos cursorPosition
#define glfwGetKey(window, key) ((key) == heldKey ? GLFW_PRESS : GLFW_RELEASE)
#define glfwGetMouseButton(window, button) (mouseButton)
#define glfwGetTime() (now)
#define LIMEN_SHOTS
#define main settings_test_unused_client_main
#include "../src/game_core.h"
#undef main

static void tick() {
    now += 0.016;
    settingsInput(nullptr, false);
}
static void tap(int key) {
    heldKey = key;
    tick();
    heldKey = -1;
    tick();
}

int main() {
    gameState = STATE_SETTINGS;
    settingsTab = SETTINGS_TAB_VIDEO;
    for (menuSel = 0; settingsRowAt(settingsTab, menuSel).id != SET_AA; ++menuSel)
        assert(menuSel < settingsRowCount(settingsTab));
    tick();
    settings.aaMode = 0;
    tap(GLFW_KEY_LEFT);
    assert(settings.aaMode == AA_MODE_COUNT - 1);
    tap(GLFW_KEY_RIGHT);
    assert(settings.aaMode == 0);

    ui.zoneScreen = gameState;
    uiZone(UI_ZONE_ITEM, menuSel, -100, -10, 100, 10);
    uiZone(UI_ZONE_PREV, menuSel, -100, -10, -50, 10);
    uiZone(UI_ZONE_NEXT, menuSel, 50, -10, 100, 10);
    for (int side : {-1, 1}) {
        cursorX = 640.0 + side * 75.0 * 640.0 / uiHalfW();
        mouseButton = GLFW_PRESS;
        tick();
        const int expected = side < 0 ? AA_MODE_COUNT - 1 : 0;
        assert(settings.aaMode == expected);
        mouseButton = GLFW_RELEASE;
        tick();
        assert(settings.aaMode == expected);
    }

    settingsTab = SETTINGS_TAB_AUDIO;
    menuSel = 0;
    settings.masterVol = 1.f;
    tap(GLFW_KEY_RIGHT);
    assert(settings.masterVol == 1.f);
    settings.masterVol = 0.f;
    tap(GLFW_KEY_LEFT);
    assert(settings.masterVol == 0.f);
}
