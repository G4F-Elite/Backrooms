#pragma once
// Heist HUD: the run told in pictograms. Quota gauge, wallet, carried loot, prompts,
// world markers, toasts, the kiosk price list and the tutorial hints. No sentences.

#include "heist_play.h"

struct HudColor { float r, g, b; };
inline constexpr HudColor HUD_PAPER = {0.94f, 0.89f, 0.64f};
inline constexpr HudColor HUD_GOOD = {0.56f, 0.93f, 0.56f};
inline constexpr HudColor HUD_BAD = {0.97f, 0.43f, 0.36f};
inline constexpr HudColor HUD_COOL = {0.56f, 0.90f, 0.96f};
inline constexpr HudColor HUD_DIM = {0.50f, 0.49f, 0.44f};

inline void drawIconC(int icon, float cx, float cy, float h, HudColor c, float a = 0.95f) {
    drawIcon(icon, cx, cy, h, c.r, c.g, c.b, a);
}

inline float hudPulse(float speed) {
    return 0.5f + 0.5f * sinf(vhsTime * speed);
}

inline int lootIcon(int loot) {
    return loot == LOOT_CORE ? HUD_ICON_CORE : HUD_ICON_LOOT;
}

inline int supplyIcon(int itemType) {
    if (itemType == ITEM_BATTERY) return HUD_ICON_BATTERY;
    if (itemType == ITEM_MED_SPRAY) return HUD_ICON_MED;
    if (itemType == ITEM_PLUSH_TOY) return HUD_ICON_PLUSH;
    return HUD_ICON_LOOT;
}

// The rig in two pictograms. House: lit while you stand in its haven. Wrench: it broke
// down, and the bar next to it is what is left of the spare battery that keeps the lamp on.
inline void drawRigStatus(float x, float y, float h) {
    float step = h / hudAspect();
    bool broken = heist.rigFault != RIG_OK;
    bool safeHere = gRigSheltered && !gCoopDead;
    HudColor home = safeHere ? (broken ? HUD_PAPER : HUD_GOOD) : HUD_DIM;
    drawIconC(HUD_ICON_HOME, x, y, h * 0.8f, home, safeHere ? 0.95f : 0.55f);
    if (!broken) return;
    float left = rigReserveLeft(heist.rigFault, heist.rigFaultSecs);
    float wx = x + step * 0.95f;
    drawIconC(HUD_ICON_WRENCH, wx, y, h * (0.8f + 0.12f * hudPulse(7.0f)), HUD_BAD);
    drawGauge(wx + step * 0.6f, y - 0.012f, wx + step * 0.6f + 0.12f, y + 0.012f, left,
              left > 0.0f ? HUD_PAPER.r : HUD_BAD.r, left > 0.0f ? HUD_PAPER.g : HUD_BAD.g, left > 0.0f ? HUD_PAPER.b : HUD_BAD.b);
}

// Top-left: what the floor still wants from the squad.
inline void drawHeistGoals() {
    const float x = -0.93f, y = 0.88f, h = 0.085f;
    float step = h / hudAspect();
    if (heistStageIsSafe(heist.stage)) {
        drawIconC(HUD_ICON_PALLET, x, y, h, HUD_PAPER);
        for (int i = 0; i < HEIST_TUTORIAL_LOOT; i++) {
            float cx = x + step * (1.3f + (float)i * 1.05f);
            bool done = i < heist.tutDelivered;
            drawIconC(done ? HUD_ICON_CHECK : HUD_ICON_LOOT, cx, y, h * 0.8f, done ? HUD_GOOD : HUD_DIM, done ? 0.95f : 0.55f);
        }
        if (heist.stage == HEIST_TUT_CORE) drawIconC(HUD_ICON_CORE, x + step * 4.6f, y, h, HUD_COOL, 0.6f + 0.35f * hudPulse(4.0f));
        return;
    }
    bool ready = heistDrillReady() || heist.stage == HEIST_DRILLING;
    float frac = heist.quotaNeed > 0 ? (float)heist.quotaHave / (float)heist.quotaNeed : 0.0f;
    HudColor fill = ready ? HUD_GOOD : HUD_PAPER;
    drawIconC(HUD_ICON_DRILL, x, y, h, fill);
    float gx0 = x + step * 0.8f, gx1 = gx0 + 0.30f;
    drawGauge(gx0, y - 0.020f, gx1, y + 0.020f, frac, fill.r, fill.g, fill.b);
    for (int i = 1; i < DRILL_GAUGE_LAMPS; i++) {
        float tx = gx0 + (gx1 - gx0) * (float)i / (float)DRILL_GAUGE_LAMPS;
        drawOverlayRectNdc(tx, y - 0.020f, tx + 0.002f, y + 0.020f, 0.0f, 0.0f, 0.0f, 0.55f);
    }
    if (ready) drawIconC(HUD_ICON_ARROW_DOWN, gx1 + step * 0.7f, y, h * (0.8f + 0.15f * hudPulse(6.0f)), HUD_GOOD);

    float y2 = y - 0.10f;
    drawIconC(HUD_ICON_COIN, x, y2, h * 0.9f, HUD_PAPER);
    drawHudNumber(heist.cash, x + step * 0.75f, y2 - 0.022f, 1.9f, HUD_PAPER.r, HUD_PAPER.g, HUD_PAPER.b);
    if (heist.hasKiosk) {
        int price = extractionPrice(heistSquadSize());
        float px = x + 0.26f;
        bool can = heist.cash >= price;
        drawIconC(HUD_ICON_EXTRACT, px, y2, h * 0.8f, can ? HUD_GOOD : HUD_DIM);
        drawGauge(px + step * 0.6f, y2 - 0.012f, px + step * 0.6f + 0.14f, y2 + 0.012f, (float)heist.cash / (float)price,
                  can ? HUD_GOOD.r : HUD_DIM.r, can ? HUD_GOOD.g : HUD_DIM.g, can ? HUD_GOOD.b : HUD_DIM.b);
    }
    // depth: one arrow, then the floor number
    float y3 = y2 - 0.09f;
    drawIconC(HUD_ICON_ARROW_DOWN, x, y3, h * 0.7f, HUD_DIM);
    drawHudNumber(gCurrentLevel + 1, x + step * 0.6f, y3 - 0.018f, 1.6f, HUD_DIM.r, HUD_DIM.g, HUD_DIM.b);
    drawRigStatus(x + 0.13f, y3, h);
}

