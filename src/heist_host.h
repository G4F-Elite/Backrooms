#pragma once
// Heist host rules: everything that changes the shared run (pickups, deposits, sales,
// broken walls, the accident, the drill). Runs in solo and on the co-op host; clients ask
// through interact requests and see the result in the mirrored heist state.

#include "heist_sets.h"

enum HeistAction {
    HEIST_ACT_DEPOSIT = 1,
    HEIST_ACT_SELL = 2,
    HEIST_ACT_DROP = 3,
    HEIST_ACT_START_DRILL = 4,
    HEIST_ACT_REPAIR = 5,      // fix whatever broke on the rig this time
    HEIST_ACT_BATTERY = 6      // take the battery out of the rig's charger
};

inline constexpr float HEIST_PAD_REACH = 2.8f;
inline constexpr float HEIST_KIOSK_REACH = 3.2f;
inline constexpr float HEIST_PORTAL_TRIP_RANGE = 2.9f;
inline constexpr float HEIST_ACCIDENT_SECONDS = 4.5f;
inline constexpr float HEIST_DRILL_SECONDS = 4.2f;
inline constexpr float HEIST_BREAK_NOISE = 24.0f;

inline int gHeistLessonsSpawned = 0;

inline bool heistPlayerStanding(int pid) {
    if (pid == heistMe()) return !playerDowned && !gCoopDead && !isPlayerDead;
    return isRemotePlayerStanding(pid);
}

inline bool heistPlayerPos(int pid, Vec3& out) {
    if (pid == heistMe()) {
        out = cam.pos;
        return true;
    }
    if (pid < 0 || pid >= MAX_PLAYERS || !netMgr.players[pid].active || !netMgr.players[pid].hasValidPos) return false;
    out = netMgr.players[pid].pos;
    return true;
}

inline void heistSaveTutorialSeen() {
    gHeistTutorialSeen = true;
    if (gSelftest) return;
    FILE* f = std::fopen("save_heist.dat", "wb");
    if (!f) return;
    int seen = 1;
    std::fwrite(&seen, sizeof(int), 1, f);
    std::fclose(f);
}

inline void heistLoadTutorialSeen() {
    FILE* f = std::fopen("save_heist.dat", "rb");
    if (!f) return;
    int seen = 0;
    if (std::fread(&seen, sizeof(int), 1, f) == 1) gHeistTutorialSeen = seen != 0;
    std::fclose(f);
}

inline void heistGiveSupply(int pid, int itemType) {
    if (pid == heistMe()) {
        if (itemType == ITEM_BATTERY) invBattery++;
        else if (itemType == ITEM_PLUSH_TOY) invPlush++;
        else if (itemType == ITEM_MED_SPRAY) applyItemUse(ITEM_MED_SPRAY);
        return;
    }
    // Remote med sprays are applied by the client itself the moment it asks for the pickup.
    if (itemType == ITEM_BATTERY) netMgr.inventoryBattery[pid]++;
    else if (itemType == ITEM_PLUSH_TOY) netMgr.inventoryPlush[pid]++;
}

#include "rig_host.h"

// Junk piles: usually something, sometimes two things, now and then nothing at all.
inline void heistOpenStash(const WorldItem& stash) {
    unsigned roll = (unsigned)rng();
    int count = (roll % 100u) < 22u ? 0 : ((roll % 100u) < 80u ? 1 : 2);
    for (int i = 0; i < count; i++) {
        Vec3 pos(stash.pos.x + (i == 0 ? 0.35f : -0.35f), 0.0f, stash.pos.z + 0.25f);
        heistSpawnLoot(rollLootType((unsigned)rng(), gCurrentLevel), pos);
    }
    if (count == 0 && (roll >> 8) % 2u) heistSpawnItem(ITEM_BATTERY, stash.pos);
}

inline bool heistHostPick(int pid, int itemId) {
    Vec3 pos;
    if (!heistPlayerPos(pid, pos)) return false;
    for (auto& it : worldItems) {
        if (!it.active || it.id != itemId) continue;
        if (!nearPoint2D(pos, it.pos, 3.6f)) return false;
        if (isLootItemType(it.type)) {
            if (!carryPush(heist.carry[pid], lootFromItemType(it.type))) return false;
        } else if (it.type == ITEM_STASH) {
            heistOpenStash(it);
        } else {
            heistGiveSupply(pid, it.type);
        }
        it.active = false;
        return true;
    }
    return false;
}

