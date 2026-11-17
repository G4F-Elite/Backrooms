#pragma once

// World item / loot identifiers.
// Keep this header dependency-free so it can be included from many modules.

#include "heist_rules.h"

// Supplies. Loot starts at ITEM_LOOT_BASE and junk piles are ITEM_STASH (see heist_rules.h).
enum WorldItemType {
    ITEM_BATTERY = 0,
    ITEM_PLUSH_TOY = 1,
    ITEM_MED_SPRAY = 2
};
