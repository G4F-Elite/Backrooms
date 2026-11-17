#pragma once
// Grid navigation for monsters: line of sight, bounded A* and reachable-cell picking.
// Pure functions templated on a cell query (0 = open, anything else = wall), so the
// tests can run them on a hand-drawn maze without the world streaming code.

#include <cmath>
#include <cstdlib>

struct NavCell {
    int x;
    int z;
};

inline constexpr int NAV_WINDOW = 40;           // side of the square search window, in cells
inline constexpr int NAV_NODES = NAV_WINDOW * NAV_WINDOW;
inline constexpr int NAV_PATH_KEEP = 24;        // waypoints an entity remembers before repathing

inline int navCellCoord(float v, float cs) {
    return (int)floorf(v / cs);
}

inline float navCellCenter(int c, float cs) {
    return ((float)c + 0.5f) * cs;
}

// Walks the grid cells crossed by the segment (Amanatides & Woo). The start cell is not tested,
// so something standing flush against a wall can still see out.
template <typename CellAt>
inline bool navLineOfSight(float ax, float az, float bx, float bz, float cs, CellAt&& cellAt) {
    int cx = navCellCoord(ax, cs), cz = navCellCoord(az, cs);
    int ex = navCellCoord(bx, cs), ez = navCellCoord(bz, cs);
    float dx = bx - ax, dz = bz - az;
    int stepX = dx > 0.0f ? 1 : -1;
    int stepZ = dz > 0.0f ? 1 : -1;
    float invX = fabsf(dx) > 1e-6f ? 1.0f / fabsf(dx) : 1e30f;
    float invZ = fabsf(dz) > 1e-6f ? 1.0f / fabsf(dz) : 1e30f;
    float edgeX = dx > 0.0f ? ((float)(cx + 1) * cs - ax) : (ax - (float)cx * cs);
    float edgeZ = dz > 0.0f ? ((float)(cz + 1) * cs - az) : (az - (float)cz * cs);
    float tMaxX = edgeX * invX;
    float tMaxZ = edgeZ * invZ;
    float tDeltaX = cs * invX;
    float tDeltaZ = cs * invZ;

    int guard = abs(ex - cx) + abs(ez - cz) + 2;
    while ((cx != ex || cz != ez) && guard-- > 0) {
        if (tMaxX < tMaxZ) {
            tMaxX += tDeltaX;
            cx += stepX;
        } else {
            tMaxZ += tDeltaZ;
            cz += stepZ;
        }
        if (cellAt(cx, cz) != 0) return false;
    }
    return true;
}

// Line of sight for a body of the given radius: both shoulders have to fit.
template <typename CellAt>
inline bool navWalkableLine(float ax, float az, float bx, float bz, float radius, float cs, CellAt&& cellAt) {
    float dx = bx - ax, dz = bz - az;
    float len = sqrtf(dx * dx + dz * dz);
    if (len < 1e-4f) return true;
    float px = -dz / len * radius, pz = dx / len * radius;
    // A shoulder that already pokes into a wall cell means the first step would scrape it.
    if (cellAt(navCellCoord(ax + px, cs), navCellCoord(az + pz, cs)) != 0) return false;
    if (cellAt(navCellCoord(ax - px, cs), navCellCoord(az - pz, cs)) != 0) return false;
    if (!navLineOfSight(ax + px, az + pz, bx + px, bz + pz, cs, cellAt)) return false;
    return navLineOfSight(ax - px, az - pz, bx - px, bz - pz, cs, cellAt);
}

struct NavWindow {
    int ox;
    int oz;

    bool contains(int x, int z) const {
        return x >= ox && x < ox + NAV_WINDOW && z >= oz && z < oz + NAV_WINDOW;
    }
    int index(int x, int z) const {
        return (x - ox) * NAV_WINDOW + (z - oz);
    }
    NavCell cell(int idx) const {
        return {ox + idx / NAV_WINDOW, oz + idx % NAV_WINDOW};
    }
};

inline NavWindow navWindowAround(NavCell a, NavCell b) {
    NavWindow w;
    w.ox = (a.x + b.x) / 2 - NAV_WINDOW / 2;
    w.oz = (a.z + b.z) / 2 - NAV_WINDOW / 2;
    return w;
}

