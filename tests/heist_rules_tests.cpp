// Heist rules: loot values, carrying, quotas, prices, tutorial order.
// g++ -std=c++17 tests/heist_rules_tests.cpp -o build/heist_rules_tests.exe
#include <cassert>
#include <cstdio>
#include "../src/heist_rules.h"

static void testLootTable() {
    for (int loot = 0; loot < LOOT_TYPE_COUNT; loot++) {
        assert(lootInfo(loot).value > 0);
        assert(lootInfo(loot).weight >= 1 && lootInfo(loot).weight <= 3);
        assert(isLootItemType(itemTypeFromLoot(loot)));
        assert(lootFromItemType(itemTypeFromLoot(loot)) == loot);
        assert(lootValueAt(loot, 3) > lootValueAt(loot, 0)); // deeper = pricier
    }
    assert(lootInfo(-1).value == 0 && lootInfo(LOOT_TYPE_COUNT).value == 0);
    assert(!isLootItemType(0) && !isLootItemType(2) && !isLootItemType(ITEM_STASH));
    assert(lootValueAt(LOOT_CLOCK, -5) == lootValueAt(LOOT_CLOCK, 0));
}

static void testCarry() {
    signed char carry[HEIST_CARRY_SLOTS];
    carryClear(carry);
    assert(carryCount(carry) == 0 && carryWeight(carry) == 0 && carryPop(carry) == -1);
    assert(carryPush(carry, LOOT_SAFE));
    assert(carryPush(carry, LOOT_CLOCK));
    assert(carryCount(carry) == 2);
    assert(carryWeight(carry) == lootInfo(LOOT_SAFE).weight + lootInfo(LOOT_CLOCK).weight);
    assert(carryValue(carry, 0) == lootInfo(LOOT_SAFE).value + lootInfo(LOOT_CLOCK).value);
    assert(carryPop(carry) == LOOT_CLOCK); // last in, first out
    assert(carryPush(carry, LOOT_PHONE) && carryPush(carry, LOOT_PHONE) && carryPush(carry, LOOT_PHONE));
    assert(!carryPush(carry, LOOT_PHONE)); // hands full
    assert(carryCount(carry) == HEIST_CARRY_SLOTS);
}

static void testWeightSlowsDown() {
    assert(carrySpeedScale(0) == 1.0f);
    for (int w = 1; w <= 12; w++) {
        assert(carrySpeedScale(w) <= carrySpeedScale(w - 1));
        assert(carrySpeedScale(w) >= 0.55f);
    }
}

static void testEconomy() {
    for (int level = 0; level < 8; level++) {
        assert(drillQuotaForLevel(level + 1) > drillQuotaForLevel(level));
        assert(studyReward(level + 1) > studyReward(level));
    }
    // The tutorial floor must be finishable with three light pieces of junk.
    assert(drillQuotaForLevel(0) <= 3 * lootInfo(LOOT_CLOCK).value);
    // Extraction is the long-term goal: far more than one floor of loot.
    assert(extractionPrice(1) > drillQuotaForLevel(1) * 4);
    assert(extractionPrice(4) > extractionPrice(1));
    assert(extractionPrice(0) == extractionPrice(1));
    for (int item = 0; item < SHOP_ITEM_COUNT; item++) assert(shopPrice(item, 1) > 0);
    assert(shopPrice(SHOP_ITEM_COUNT, 1) == 0);
    assert(shopPrice(SHOP_EXTRACT, 2) == extractionPrice(2));
    assert(cashAfterWipe(1001) == 500 && cashAfterWipe(0) == 0);
}

static void testStages() {
    assert(heistStageIsSafe(HEIST_TUT_GARAGE) && heistStageIsSafe(HEIST_TUT_HAUL) && heistStageIsSafe(HEIST_TUT_CORE));
    assert(!heistStageIsSafe(HEIST_ACCIDENT) && !heistStageIsSafe(HEIST_RUN) && !heistStageIsSafe(HEIST_DRILLING));
    assert(tutorLessonForDelivery(0) == LESSON_CARRY);
    assert(tutorLessonForDelivery(1) == LESSON_LIGHT);
    assert(tutorLessonForDelivery(2) == LESSON_BREAK);
}

static void testRolls() {
    int cores = 0, cracks = 0;
    for (unsigned roll = 0; roll < 20000u; roll++) {
        unsigned mixed = roll * 2654435761u;
        int shallow = rollLootType(mixed, 0);
        assert(shallow >= 0 && shallow < LOOT_TYPE_COUNT);
        assert(shallow != LOOT_CORE && shallow != LOOT_SAFE); // cores are deep, safes are in walls
        int deep = rollLootType(mixed, 5);
        assert(deep >= 0 && deep < LOOT_TYPE_COUNT);
        if (deep == LOOT_CORE) cores++;
        int cache = rollCacheLoot(mixed);
        assert(cache == LOOT_SAFE || cache == LOOT_MONITOR || cache == LOOT_ALMOND_WATER);
        if (crackRoll(mixed)) cracks++;
    }
    assert(cores > 0);
    assert(cracks > 500 && cracks < 6000); // some walls, not most walls
}

int main() {
    testLootTable();
    testCarry();
    testWeightSlowsDown();
    testEconomy();
    testStages();
    testRolls();
    printf("heist rules tests passed\n");
    return 0;
}
