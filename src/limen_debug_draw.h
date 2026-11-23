#pragma once
// Part of limen_debug.h: menu input, cheats, drawing and the game shims.

struct LmDbgRepeat { bool held = false; double next = 0.0; };

// True on the press and then at the menu's key-repeat rate.
inline bool lmDbgPulse(LmDbgRepeat& r, bool down, bool repeat = true) {
    double now = glfwGetTime();
    if (!down) { r.held = false; return false; }
    if (!r.held) { r.held = true; r.next = now + 0.26; return true; }
    if (repeat && now >= r.next) { r.next = now + 0.06; return true; }
    return false;
}

inline void lmDbgClose() {
    gLmDbg.open = false;
    gLmDbg.swallowUntilRelease = true;   // the ESC that closed the menu must not open the pause screen
}

inline void lmDbgInput() {
    static LmDbgRepeat kToggle, kUp, kDown, kLeft, kRight, kEnter, kTab, kEsc;
    bool live = settings.debugMode && gameState == STATE_GAME && !isPlayerDead && !playerEscaped;
    bool toggle = lmDbgPulse(kToggle, lmDbgKeyDown(GLFW_KEY_F10), false);
    if (!live) {
        gLmDbg.open = false;
        gLmDbg.swallowUntilRelease = false;
        return;
    }
    if (gLmDbg.swallowUntilRelease) {
        bool any = false;
        for (int k = GLFW_KEY_ESCAPE; k <= GLFW_KEY_PAGE_DOWN; k++) if (lmDbgOwnsKey(k) && lmDbgKeyDown(k)) any = true;
        if (!any && !lmDbgKeyDown(GLFW_KEY_KP_ENTER)) gLmDbg.swallowUntilRelease = false;
    }
    if (toggle) {
        if (gLmDbg.open) lmDbgClose();
        else { gLmDbg.open = true; gLmDbg.levelPick = gCurrentLevel; }
        triggerMenuConfirmSound();
    }
    bool shift = lmDbgKeyDown(GLFW_KEY_LEFT_SHIFT) || lmDbgKeyDown(GLFW_KEY_RIGHT_SHIFT);
    bool up = lmDbgPulse(kUp, lmDbgKeyDown(GLFW_KEY_UP));
    bool down = lmDbgPulse(kDown, lmDbgKeyDown(GLFW_KEY_DOWN));
    bool left = lmDbgPulse(kLeft, lmDbgKeyDown(GLFW_KEY_LEFT));
    bool right = lmDbgPulse(kRight, lmDbgKeyDown(GLFW_KEY_RIGHT));
    bool enter = lmDbgPulse(kEnter, lmDbgKeyDown(GLFW_KEY_ENTER) || lmDbgKeyDown(GLFW_KEY_KP_ENTER), false);
    bool tabPrev = lmDbgKeyDown(GLFW_KEY_PAGE_UP) || (lmDbgKeyDown(GLFW_KEY_TAB) && shift);
    bool tabNext = lmDbgKeyDown(GLFW_KEY_PAGE_DOWN) || (lmDbgKeyDown(GLFW_KEY_TAB) && !shift);
    bool tab = lmDbgPulse(kTab, tabPrev || tabNext, false);
    bool esc = lmDbgPulse(kEsc, lmDbgKeyDown(GLFW_KEY_ESCAPE), false);
    if (!gLmDbg.open || toggle) return;

    if (esc) { lmDbgClose(); triggerMenuConfirmSound(); return; }
    if (tab) {
        gLmDbg.tab = (gLmDbg.tab + (tabPrev ? LM_DBG_TAB_COUNT - 1 : 1)) % LM_DBG_TAB_COUNT;
        triggerMenuNavigateSound();
    }
    const LmDbgTabDef& t = LM_DBG_TABS[gLmDbg.tab];
    int& sel = gLmDbg.sel[gLmDbg.tab];
    if (up != down) {
        sel = (sel + (up ? t.count - 1 : 1)) % t.count;
        triggerMenuNavigateSound();
    }
    int item = t.first + sel, kind = lmDbgKind(item);
    if (left != right) {
        if (kind == LM_DBG_STEPPER) {
            if (lmDbgStep(item, right ? 1 : -1, shift)) triggerMenuNavigateSound();
        } else if (kind == LM_DBG_SWITCH) {
            bool* v = lmDbgSwitchPtr(item);
            if (*v != right) { *v = right; triggerMenuNavigateSound(); }
        }
    }
    if (enter) {
        triggerMenuConfirmSound();
        if (kind == LM_DBG_SWITCH) { bool* v = lmDbgSwitchPtr(item); *v = !*v; }
        else if (item == DI_FLY_SPEED) { if (!lmDbgStep(item, 1, false)) gLmDbg.flySpeed = 0; }
        else lmDbgActivate(item);
    }
}

