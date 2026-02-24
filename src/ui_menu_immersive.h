#pragma once

// ── Immersive menu rendering ────────────────────────────────────────────────
// All coordinates are in virtual 16:9 space (−1..1).
// The uiX()/uiY() mapping in the primitives handles aspect correction.

inline float uiImmersivePanelDrift(float tm, float phase, float amp = 0.008f) {
    return uiReducedMotionEnabled() ? 0.0f : sinf(tm * UiMotion::kDriftAmbientRate + phase) * amp;
}

inline void uiImmersiveDrawBackdrop(float tm, bool dimmed) {
    drawMainMenuBackdrop(tm);
    drawMenuAtmosphere(tm);
    drawFullscreenOverlay(UiColor::kOverlayBase.r, UiColor::kOverlayBase.g, UiColor::kOverlayBase.b, dimmed ? 0.76f : UiDepth::kAlphaOverlayBase);
    drawFullscreenOverlay(UiColor::kOverlayWarm.r, UiColor::kOverlayWarm.g, UiColor::kOverlayWarm.b, dimmed ? 0.16f : UiDepth::kAlphaOverlayWarm);
}

// Shared header: large title text + muted subtitle — no boxes
inline void uiImmersiveDrawHeader(const UiPrimitiveTheme& th, const char* title, const char* sub, bool warn = false) {
    UiColorToken tc = warn ? UiColor::kStateWarning : UiColor::kAccent;
    uiPrimitiveTextCentered(title, 0.0f, uiY(0.68f), UiTypography::kScaleTitle, tc, 0.85f, UI_FONT_ROLE_DISPLAY);
    if (sub && sub[0])
        uiPrimitiveTextCentered(sub, 0.0f, uiY(0.58f), UiTypography::kScaleMeta, th.mutedText, 0.48f, UI_FONT_ROLE_META);
}

// Shared footer: muted hint text at bottom — no box
inline void uiImmersiveDrawFooter(const UiPrimitiveTheme& th, const char* hint) {
    uiPrimitiveTextCentered(hint, 0.0f, uiY(-0.88f), UiTypography::kScaleMeta, th.mutedText, 0.38f, UI_FONT_ROLE_META);
}

// GL state wrappers
inline void uiImmersiveGlBegin() { glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
inline void uiImmersiveGlEnd()   { glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); }

// ── Main menu ───────────────────────────────────────────────────────────────
inline void uiImmersiveDrawMainMenu(float tm) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, false);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    char lvl[48]; buildLevelLabel(gCurrentLevel, lvl, 48);
    uiImmersiveDrawHeader(th, "VOID SHIFT", lvl, false);

    const char* items[] = {"START CONTRACT", "MULTIPLAYER", "SETTINGS", "GUIDE", "QUIT"};
    drawUiListPrimitive(th, -0.28f, -0.18f, 0.28f, 0.42f, "", items, 5, menuSel);

    uiImmersiveDrawFooter(th, "UP/DOWN   ENTER/A CONFIRM");

    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Pause ───────────────────────────────────────────────────────────────────
inline void uiImmersiveDrawPause(float tm, bool mp) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, true);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, mp ? "MISSION PAUSED" : "PAUSED", mp ? "CO-OP" : "SOLO", mp);

    const char* soloI[] = {"RESUME", "SETTINGS", "GUIDE", "MAIN MENU", "QUIT"};
    const char* mpI[]   = {"RESUME", "TELEPORT", "SETTINGS", "GUIDE", "DISCONNECT", "QUIT"};
    if (mp) drawUiListPrimitive(th, -0.24f, -0.28f, 0.24f, 0.38f, "", mpI, 6, menuSel);
    else    drawUiListPrimitive(th, -0.24f, -0.16f, 0.24f, 0.36f, "", soloI, 5, menuSel);

    uiImmersiveDrawFooter(th, "ESC/B RESUME   ENTER/A SELECT");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Settings ────────────────────────────────────────────────────────────────
