#pragma once
// Heist world: stamps the garage + portal on the first floor, places the drill, the rival
// kiosk, loot and junk piles, and owns the breakable (cracked) walls.

#include "objective_layout.h"

inline constexpr int GARAGE_HALF_W = 1;   // interior spans base x -1 .. +1
inline constexpr int GARAGE_NEAR_Z = -4;  // interior rows, relative to the base cell
inline constexpr int GARAGE_FAR_Z = -7;
inline constexpr int PORTAL_DZ = -3;
inline const float HEIST_DARK_RADIUS = CS * 2.7f;

inline bool isFallCell(int wx, int wz);

#include "rig_world.h"

inline int heistCellX(const Vec3& p) { return (int)floorf(p.x / CS); }
inline int heistCellZ(const Vec3& p) { return (int)floorf(p.z / CS); }

inline bool heistInGarageRect(int wx, int wz, int grow) {
    if (!heist.hasGarage) return false;
    int dx = wx - heist.baseX, dz = wz - heist.baseZ;
    return dx >= -GARAGE_HALF_W - grow && dx <= GARAGE_HALF_W + grow &&
           dz >= GARAGE_FAR_Z - grow && dz <= GARAGE_NEAR_Z + grow;
}

// Cells that belong to a built set: no random props, pits or cracks there.
inline bool heistIsReservedCell(int wx, int wz) {
    if (rigIsLaneCell(wx, wz, RIG_LANE_KEEP_CLEAR)) return true;
    if (heist.hasKiosk && wx == heistCellX(heist.kioskPos) && wz == heistCellZ(heist.kioskPos)) return true;
    if (!heist.hasGarage) return false;
    int dx = wx - heist.baseX, dz = wz - heist.baseZ;
    if (heistInGarageRect(wx, wz, 1)) return true;
    if (dx == 0 && dz <= 0 && dz >= PORTAL_DZ) return true;        // portal + approach corridor
    if (dz == -1 && dx >= -3 && dx <= -1) return true;             // tutorial vault and its wall
    if (dx == -2 && (dz == 0 || dz == -2)) return true;
    return false;
}

inline bool heistPosInGarage(const Vec3& p) {
    // The garage itself plus the portal cell: not the whole half of the floor behind them.
    int wx = heistCellX(p), wz = heistCellZ(p);
    if (heistInGarageRect(wx, wz, 0)) return true;
    return heist.hasGarage && wx == heist.baseX && wz == heist.baseZ + PORTAL_DZ;
}

inline void heistSpawnItem(int type, const Vec3& pos) {
    WorldItem it;
    it.id = nextWorldItemId++ % 250;
    it.type = type;
    it.pos = Vec3(pos.x, 0.0f, pos.z);
    it.active = true;
    worldItems.push_back(it);
}

inline void heistSpawnLoot(int loot, const Vec3& pos) {
    heistSpawnItem(itemTypeFromLoot(loot), pos);
}

// ---- cracked walls --------------------------------------------------------------------
inline bool heistIsTutorialCrack(int wx, int wz) {
    return heist.hasGarage && wx == heist.baseX - 1 && wz == heist.baseZ - 1;
}

inline bool isCrackedWall(int wx, int wz) {
    if (getCellWorld(wx, wz) != 1) return false;
    if (heistIsTutorialCrack(wx, wz)) return true;
    if (heistIsReservedCell(wx, wz)) return false;
    bool l = getCellWorld(wx - 1, wz) == 0, r = getCellWorld(wx + 1, wz) == 0;
    bool b = getCellWorld(wx, wz - 1) == 0, f = getCellWorld(wx, wz + 1) == 0;
    if (!((l && r) || (b && f))) return false;
    return crackRoll(hashCell(wx, wz, 0xC4AC3Du));
}

inline bool heistCrackHasCache(int wx, int wz) {
    return (hashCell(wx, wz, 0x5AFEu) % 100u) < 60u;
}

inline void heistCarveBreak(int wx, int wz) {
    setCellPersistent(wx, wz, 0);
}

