#pragma once
// Dev self-test, UI part: draws every menu screen straight into the frame and saves it as
// build/selftest_ui_*.bmp. No input runs and no sockets open: the screens are only drawn.

// Draws one screen with its animations settled and saves the frame.
inline void selftestUiShot(const char* name, void (*draw)()) {
    ui.dt = 100.0f; // one giant step: every eased value lands on its target
    draw();
    selftestShot(name);
}

inline void selftestSeatPlayer(int seat, const char* name, bool active) {
    netMgr.players[seat].active = active;
    std::snprintf(netMgr.players[seat].name, sizeof(netMgr.players[seat].name), "%s", name);
}

// Screens that sit on the corridor backdrop. Call while the game is in the main menu.
inline void selftestMenuShots() {
    int keepSel = menuSel, keepTab = settingsTab;
    ui.screenAge = 10.0f; // past every fade-in

    menuSel = MAIN_MENU_MULTIPLAYER;
    selftestUiShot("ui_01_main_menu", [] { drawMenu(vhsTime); });

    settingsTab = SETTINGS_TAB_VIDEO;
    menuSel = 1;
    selftestUiShot("ui_02_settings_video", [] { drawSettings(false, vhsTime); });
    settingsTab = SETTINGS_TAB_EFFECTS;
    menuSel = 5;
    selftestUiShot("ui_03_settings_effects", [] { drawSettings(false, vhsTime); });
    settingsTab = SETTINGS_TAB_AUDIO;
    menuSel = 0;
    sliderInputActive = true;
    std::snprintf(sliderInputBuf, sizeof(sliderInputBuf), "45");
    selftestUiShot("ui_04_settings_audio_typing", [] { drawSettings(false, vhsTime); });
    sliderInputActive = false;
    sliderInputBuf[0] = '\0';
    settingsTab = SETTINGS_TAB_CONTROLS;
    menuSel = 7;
    keybindCaptureIndex = 6;
    selftestUiShot("ui_05_settings_controls_capture", [] { drawSettings(false, vhsTime); });
    keybindCaptureIndex = -1;

    menuSel = MULTI_ITEM_NICK;
    selftestUiShot("ui_06_multiplayer", [] { drawMultiMenuScreen(vhsTime); });
    menuSel = JOIN_ITEM_ADDRESS;
    selftestUiShot("ui_07_join", [] { drawJoinMenuScreen(vhsTime); });

    selftestSeatPlayer(0, "Mara", true);
    selftestSeatPlayer(1, "Otto", true);
    menuSel = HOST_ITEM_START;
    selftestUiShot("ui_08_host_lobby", [] { drawHostLobbyScreen(vhsTime, 2); });
    selftestUiShot("ui_09_waiting_room", [] { drawWaitingScreen(vhsTime); });
    selftestSeatPlayer(0, "", false);
    selftestSeatPlayer(1, "", false);

    menuSel = keepSel;
    settingsTab = keepTab;
}

// Screens that sit on top of a running game. They have no opaque body, so each one needs a
// freshly rendered frame under it: `which` 0..2, one per scripted step.
inline void selftestPauseShot(int which) {
    int keepSel = menuSel, keepTab = settingsTab;
    ui.screenAge = 10.0f;
    if (which == 0) {
        menuSel = 0;
        selftestUiShot("ui_10_pause", [] { drawPause(false, nullptr); });
    } else if (which == 1) {
        menuSel = 2;
        selftestUiShot("ui_11_pause_coop", [] { drawPause(true, "SQUAD 3/4"); });
    } else {
        settingsTab = SETTINGS_TAB_VIDEO;
        menuSel = 6;
        selftestUiShot("ui_12_settings_in_run", [] { drawSettings(true, vhsTime); });
    }
    menuSel = keepSel;
    settingsTab = keepTab;
}