inline const char* uiSettingsLabelForCurrentTab(int idx) {
    if (idx == 0) return "CATEGORY";
    if (settingsTab == SETTINGS_TAB_AUDIO) {
        const char* r[] = {"CATEGORY","MASTER VOL","MUSIC VOL","AMBIENCE VOL","SFX VOL","VOICE VOL","BACK"};
        if (idx >= 0 && idx < 7) return r[idx];
    } else if (settingsTab == SETTINGS_TAB_EFFECTS) {
        const char* r[] = {"CATEGORY","SSAO","GI","GOD RAYS","BLOOM","DENOISER","DENOISE STR","BACK"};
        if (idx >= 0 && idx < 8) return r[idx];
    } else if (settingsTab == SETTINGS_TAB_BINDS) {
        const char* r[] = {"CATEGORY","MOUSE SENS","OPEN BIND MENU","BACK"};
        if (idx >= 0 && idx < 4) return r[idx];
    } else {
        const char* r[] = {"CATEGORY","VHS EFFECT","UPSCALER","RESOLUTION","FSR SHARPNESS","ANTI-ALIASING","FAST MATH","FRAME GEN","V-SYNC","REDUCED MOTION","DEBUG MODE","BACK"};
        if (idx >= 0 && idx < 12) return r[idx];
    }
    return "OPTION";
}

inline void uiImmersiveDrawSettings(float tm, bool fromPause) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, fromPause);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, "SETTINGS", fromPause ? "IN-MISSION" : "CONFIGURATION", false);

    const char* tabL[] = {"VIDEO","EFFECTS","AUDIO","CONTROLS"};
    uiPrimitiveTextCentered(tabL[settingsTab], 0.0f, uiY(0.48f), UiTypography::kScaleHint, UiColor::kAccent, 0.70f, UI_FONT_ROLE_BODY);

    int cnt = settingsItemsForTab(settingsTab); if (cnt > 12) cnt = 12;
    const char* rows[12];
    for (int i = 0; i < cnt; ++i) rows[i] = uiSettingsLabelForCurrentTab(i);
    drawUiListPrimitive(th, -0.42f, -0.38f, 0.04f, 0.38f, "OPTIONS", rows, cnt, menuSel);

    drawUiInputPrimitive(th, 0.10f, 0.14f, 0.42f, 0.28f, "SELECTED", uiSettingsLabelForCurrentTab(menuSel), true);

    uiImmersiveDrawFooter(th, "LEFT/RIGHT ADJUST   TAB CATEGORY   ESC/B BACK");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Multiplayer Hub ─────────────────────────────────────────────────────────
inline void uiImmersiveDrawMultiplayerHub(float tm) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, false);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, "MULTIPLAYER", multiNetworkMode == 0 ? "LAN" : "PUBLIC", true);

    const char* opts[] = {"HOST GAME","JOIN GAME","BACK"};
    drawUiListPrimitive(th, -0.30f, -0.04f, 0.04f, 0.36f, "", opts, 3, menuSel);
    drawUiInputPrimitive(th, 0.10f, 0.18f, 0.40f, 0.32f, "OPERATIVE", multiNickname, multiEditingNickname);
    uiPrimitiveTextCentered(multiNetworkMode == 0 ? "MODE: LAN" : "MODE: SERVERS",
        uiX(0.25f), uiY(0.06f), UiTypography::kScaleMeta, UiColor::kAccent, 0.60f, UI_FONT_ROLE_META);

    uiImmersiveDrawFooter(th, "TAB/X EDIT   LEFT/RIGHT MODE   ENTER/A SELECT   ESC/B BACK");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Join ────────────────────────────────────────────────────────────────────
