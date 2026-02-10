#pragma once

// World item / loot identifiers.
// Keep this header dependency-free so it can be included from many modules.

enum WorldItemType {
    ITEM_BATTERY = 0,
    ITEM_PLUSH_TOY = 1
};

inline const char* worldItemPickupPrompt(int type) {
    if (type == ITEM_BATTERY) return "[E] PICK BATTERY";
    if (type == ITEM_PLUSH_TOY) return "[E] PICK PLUSH TOY";
    return "[E] PICK ITEM";
}
