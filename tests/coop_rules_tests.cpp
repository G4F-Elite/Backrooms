#include <cassert>
#include <iostream>

#include "../src/coop_rules.h"

void testDoorBlockedOnlyInMultiplayerGame() {
    const int multiInGame = 2;
    assert(!shouldBlockCoopDoor(true, false, 0, multiInGame));
    assert(!shouldBlockCoopDoor(true, false, 1, multiInGame));
    assert(shouldBlockCoopDoor(true, false, multiInGame, multiInGame));
}

void testDoorNotBlockedWhenOpenOrUninitialized() {
    const int multiInGame = 2;
    assert(!shouldBlockCoopDoor(false, false, multiInGame, multiInGame));
    assert(!shouldBlockCoopDoor(true, true, multiInGame, multiInGame));
}

void testStoryDoorSingleplayerByNotes() {
    const int multiInGame = 2;
    assert(shouldBlockStoryDoor(true, false, 0, multiInGame, 0, 5));
    assert(shouldBlockStoryDoor(true, false, 0, multiInGame, 4, 5));
    assert(!shouldBlockStoryDoor(true, false, 0, multiInGame, 5, 5));
}

void testStoryDoorMultiplayerBySwitches() {
    const int multiInGame = 2;
    assert(shouldBlockStoryDoor(true, false, multiInGame, multiInGame, 99, 5));
    assert(!shouldBlockStoryDoor(true, true, multiInGame, multiInGame, 0, 5));
}

void testDoorFootprintClearRequiresWidthAndApproach() {
    auto openField = [](int, int) { return 0; };
    assert(isDoorFootprintClear(0, 0, openField));

    auto blockedLeft = [](int x, int z) {
        if (x == -1 && z == 0) return 1;
        return 0;
    };
    assert(!isDoorFootprintClear(0, 0, blockedLeft));

    auto blockedApproach = [](int x, int z) {
        if ((x == 0 && z == 1) || (x == 0 && z == -1)) return 1;
        return 0;
    };
    assert(!isDoorFootprintClear(0, 0, blockedApproach));
}

int main() {
    testDoorBlockedOnlyInMultiplayerGame();
    testDoorNotBlockedWhenOpenOrUninitialized();
    testStoryDoorSingleplayerByNotes();
    testStoryDoorMultiplayerBySwitches();
    testDoorFootprintClearRequiresWidthAndApproach();
    std::cout << "All coop rules tests passed.\n";
    return 0;
}
