#pragma once
// Full-screen pictogram pages: the run report after an extraction, the death screen and
// the key sheet (pause menu -> guide). Names and numbers only, no sentences.

#include "squad_play.h"
#include "hud_heist.h"

inline void beginHudPage(float r, float g, float b, float a) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawFullscreenOverlay(r, g, b, a);
}

inline void endHudPage() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// [key] + icon at the bottom of a page.
inline void drawPageKey(const char* key, int icon, float cx, HudColor c) {
    drawKeyCap(key, cx - 0.05f, -0.78f, 0.07f, 0.9f);
    drawIconC(icon, cx + 0.05f, -0.78f, 0.08f, c, 0.9f);
}

inline void drawReportStat(int icon, int value, float cx, float y, HudColor c) {
    drawIconC(icon, cx - 0.05f, y, 0.10f, c);
    drawHudNumber(value, cx, y - 0.028f, 2.3f, c.r, c.g, c.b);
}

inline int awardIcon(int category) {
    static const int icons[SHIFT_AWARD_CATEGORIES] = {
        HUD_ICON_LOOT, HUD_ICON_REVIVE, HUD_ICON_DOWNED, HUD_ICON_HAND, HUD_ICON_NOISE, HUD_ICON_STAMINA
    };
    return (category >= 0 && category < SHIFT_AWARD_CATEGORIES) ? icons[category] : HUD_ICON_PERSON;
}

// Co-op: who hauled, who revived, who lay on the floor, who shoved, who squeaked, who ran.
inline void drawShiftAwards(float y) {
    bool active[MAX_PLAYERS];
    for (int p = 0; p < MAX_PLAYERS; p++) active[p] = netMgr.players[p].active;
    ShiftAward awards[SHIFT_AWARD_CATEGORIES];
    int count = pickShiftAwards(netMgr.squadStats, active, MAX_PLAYERS, awards);
    float colW = 0.30f, x0 = -colW * 0.5f * (float)(count - 1);
    for (int i = 0; i < count; i++) {
        float cx = x0 + colW * (float)i;
        drawIconC(awardIcon(awards[i].category), cx, y, 0.11f, HUD_PAPER);
        drawTextCentered(squadPlayerName(awards[i].playerId), cx, y - 0.11f, 1.3f, 0.92f, 0.86f, 0.52f, 0.94f);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", awards[i].value);
        drawTextCentered(buf, cx, y - 0.17f, 1.2f, 0.66f, 0.74f, 0.64f, 0.85f);
    }
}

inline void drawSoloShiftCounters(float y) {
    const ShiftStats& s = myShiftStats();
    drawReportStat(HUD_ICON_LOOT, (int)s.work, -0.36f, y, HUD_PAPER);
    drawReportStat(HUD_ICON_DOWNED, (int)s.downs, -0.10f, y, HUD_PAPER);
    drawReportStat(HUD_ICON_STAMINA, (int)s.meters, 0.16f, y, HUD_PAPER);
    drawReportStat(HUD_ICON_NOISE, (int)s.squeaks, 0.42f, y, HUD_PAPER);
}

// The rivals winched the squad out: up arrow, what was left in the wallet, how deep, how long.
inline void drawShiftReportScreen(float tm) {
    beginHudPage(0.02f, 0.03f, 0.02f, 0.70f);
    float pulse = 0.80f + 0.15f * sinf(tm * 2.4f);
    drawIcon(HUD_ICON_EXTRACT, 0.0f, 0.62f, 0.30f, HUD_GOOD.r, HUD_GOOD.g, HUD_GOOD.b, pulse);
    drawReportStat(HUD_ICON_COIN, heist.cash, -0.40f, 0.30f, HUD_PAPER);
    drawReportStat(HUD_ICON_ARROW_DOWN, gCurrentLevel + 1, -0.02f, 0.30f, HUD_PAPER);
    drawIconC(HUD_ICON_CLOCK, 0.31f, 0.30f, 0.10f, HUD_PAPER);
    char tb[16];
    snprintf(tb, sizeof(tb), "%d:%02d", (int)(gSurvivalTime / 60), (int)gSurvivalTime % 60);
    drawText(tb, 0.36f, 0.272f, 2.3f, HUD_PAPER.r, HUD_PAPER.g, HUD_PAPER.b, 0.95f);

    if (multiState == MULTI_IN_GAME) drawShiftAwards(0.02f);
    else drawSoloShiftCounters(0.02f);
    drawPageKey("ENTER", HUD_ICON_CHECK, -0.18f, HUD_GOOD);
    drawPageKey("ESC", HUD_ICON_CROSS, 0.18f, HUD_DIM);
    endHudPage();
}