// Host/solo: knocks the wall out, remembers it for the squad and drops what was inside.
inline bool heistBreakWall(int wx, int wz) {
    if (!isCrackedWall(wx, wz) || heist.breakCount >= HEIST_MAX_BREAKS) return false;
    bool tutorial = heistIsTutorialCrack(wx, wz);
    bool cache = !tutorial && heistCrackHasCache(wx, wz);
    heistCarveBreak(wx, wz);
    heist.breakDX[heist.breakCount] = (signed char)(wx - heist.baseX);
    heist.breakDZ[heist.breakCount] = (signed char)(wz - heist.baseZ);
    heist.breakCount++;
    gHeistAppliedBreaks = heist.breakCount;
    if (cache) heistSpawnLoot(rollCacheLoot(hashCell(wx, wz, 0x10075u)), heistCellCenter(wx, wz));
    buildGeom();
    return true;
}

// ---- first floor: garage, portal, tutorial spots --------------------------------------
inline void heistStampGarage() {
    int bx = heist.baseX, bz = heist.baseZ;
    for (int dx = -GARAGE_HALF_W - 1; dx <= GARAGE_HALF_W + 1; dx++) {
        for (int dz = GARAGE_FAR_Z - 1; dz <= GARAGE_NEAR_Z + 1; dz++) {
            bool inside = dx >= -GARAGE_HALF_W && dx <= GARAGE_HALF_W && dz >= GARAGE_FAR_Z && dz <= GARAGE_NEAR_Z;
            setCellPersistent(bx + dx, bz + dz, inside ? 0 : 1);
        }
    }
    for (int dz = PORTAL_DZ; dz <= 0; dz++) setCellPersistent(bx, bz + dz, 0);
    // Vault behind a cracked wall: the wall is the lesson, the vault holds the prize.
    setCellPersistent(bx - 1, bz - 1, 1);
    setCellPersistent(bx - 2, bz - 1, 0);
    setCellPersistent(bx - 3, bz - 1, 1);
    setCellPersistent(bx - 2, bz, 1);
    setCellPersistent(bx - 2, bz - 2, 1);
    heist.portalPos = heistCellCenter(bx, bz + PORTAL_DZ);
    heist.padPos = heistCellCenter(bx + 1, bz + GARAGE_NEAR_Z - 1);
}

inline void heistSealPortal() {
    forcedDoorways.clear();
    setCellPersistent(heist.baseX, heist.baseZ + PORTAL_DZ, 1);
}

inline int heistReachCount(const ObjectiveGrid& grid) {
    int n = 0;
    for (unsigned char r : grid.reach) n += r ? 1 : 0;
    return n;
}

// The stamps may wall the arrival area in; dig a straight way out until the maze is in reach.
inline void heistEnsureBaseConnected() {
    for (int step = 1; step <= 14; step++) {
        ObjectiveGrid grid;
        buildObjectiveGrid(grid, heistCellCenter(heist.baseX, heist.baseZ), false);
        if (heistReachCount(grid) >= 90) return;
        setCellPersistent(heist.baseX, heist.baseZ + step, 0);
    }
}

inline void heistPlaceSets(const Vec3& base) {
    ObjectiveGrid grid;
    buildObjectiveGrid(grid, base, false);
    markObjectiveCellUsed(grid, base);
    if (heist.hasGarage) {
        for (int dz = PORTAL_DZ; dz < 0; dz++) markObjectiveCellUsed(grid, heistCellCenter(heist.baseX, heist.baseZ + dz));
        for (int dx = -GARAGE_HALF_W; dx <= GARAGE_HALF_W; dx++) {
            for (int dz = GARAGE_FAR_Z; dz <= GARAGE_NEAR_Z; dz++) {
                markObjectiveCellUsed(grid, heistCellCenter(heist.baseX + dx, heist.baseZ + dz));
            }
        }
    }
    // nothing is built on the stretch of lane the rig is about to drive through
    for (int step = RIG_LANE_BACK_STEP; step <= 14; step++) {
        int wx = 0, wz = 0;
        rigLaneCell(step, wx, wz);
        markObjectiveCellUsed(grid, heistCellCenter(wx, wz));
    }

    std::mt19937 r(worldSeed ^ 0x4E157u);
    if (heist.hasKiosk) heist.kioskPos = snapObjectiveToFloor(grid, rigKioskWant(r));
    if (heist.hasGarage) {
        heist.lessonPos[LESSON_CARRY] = base;
        heist.lessonPos[LESSON_LIGHT] = snapObjectiveToFloor(grid, Vec3(base.x + CS * 4.0f, 0.0f, base.z + CS * 5.0f));
        heist.lessonPos[LESSON_BREAK] = heistCellCenter(heist.baseX - 2, heist.baseZ - 1);
        heist.corePos = snapObjectiveToFloor(grid, Vec3(base.x - CS * 3.0f, 0.0f, base.z + CS * 7.0f));
    }
}

