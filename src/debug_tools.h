#pragma once

#include "entity_types.h"

enum DebugAction {
    DEBUG_ACT_TOGGLE_FLY = 0,
    DEBUG_ACT_TOGGLE_INFINITE_STAMINA = 1,
    DEBUG_ACT_TP_NOTE = 2,
    DEBUG_ACT_TP_ECHO = 3,
    DEBUG_ACT_TP_EXIT = 4,
    DEBUG_ACT_SPAWN_STALKER = 5,
    DEBUG_ACT_SPAWN_CRAWLER = 6,
    DEBUG_ACT_SPAWN_SHADOW = 7,
    DEBUG_ACT_FORCE_HOLES = 8,
    DEBUG_ACT_FORCE_SUPPLY = 9
};

inline constexpr int DEBUG_ACTION_COUNT = 10;

struct DebugToolsState {
    bool open;
    bool flyMode;
    bool infiniteStamina;
    int selectedAction;
};

inline int clampDebugActionIndex(int idx) {
    if (idx < 0) return 0;
    if (idx >= DEBUG_ACTION_COUNT) return DEBUG_ACTION_COUNT - 1;
    return idx;
}

inline const char* debugActionLabel(int idx) {
    switch (clampDebugActionIndex(idx)) {
        case DEBUG_ACT_TOGGLE_FLY: return "TOGGLE FLY";
        case DEBUG_ACT_TOGGLE_INFINITE_STAMINA: return "INFINITE STAMINA";
        case DEBUG_ACT_TP_NOTE: return "TELEPORT NOTE";
        case DEBUG_ACT_TP_ECHO: return "TELEPORT ECHO";
        case DEBUG_ACT_TP_EXIT: return "TELEPORT EXIT";
        case DEBUG_ACT_SPAWN_STALKER: return "SPAWN STALKER";
        case DEBUG_ACT_SPAWN_CRAWLER: return "SPAWN CRAWLER";
        case DEBUG_ACT_SPAWN_SHADOW: return "SPAWN SHADOW";
        case DEBUG_ACT_FORCE_HOLES: return "FORCE FLOOR HOLES";
        default: return "FORCE SUPPLY CACHE";
    }
}

inline bool debugActionSpawnsEntity(int idx) {
    int act = clampDebugActionIndex(idx);
    return act == DEBUG_ACT_SPAWN_STALKER || act == DEBUG_ACT_SPAWN_CRAWLER || act == DEBUG_ACT_SPAWN_SHADOW;
}

inline EntityType debugActionEntityType(int idx) {
    int act = clampDebugActionIndex(idx);
    if (act == DEBUG_ACT_SPAWN_CRAWLER) return ENTITY_CRAWLER;
    if (act == DEBUG_ACT_SPAWN_SHADOW) return ENTITY_SHADOW;
    return ENTITY_STALKER;
}
