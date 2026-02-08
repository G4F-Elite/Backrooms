#include <cassert>
#include <iostream>
#include <string>

#include "../src/debug_tools.h"

void testClampDebugActionIndex() {
    assert(clampDebugActionIndex(-5) == 0);
    assert(clampDebugActionIndex(0) == 0);
    assert(clampDebugActionIndex(DEBUG_ACTION_COUNT - 1) == DEBUG_ACTION_COUNT - 1);
    assert(clampDebugActionIndex(999) == DEBUG_ACTION_COUNT - 1);
}

void testDebugActionLabels() {
    assert(std::string(debugActionLabel(DEBUG_ACT_TOGGLE_FLY)) == "TOGGLE FLY");
    assert(std::string(debugActionLabel(DEBUG_ACT_FORCE_SUPPLY)) == "FORCE SUPPLY CACHE");
}

void testSpawnActionMapping() {
    assert(debugActionSpawnsEntity(DEBUG_ACT_SPAWN_STALKER));
    assert(debugActionSpawnsEntity(DEBUG_ACT_SPAWN_CRAWLER));
    assert(debugActionSpawnsEntity(DEBUG_ACT_SPAWN_SHADOW));
    assert(!debugActionSpawnsEntity(DEBUG_ACT_TP_NOTE));
    assert(debugActionEntityType(DEBUG_ACT_SPAWN_STALKER) == ENTITY_STALKER);
    assert(debugActionEntityType(DEBUG_ACT_SPAWN_CRAWLER) == ENTITY_CRAWLER);
    assert(debugActionEntityType(DEBUG_ACT_SPAWN_SHADOW) == ENTITY_SHADOW);
}

int main() {
    testClampDebugActionIndex();
    testDebugActionLabels();
    testSpawnActionMapping();
    std::cout << "All debug tools tests passed.\n";
    return 0;
}
