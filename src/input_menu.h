#pragma once
// Main menu, key sheet and pause menu input, and the dispatch to the multiplayer screens.
// Item order: menu_screens.h.

#include "progression.h"

inline void startSoloRun(bool tutorial) {
    extern void genWorld();
    extern void buildGeom();
    gCurrentLevel = 0;
    // veterans start at the drill; the tutorial entry is the first shift again
    gHeistSkipTutorial = tutorial ? false : gHeistTutorialSeen;
    genWorld();
    buildGeom();
    gameState = STATE_INTRO;
}

inline void mainMenuInput(GLFWwindow* w) {
    uiPollInput(w, false);
    if (!uiMenuNav(MAIN_MENU_COUNT)) return;
    triggerMenuConfirmSound();
    if (menuSel == MAIN_MENU_START) {
        startSoloRun(false);
    } else if (menuSel == MAIN_MENU_MULTIPLAYER) {
        gameState = STATE_MULTI;
        menuSel = MULTI_ITEM_HOST;
    } else if (menuSel == MAIN_MENU_SETTINGS) {
        settingsTab = SETTINGS_TAB_VIDEO;
        gameState = STATE_SETTINGS;
        menuSel = 0;
    } else if (menuSel == MAIN_MENU_TUTORIAL) {
        startSoloRun(true);
    } else {
        glfwSetWindowShouldClose(w, 1);
    }
}

inline void guideInput(GLFWwindow* w) {
    uiPollInput(w, false);
    bool clicked = ui.mouseReleased;
    if (!ui.esc && !ui.enter && !clicked) return;
    triggerMenuConfirmSound();
    if (guideReturnToPause) {
        gameState = STATE_PAUSE;
        menuSel = pauseRowOf(multiState == MULTI_IN_GAME, PAUSE_GUIDE);
    } else {
        gameState = STATE_MENU;
        menuSel = MAIN_MENU_TUTORIAL;
    }
}

inline void pauseInput(GLFWwindow* w) {
    bool multiplayer = multiState == MULTI_IN_GAME;
    uiPollInput(w, false);
    bool activate = uiMenuNav(pauseItemCount(multiplayer));
    if (uiBackRequested()) {
        triggerMenuConfirmSound();
        resumeGameFromMenu(w);
        return;
    }
    if (!activate) return;
    triggerMenuConfirmSound();
    int item = pauseItemAt(multiplayer, menuSel);
    if (item == PAUSE_RESUME) {
        resumeGameFromMenu(w);
    } else if (item == PAUSE_TELEPORT) {
        extern void teleportToPlayer();
        teleportToPlayer();
        resumeGameFromMenu(w);
    } else if (item == PAUSE_SETTINGS) {
        settingsTab = SETTINGS_TAB_VIDEO;
        gameState = STATE_SETTINGS_PAUSE;
        menuSel = 0;
    } else if (item == PAUSE_GUIDE) {
        guideReturnToPause = true;
        gameState = STATE_GUIDE;
    } else if (item == PAUSE_LEAVE) {
        if (multiplayer) {
            closeMultiSession();
        } else {
            extern void genWorld();
            extern void buildGeom();
            genWorld();
            buildGeom();
        }
        gameState = STATE_MENU;
        menuSel = 0;
    } else {
        glfwSetWindowShouldClose(w, 1);
    }
}

inline void menuInput(GLFWwindow* w) {
    if (gameState == STATE_MENU) mainMenuInput(w);
    else if (gameState == STATE_GUIDE) guideInput(w);
    else if (gameState == STATE_PAUSE) pauseInput(w);
    else if (gameState == STATE_MULTI) multiMenuInput(w);
    else if (gameState == STATE_MULTI_HOST) hostLobbyInput(w);
    else if (gameState == STATE_MULTI_JOIN) joinMenuInput(w);
    else if (gameState == STATE_MULTI_WAIT) waitingRoomInput(w);
}
