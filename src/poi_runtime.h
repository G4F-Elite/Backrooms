#pragma once

#include <unordered_set>

#include "coop.h"
#include "poi_logic.h"

std::unordered_set<int> gClearedPoiIds;
float gPoiTriggerCooldown = 0.0f;

inline void resetPoiRuntime() {
    gClearedPoiIds.clear();
    gPoiTriggerCooldown = 0.0f;
}

inline void applyPoiLoot(const MapPoi& poi) {
    int spawnCount = 1 + (int)(rng() % 2);
    if (poi.type == MAP_POI_STORAGE) spawnCount++;
    // Furnished rooms are where the good stuff is: loot first, then a supply or two.
    hostSpawnItem(itemTypeFromLoot(rollLootType((unsigned)rng(), gCurrentLevel + 1)), poi.pos);
    for (int i = 1; i < spawnCount; i++) {
        int itemType = choosePoiLootType(poi.type, (int)rng());
        hostSpawnItem(itemType, poi.pos);
    }
    if (poi.type == MAP_POI_RESTROOM) {
        playerStamina += 15.0f;
        if (playerStamina > 125.0f) playerStamina = 125.0f;
    }
    setEchoStatus("NICE ROOM. EVERYTHING IN IT IS NOW INVENTORY");
}

inline void applyPoiRisk(const MapPoi& poi) {
    int roamType = choosePoiRiskRoamEvent(poi.type, (int)rng());
    float duration = choosePoiRiskDuration(roamType, (int)rng());
    applyRoamEvent(roamType, playerChunkX, playerChunkZ, duration);
    if (multiState == MULTI_IN_GAME && netMgr.isHost) {
        netMgr.sendRoamEvent(roamType, playerChunkX & 0xFF, playerChunkZ & 0xFF, duration);
    }
    setTrapStatus("THE ROOM NOTICED YOU TOUCHED ITS STUFF");
}

inline void updatePoiRuntime() {
    if (gPoiTriggerCooldown > 0.0f) {
        gPoiTriggerCooldown -= dTime;
        if (gPoiTriggerCooldown < 0.0f) gPoiTriggerCooldown = 0.0f;
    }
    bool hostOwnsEvents = (multiState != MULTI_IN_GAME) || netMgr.isHost;
    if (!hostOwnsEvents || gPoiTriggerCooldown > 0.0f || !heistDangerActive()) return;

    int poiIdx = nearestMapPoiIndex(cam.pos, CS * 1.60f);
    if (poiIdx < 0 || poiIdx >= (int)mapPois.size()) return;

    const MapPoi& poi = mapPois[poiIdx];
    if (gClearedPoiIds.find(poi.id) != gClearedPoiIds.end()) return;
    gClearedPoiIds.insert(poi.id);
    gPoiTriggerCooldown = 6.0f;

    int outcome = choosePoiOutcomeType(poi.type, (int)rng());
    if (rigPosInQuiet(poi.pos)) outcome = POI_OUTCOME_LOOT; // rooms next to the running drill only pay
    if (outcome == POI_OUTCOME_LOOT) {
        applyPoiLoot(poi);
        return;
    }
    if (outcome == POI_OUTCOME_RISK) {
        applyPoiRisk(poi);
        return;
    }
    applyPoiLoot(poi);
    if ((rng() % 100) < 55) applyPoiRisk(poi);
}