// Cheats that have to be held every frame.
inline void lmDbgCheats() {
    static Vec3 prev;
    static bool hasPrev = false;
    bool live = settings.debugMode && gameState == STATE_GAME && !isPlayerDead && !playerEscaped;
    if (!live) { hasPrev = false; return; }
    if (gLmDbg.god) {
        playerHealth = 100.0f; playerSanity = 100.0f;
        playerDowned = false; playerDownedTimer = 0.0f;
    }
    if (gLmDbg.infBattery) flashlightBattery = 100.0f;
    // The game flies at walking pace. Stretch this frame's own step; a jump too long to be a
    // step is a teleport and is left alone.
    float mul = LM_DBG_FLY_MUL[gLmDbg.flySpeed];
    if (debugTools.flyMode && mul > 1.0f && hasPrev) {
        Vec3 d = cam.pos - prev;
        float len = d.len();
        if (len > 1e-6f && len < 4.0f * 1.6f * 1.8f * dTime + 0.01f) cam.pos = cam.pos + d * (mul - 1.0f);
    }
    prev = cam.pos;
    hasPrev = true;
}

// ---------------------------------------------------------------- drawing

inline void lmDbgDrawMenu() {
    const LmDbgTabDef& t = LM_DBG_TABS[gLmDbg.tab];
    const float rowH = 26.0f, w = 400.0f, pad = 18.0f;
    int maxRows = 0;
    for (int k = 0; k < LM_DBG_TAB_COUNT; k++) if (LM_DBG_TABS[k].count > maxRows) maxRows = LM_DBG_TABS[k].count;
    float l = -uiHalfW() + 28.0f, r = l + w;
    float top = 262.0f, h = 40.0f + 34.0f + rowH * (float)maxRows + 62.0f, b = top - h;
    uiRect(l, b, r, top, UI_BLACK, 0.80f);
    uiShadow(l, b, r, top, 40.0f, 0.45f);

    float y = top - 22.0f;
    uiText("LIMEN DEBUG", l + pad, y, UI_ROW_TEXT, UI_TEXT);
    char buf[96];
    snprintf(buf, sizeof(buf), "L%d  SEED %u", gCurrentLevel, worldSeed);
    uiTextR(buf, r - pad, y, UI_SMALL_TEXT, UI_DIM, 0.7f);

    y -= 34.0f;
    float x = l + pad;
    for (int k = 0; k < LM_DBG_TAB_COUNT; k++) {
        bool cur = k == gLmDbg.tab;
        uiText(LM_DBG_TABS[k].name, x, y, UI_SMALL_TEXT, cur ? UI_ACCENT : UI_DIM, cur ? 1.0f : 0.7f);
        float tw = uiTextW(LM_DBG_TABS[k].name, UI_SMALL_TEXT);
        if (cur) uiRect(x, y - 11.0f, x + tw, y - 10.0f, UI_ACCENT, 0.9f);
        x += tw + 22.0f;
    }

    y -= 34.0f;
    for (int k = 0; k < t.count; k++) {
        int item = t.first + k, kind = lmDbgKind(item);
        bool selected = k == gLmDbg.sel[gLmDbg.tab];
        float ry = y - rowH * (float)k;
        if (selected) {
            uiRect(l, ry - rowH * 0.5f, r, ry + rowH * 0.5f, UI_ACCENT, 0.08f);
            uiRect(l, ry - 8.0f, l + 2.0f, ry + 8.0f, UI_ACCENT, 1.0f);
        }
        UiCol c = selected ? UI_ACCENT : UI_DIM;
        uiText(lmDbgLabel(item), l + pad, ry, UI_SMALL_TEXT, c);
        if (kind == LM_DBG_SWITCH) {
            bool on = *lmDbgSwitchPtr(item);
            uiTextR(on ? "ON" : "OFF", r - pad, ry, UI_SMALL_TEXT, on ? UI_GOOD : UI_MUTE);
        } else if (kind == LM_DBG_STEPPER) {
            lmDbgStepperValue(item, buf, sizeof(buf));
            float vr = r - pad - (selected ? 16.0f : 0.0f);
            uiTextR(buf, vr, ry, UI_SMALL_TEXT, selected ? UI_TEXT : UI_DIM);
            if (selected) {
                uiTextR(">", r - pad, ry, UI_SMALL_TEXT, UI_ACCENT);
                uiTextR("<", vr - uiTextW(buf, UI_SMALL_TEXT) - 8.0f, ry, UI_SMALL_TEXT, UI_ACCENT);
            }
        }
    }

    int item = t.first + gLmDbg.sel[gLmDbg.tab], kind = lmDbgKind(item);
    const char* hint = "ENTER RUN";
    if (kind == LM_DBG_SWITCH) hint = "ENTER TOGGLE";
    else if (item == DI_LEVEL) hint = "</> PICK (SHIFT X10)   ENTER LOAD";
    else if (item == DI_FLY_SPEED || item == DI_NAR_PACE) hint = "</> CHANGE";
    else if (item == DI_NAR_WISH) hint = "</> PICK   ENTER ASK FOR IT NOW";
    else if (kind == LM_DBG_STEPPER) hint = "</> PICK   ENTER GO TO NEAREST";
    uiText(hint, l + pad, b + 40.0f, UI_SMALL_TEXT, UI_DIM);
    uiText("TAB SECTION   F10 CLOSE   WASD STILL MOVES", l + pad, b + 18.0f, UI_SMALL_TEXT, UI_DIM, 0.7f);
}

