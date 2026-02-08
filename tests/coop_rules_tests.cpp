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

int main() {
    testDoorBlockedOnlyInMultiplayerGame();
    testDoorNotBlockedWhenOpenOrUninitialized();
    std::cout << "All coop rules tests passed.\n";
    return 0;
}