inline void heistDropAll(int pid, const Vec3& pos) {
    int n = 0;
    for (int loot = carryPop(heist.carry[pid]); loot >= 0; loot = carryPop(heist.carry[pid])) {
        float a = 1.3f * (float)n++;
        heistSpawnLoot(loot, Vec3(pos.x + sinf(a) * 0.5f, 0.0f, pos.z + cosf(a) * 0.5f));
    }
}

inline bool heistHostDeposit(int pid, const Vec3& pos) {
    signed char* carry = heist.carry[pid];
    if (heist.stage == HEIST_RUN && rigPosAtHopper(pos)) {
        bool fed = false;
        while (heist.quotaHave < heist.quotaNeed) {
            int loot = carryPop(carry);
            if (loot < 0) break;
            heist.quotaHave += lootValueAt(loot, gCurrentLevel);
            fed = true;
        }
        if (heist.quotaHave > heist.quotaNeed) heist.quotaHave = heist.quotaNeed;
        return fed;
    }
    bool hauling = heist.stage == HEIST_TUT_HAUL || heist.stage == HEIST_TUT_CORE;
    if (!hauling || !nearPoint2D(pos, heist.padPos, HEIST_PAD_REACH)) return false;
    bool delivered = false;
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) {
        if (carry[i] < 0 || carry[i] == LOOT_CORE) continue;
        carry[i] = -1;
        if (heist.tutDelivered < HEIST_TUTORIAL_LOOT) heist.tutDelivered++;
        delivered = true;
    }
    return delivered;
}

inline bool heistHostAction(int pid, int action) {
    Vec3 pos;
    if (pid < 0 || pid >= MAX_PLAYERS || !heistPlayerPos(pid, pos)) return false;
    signed char* carry = heist.carry[pid];
    if (action == HEIST_ACT_DEPOSIT) return heistHostDeposit(pid, pos);
    if (action == HEIST_ACT_SELL) {
        if (!heist.hasKiosk || !nearPoint2D(pos, heist.kioskPos, HEIST_KIOSK_REACH) || carryCount(carry) == 0) return false;
        heist.cash += carryValue(carry, gCurrentLevel);
        carryClear(carry);
        return true;
    }
    if (action == HEIST_ACT_DROP) {
        int loot = carryPop(carry);
        if (loot < 0) return false;
        heistSpawnLoot(loot, pos);
        return true;
    }
    if (action == HEIST_ACT_START_DRILL) {
        if (!heistDrillReady() || !rigPosAtHopper(pos)) return false;
        heist.stage = HEIST_DRILLING;
        heist.stageTimer = 0.0f;
        return true;
    }
    if (action == HEIST_ACT_REPAIR) return rigHostRepair(pid);
    if (action == HEIST_ACT_BATTERY) return rigHostTakeBattery(pid);
    return false;
}

inline bool heistHostBuy(int pid, int item) {
    Vec3 pos;
    if (!heist.hasKiosk || heist.stage != HEIST_RUN || !heistPlayerPos(pid, pos)) return false;
    if (!nearPoint2D(pos, heist.kioskPos, HEIST_KIOSK_REACH)) return false;
    int price = shopPrice(item, heistSquadSize());
    if (price <= 0 || heist.cash < price) return false;
    heist.cash -= price;
    if (item == SHOP_EXTRACT) {
        heist.stage = HEIST_EXTRACTED;
        heist.stageTimer = 0.0f;
        return true;
    }
    int type = item == SHOP_BATTERY ? ITEM_BATTERY : (item == SHOP_MED ? ITEM_MED_SPRAY : ITEM_PLUSH_TOY);
    Vec3 toBuyer = pos - heist.kioskPos;
    toBuyer.y = 0.0f;
    float len = toBuyer.len();
    if (len < 0.1f) toBuyer = Vec3(0.0f, 0.0f, 1.0f);
    else toBuyer = toBuyer / len;
    heistSpawnItem(type, heist.kioskPos + toBuyer * 1.6f);
    return true;
}

inline bool heistHostBreak(int pid, int side) {
    static const int dx[4] = {1, -1, 0, 0};
    static const int dz[4] = {0, 0, 1, -1};
    Vec3 pos;
    if (side < 0 || side > 3 || !heistPlayerPos(pid, pos)) return false;
    if (!heistBreakWall(heistCellX(pos) + dx[side], heistCellZ(pos) + dz[side])) return false;
    entityMgr.emitNoise(pos, HEIST_BREAK_NOISE, pid);
    addAttention(14.0f);
    return true;
}

