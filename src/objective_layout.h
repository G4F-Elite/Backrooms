#pragma once
// Objective layout: finds reachable floor cells for the built sets (drill, kiosk, lesson
// spots), keeps them open after world mutations, and places the abyss away from them.

#include <vector>
#include <random>

inline constexpr int OBJECTIVE_GRID_RADIUS = 26;
inline constexpr int OBJECTIVE_GRID_SIDE = OBJECTIVE_GRID_RADIUS * 2 + 1;

struct ObjectiveGrid {
    int baseX = 0;
    int baseZ = 0;
    std::vector<unsigned char> reach;
    std::vector<unsigned char> used;

    int index(int wx, int wz) const {
        int lx = wx - baseX + OBJECTIVE_GRID_RADIUS;
        int lz = wz - baseZ + OBJECTIVE_GRID_RADIUS;
        if (lx < 0 || lz < 0 || lx >= OBJECTIVE_GRID_SIDE || lz >= OBJECTIVE_GRID_SIDE) return -1;
        return lx * OBJECTIVE_GRID_SIDE + lz;
    }
};

inline int objectiveCellCoord(float v) {
    return (int)floorf(v / CS);
}

// Flood fill from the squad spawn so objectives never land in sealed pockets.
// With blockExitDoor the locked exit door counts as a wall: objectives must be reachable without it.
inline void buildObjectiveGrid(ObjectiveGrid& grid, const Vec3& base, bool blockExitDoor) {
    grid.baseX = objectiveCellCoord(base.x);
    grid.baseZ = objectiveCellCoord(base.z);
    grid.reach.assign(OBJECTIVE_GRID_SIDE * OBJECTIVE_GRID_SIDE, 0);
    grid.used.assign(OBJECTIVE_GRID_SIDE * OBJECTIVE_GRID_SIDE, 0);

    int doorX = objectiveCellCoord(coop.doorPos.x);
    int doorZ = objectiveCellCoord(coop.doorPos.z);
    std::vector<std::pair<int, int>> open;
    if (getCellWorld(grid.baseX, grid.baseZ) == 0) {
        grid.reach[grid.index(grid.baseX, grid.baseZ)] = 1;
        open.push_back({grid.baseX, grid.baseZ});
    }
    const int dx[4] = {1, -1, 0, 0};
    const int dz[4] = {0, 0, 1, -1};
    for (size_t head = 0; head < open.size(); head++) {
        int cx = open[head].first;
        int cz = open[head].second;
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d];
            int nz = cz + dz[d];
            int idx = grid.index(nx, nz);
            if (idx < 0 || grid.reach[idx] || getCellWorld(nx, nz) != 0) continue;
            if (blockExitDoor && nx == doorX && nz == doorZ) continue;
            grid.reach[idx] = 1;
            open.push_back({nx, nz});
        }
    }
}

inline void markObjectiveCellUsed(ObjectiveGrid& grid, const Vec3& pos) {
    int idx = grid.index(objectiveCellCoord(pos.x), objectiveCellCoord(pos.z));
    if (idx >= 0) grid.used[idx] = 1;
}

// Nearest reachable, unoccupied floor cell to the wanted spot (deterministic ring scan).
inline Vec3 snapObjectiveToFloor(ObjectiveGrid& grid, const Vec3& want) {
    int wantX = objectiveCellCoord(want.x);
    int wantZ = objectiveCellCoord(want.z);
    for (int r = 0; r <= OBJECTIVE_GRID_RADIUS; r++) {
        for (int ox = -r; ox <= r; ox++) {
            for (int oz = -r; oz <= r; oz++) {
                if (abs(ox) != r && abs(oz) != r) continue;
                int wx = wantX + ox;
                int wz = wantZ + oz;
                int idx = grid.index(wx, wz);
                if (idx < 0 || !grid.reach[idx] || grid.used[idx]) continue;
                float cx = (wx + 0.5f) * CS;
                float cz = (wz + 0.5f) * CS;
                if (collideWorld(cx, cz, 0.42f)) continue;
                grid.used[idx] = 1;
                return Vec3(cx, 0.0f, cz);
            }
        }
    }
    return want;
}

