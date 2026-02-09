#pragma once

#include "map_content.h"

enum PoiOutcomeType {
    POI_OUTCOME_LOOT = 0,
    POI_OUTCOME_RISK = 1,
    POI_OUTCOME_MIXED = 2
};

inline int choosePoiOutcomeType(int poiType, int roll) {
    int norm = roll % 100;
    if (norm < 0) norm += 100;
    if (poiType == MAP_POI_OFFICE) {
        if (norm < 58) return POI_OUTCOME_LOOT;
        if (norm < 84) return POI_OUTCOME_MIXED;
        return POI_OUTCOME_RISK;
    }
    if (poiType == MAP_POI_SERVER) {
        if (norm < 32) return POI_OUTCOME_LOOT;
        if (norm < 70) return POI_OUTCOME_MIXED;
        return POI_OUTCOME_RISK;
    }
    if (poiType == MAP_POI_STORAGE) {
        if (norm < 68) return POI_OUTCOME_LOOT;
        if (norm < 90) return POI_OUTCOME_MIXED;
        return POI_OUTCOME_RISK;
    }
    if (poiType == MAP_POI_RESTROOM) {
        if (norm < 44) return POI_OUTCOME_LOOT;
        if (norm < 80) return POI_OUTCOME_MIXED;
        return POI_OUTCOME_RISK;
    }
    return POI_OUTCOME_LOOT;
}

inline int choosePoiLootType(int poiType, int roll) {
    int norm = roll % 100;
    if (norm < 0) norm += 100;
    if (poiType == MAP_POI_STORAGE) {
        if (norm < 44) return 0; // battery
        if (norm < 66) return 1; // medkit
        return 2;                // bait
    }
    if (poiType == MAP_POI_SERVER) {
        if (norm < 55) return 0;
        if (norm < 72) return 1;
        return 2;
    }
    if (poiType == MAP_POI_OFFICE) {
        if (norm < 30) return 0;
        if (norm < 72) return 1;
        return 2;
    }
    if (norm < 36) return 0;
    if (norm < 84) return 1;
    return 2;
}

inline int choosePoiRiskRoamEvent(int poiType, int roll) {
    int norm = roll % 100;
    if (norm < 0) norm += 100;
    if (poiType == MAP_POI_SERVER) {
        if (norm < 35) return 1; // ROAM_LIGHTS_OUT
        if (norm < 70) return 2; // ROAM_GEOM_SHIFT
        return 4;                // ROAM_FLOOR_HOLES
    }
    if (poiType == MAP_POI_STORAGE) {
        if (norm < 42) return 4;
        if (norm < 72) return 3; // ROAM_FALSE_DOOR
        return 2;
    }
    if (poiType == MAP_POI_OFFICE) {
        if (norm < 34) return 3;
        if (norm < 68) return 2;
        return 4;
    }
    if (norm < 45) return 1;
    if (norm < 76) return 3;
    return 4;
}

inline float choosePoiRiskDuration(int riskEventType, int roll) {
    int norm = roll % 7;
    if (norm < 0) norm += 7;
    if (riskEventType == 2) return 0.10f;
    return 10.0f + (float)norm;
}