// Host/solo: loot lying around plus junk piles worth rummaging through. It is laid out along
// the way the rig is heading; more turns up ahead of it as it crawls (see rig_host.h).
inline void heistScatterLoot() {
    ObjectiveGrid grid;
    Vec3 base = heistCellCenter(heist.baseX, heist.baseZ);
    buildObjectiveGrid(grid, base, false);
    std::vector<std::pair<int, int>> cells;
    for (int wx = heist.baseX - OBJECTIVE_GRID_RADIUS; wx <= heist.baseX + OBJECTIVE_GRID_RADIUS; wx++) {
        for (int wz = heist.baseZ - OBJECTIVE_GRID_RADIUS; wz <= heist.baseZ + OBJECTIVE_GRID_RADIUS; wz++) {
            int idx = grid.index(wx, wz);
            if (idx < 0 || !grid.reach[idx] || heistIsReservedCell(wx, wz) || isFallCell(wx, wz)) continue;
            int dx = wx - heist.baseX, dz = wz - heist.baseZ;
            if (dx * dx + dz * dz < 9) continue;
            int along = 0, across = 0;
            rigCellOffset(wx, wz, along, across);
            if (along < -4 || along > 16 || abs(across) > 10) continue;
            cells.push_back({wx, wz});
        }
    }
    std::shuffle(cells.begin(), cells.end(), rng);
    int lootCount = 9 + gCurrentLevel;
    if (lootCount > 13) lootCount = 13;
    int stashCount = 6;
    for (size_t i = 0; i < cells.size() && (int)i < lootCount + stashCount; i++) {
        Vec3 pos = heistCellCenter(cells[i].first, cells[i].second);
        pos.x += ((float)(rng() % 200u) / 100.0f - 1.0f) * CS * 0.22f;
        pos.z += ((float)(rng() % 200u) / 100.0f - 1.0f) * CS * 0.22f;
        if ((int)i < lootCount) heistSpawnLoot(rollLootType((unsigned)rng(), gCurrentLevel), pos);
        else heistSpawnItem(ITEM_STASH, pos);
    }
}

inline void heistEnterRun() {
    heist.stage = HEIST_RUN;
    heist.stageTimer = 0.0f;
    heist.quotaHave = 0;
    heist.quotaNeed = drillQuotaForLevel(gCurrentLevel);
    if (heistIsAuthority()) heistScatterLoot();
}

// Called at the end of genWorld, after the old world items were cleared.
inline void heistBeginLevel(const Vec3& base) {
    // The wallet and whatever is in the squad's hands ride the drill down to the next floor.
    HeistState before = heist;
    heist = HeistState();
    for (int p = 0; p < MAX_PLAYERS; p++) carryClear(heist.carry[p]);
    if (gCurrentLevel > 0) {
        heist.cash = before.cash;
        memcpy(heist.carry, before.carry, sizeof(heist.carry));
    }
    for (int i = 0; i < HEIST_TOAST_MAX; i++) gHeistToasts[i] = {};
    gHeistAppliedBreaks = 0;
    gHeistHold = gHeistStudy = gHeistFlash = 0.0f;
    gHeistShopOpen = false;

    heist.baseX = heistCellX(base);
    heist.baseZ = heistCellZ(base);
    heist.hasGarage = gCurrentLevel == 0;
    heist.hasKiosk = gCurrentLevel > 0;
    forcedDoorways.clear();

    bool tutorial = heist.hasGarage && !(heistIsAuthority() && gHeistSkipTutorial);
    if (heist.hasGarage) heistStampGarage();
    rigPlace();
    if (heist.hasGarage) {
        heistEnsureBaseConnected();
        if (tutorial) forcedDoorways.push_back({heist.baseX, heist.baseZ + PORTAL_DZ});
        else heistSealPortal();
    }
    heistPlaceSets(base);

    // Lamps and spawn slots were picked before the stamps: fix whatever ended up inside a wall.
    lights.erase(std::remove_if(lights.begin(), lights.end(), [](const Light& l) {
        return getCellWorld(heistCellX(l.pos), heistCellZ(l.pos)) == 1;
    }), lights.end());
    // Everybody rides the rig down: the floor starts in the break room.
    rigSeatPlayer(heistMe());

    if (tutorial) {
        heist.stage = HEIST_TUT_GARAGE;
        Vec3 home = heistCellCenter(heist.baseX, heist.baseZ + GARAGE_NEAR_Z - 2);
        cam.pos = Vec3(home.x + ((float)heistMe() - 1.5f) * 1.3f, PH, home.z);
        cam.yaw = 0.0f;
        cam.pitch = 0.0f;
        return;
    }
    heistEnterRun();
}