// Bottom-centre: the four things you can carry, and how heavy they make you.
inline void drawHeistCarry() {
    const signed char* carry = heistMyCarry();
    int count = carryCount(carry);
    if (count == 0 && heist.stage == HEIST_TUT_GARAGE) return;
    const float y = -0.90f, h = 0.085f;
    float step = h / hudAspect() * 1.15f;
    float x0 = -step * 1.5f;
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) {
        float cx = x0 + step * (float)i, hw = h * 0.5f / hudAspect();
        drawOverlayRectNdc(cx - hw, y - h * 0.5f, cx + hw, y + h * 0.5f, 0.02f, 0.02f, 0.03f, 0.50f);
        drawBorderRect(cx - hw, y - h * 0.5f, cx + hw, y + h * 0.5f, 0.40f, 0.36f, 0.30f, 0.80f, 0.004f);
        if (carry[i] < 0) continue;
        HudColor c = lootInfo(carry[i]).weight >= 3 ? HUD_BAD : (lootInfo(carry[i]).weight == 2 ? HUD_PAPER : HUD_GOOD);
        drawIconC(lootIcon(carry[i]), cx, y, h * 0.78f, carry[i] == LOOT_CORE ? HUD_COOL : c);
    }
    if (count == 0) return;
    int weight = carryWeight(carry);
    float wx = x0 + step * 4.1f;
    drawIconC(HUD_ICON_WEIGHT, wx, y, h * 0.7f, weight >= 6 ? HUD_BAD : (weight >= 3 ? HUD_PAPER : HUD_DIM));
    drawKeyCap("R", x0 - step * 1.2f, y, h * 0.62f, 0.8f);
    drawIconC(HUD_ICON_ARROW_DOWN, x0 - step * 0.62f, y, h * 0.5f, HUD_DIM, 0.8f);
}

// One row of [key] icon icon number, centred on x = 0.
inline void drawPromptRow(const char* key, int iconA, int iconB, int amount, HudColor c, bool hold) {
    const float y = -0.42f, h = 0.10f;
    float step = h / hudAspect() * 1.1f;
    int cells = (key ? 1 : 0) + (iconA >= 0 ? 1 : 0) + (iconB >= 0 ? 2 : 0);
    float x = -step * 0.5f * (float)(cells - 1);
    if (key) {
        if (hold) drawHoldRing(x, y, h * 0.72f, gHeistHold, c.r, c.g, c.b);
        drawKeyCap(key, x, y, h * 0.72f);
        x += step;
    }
    if (iconA >= 0) { drawIconC(iconA, x, y, h, c); x += step; }
    if (iconB >= 0) {
        drawIconC(HUD_ICON_ARROW_RIGHT, x, y, h * 0.5f, HUD_DIM);
        x += step;
        drawIconC(iconB, x, y, h, c);
        x += step;
    }
    if (amount != 0) drawHudNumber(amount, x - step * 0.4f, y - 0.024f, 2.0f, c.r, c.g, c.b);
}