inline void uiImmersiveDrawMultiplayerJoin(float tm) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, false);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, "JOIN SESSION", multiNetworkMode == 0 ? "LAN" : "PUBLIC", true);

    const char* jo[] = {"CONNECT","BACK"};
    drawUiListPrimitive(th, -0.32f, 0.00f, -0.04f, 0.30f, "", jo, 2, menuSel);
    const char* ip   = multiNetworkMode == 0 ? multiJoinIP : multiMasterIP;
    const char* port = multiNetworkMode == 0 ? multiJoinPort : multiMasterPort;
    drawUiInputPrimitive(th, 0.04f, 0.16f, 0.38f, 0.30f, multiInputField == 0 ? "IP (ACTIVE)" : "IP", ip, multiInputField == 0);
    drawUiInputPrimitive(th, 0.04f, -0.02f, 0.38f, 0.10f, multiInputField == 1 ? "PORT (ACTIVE)" : "PORT", port, multiInputField == 1);
    if (multiConnectStatus[0])
        uiPrimitiveTextCentered(multiConnectStatus, 0.0f, uiY(-0.16f), UiTypography::kScaleMeta, UiColor::kStateWarning, 0.68f, UI_FONT_ROLE_META);

    uiImmersiveDrawFooter(th, "TAB/X FIELD   ENTER/A CONNECT   ESC/B BACK");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Host ────────────────────────────────────────────────────────────────────
inline void uiImmersiveDrawMultiplayerHost(float tm, int playerCount) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, false);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    char sub[80]; std::snprintf(sub, sizeof(sub), "PLAYERS %d/%d", playerCount, MAX_PLAYERS);
    uiImmersiveDrawHeader(th, "HOST LOBBY", sub, true);

    const char* ho[] = {"START GAME","BACK"};
    drawUiListPrimitive(th, -0.22f, 0.06f, 0.22f, 0.34f, "", ho, 2, menuSel);

    uiImmersiveDrawFooter(th, "ENTER/A START   ESC/B BACK");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Wait ────────────────────────────────────────────────────────────────────
inline void uiImmersiveDrawMultiplayerWait(float tm) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, false);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, netMgr.welcomeReceived ? "CONNECTED" : "CONNECTING",
                          netMgr.welcomeReceived ? "WAITING FOR HOST" : "HANDSHAKE", true);
    uiPrimitiveTextCentered(
        netMgr.welcomeReceived ? "WELCOME RECEIVED" : "PENDING...",
        0.0f, uiY(0.16f), UiTypography::kScaleHint,
        netMgr.welcomeReceived ? UiColor::kStateDefault : UiColor::kStateWarning, 0.68f, UI_FONT_ROLE_BODY);
    if (multiConnectStatus[0])
        uiPrimitiveTextCentered(multiConnectStatus, 0.0f, uiY(0.04f), UiTypography::kScaleMeta, UiColor::kStateWarning, 0.58f, UI_FONT_ROLE_META);

    uiImmersiveDrawFooter(th, "ESC/B DISCONNECT");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Guide ───────────────────────────────────────────────────────────────────
inline void uiImmersiveDrawGuide(float tm) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, true);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, "FIELD GUIDE", "SURVIVAL PROTOCOL", false);

    float x = uiX(-0.38f), y = uiY(0.36f), step = uiH(0.09f);
    uiPrimitiveText("COMPLETE CONTRACTS, STABILIZE, EXTRACT", x, y, UiTypography::kScaleHint, UiColor::kStateDefault, 0.70f, UI_FONT_ROLE_BODY);
    uiPrimitiveText("RESONATOR: SCAN / RECORD / PLAYBACK", x, y - step, UiTypography::kScaleHint, UiColor::kStateWarning, 0.62f, UI_FONT_ROLE_BODY);
    uiPrimitiveText("ATTENTION RISES FROM NOISE AND LIGHT", x, y - step*2, UiTypography::kScaleHint, UiColor::kStateCritical, 0.62f, UI_FONT_ROLE_BODY);
    uiPrimitiveText("L1: 3 NODES   L2: BATTERY + FUSES + LIFT", x, y - step*3, UiTypography::kScaleHint, th.text, 0.58f, UI_FONT_ROLE_BODY);
    uiPrimitiveText("NPCS: CARTOGRAPHER / DISPATCHER / SURVIVOR", x, y - step*4, UiTypography::kScaleHint, th.text, 0.58f, UI_FONT_ROLE_BODY);
    uiPrimitiveText("WASD MOVE   SHIFT RUN   C CROUCH   E USE", x, y - step*5, UiTypography::kScaleHint, th.mutedText, 0.50f, UI_FONT_ROLE_BODY);

    uiImmersiveDrawFooter(th, "ESC/B BACK");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Keybinds ────────────────────────────────────────────────────────────────