// Lamps the heist cares about. The garage is the bright, ordinary side of the portal, so it
// always has its own lamps; tutorial lesson two happens in the dark, so the lamps around
// that spot stay off. Called before every mesh rebuild (lamps are regenerated with chunks).
inline void heistApplyDarkZone() {
    if (!heist.hasGarage || !heistStageIsSafe(heist.stage)) return;
    for (auto& l : lights) {
        if (nearPoint2D(l.pos, heist.lessonPos[LESSON_LIGHT], HEIST_DARK_RADIUS)) l.on = false;
        if (heistPosInGarage(l.pos)) l.on = true;
    }
    for (int dz = GARAGE_FAR_Z; dz <= GARAGE_NEAR_Z; dz++) {
        for (int dx = -GARAGE_HALF_W; dx <= GARAGE_HALF_W; dx += 2) {
            Vec3 at = heistCellCenter(heist.baseX + dx, heist.baseZ + dz);
            bool have = false;
            for (const auto& l : lights) if (nearPoint2D(l.pos, at, 0.5f)) have = true;
            if (have) continue;
            Light l;
            l.pos = Vec3(at.x, WH - 0.03f, at.z);
            l.sizeX = l.sizeZ = 1.2f;
            l.intensity = 1.0f;
            l.on = true;
            lights.push_back(l);
        }
    }
}

// Clients: follow the host. Layout changes (portal sealed, walls broken) are replayed locally.
inline void applyHeistNet(const NetHeistState& in) {
    bool sealNow = heist.hasGarage && heistStageIsSafe(heist.stage) && !heistStageIsSafe(in.stage);
    heist.stage = in.stage;
    heist.tutDelivered = in.tutDelivered;
    heist.studiedMask = in.studiedMask;
    heist.breakCount = in.breakCount > HEIST_MAX_BREAKS ? HEIST_MAX_BREAKS : in.breakCount;
    heist.quotaHave = in.quotaHave;
    heist.quotaNeed = in.quotaNeed;
    heist.cash = in.cash;
    bool moved = !nearPoint2D(heist.kioskPos, in.kioskPos, 0.1f);
    // The rig is predicted locally (rig_play.h); the host only nudges it back on course.
    bool rigJumped = heist.rigDir != in.rigDir || !nearPoint2D(heist.rigStart, in.rigStart, 0.1f) ||
                     !nearPoint2D(heist.drillPos, in.drillPos, 1.5f);
    heist.rigDir = in.rigDir;
    heist.rigStart = in.rigStart;
    if (rigJumped) heist.drillPos = in.drillPos;
    else heist.drillPos = heist.drillPos + (in.drillPos - heist.drillPos) * 0.2f;
    // whole seconds on the wire; the local clock keeps the fractions (lamp flicker)
    if (heist.rigFault != in.rigFault || fabsf(heist.rigFaultSecs - (float)in.rigFaultSecs) > 1.5f) heist.rigFaultSecs = (float)in.rigFaultSecs;
    heist.rigFault = in.rigFault;
    heist.rigCharge = (float)in.rigCharge / 255.0f;
    heist.kioskPos = in.kioskPos;
    for (int i = 0; i < HEIST_TUTORIAL_LOOT; i++) heist.lessonPos[i] = in.lessonPos[i];
    heist.corePos = in.corePos;
    memcpy(heist.carry, in.carry, sizeof(heist.carry));
    memcpy(heist.breakDX, in.breakDX, sizeof(heist.breakDX));
    memcpy(heist.breakDZ, in.breakDZ, sizeof(heist.breakDZ));

    bool rebuilt = moved;
    if (sealNow) {
        heistSealPortal();
        updateLightsAndPillars(playerChunkX, playerChunkZ);
        rebuilt = true;
    }
    while (gHeistAppliedBreaks < heist.breakCount) {
        int i = gHeistAppliedBreaks++;
        heistCarveBreak(heist.baseX + heist.breakDX[i], heist.baseZ + heist.breakDZ[i]);
        rebuilt = true;
    }
    if (rebuilt) buildGeom();
}