inline void drawHeistPrompt() {
    const HeistPrompt& p = gHeistPrompt;
    const char* key = keyNameForUi(settings.binds.interact);
    if (p.kind == HEIST_PROMPT_PICK_LOOT) {
        drawPromptRow(key, HUD_ICON_HAND, -1, 0, HUD_PAPER, false);
        drawIconC(HUD_ICON_COIN, 0.10f, -0.42f, 0.07f, HUD_PAPER);
        drawHudNumber(p.amount, 0.135f, -0.44f, 1.8f, HUD_PAPER.r, HUD_PAPER.g, HUD_PAPER.b);
    } else if (p.kind == HEIST_PROMPT_PICK_SUPPLY) {
        drawPromptRow(key, HUD_ICON_HAND, supplyIcon(p.itemType), 0, HUD_PAPER, false);
    } else if (p.kind == HEIST_PROMPT_HANDS_FULL) {
        drawPromptRow(nullptr, HUD_ICON_HAND, HUD_ICON_CROSS, 0, HUD_BAD, false);
    } else if (p.kind == HEIST_PROMPT_RUMMAGE) {
        drawPromptRow(key, HUD_ICON_SEARCH, HUD_ICON_STASH, 0, HUD_PAPER, true);
    } else if (p.kind == HEIST_PROMPT_FEED_DRILL) {
        drawPromptRow(key, HUD_ICON_LOOT, HUD_ICON_DRILL, p.amount, HUD_PAPER, false);
    } else if (p.kind == HEIST_PROMPT_DRILL_HUNGRY) {
        drawPromptRow(nullptr, HUD_ICON_LOOT, HUD_ICON_DRILL, p.amount, HUD_BAD, false);
    } else if (p.kind == HEIST_PROMPT_START_DRILL) {
        drawPromptRow(key, HUD_ICON_DRILL, HUD_ICON_ARROW_DOWN, 0, HUD_GOOD, false);
    } else if (p.kind == HEIST_PROMPT_DELIVER) {
        drawPromptRow(key, HUD_ICON_LOOT, HUD_ICON_PALLET, 0, HUD_PAPER, false);
    } else if (p.kind == HEIST_PROMPT_SELL) {
        drawPromptRow(key, HUD_ICON_LOOT, HUD_ICON_COIN, p.amount, HUD_GOOD, false);
    } else if (p.kind == HEIST_PROMPT_BREAK) {
        drawPromptRow(key, HUD_ICON_PICKAXE, HUD_ICON_CRACK, 0, HUD_PAPER, true);
    } else if (p.kind == HEIST_PROMPT_REPAIR) {
        drawPromptRow(key, HUD_ICON_WRENCH, HUD_ICON_DRILL, 0, HUD_BAD, true);
    } else if (p.kind == HEIST_PROMPT_CHARGER) {
        drawPromptRow(key, HUD_ICON_HAND, HUD_ICON_BATTERY, 0, HUD_GOOD, false);
    }
}

// Right side: what just happened to the wallet / the drill.
inline void drawHeistToasts() {
    for (int i = 0; i < HEIST_TOAST_MAX; i++) {
        const HeistToast& t = gHeistToasts[i];
        if (t.ttl <= 0.0f) continue;
        float a = t.ttl < 0.6f ? t.ttl / 0.6f : 1.0f;
        float rise = (HEIST_TOAST_SECONDS - t.ttl) * 0.02f;
        float y = 0.30f - (float)i * 0.10f + rise;
        HudColor c = t.good ? HUD_GOOD : HUD_BAD;
        drawIcon(t.icon, 0.62f, y, 0.085f, c.r, c.g, c.b, a);
        if (t.amount != 0) drawHudNumber(t.amount, 0.66f, y - 0.024f, 2.0f, c.r, c.g, c.b, a, true);
    }
}

// Kiosk: four keys, four pictures, four prices.
inline void drawHeistShop() {
    if (!gHeistShopOpen) return;
    static const int icons[SHOP_ITEM_COUNT] = {HUD_ICON_BATTERY, HUD_ICON_MED, HUD_ICON_PLUSH, HUD_ICON_EXTRACT};
    const int keys[SHOP_ITEM_COUNT] = {settings.binds.item1, settings.binds.item2, settings.binds.item3, settings.binds.item4};
    const float h = 0.10f, y = -0.05f;
    float colW = 0.20f, x0 = -colW * 1.5f;
    drawOverlayRectNdc(x0 - 0.12f, y - 0.20f, -x0 + 0.12f, y + 0.12f, 0.02f, 0.02f, 0.03f, 0.66f);
    drawBorderRect(x0 - 0.12f, y - 0.20f, -x0 + 0.12f, y + 0.12f, 0.50f, 0.45f, 0.36f, 0.85f, 0.004f);
    drawIconC(HUD_ICON_SHOP, x0 - 0.07f, y + 0.07f, 0.06f, HUD_DIM);
    for (int i = 0; i < SHOP_ITEM_COUNT; i++) {
        int price = shopPrice(i, heistSquadSize());
        bool can = heist.cash >= price;
        HudColor c = can ? (i == SHOP_EXTRACT ? HUD_GOOD : HUD_PAPER) : HUD_DIM;
        float cx = x0 + colW * (float)i;
        float pulse = (i == SHOP_EXTRACT && can) ? 1.0f + 0.12f * hudPulse(5.0f) : 1.0f;
        drawIconC(icons[i], cx, y + 0.02f, h * pulse, c);
        drawKeyCap(keyNameForUi(keys[i]), cx - 0.055f, y - 0.11f, 0.055f, can ? 0.95f : 0.5f);
        drawHudNumber(price, cx - 0.025f, y - 0.13f, 1.6f, can ? HUD_PAPER.r : HUD_BAD.r, can ? HUD_PAPER.g : HUD_BAD.g, can ? HUD_PAPER.b : HUD_BAD.b);
    }
}