inline int deathCauseIcon(const char* reason) {
    if (!reason) return HUD_ICON_SKULL;
    if (strstr(reason, "COGNITIVE")) return HUD_ICON_SANITY;
    if (strstr(reason, "FALL")) return HUD_ICON_ARROW_DOWN;
    if (strstr(reason, "BLEED")) return HUD_ICON_DOWNED;
    return HUD_ICON_SKULL;
}

// Solo death: the floor starts over and it costs half of what was in the wallet.
inline void drawHeistDeathScreen(float tm) {
    float flicker = (rand() % 100 < 22) ? 0.35f : 1.0f;
    float pulse = 0.68f + 0.22f * sinf(tm * 4.2f);
    beginHudPage(0.01f, 0.0f, 0.0f, 0.88f);
    drawOverlayRectNdc(-1.0f, -1.0f, 1.0f, -0.86f, 0.32f, 0.02f, 0.02f, 0.55f);
    drawOverlayRectNdc(-1.0f, 0.86f, 1.0f, 1.0f, 0.32f, 0.02f, 0.02f, 0.55f);
    drawIcon(HUD_ICON_SKULL, 0.0f, 0.38f, 0.42f, 0.9f * flicker, 0.10f * flicker, 0.10f * flicker, pulse);
    drawIconC(deathCauseIcon(gDeathReason), 0.0f, 0.06f, 0.10f, HUD_BAD, 0.85f);

    drawIconC(HUD_ICON_COIN, -0.20f, -0.20f, 0.10f, HUD_DIM);
    drawHudNumber(heist.cash, -0.15f, -0.228f, 2.2f, HUD_DIM.r, HUD_DIM.g, HUD_DIM.b);
    drawIconC(HUD_ICON_ARROW_RIGHT, 0.04f, -0.20f, 0.05f, HUD_DIM);
    drawHudNumber(cashAfterWipe(heist.cash), 0.10f, -0.228f, 2.2f, HUD_BAD.r, HUD_BAD.g, HUD_BAD.b);
    drawReportStat(HUD_ICON_ARROW_DOWN, gCurrentLevel + 1, 0.0f, -0.42f, HUD_DIM);

    drawPageKey("ENTER", HUD_ICON_DRILL, -0.18f, HUD_PAPER);
    drawPageKey("ESC", HUD_ICON_CROSS, 0.18f, HUD_DIM);
    endHudPage();
}

inline void drawGuideRow(const char* key, int iconA, int iconB, float x, float y) {
    drawKeyCap(key, x, y, 0.062f);
    drawIconC(iconA, x + 0.13f, y, 0.075f, HUD_PAPER);
    if (iconB < 0) return;
    drawIconC(HUD_ICON_ARROW_RIGHT, x + 0.20f, y, 0.04f, HUD_DIM);
    drawIconC(iconB, x + 0.27f, y, 0.075f, HUD_PAPER);
}