inline void lmDbgDrawInfo() {
    static const char* geneNames[LG_COUNT] = {
        "HEIGHT", "MASS", "GRAIN", "ORDER", "CELLS", "LATTICE", "SINK", "ABYSS",
        "SHELL", "SPAN", "EDGE", "DECAY", "GRAFT", "LIGHT", "HUE", "USE"
    };
    static const char* flagNames[14] = {
        "LOW_WALL", "OVER_VOID", "OVER_BASIN", "THIN_W", "THIN_N", "DOOR_W", "DOOR_N",
        "NARROW_X", "NARROW_Z", "STAIR", "WALK_BASIN", "CRAMPED", "MEZZ", "CHAMFER"
    };
    static const char* zoneNames[4] = {"XENO", "PLUG_CORR", "ECHO_CORR", "STORY"};

    int wx = (int)floorf(cam.pos.x / CS), wz = (int)floorf(cam.pos.z / CS);
    LimenRect rc = lmSlabAt(wx, wz);
    const LimenSlab& s = lmSlab(rc.j, rc.i);
    int lx = wx - rc.x0 - 1, lz = wz - rc.z0 - 1;
    bool inside = lx >= 0 && lz >= 0 && lx < s.w && lz < s.h;
    int tag = lmTagAt(wx, wz);
    unsigned short flg = lmFlagAt(wx, wz);
    unsigned char zn = lmZoneAt(wx, wz);

    const float lineH = 17.0f, w = 396.0f, pad = 14.0f;
    const int textLines = 12, geneRows = LG_COUNT / 2;
    float r = uiHalfW() - 28.0f, l = r - w;
    float top = 196.0f, h = lineH * (float)(textLines + geneRows) + pad * 2.0f + 8.0f, b = top - h;
    uiRect(l, b, r, top, UI_BLACK, 0.80f);
    uiShadow(l, b, r, top, 40.0f, 0.45f);

    char buf[160];
    float y = top - pad - lineH * 0.5f;
    auto line = [&](UiCol c) { uiText(buf, l + pad, y, UI_SMALL_TEXT, c); y -= lineH; };

    snprintf(buf, sizeof(buf), "LEVEL %d   SEED %u", gCurrentLevel, worldSeed); line(UI_TEXT);
    snprintf(buf, sizeof(buf), "SPACE %s   LIGHT %s", lmSpacePoleName(gLimenDNA.spacePole), lmLightPoleName(gLimenDNA.lightPole)); lmDbgUpper(buf); line(UI_ACCENT);
    snprintf(buf, sizeof(buf), "QUIRK %s%s%s", lmQuirkName(gLimenDNA.quirk), gLimenDNA.quirk2 != LQ_NONE ? " + " : "",
             gLimenDNA.quirk2 != LQ_NONE ? lmQuirkName(gLimenDNA.quirk2) : ""); lmDbgUpper(buf); line(UI_ACCENT);
    snprintf(buf, sizeof(buf), "WALL %s   FLOOR %s   CEIL %s", lmWallPoleName(gLimenDNA.wallPole),
             lmFloorPoleName(gLimenDNA.floorPole), lmCeilPoleName(gLimenDNA.ceilPole)); lmDbgUpper(buf); line(UI_ACCENT);
    snprintf(buf, sizeof(buf), "POS %.1f %.1f %.1f   YAW %d", cam.pos.x, cam.pos.y, cam.pos.z,
             (int)lroundf(cam.yaw * 57.29578f) % 360); line(UI_DIM);
    snprintf(buf, sizeof(buf), "CELL %d %d   GROUND %.2f   %s", wx, wz, lmGroundHeightAt(cam.pos.x, cam.pos.z, CS), lmDbgTagName(tag));
    line(tag == LT_WALL ? UI_BAD : UI_ACCENT);

    int n = snprintf(buf, sizeof(buf), "FLAGS");
    for (int k = 0; k < 14; k++) if ((flg & (1u << k)) && n < 40) n += snprintf(buf + n, sizeof(buf) - n, " %s", flagNames[k]);
    if (!(flg & 0x3FFFu)) snprintf(buf + n, sizeof(buf) - n, " -");
    line(UI_DIM);
    n = snprintf(buf, sizeof(buf), "ZONE");
    for (int k = 0; k < 4; k++) if (zn & (1u << k)) n += snprintf(buf + n, sizeof(buf) - n, " %s", zoneNames[k]);
    if (!(zn & 0xFu)) n += snprintf(buf + n, sizeof(buf) - n, " -");
    if (inside) snprintf(buf + n, sizeof(buf) - n, "   NODE %u", (unsigned)s.nodeKey[lx][lz]);
    line(UI_DIM);

    char form[24];
    snprintf(form, sizeof(form), "%s", lmMegaFormName(s.megaForm));
    lmDbgUpper(form);
    snprintf(buf, sizeof(buf), "SLAB %d %d   %dx%d   DEPTH %d", rc.j, rc.i, s.w, s.h, lmDepth(rc.j, rc.i)); line(UI_TEXT);
    snprintf(buf, sizeof(buf), "FORM %s   STOREYS %d%s%s", form, s.ceilTiers, s.dark ? "   DARK" : "", s.fallback ? "   FALLBACK" : "");
    line(s.fallback ? UI_BAD : UI_DIM);
    snprintf(buf, sizeof(buf), "DELIRIUM %d/%d   SITES %d   LAMP PITCH %d", s.deliriumMean, s.deliriumPeak, s.siteCount, s.lampPitch); line(UI_DIM);
    snprintf(buf, sizeof(buf), "ROOM %d  HALL %d  CORR %d  PIT %d  WALL %d %%", s.share[LT_ROOM] + s.share[LT_LOW], s.share[LT_HALL],
             s.share[LT_CORR], lmPitShare(s), s.share[LT_WALL]); line(UI_DIM);
    snprintf(buf, sizeof(buf), "%.1f MS  %d FPS   LIGHTS %d  ENT %d  SLABS %d", gPerfFrameMs, (int)lroundf(gPerfFpsSmoothed),
             (int)lights.size(), (int)entityMgr.entities.size(), (int)gLimenSlabs.size()); line(UI_DIM);

    // Genes: two columns of name, bar, value.
    y -= 8.0f;
    float colW = (w - pad * 2.0f) * 0.5f;
    for (int g = 0; g < LG_COUNT; g++) {
        float gx = l + pad + colW * (float)(g / geneRows), gy = y - lineH * (float)(g % geneRows);
        int v = s.genome.g[g];
        uiText(geneNames[g], gx, gy, UI_SMALL_TEXT, UI_DIM);
        float bl = gx + 76.0f, br = gx + colW - 44.0f;
        uiRect(bl, gy - 3.0f, br, gy + 3.0f, UI_LINE, 0.45f);
        uiRect(bl, gy - 3.0f, bl + (br - bl) * (float)v / 255.0f, gy + 3.0f, UI_ACCENT, 0.85f);
        snprintf(buf, sizeof(buf), "%d", v);
        uiTextR(buf, gx + colW - 12.0f, gy, UI_SMALL_TEXT, UI_TEXT);
    }
}

