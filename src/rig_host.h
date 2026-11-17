#pragma once
// Rig, host side: breakdowns and repairs, the reserve battery running out, the battery
// charger and the loot that keeps turning up ahead of the rig. Runs in solo and on the
// co-op host. Included from heist_host.h.

inline constexpr int RIG_LOOT_CAP = 24;          // loot + junk piles alive at once (item snapshots hold 32)
inline constexpr float RIG_LOOT_BEHIND = 45.0f;  // what is left this far behind the rig is written off
inline constexpr int RIG_LOOT_NEAR = 3;          // new loot shows up this many cells ahead...
inline constexpr int RIG_LOOT_FAR = 8;           // ...up to this many
inline constexpr int RIG_LOOT_SIDE = 5;          // and this far off the lane

inline bool rigHostRepair(int pid) {
    Vec3 pos;
    if (heist.rigFault == RIG_OK || !heistPlayerPos(pid, pos)) return false;
    RigPoint p = rigLocal(pos);
    if (!rigCanFix(p.x, p.z, heist.rigFault, 0.4f)) return false;
    heist.rigFault = RIG_OK;
    heist.rigFaultSecs = 0.0f;
    heist.rigNextFault = rigFaultDelay(gCurrentLevel, (float)(rng() % 1000u) * 0.001f);
    return true;
}

inline bool rigHostTakeBattery(int pid) {
    Vec3 pos;
    if (!rigChargerReady() || !heistPlayerPos(pid, pos)) return false;
    RigPoint p = rigLocal(pos);
    if (!rigInCabin(p.x, p.z) || rigPointDist(p.x, p.z, RIG_CHARGER) > RIG_REACH + 0.4f) return false;
    heist.rigCharge = 0.0f;
    heistGiveSupply(pid, ITEM_BATTERY);
    return true;
}

inline bool rigCellHasItem(const Vec3& at) {
    for (const auto& it : worldItems) {
        if (it.active && nearPoint2D(it.pos, at, CS * 0.6f)) return true;
    }
    return false;
}

// Every cell the rig gains, the floor ahead gets a little more worth walking to and whatever
// was left far behind is forgotten. That is what keeps the crew moving with it.
inline void rigHostLayLoot() {
    float travelled = rigTravelled();
    int front = rigFrontStep(travelled, CS);
    if (front <= heist.rigLootStep) return;
    heist.rigLootStep = front;

    float dx = (float)rigDirX(heist.rigDir), dz = (float)rigDirZ(heist.rigDir);
    int live = 0;
    for (auto& it : worldItems) {
        if (!it.active || (!isLootItemType(it.type) && it.type != ITEM_STASH)) continue;
        Vec3 d = it.pos - heist.rigStart;
        if (d.x * dx + d.z * dz < travelled - RIG_LOOT_BEHIND) it.active = false;
        else live++;
    }
    if (live >= RIG_LOOT_CAP) return;

    ObjectiveGrid grid;
    buildObjectiveGrid(grid, heist.drillPos, false);
    std::vector<std::pair<int, int>> cells;
    for (int along = front + RIG_LOOT_NEAR; along <= front + RIG_LOOT_FAR; along++) {
        for (int across = -RIG_LOOT_SIDE; across <= RIG_LOOT_SIDE; across++) {
            int wx = 0, wz = 0;
            rigLaneCell(along, wx, wz);
            wx += rigDirZ(heist.rigDir) * across;
            wz -= rigDirX(heist.rigDir) * across;
            int idx = grid.index(wx, wz);
            if (idx < 0 || !grid.reach[idx] || heistIsReservedCell(wx, wz) || isFallCell(wx, wz)) continue;
            if (rigCellHasItem(heistCellCenter(wx, wz))) continue;
            cells.push_back({wx, wz});
        }
    }
    if (cells.empty()) return;
    auto cell = cells[rng() % cells.size()];
    Vec3 pos = heistCellCenter(cell.first, cell.second);
    pos.x += ((float)(rng() % 200u) / 100.0f - 1.0f) * CS * 0.22f;
    pos.z += ((float)(rng() % 200u) / 100.0f - 1.0f) * CS * 0.22f;
    if (front % 3 == 0) heistSpawnItem(ITEM_STASH, pos);
    else heistSpawnLoot(rollLootType((unsigned)rng(), gCurrentLevel), pos);
}

// A dead rig is not quiet about it: every backfire is heard across the floor.
inline void rigHostBackfire(float before, float now) {
    if (now < RIG_RESERVE_SECONDS) return;
    int was = before < RIG_RESERVE_SECONDS ? -1 : (int)((before - RIG_RESERVE_SECONDS) / RIG_BACKFIRE_SECONDS);
    int is = (int)((now - RIG_RESERVE_SECONDS) / RIG_BACKFIRE_SECONDS);
    if (is == was) return;
    entityMgr.emitNoise(heist.drillPos, RIG_BACKFIRE_NOISE, -1);
    addAttention(5.0f);
}

inline void rigHostUpdate(float dt) {
    if (heist.stage != HEIST_RUN) return;
    if (heist.rigFault != RIG_OK) {
        float before = heist.rigFaultSecs;
        heist.rigFaultSecs += dt;
        rigHostBackfire(before, heist.rigFaultSecs);
        return;
    }
    heist.rigCharge += dt / RIG_CHARGE_SECONDS;
    if (heist.rigCharge > 1.0f) heist.rigCharge = 1.0f;
    if (!rigMoving()) return;

    rigHostLayLoot();
    heist.rigNextFault -= dt;
    if (heist.rigNextFault > 0.0f) return;
    heist.rigFault = rigRollFault((unsigned)rng());
    heist.rigFaultSecs = 0.0f;
    setTrapStatus("THE DRILL STOPPED AGAIN. THE LAMP IS ON THE SPARE BATTERY");
}
