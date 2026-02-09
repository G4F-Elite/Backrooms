#include <cassert>
#include <iostream>

#include "../src/poi_logic.h"

void testOutcomeRanges() {
    for (int t = MAP_POI_OFFICE; t <= MAP_POI_RESTROOM; t++) {
        for (int i = 0; i < 300; i++) {
            int out = choosePoiOutcomeType(t, i);
            assert(out >= POI_OUTCOME_LOOT && out <= POI_OUTCOME_MIXED);
        }
    }
}

void testLootTypeRanges() {
    for (int t = MAP_POI_OFFICE; t <= MAP_POI_RESTROOM; t++) {
        for (int i = 0; i < 300; i++) {
            int loot = choosePoiLootType(t, i);
            assert(loot >= 0 && loot <= 2);
        }
    }
}

void testRiskEventRanges() {
    for (int t = MAP_POI_OFFICE; t <= MAP_POI_RESTROOM; t++) {
        for (int i = 0; i < 300; i++) {
            int risk = choosePoiRiskRoamEvent(t, i);
            assert(risk == 1 || risk == 2 || risk == 3 || risk == 4);
            float dur = choosePoiRiskDuration(risk, i);
            if (risk == 2) assert(dur == 0.10f);
            else assert(dur >= 7.0f && dur <= 13.0f);
        }
    }
}

int main() {
    testOutcomeRanges();
    testLootTypeRanges();
    testRiskEventRanges();
    std::cout << "All poi logic tests passed.\n";
    return 0;
}