inline void uiImmersiveDrawKeybinds(float tm, bool fromPause, int selected, int captureIndex) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, fromPause);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, "KEY BINDS", fromPause ? "IN-MISSION" : "INPUT MAP", false);

    drawUiPanelPrimitive(th, -0.46f, -0.40f, 0.46f, 0.44f, "BINDS", UI_PRIMITIVE_TONE_DEFAULT);
    const int leftCount = (GAMEPLAY_BIND_COUNT + 1) / 2;
    for (int i = 0; i < GAMEPLAY_BIND_COUNT; ++i) {
        bool leftCol = i < leftCount;
        int row = leftCol ? i : (i - leftCount);
        float rowTop = 0.34f - row * 0.08f;
        float rowBot = rowTop - 0.06f;
        float cL = leftCol ? -0.42f : 0.02f;
        float cR = leftCol ? -0.02f : 0.42f;
        UiPrimitiveTone tone = (selected == i) ? UI_PRIMITIVE_TONE_WARNING : UI_PRIMITIVE_TONE_DEFAULT;
        char lbl[96];
        const char* kn = keyNameForUi(*gameplayBindByIndex(settings.binds, i));
        if (captureIndex == i) kn = "...";
        std::snprintf(lbl, sizeof(lbl), "%s : %s", gameplayBindLabel(i), kn);
        drawUiStatusIndicatorPrimitive(th, cL, rowBot, cR, rowTop, lbl, tone);
    }
    UiColorToken bc = (selected == KEYBINDS_BACK_INDEX) ? UiColor::kStateWarning : UiColor::kStateDefault;
    uiPrimitiveTextCentered("BACK", 0.0f, uiY(-0.52f), UiTypography::kScaleHint, bc, 0.76f, UI_FONT_ROLE_BODY);

    uiImmersiveDrawFooter(th, captureIndex >= 0 ? "PRESS KEY TO REBIND" : "ENTER/A REBIND   ESC/B BACK");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Intro / Briefing ────────────────────────────────────────────────────────
inline void uiImmersiveDrawIntro(float tm, int line, const char** introLines) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, false);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, "ARCHIVE UPLINK", "BRIEFING", false);
    if (line >= 0 && line < INTRO_LINE_COUNT)
        drawUiModalPrimitive(th, "TRANSMISSION", introLines[line], UI_PRIMITIVE_TONE_WARNING);
    else
        drawUiModalPrimitive(th, "TRANSMISSION", "LINK STABILIZING", UI_PRIMITIVE_TONE_DEFAULT);

    int ls = line + 1; if (ls < 1) ls = 1; if (ls > INTRO_LINE_COUNT) ls = INTRO_LINE_COUNT;
    char prog[64]; std::snprintf(prog, sizeof(prog), "LINE %d/%d", ls, INTRO_LINE_COUNT);
    uiPrimitiveTextCentered(prog, 0.0f, uiY(-0.22f), UiTypography::kScaleMeta, th.mutedText, 0.48f, UI_FONT_ROLE_META);

    uiImmersiveDrawFooter(th, "SPACE  ENTER/A SKIP");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Multiline text helper ───────────────────────────────────────────────────