// The narrator's journal: its state, what waits on this level, what it did last, and the whole
// catalogue (bright: witnessed on this floor, plain: witnessed before, dark: never).
inline void lmDbgDrawJournal() {
    const LmNarrator& N = gNar;
    const float lineH = 16.0f, w = 560.0f, pad = 14.0f;
    const int cols = 3, rowsCat = (LN_COUNT + cols - 1) / cols;
    const int headLines = 4, waitLines = 5, logLines = 5;
    float r = uiHalfW() - 28.0f, l = r - w;
    float h = lineH * (float)(headLines + waitLines + logLines + rowsCat + 3) + pad * 2.0f;
    float top = gLmDbg.info ? -84.0f : 262.0f, b = top - h;
    if (b < -uiHalfH() + 12.0f) { b = -uiHalfH() + 12.0f; top = b + h; }
    uiRect(l, b, r, top, UI_BLACK, 0.80f);
    uiShadow(l, b, r, top, 40.0f, 0.45f);
    char buf[200];
    float y = top - pad - lineH * 0.5f;
    auto line = [&](UiCol c, float a = 1.0f) { uiText(buf, l + pad, y, UI_SMALL_TEXT, c, a); y -= lineH; };

    snprintf(buf, sizeof(buf), "NARRATOR  %s", !N.enabled ? "OFF (WATCHING ONLY)" : (N.adaptive ? (N.rewrite ? "ADAPTIVE" : "ADAPTIVE, NO REWRITES") : "SCRIPT (CO-OP)"));
    line(N.enabled ? UI_TEXT : UI_BAD);
    snprintf(buf, sizeof(buf), "WITNESSED %d OF %d   MEGA %d/%d  STRANGE %d/%d  QUIET %d/%d  SPACE %d/%d  QUIRKS %d/%d", lmNarSeenCount(), (int)LN_COUNT,
             lmNarSeenCount(LNC_MEGA), lmNarClassSize(LNC_MEGA), lmNarSeenCount(LNC_STRANGE), lmNarClassSize(LNC_STRANGE),
             lmNarSeenCount(LNC_QUIET), lmNarClassSize(LNC_QUIET), lmNarSeenCount(LNC_SPACE), lmNarClassSize(LNC_SPACE),
             lmNarSeenCount(LNC_QUIRK), lmNarClassSize(LNC_QUIRK));
    line(UI_ACCENT);
    snprintf(buf, sizeof(buf), "FLOOR TIME %d:%02d   LAST BIG THING %ds AGO   BREATHER %ds   SPEED %.1f M/S   REWRITES %d", (int)N.time / 60, (int)N.time % 60,
             (int)N.sinceMajor, (int)N.rest, N.speed, N.rewrites);
    line(UI_DIM);
    {
        int seenSlabs = 0;
        for (const auto& kv : N.slabs) if (kv.second.entered || kv.second.seenCount > 0 || kv.second.glimpsed) seenSlabs++;
        int wx = (int)floorf(cam.pos.x / CS), wz = (int)floorf(cam.pos.z / CS);
        LimenRect rc = lmSlabAt(wx, wz);
        const LmNarSlab* rec = lmNarRecPeek(rc.j, rc.i);
        snprintf(buf, sizeof(buf), "SLABS LAID EYES ON %d   THIS SLAB: %d CELLS SEEN%s%s", seenSlabs, rec ? rec->seenCount : 0,
                 lmBeatAt(rc.j, rc.i) ? "   HAS A BEAT" : "", N.hasRoad ? "   ROAD KNOWN" : "");
        line(UI_DIM);
    }
    y -= lineH * 0.5f;
    // what waits, nearest first
    {
        struct W { const LmNarPlaced* p; float d; };
        W ws[64]; int n = 0;
        for (const LmNarPlaced& p : N.placed) {
            if (p.witnessed || n >= 64) continue;
            LimenRect q = lmSlabRect((int)(p.slab >> 32), (int)(p.slab & 0xFFFFFFFFll));
            float mx = (float)(q.x0 + q.x1) * 0.5f * CS - cam.pos.x, mz = (float)(q.z0 + q.z1) * 0.5f * CS - cam.pos.z;
            ws[n++] = {&p, sqrtf(mx * mx + mz * mz)};
        }
        std::sort(ws, ws + n, [](const W& a, const W& b) { return a.d < b.d; });
        snprintf(buf, sizeof(buf), "WAITING ON THIS FLOOR: %d", n); line(UI_TEXT);
        for (int k = 0; k < waitLines - 1; k++) {
            if (k < n) {
                snprintf(buf, sizeof(buf), "  %s   %d M", lmFeatInfo(ws[k].p->feat).name, (int)ws[k].d);
                lmDbgUpper(buf);
                line(ws[k].p->major ? UI_ACCENT : UI_DIM);
            } else y -= lineH;
        }
    }
    y -= lineH * 0.5f;
    for (int k = 0; k < logLines; k++) {
        int idx = N.logCount - logLines + k;
        if (idx >= 0 && idx < N.logCount) {
            snprintf(buf, sizeof(buf), "%3d:%02d  %s", (int)N.log[idx].t / 60, (int)N.log[idx].t % 60, N.log[idx].text);
            lmDbgUpper(buf);
            line(UI_DIM, 0.85f);
        } else y -= lineH;
    }
    y -= lineH * 0.5f;
    float colW = (w - pad * 2.0f) / (float)cols;
    for (int f = 0; f < LN_COUNT; f++) {
        float gx = l + pad + colW * (float)(f / rowsCat), gy = y - lineH * (float)(f % rowsCat);
        snprintf(buf, sizeof(buf), "%s", lmFeatInfo(f).name);
        lmDbgUpper(buf);
        bool now = (N.levelWitnessed & lmFeatBit(f)) != 0, ever = N.mem.count[f] != 0;
        uiText(buf, gx, gy, UI_SMALL_TEXT, now ? UI_GOOD : (ever ? UI_TEXT : UI_MUTE), now ? 1.0f : (ever ? 0.8f : 0.55f));
        if (N.levelPlaced[f] && !now) uiText("*", gx - 9.0f, gy, UI_SMALL_TEXT, UI_ACCENT);   // placed on this floor, not found yet
    }
}