// Key sheet: every key next to what it does. Left: tools and hauling. Right: squad and danger.
inline void drawGuideScreen() {
    beginHudPage(0.02f, 0.02f, 0.03f, 0.84f);
    drawHudPlate(-0.76f, -0.30f, 0.80f, 0.80f, 0.55f);
    const GameplayBinds& k = settings.binds;
    float y = 0.66f;
    const float left = -0.62f, right = 0.14f, step = 0.115f;
    drawGuideRow(keyNameForUi(k.interact), HUD_ICON_HAND, HUD_ICON_LOOT, left, y);
    drawGuideRow("Q", HUD_ICON_NOISE, HUD_ICON_WARNING, right, y);
    y -= step;
    drawGuideRow("R", HUD_ICON_LOOT, HUD_ICON_ARROW_DOWN, left, y);
    drawGuideRow("LMB", HUD_ICON_HAND, HUD_ICON_PERSON, right, y);
    y -= step;
    drawGuideRow(keyNameForUi(k.item1), HUD_ICON_FLASHLIGHT, -1, left, y);
    drawGuideRow("G", HUD_ICON_PING, HUD_ICON_GROUP, right, y);
    y -= step;
    drawGuideRow(keyNameForUi(k.item2), HUD_ICON_SCANNER, HUD_ICON_LOOT, left, y);
    drawGuideRow(keyNameForUi(k.interact), HUD_ICON_REVIVE, HUD_ICON_DOWNED, right, y);
    y -= step;
    drawGuideRow(keyNameForUi(k.item2), HUD_ICON_SCANNER, HUD_ICON_SKULL, left, y);
    drawIconC(HUD_ICON_ARROW_RIGHT, left + 0.34f, y, 0.04f, HUD_DIM);
    drawIconC(HUD_ICON_COIN, left + 0.41f, y, 0.075f, HUD_GOOD);
    drawGuideRow(keyNameForUi(k.item3), HUD_ICON_PLUSH, HUD_ICON_SANITY, right, y);
    y -= step;
    drawGuideRow(keyNameForUi(k.interact), HUD_ICON_PICKAXE, HUD_ICON_CRACK, left, y);
    drawGuideRow(keyNameForUi(k.item4), HUD_ICON_BATTERY, HUD_ICON_FLASHLIGHT, right, y);
    y -= step;
    drawGuideRow(keyNameForUi(k.interact), HUD_ICON_SEARCH, HUD_ICON_STASH, left, y);
    drawGuideRow(keyNameForUi(k.sprint), HUD_ICON_STAMINA, HUD_ICON_NOISE, right, y);
    y -= step * 1.3f;

    // the loop: loot feeds the drill (down) or the kiosk (cash, and one day the way up)
    drawIconC(HUD_ICON_LOOT, -0.40f, y, 0.09f, HUD_PAPER);
    drawIconC(HUD_ICON_ARROW_RIGHT, -0.31f, y, 0.045f, HUD_DIM);
    drawIconC(HUD_ICON_DRILL, -0.22f, y, 0.09f, HUD_PAPER);
    drawIconC(HUD_ICON_ARROW_RIGHT, -0.13f, y, 0.045f, HUD_DIM);
    drawIconC(HUD_ICON_ARROW_DOWN, -0.04f, y, 0.09f, HUD_GOOD);
    drawIconC(HUD_ICON_LOOT, 0.12f, y, 0.09f, HUD_PAPER);
    drawIconC(HUD_ICON_ARROW_RIGHT, 0.21f, y, 0.045f, HUD_DIM);
    drawIconC(HUD_ICON_SHOP, 0.30f, y, 0.09f, HUD_PAPER);
    drawIconC(HUD_ICON_ARROW_RIGHT, 0.39f, y, 0.045f, HUD_DIM);
    drawIconC(HUD_ICON_COIN, 0.48f, y, 0.09f, HUD_PAPER);
    drawIconC(HUD_ICON_ARROW_RIGHT, 0.57f, y, 0.045f, HUD_DIM);
    drawIconC(HUD_ICON_EXTRACT, 0.66f, y, 0.09f, HUD_GOOD);

    static const UiHint hints[] = {{"ESC", "BACK"}};
    uiHintBar(hints, 1);
    endHudPage();
}