inline float uiImmersiveDrawMultilineRows(float left, float top, const char* content, float rowStep, float maxBottom) {
    if (!content || !*content) return top;
    char buf[96]; int len = 0; float rowY = top;
    for (const char* p = content;; ++p) {
        bool flush = (*p == '\n' || *p == '\0' || len >= 92);
        if (!flush) { buf[len++] = *p; continue; }
        buf[len] = '\0';
        uiPrimitiveText(buf, uiX(left), uiY(rowY), UiTypography::kScaleMeta, UiColor::kTextPrimary, 0.90f, UI_FONT_ROLE_META);
        rowY -= rowStep; len = 0;
        if (rowY < maxBottom || *p == '\0') break;
    }
    return rowY;
}

// ── Note viewer ─────────────────────────────────────────────────────────────
inline void uiImmersiveDrawNote(float tm, int noteId, const char* title, const char* content) {
    uiImmersiveGlBegin();
    uiImmersiveDrawBackdrop(tm, true);
    UiPrimitiveTheme th = makeUiPrimitiveTheme();
    uiPrimitiveBeginFrame(tm);

    uiImmersiveDrawHeader(th, "ARCHIVE NOTE", "RECOVERED RECORD", false);
    drawUiPanelPrimitive(th, -0.40f, -0.38f, 0.40f, 0.38f, title && title[0] ? title : "UNTITLED", UI_PRIMITIVE_TONE_DEFAULT);
    if (noteId >= 0) {
        char meta[64]; std::snprintf(meta, sizeof(meta), "NOTE %d/%d", noteId + 1, 12);
        uiPrimitiveTextCentered(meta, 0.0f, uiY(0.26f), UiTypography::kScaleMeta, UiColor::kAccent, 0.55f, UI_FONT_ROLE_META);
    }
    uiImmersiveDrawMultilineRows(-0.36f, 0.18f, content ? content : "", 0.08f, -0.30f);

    uiImmersiveDrawFooter(th, "E OR ESC/B CLOSE");
    uiPrimitiveEndFrame();
    uiImmersiveGlEnd();
}

// ── Dispatcher ──────────────────────────────────────────────────────────────
inline bool drawUiImmersiveMenuLayer(float tm) {
    if (gameState == STATE_MENU)   { uiImmersiveDrawMainMenu(tm); return true; }
    if (gameState == STATE_PAUSE)  { uiImmersiveDrawPause(tm, multiState == MULTI_IN_GAME); return true; }
    if (gameState == STATE_SETTINGS || gameState == STATE_SETTINGS_PAUSE) { uiImmersiveDrawSettings(tm, gameState == STATE_SETTINGS_PAUSE); return true; }
    if (gameState == STATE_MULTI)      { uiImmersiveDrawMultiplayerHub(tm); return true; }
    if (gameState == STATE_MULTI_JOIN) { uiImmersiveDrawMultiplayerJoin(tm); return true; }
    if (gameState == STATE_MULTI_HOST) { uiImmersiveDrawMultiplayerHost(tm, netMgr.getPlayerCount()); return true; }
    if (gameState == STATE_MULTI_WAIT) { uiImmersiveDrawMultiplayerWait(tm); return true; }
    if (gameState == STATE_GUIDE)      { uiImmersiveDrawGuide(tm); return true; }
    if (gameState == STATE_KEYBINDS || gameState == STATE_KEYBINDS_PAUSE) { uiImmersiveDrawKeybinds(tm, gameState == STATE_KEYBINDS_PAUSE, menuSel, keybindCaptureIndex); return true; }
    if (gameState == STATE_INTRO) { uiImmersiveDrawIntro(tm, storyMgr.introLine, INTRO_LINES); return true; }
    if (gameState == STATE_NOTE && storyMgr.readingNote && storyMgr.currentNote >= 0) {
        uiImmersiveDrawNote(tm, storyMgr.currentNote, NOTE_TITLES[storyMgr.currentNote], NOTE_CONTENTS[storyMgr.currentNote]);
        return true;
    }
    return false;
}