// Bounded 4-connected A*. Writes the cells after `start` up to and including `goal` into `out`
// (at most maxOut of them, nearest first) and returns how many were written; 0 = no route.
// `costAt(x, z)` adds a penalty (in steps) for entering a cell: sneaky monsters use it to
// route around the places their prey is looking at.
template <typename CellAt, typename CostAt>
inline int navFindPathCost(NavCell start, NavCell goal, CellAt&& cellAt, CostAt&& costAt, NavCell* out, int maxOut) {
    if (start.x == goal.x && start.z == goal.z) return 0;
    NavWindow win = navWindowAround(start, goal);
    if (!win.contains(start.x, start.z) || !win.contains(goal.x, goal.z)) return 0;
    if (cellAt(goal.x, goal.z) != 0) return 0;

    static thread_local unsigned short gCost[NAV_NODES];
    static thread_local short parent[NAV_NODES];
    static thread_local unsigned char closed[NAV_NODES];
    static thread_local int heap[NAV_NODES * 4];
    static thread_local unsigned short heapKey[NAV_NODES * 4];
    for (int i = 0; i < NAV_NODES; i++) {
        gCost[i] = 0xFFFF;
        parent[i] = -1;
        closed[i] = 0;
    }

    int heapSize = 0;
    auto push = [&](int idx, unsigned short key) {
        if (heapSize >= NAV_NODES * 4) return;
        int i = heapSize++;
        while (i > 0) {
            int p = (i - 1) / 2;
            if (heapKey[p] <= key) break;
            heap[i] = heap[p];
            heapKey[i] = heapKey[p];
            i = p;
        }
        heap[i] = idx;
        heapKey[i] = key;
    };
    auto pop = [&]() {
        int top = heap[0];
        heapSize--;
        int lastIdx = heap[heapSize];
        unsigned short lastKey = heapKey[heapSize];
        int i = 0;
        while (true) {
            int c = i * 2 + 1;
            if (c >= heapSize) break;
            if (c + 1 < heapSize && heapKey[c + 1] < heapKey[c]) c++;
            if (heapKey[c] >= lastKey) break;
            heap[i] = heap[c];
            heapKey[i] = heapKey[c];
            i = c;
        }
        heap[i] = lastIdx;
        heapKey[i] = lastKey;
        return top;
    };
    auto heuristic = [&](int x, int z) {
        return (unsigned short)(abs(x - goal.x) + abs(z - goal.z));
    };

    int startIdx = win.index(start.x, start.z);
    int goalIdx = win.index(goal.x, goal.z);
    gCost[startIdx] = 0;
    push(startIdx, heuristic(start.x, start.z));

    static const int dirX[4] = {1, -1, 0, 0};
    static const int dirZ[4] = {0, 0, 1, -1};
    bool found = false;
    while (heapSize > 0) {
        int cur = pop();
        if (closed[cur]) continue;
        closed[cur] = 1;
        if (cur == goalIdx) {
            found = true;
            break;
        }
        NavCell c = win.cell(cur);
        for (int d = 0; d < 4; d++) {
            int nx = c.x + dirX[d], nz = c.z + dirZ[d];
            if (!win.contains(nx, nz)) continue;
            int ni = win.index(nx, nz);
            if (closed[ni] || cellAt(nx, nz) != 0) continue;
            int stepCost = 1 + costAt(nx, nz);
            unsigned short ng = (unsigned short)(gCost[cur] + stepCost);
            if (ng >= gCost[ni]) continue;
            gCost[ni] = ng;
            parent[ni] = (short)cur;
            push(ni, (unsigned short)(ng + heuristic(nx, nz)));
        }
    }
    if (!found) return 0;

    int length = 0;
    for (int idx = goalIdx; idx != startIdx; idx = parent[idx]) length++;
    int idx = goalIdx;
    for (int i = length - 1; i >= 0; i--) {
        if (i < maxOut) out[i] = win.cell(idx);
        idx = parent[idx];
    }
    return length < maxOut ? length : maxOut;
}

template <typename CellAt>
inline int navFindPath(NavCell start, NavCell goal, CellAt&& cellAt, NavCell* out, int maxOut) {
    return navFindPathCost(start, goal, cellAt, [](int, int) { return 0; }, out, maxOut);
}

// Flood fill from `start` and pick a reachable cell between minSteps and maxSteps away.
// With `away` set the cell farthest from it wins (retreat); otherwise `roll` picks one (roam).
// `accept(x, z)` can veto cells (e.g. "somebody would see a monster appear there").
// Returns false when nothing qualifies.
template <typename CellAt, typename Accept>
inline bool navPickReachableIf(NavCell start, int minSteps, int maxSteps, const NavCell* away, unsigned roll,
                               CellAt&& cellAt, Accept&& accept, NavCell& out) {
    NavWindow win = navWindowAround(start, start);
    static thread_local unsigned short depth[NAV_NODES];
    static thread_local int queue[NAV_NODES];
    for (int i = 0; i < NAV_NODES; i++) depth[i] = 0xFFFF;

    int head = 0, tail = 0;
    int startIdx = win.index(start.x, start.z);
    depth[startIdx] = 0;
    queue[tail++] = startIdx;

    static const int dirX[4] = {1, -1, 0, 0};
    static const int dirZ[4] = {0, 0, 1, -1};
    int candidates = 0;
    int bestScore = -1;
    bool have = false;
    while (head < tail) {
        int cur = queue[head++];
        NavCell c = win.cell(cur);
        int steps = depth[cur];
        if (steps >= minSteps && accept(c.x, c.z)) {
            if (away) {
                int score = abs(c.x - away->x) + abs(c.z - away->z);
                if (score > bestScore) {
                    bestScore = score;
                    out = c;
                    have = true;
                }
            } else {
                // Reservoir sampling keeps the pick uniform without storing the candidates.
                candidates++;
                roll = roll * 1664525u + 1013904223u;
                if ((roll >> 8) % (unsigned)candidates == 0u) {
                    out = c;
                    have = true;
                }
            }
        }
        if (steps >= maxSteps) continue;
        for (int d = 0; d < 4; d++) {
            int nx = c.x + dirX[d], nz = c.z + dirZ[d];
            if (!win.contains(nx, nz)) continue;
            int ni = win.index(nx, nz);
            if (depth[ni] != 0xFFFF || cellAt(nx, nz) != 0) continue;
            depth[ni] = (unsigned short)(steps + 1);
            queue[tail++] = ni;
        }
    }
    return have;
}

template <typename CellAt>
inline bool navPickReachable(NavCell start, int minSteps, int maxSteps, const NavCell* away, unsigned roll,
                             CellAt&& cellAt, NavCell& out) {
    return navPickReachableIf(start, minSteps, maxSteps, away, roll, cellAt, [](int, int) { return true; }, out);
}