// The first shift teaches one tool per delivery: which key, which thing.
inline void drawHeistLessonHint() {
    if (heist.stage != HEIST_TUT_HAUL || carryCount(heistMyCarry()) > 0) return;
    int lesson = tutorLessonForDelivery(heist.tutDelivered);
    const float y = 0.70f, h = 0.085f;
    float a = 0.70f + 0.25f * hudPulse(3.0f);
    if (lesson == LESSON_LIGHT) {
        bool lit = flashlightOn;
        drawKeyCap(keyNameForUi(settings.binds.item1), -0.11f, y, h * 0.7f, lit ? 0.4f : a);
        drawKeyCap(keyNameForUi(settings.binds.flashlight), -0.04f, y, h * 0.7f, lit ? 0.4f : a);
        drawIcon(HUD_ICON_FLASHLIGHT, 0.05f, y, h, HUD_PAPER.r, HUD_PAPER.g, HUD_PAPER.b, lit ? 0.4f : a);
    } else if (lesson == LESSON_BREAK) {
        bool scanning = activeDeviceSlot == 2;
        drawKeyCap(keyNameForUi(settings.binds.item2), -0.16f, y, h * 0.7f, scanning ? 0.4f : a);
        drawIcon(HUD_ICON_SCANNER, -0.09f, y, h, HUD_COOL.r, HUD_COOL.g, HUD_COOL.b, scanning ? 0.4f : a);
        drawIcon(HUD_ICON_ARROW_RIGHT, -0.01f, y, h * 0.5f, HUD_DIM.r, HUD_DIM.g, HUD_DIM.b, a);
        drawIcon(HUD_ICON_CRACK, 0.07f, y, h, HUD_PAPER.r, HUD_PAPER.g, HUD_PAPER.b, a);
        drawIcon(HUD_ICON_PICKAXE, 0.15f, y, h, HUD_PAPER.r, HUD_PAPER.g, HUD_PAPER.b, a);
    }
}

// Monsters the squad has taken notes on, each with the one rule that keeps you alive:
// keep looking at it / keep quiet / keep together.
inline void drawHeistFieldNotes() {
    static const int ruleIcon[4] = {-1, HUD_ICON_EYE, HUD_ICON_MUTE, HUD_ICON_GROUP};
    const float x = -0.93f, h = 0.07f;
    float y = 0.30f;
    for (int type = ENTITY_STALKER; type <= ENTITY_SHADOW; type++) {
        if ((heist.studiedMask & (1 << type)) == 0) continue;
        drawIconC(HUD_ICON_SKULL, x, y, h * 0.8f, HUD_DIM);
        drawIconC(ruleIcon[type], x + h / hudAspect() * 1.1f, y, h, HUD_COOL);
        y -= 0.09f;
    }
    if (gHeistStudy <= 0.0f) return;
    drawHoldRing(0.0f, 0.0f, 0.09f, gHeistStudy, HUD_COOL.r, HUD_COOL.g, HUD_COOL.b);
    drawIconC(HUD_ICON_SCANNER, 0.0f, 0.13f, 0.06f, HUD_COOL);
}

inline void drawHeistFlash() {
    if (gHeistFlash <= 0.01f) return;
    float a = gHeistFlash > 1.0f ? 1.0f : gHeistFlash;
    bool drilling = heist.stage == HEIST_DRILLING;
    drawFullscreenOverlay(drilling ? 0.02f : 1.0f, drilling ? 0.02f : 0.96f, drilling ? 0.02f : 0.86f, a);
}

inline void drawHeistHud() {
    drawHeistGoals();
    drawHeistFieldNotes();
    drawHeistToasts();
    if (!gCoopDead && !playerDowned) {
        drawHeistCarry();
        drawHeistLessonHint();
        drawHeistShop();
    }
}