inline bool heistHostStudy(int entityType) {
    if (entityType <= ENTITY_NONE || entityType > ENTITY_SHADOW) return false;
    int bit = 1 << entityType;
    if (heist.studiedMask & bit) return false;
    heist.studiedMask |= bit;
    heist.cash += studyReward(gCurrentLevel);
    return true;
}

inline void heistSpawnLesson() {
    static const int lessonLoot[HEIST_TUTORIAL_LOOT] = {LOOT_LAMP_TUBE, LOOT_CLOCK, LOOT_SAFE};
    int lesson = gHeistLessonsSpawned++;
    heistSpawnLoot(lessonLoot[lesson], heist.lessonPos[lesson]);
}

inline bool heistCarriesCore(int pid) {
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) if (heist.carry[pid][i] == LOOT_CORE) return true;
    return false;
}

// Somebody walked the core into the doorway. It slips, it lands, the way home is gone.
inline void heistTriggerAccident(int culprit) {
    for (int i = 0; i < HEIST_CARRY_SLOTS; i++) {
        if (heist.carry[culprit][i] == LOOT_CORE) heist.carry[culprit][i] = -1;
    }
    heist.stage = HEIST_ACCIDENT;
    heist.stageTimer = 0.0f;
    gHeistSkipTutorial = true;
    heistSealPortal();
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    applyRoamEvent(ROAM_LIGHTS_OUT, 0, 0, 7.0f);
    if (multiState == MULTI_IN_GAME) netMgr.sendRoamEvent(ROAM_LIGHTS_OUT, 0, 0, 7.0f);
}

inline void heistFinishDrilling() {
    if (multiState == MULTI_IN_GAME) {
        hostAdvanceLevel(false);
        return;
    }
    gCurrentLevel++;
    gCompletedLevels++;
    reloadLevelAndResume(nullptr);
}

inline void heistHostUpdate(float dt) {
    if (!heistIsAuthority()) return;
    heist.stageTimer += dt;
    Vec3 pos;

    if (heist.stage == HEIST_TUT_GARAGE) {
        gHeistLessonsSpawned = 0;
        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (!heistPlayerPos(p, pos) || heistPosInGarage(pos)) continue;
            heist.stage = HEIST_TUT_HAUL;
            heist.stageTimer = 0.0f;
            break;
        }
    } else if (heist.stage == HEIST_TUT_HAUL) {
        if (heist.tutDelivered >= HEIST_TUTORIAL_LOOT) {
            heist.stage = HEIST_TUT_CORE;
            heist.stageTimer = 0.0f;
            heistSpawnLoot(LOOT_CORE, heist.corePos);
        } else if (gHeistLessonsSpawned <= heist.tutDelivered) {
            heistSpawnLesson();
        }
    } else if (heist.stage == HEIST_TUT_CORE) {
        for (int p = 0; p < MAX_PLAYERS; p++) {
            if (!heistPlayerPos(p, pos) || !heistCarriesCore(p)) continue;
            if (!nearPoint2D(pos, heist.portalPos, HEIST_PORTAL_TRIP_RANGE)) continue;
            heistTriggerAccident(p);
            break;
        }
    } else if (heist.stage == HEIST_ACCIDENT) {
        if (heist.stageTimer >= HEIST_ACCIDENT_SECONDS) heistEnterRun();
    } else if (heist.stage == HEIST_DRILLING) {
        if (heist.stageTimer >= HEIST_DRILL_SECONDS) heistFinishDrilling();
        return;
    } else if (heist.stage == HEIST_EXTRACTED) {
        if (!gRunComplete) hostFinishRun();
        return;
    }

    rigHostUpdate(dt);

    // Whoever goes down lets go of everything.
    for (int p = 0; p < MAX_PLAYERS; p++) {
        if (carryCount(heist.carry[p]) == 0 || heistPlayerStanding(p)) continue;
        if (heistPlayerPos(p, pos)) heistDropAll(p, pos);
        else carryClear(heist.carry[p]);
    }
}

// Requests coming from clients (see processHostInteractRequests).
inline void heistHostHandleRequest(int pid, int type, int target) {
    if (pid < 0 || pid >= MAX_PLAYERS) return;
    if (type == REQ_HEIST_ACTION) heistHostAction(pid, target);
    else if (type == REQ_HEIST_BREAK) heistHostBreak(pid, target);
    else if (type == REQ_HEIST_BUY) heistHostBuy(pid, target);
    else if (type == REQ_HEIST_STUDY) heistHostStudy(target);
}