// Every spot a built set stands on: wall shifts and pits must leave these alone.
template <typename Fn>
inline void forEachObjectivePos(Fn fn) {
    fn(heist.drillPos);
    if (heist.hasKiosk) fn(heist.kioskPos);
    if (!heist.hasGarage) return;
    for (int dz = -3; dz <= 0; dz++) fn(Vec3(((float)heist.baseX + 0.5f) * CS, 0.0f, ((float)(heist.baseZ + dz) + 0.5f) * CS));
    for (int i = 0; i < HEIST_TUTORIAL_LOOT; i++) fn(heist.lessonPos[i]);
    fn(heist.corePos);
}

// Wall reshuffles may close a cell an objective stands on; reopen those cells.
inline void reopenObjectiveCells() {
    bool portalOpen = heistStageIsSafe(heist.stage);
    forEachObjectivePos([&](const Vec3& p) {
        int wx = objectiveCellCoord(p.x), wz = objectiveCellCoord(p.z);
        if (!portalOpen && heist.hasGarage && wx == heist.baseX && wz == heist.baseZ - 3) return;
        setCellWorld(wx, wz, 0);
    });
}

inline bool isNearAnyObjective(int wx, int wz, int cellMargin) {
    bool nearAny = false;
    forEachObjectivePos([&](const Vec3& p) {
        int dx = objectiveCellCoord(p.x) - wx;
        int dz = objectiveCellCoord(p.z) - wz;
        if (dx * dx + dz * dz <= cellMargin * cellMargin) nearAny = true;
    });
    return nearAny;
}

// The abyss is seeded from the world seed so every peer carves the same pit.
inline void placeAbyss(const Vec3& base) {
    abyss = {};
    if (isLevelZero(gCurrentLevel)) return; // the first floor is the tutorial: no bottomless pits
    std::mt19937 pitRng(worldSeed ^ 0x0AB155u);
    int baseX = objectiveCellCoord(base.x);
    int baseZ = objectiveCellCoord(base.z);
    int radius = 3 + (int)(pitRng() % 3);

    for (int attempt = 0; attempt < 60; attempt++) {
        int offX = 10 + (int)(pitRng() % 20);
        int offZ = 10 + (int)(pitRng() % 20);
        if (pitRng() % 2) offX = -offX;
        if (pitRng() % 2) offZ = -offZ;
        int cx = baseX + offX;
        int cz = baseZ + offZ;

        bool centerOpen = true;
        for (int dx = -1; dx <= 1 && centerOpen; dx++) {
            for (int dz = -1; dz <= 1 && centerOpen; dz++) {
                if (getCellWorld(cx + dx, cz + dz) != 0) centerOpen = false;
            }
        }
        if (!centerOpen || isNearAnyObjective(cx, cz, radius + 4)) continue;

        abyss.centerX = cx;
        abyss.centerZ = cz;
        abyss.radius = radius;
        abyss.active = true;
        int carve = radius + 1;
        for (int dx = -carve; dx <= carve; dx++) {
            for (int dz = -carve; dz <= carve; dz++) {
                if (dx * dx + dz * dz <= carve * carve) setCellWorld(cx + dx, cz + dz, 0);
            }
        }
        return;
    }
}

// Co-op spawn slots: the Nth nearest reachable floor cell to the squad spawn, same on every peer.
inline Vec3 squadSpawnSlot(const Vec3& base, int slot) {
    ObjectiveGrid grid;
    buildObjectiveGrid(grid, base, false);
    Vec3 pos = base;
    for (int i = 0; i <= slot; i++) pos = snapObjectiveToFloor(grid, base);
    return pos;
}