inline void lmDbgDraw() {
    if (!settings.debugMode || gameState != STATE_GAME || isPlayerDead || playerEscaped) return;
    if (!gLmDbg.open && !gLmDbg.info && !gLmDbg.journal) return;
    float keepAlpha = ui.alpha;
    ui.alpha = 1.0f;
    if (gLmDbg.info) lmDbgDrawInfo();
    if (gLmDbg.journal) lmDbgDrawJournal();
    if (gLmDbg.open) lmDbgDrawMenu();
    ui.alpha = keepAlpha;
    glDisable(GL_BLEND);      // what the game's frame ends with
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

// ---------------------------------------------------------------- shims

// Every `glfwGetKey` of the game. F10 never reaches it, so its own panel stays shut.
void lmLegsUnlift();
int lmDbgGetKey(GLFWwindow* w, int key) {
    // The sprint key is polled once a frame, right before the legs are simulated: the body
    // goes back onto the simulation's own floor for that (game.cpp, lmLegsLift).
    if (key == settings.binds.sprint) lmLegsUnlift();
    if (key == GLFW_KEY_F10) return GLFW_RELEASE;
    if ((gLmDbg.open || gLmDbg.swallowUntilRelease) && lmDbgOwnsKey(key)) return GLFW_RELEASE;
    return glfwGetKey(w, key);
}

// The game's one `glfwSwapBuffers`: the frame and its UI are complete, nothing is mid-update.
void lmDbgSwapBuffers(GLFWwindow* w) {
    lmDbgInput();
    lmDbgCheats();
    lmDbgDraw();
    glfwSwapBuffers(w);
}
