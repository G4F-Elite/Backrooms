#pragma once
// Heist rules: pure numbers of the loot run (no engine dependencies, covered by tests).
// The squad hauls loot out of the Backrooms. Loot is either fed to the drill (the only way
// down) or sold to the rival crew's kiosk for cash (supplies and, eventually, the way out).

inline constexpr int HEIST_CARRY_SLOTS = 4;
inline constexpr int HEIST_MAX_BREAKS = 20;
inline constexpr int HEIST_TUTORIAL_LOOT = 3;

// World item ids at or above this value are loot (see item_types.h for supplies).
inline constexpr int ITEM_LOOT_BASE = 10;

enum LootType {
    LOOT_LAMP_TUBE = 0,
    LOOT_WALLPAPER = 1,
    LOOT_CLOCK = 2,
    LOOT_PHONE = 3,
    LOOT_MONITOR = 4,
    LOOT_ALMOND_WATER = 5,
    LOOT_SAFE = 6,
    LOOT_CORE = 7,
    LOOT_TYPE_COUNT = 8
};

inline constexpr int ITEM_STASH = ITEM_LOOT_BASE + LOOT_TYPE_COUNT; // junk pile to rummage in

struct LootInfo {
    int value;  // cash / drill units on the first floor
    int weight; // 1 light .. 3 heavy
};

inline LootInfo lootInfo(int loot) {
    static const LootInfo table[LOOT_TYPE_COUNT] = {
        {40, 1}, {30, 1}, {60, 1}, {80, 1}, {140, 2}, {100, 2}, {220, 3}, {500, 3}
    };
    if (loot < 0 || loot >= LOOT_TYPE_COUNT) return {0, 0};
    return table[loot];
}

inline bool isLootItemType(int itemType) {
    return itemType >= ITEM_LOOT_BASE && itemType < ITEM_LOOT_BASE + LOOT_TYPE_COUNT;
}

inline int lootFromItemType(int itemType) { return itemType - ITEM_LOOT_BASE; }
inline int itemTypeFromLoot(int loot) { return ITEM_LOOT_BASE + loot; }

// Deeper floors hold the same junk, but it is worth more.
inline int lootValueAt(int loot, int level) {
    if (level < 0) level = 0;
    return lootInfo(loot).value * (100 + level * 35) / 100;
}

// What the drill has to eat before it can punch through this floor.
inline int drillQuotaForLevel(int level) {
    if (level <= 0) return 180;
    return 260 + level * 170;
}

// The rival crew will winch the whole squad out. They are not cheap.
inline int extractionPrice(int players) {
    if (players < 1) players = 1;
    return 3000 + (players - 1) * 900;
}

enum ShopItem {
    SHOP_BATTERY = 0,
    SHOP_MED = 1,
    SHOP_PLUSH = 2,
    SHOP_EXTRACT = 3,
    SHOP_ITEM_COUNT = 4
};

inline int shopPrice(int item, int players) {
    if (item == SHOP_BATTERY) return 50;
    if (item == SHOP_MED) return 110;
    if (item == SHOP_PLUSH) return 90;
    if (item == SHOP_EXTRACT) return extractionPrice(players);
    return 0;
}

// Field notes on a monster are worth money to the rivals, once per kind and floor.
inline int studyReward(int level) { return 120 + level * 40; }

// carry[] holds loot ids, -1 = empty slot.
inline int carryCount(const signed char* carry) {
    int n = 0;
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) if (carry[i] >= 0) n++;
    return n;
}

inline int carryWeight(const signed char* carry) {
    int w = 0;
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) if (carry[i] >= 0) w += lootInfo(carry[i]).weight;
    return w;
}

inline int carryValue(const signed char* carry, int level) {
    int v = 0;
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) if (carry[i] >= 0) v += lootValueAt(carry[i], level);
    return v;
}

inline bool carryPush(signed char* carry, int loot) {
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) {
        if (carry[i] >= 0) continue;
        carry[i] = (signed char)loot;
        return true;
    }
    return false;
}

// Takes the most recently picked item. Returns -1 when the hands are empty.
inline int carryPop(signed char* carry) {
    for (int i = HEIST_CARRY_SLOTS - 1; i >= 0; i--) {
        if (carry[i] < 0) continue;
        int loot = carry[i];
        carry[i] = -1;
        return loot;
    }
    return -1;
}

inline void carryClear(signed char* carry) {
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) carry[i] = -1;
}

// Hauling slows you down: every unit of weight costs 6%, never below 55%.
inline float carrySpeedScale(int weight) {
    float s = 1.0f - 0.06f * (float)weight;
    return s < 0.55f ? 0.55f : s;
}

// The first shift is a guided tour; everything after the accident is the real job.
enum HeistStage {
    HEIST_TUT_GARAGE = 0,   // walk through the portal
    HEIST_TUT_HAUL = 1,     // bring loot back to the pallet
    HEIST_TUT_CORE = 2,     // one last, very shiny thing
    HEIST_ACCIDENT = 3,     // it was dropped in the doorway
    HEIST_RUN = 4,          // feed the drill, go deeper
    HEIST_DRILLING = 5,     // drill is punching through
    HEIST_EXTRACTED = 6     // the rivals winched the squad out
};

inline bool heistStageIsSafe(int stage) { return stage < HEIST_ACCIDENT; }

// Which tool the tutorial is teaching for the Nth pallet delivery.
enum TutorLesson { LESSON_CARRY = 0, LESSON_LIGHT = 1, LESSON_BREAK = 2 };
inline int tutorLessonForDelivery(int delivered) {
    if (delivered <= 0) return LESSON_CARRY;
    if (delivered == 1) return LESSON_LIGHT;
    return LESSON_BREAK;
}

// Loot rolled for a floor: common junk up top, pricier pieces further down.
inline int rollLootType(unsigned roll, int level) {
    int r = (int)(roll % 100u);
    int rare = 8 + level * 4;
    if (rare > 30) rare = 30;
    if (r < rare) return (roll >> 8) % 2u ? LOOT_MONITOR : LOOT_ALMOND_WATER;
    if (level >= 2 && r >= 97) return LOOT_CORE;
    return (int)((roll >> 12) % 4u); // tube, wallpaper, clock, phone
}

// Wall caches hold the heavy stuff.
inline int rollCacheLoot(unsigned roll) {
    int r = (int)(roll % 100u);
    if (r < 45) return LOOT_SAFE;
    if (r < 75) return LOOT_MONITOR;
    return LOOT_ALMOND_WATER;
}

// Cracked walls: roughly one in eleven of the walls that separate two open cells.
inline bool crackRoll(unsigned hash) { return (hash % 100u) < 9u; }

// Losing the whole squad costs half the wallet.
inline int cashAfterWipe(int cash) { return cash / 2; }
