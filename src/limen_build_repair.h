#pragma once
// LIMEN slab builder: grows a slab from its genome, repairs connectivity without
// smashing room corners, guarantees navigable loops, carves walkable basin stairs,
// and stamps human-scale thin-wall & narrow-corridor cell facts.
#include "limen_grammar.h"

struct LimenPort { int lx, lz; };

const int LM_MAX_OPENINGS = 256;
const int LM_KEEP_CELLS = 13;    // stranded floor at least this big gets linked instead of sealed
const int LM_MAX_LINKS = 24;

// Interior cells that face an open seam cell, in a fixed order; perSide counts W, E, N, S.
inline int lmCollectOpenings(const LimenRect& r, LimenPort* out, int* perSide) {
    int n = 0, w = r.x1 - r.x0 - 1, h = r.z1 - r.z0 - 1;
    perSide[0] = perSide[1] = perSide[2] = perSide[3] = 0;
    LimenRect east = lmSlabRect(r.j, r.i + 1);
    for (int z = r.z0 + 1; z < r.z1; z++) {
        if (lmWallCellOpen(r, r.x0, z)) { out[n++] = {0, z - r.z0 - 1}; perSide[0]++; }
        if (lmWallCellOpen(east, east.x0, z)) { out[n++] = {w - 1, z - r.z0 - 1}; perSide[1]++; }
    }
    for (int x = r.x0 + 1; x < r.x1; x++) {
        if (lmWallCellOpen(r, x, r.z0)) { out[n++] = {x - r.x0 - 1, 0}; perSide[2]++; }
        LimenRect south = lmSlabRect(r.j + 1, lmColOf(r.j + 1, x));
        if (lmWallCellOpen(south, x, south.z0)) { out[n++] = {x - r.x0 - 1, h - 1}; perSide[3]++; }
    }
    return n;
}

// Turns any cell into something one can stand on, in the idiom of what surrounds it:
// a deck over a drop, a low doorway through a low shell, a passage through anything else.
inline void lmOpenCell(LimenSlab& s, int x, int z) {
    int t = s.tag[x][z];
    if (t == LT_VOID) { s.tag[x][z] = LT_BRIDGE; s.flag[x][z] = LF_OVER_VOID; return; }
    if (t == LT_BASIN) { s.tag[x][z] = LT_BRIDGE; s.flag[x][z] = LF_OVER_BASIN; return; }
    if (t != LT_WALL) return;
    bool low = false, mezz = false;
    if (lmSlabIsVault(s)) {
        low = (s.flag[x][z] & LF_LOW_WALL) != 0 ||
              lmIsLowShellTag(lmTagLocal(s, x - 1, z)) || lmIsLowShellTag(lmTagLocal(s, x + 1, z)) ||
              lmIsLowShellTag(lmTagLocal(s, x, z - 1)) || lmIsLowShellTag(lmTagLocal(s, x, z + 1));
        mezz = (s.flag[x][z] & LF_MEZZ) != 0 ||
               ((lmFlagLocal(s, x - 1, z) | lmFlagLocal(s, x + 1, z) |
                 lmFlagLocal(s, x, z - 1) | lmFlagLocal(s, x, z + 1)) & LF_MEZZ) != 0;
    }
    s.tag[x][z] = (unsigned char)(low ? LT_LOW : LT_CORR);
    s.flag[x][z] = (low && mezz) ? LF_MEZZ : 0u;
}

// Marks the walkable region containing (sx, sz) with `value`; returns its size.
inline int lmFlood(const LimenSlab& s, int sx, int sz, unsigned char mark[LM_MAX][LM_MAX], unsigned char value) {
    static short queue[LM_MAX * LM_MAX][2];
    static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
    if (!lmIsWalkableTag(s.tag[sx][sz]) || mark[sx][sz]) return 0;
    int head = 0, tail = 0;
    queue[tail][0] = (short)sx; queue[tail][1] = (short)sz; tail++;
    mark[sx][sz] = value;
    while (head < tail) {
        int x = queue[head][0], z = queue[head][1]; head++;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h) continue;
            if (mark[nx][nz] || !lmIsWalkableTag(s.tag[nx][nz])) continue;
            mark[nx][nz] = value;
            queue[tail][0] = (short)nx; queue[tail][1] = (short)nz; tail++;
        }
    }
    return tail;
}

// Architectural cost of stepping into (nx, nz): penalizes smashing room corners or cutting
// across sealed gap-doors, rewards walking existing floor and extending straight corridors.
inline int lmStepCost(const LimenSlab& s, int px, int pz, int nx, int nz, int gapX, int gapZ) {
    int nt = s.tag[nx][nz];
    if (lmIsWalkableTag(nt)) return (nt == LT_ROOM) ? 2 : 1;
    if (nx == gapX && nz == gapZ) return 60;
    if (nt == LT_BASIN) return 4;
    if (nt == LT_VOID) return 7;
    // nt == LT_WALL: check if cutting this wall cell mangles a room corner
    int roomCard = (lmTagLocal(s, nx - 1, nz) == LT_ROOM ? 1 : 0) +
                   (lmTagLocal(s, nx + 1, nz) == LT_ROOM ? 1 : 0) +
                   (lmTagLocal(s, nx, nz - 1) == LT_ROOM ? 1 : 0) +
                   (lmTagLocal(s, nx, nz + 1) == LT_ROOM ? 1 : 0);
    bool perpRooms = (lmTagLocal(s, nx - 1, nz) == LT_ROOM || lmTagLocal(s, nx + 1, nz) == LT_ROOM) &&
                     (lmTagLocal(s, nx, nz - 1) == LT_ROOM || lmTagLocal(s, nx, nz + 1) == LT_ROOM);
    if (perpRooms) return 22;   // L-corner of rooms: go around via corridor!
    int diagRooms = (lmTagLocal(s, nx - 1, nz - 1) == LT_ROOM ? 1 : 0) +
                    (lmTagLocal(s, nx + 1, nz - 1) == LT_ROOM ? 1 : 0) +
                    (lmTagLocal(s, nx - 1, nz + 1) == LT_ROOM ? 1 : 0) +
                    (lmTagLocal(s, nx + 1, nz + 1) == LT_ROOM ? 1 : 0);
    if (roomCard == 0 && diagRooms >= 2) return 18;  // pillar junction between rooms
    int backX = px - (nx - px), backZ = pz - (nz - pz);
    int backT = lmTagLocal(s, backX, backZ);
    if (backT == LT_CORR || backT == LT_LOW) return 5;  // clean straight corridor extension
    return (s.flag[nx][nz] & LF_LOW_WALL) ? 6 : 9;
}

// Cheapest architectural route from (sx, sz) to the main region (mark == 1).
inline bool lmLink(LimenSlab& s, int sx, int sz, const unsigned char mark[LM_MAX][LM_MAX], int gapX, int gapZ) {
    static int cost[LM_MAX][LM_MAX];
    static signed char from[LM_MAX][LM_MAX];
    static unsigned char queued[LM_MAX][LM_MAX];
    static short ring[LM_MAX * LM_MAX][2];
    static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
    const int N = LM_MAX * LM_MAX, INF = 1 << 28;
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) { cost[x][z] = INF; from[x][z] = -1; queued[x][z] = 0; }
    int head = 0, count = 0, best = INF, bestX = -1, bestZ = -1;
    cost[sx][sz] = 0;
    ring[0][0] = (short)sx; ring[0][1] = (short)sz; count = 1; queued[sx][sz] = 1;
    while (count > 0) {
        int x = ring[head][0], z = ring[head][1];
        head = (head + 1) % N; count--;
        queued[x][z] = 0;
        int c = cost[x][z];
        if (c >= best) continue;
        if (mark[x][z] == 1) { best = c; bestX = x; bestZ = z; continue; }
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h) continue;
            int step = lmStepCost(s, x, z, nx, nz, gapX, gapZ);
            if (c + step >= cost[nx][nz]) continue;
            cost[nx][nz] = c + step;
            from[nx][nz] = (signed char)d;
            if (queued[nx][nz]) continue;
            int tail = (head + count) % N;
            ring[tail][0] = (short)nx; ring[tail][1] = (short)nz; count++;
            queued[nx][nz] = 1;
        }
    }
    if (bestX < 0) return false;
    int x = bestX, z = bestZ;
    while (from[x][z] >= 0) {
        int d = from[x][z];
        lmOpenCell(s, x, z);
        x -= dx[d]; z -= dz[d];
    }
    return true;
}

inline void lmMarkMain(const LimenSlab& s, const LimenPort& p0, unsigned char mark[LM_MAX][LM_MAX]) {
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) if (mark[x][z] != 2) mark[x][z] = 0;
    lmFlood(s, p0.lx, p0.lz, mark, 1);
}

// Bounded walking distance between two cells (returns limit if >= limit).
inline int lmShortWalkDist(const LimenSlab& s, int ax, int az, int bx, int bz, int limit) {
    static short q[LM_MAX * LM_MAX][2];
    static unsigned char dist[LM_MAX][LM_MAX];
    static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) dist[x][z] = 255;
    int head = 0, tail = 0;
    dist[ax][az] = 0;
    q[tail][0] = (short)ax; q[tail][1] = (short)az; tail++;
    while (head < tail) {
        int x = q[head][0], z = q[head][1]; head++;
        int d0 = dist[x][z];
        if (x == bx && z == bz) return d0;
        if (d0 + 1 >= limit) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h) continue;
            if (dist[nx][nz] != 255 || !lmIsWalkableTag(s.tag[nx][nz])) continue;
            dist[nx][nz] = (unsigned char)(d0 + 1);
            q[tail][0] = (short)nx; q[tail][1] = (short)nz; tail++;
        }
    }
    return limit;
}

// Eliminates long dead-end tree branches by punching clean doorway loops across thin 1-cell
// walls whose two walkable sides are currently >= 16 walking steps apart.
inline void lmEnsureLoops(LimenSlab& s, int gapX, int gapZ, u32 h) {
    int added = 0;
    int startX = 1 + (int)(h % (u32)(s.w > 3 ? s.w - 2 : 1));
    for (int step = 0; step < s.w - 2 && added < 3; step++) {
        int x = 1 + (startX - 1 + step) % (s.w - 2);
        for (int z = 1; z < s.h - 1 && added < 3; z++) {
            if (s.tag[x][z] != LT_WALL || (x == gapX && z == gapZ)) continue;
            int ax = -1, az = -1, bx = -1, bz = -1;
            if (lmIsCirculationTag(s.tag[x - 1][z]) && lmIsCirculationTag(s.tag[x + 1][z]) &&
                s.tag[x][z - 1] == LT_WALL && s.tag[x][z + 1] == LT_WALL) {
                ax = x - 1; az = z; bx = x + 1; bz = z;
            } else if (lmIsCirculationTag(s.tag[x][z - 1]) && lmIsCirculationTag(s.tag[x][z + 1]) &&
                       s.tag[x - 1][z] == LT_WALL && s.tag[x + 1][z] == LT_WALL) {
                ax = x; az = z - 1; bx = x; bz = z + 1;
            }
            if (ax < 0) continue;
            if (lmShortWalkDist(s, ax, az, bx, bz, 16) >= 16) {
                lmOpenCell(s, x, z);
                added++;
            }
        }
    }
    // Measure actual cycle rank of circulation + room skeleton: E - V + 1 (excluding 2x2 open squares)
    int V = 0, E = 0, Q = 0;
    for (int x = 0; x < s.w; x++) {
        for (int z = 0; z < s.h; z++) {
            if (!lmIsWalkableTag(s.tag[x][z])) continue;
            V++;
            if (x + 1 < s.w && lmIsWalkableTag(s.tag[x + 1][z])) E++;
            if (z + 1 < s.h && lmIsWalkableTag(s.tag[x][z + 1])) E++;
            if (x + 1 < s.w && z + 1 < s.h && lmIsWalkableTag(s.tag[x + 1][z]) &&
                lmIsWalkableTag(s.tag[x][z + 1]) && lmIsWalkableTag(s.tag[x + 1][z + 1])) Q++;
        }
    }
    int cycles = E - V - Q + 1;
    s.loops = cycles > 0 ? cycles : added;
}

inline bool lmRepair(LimenSlab& s, const LimenPort* ports, int portCount, int gapX, int gapZ, u32 h) {
    static unsigned char mark[LM_MAX][LM_MAX];
    static short queue[LM_MAX * LM_MAX][2];
    static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
    if (portCount <= 0) return false;
    for (int k = 0; k < portCount; k++) lmOpenCell(s, ports[k].lx, ports[k].lz);
    memset(mark, 0, sizeof(mark));
    lmMarkMain(s, ports[0], mark);
    for (int k = 1; k < portCount; k++) {
        if (mark[ports[k].lx][ports[k].lz] == 1) continue;
        if (!lmLink(s, ports[k].lx, ports[k].lz, mark, gapX, gapZ)) return false;
        lmMarkMain(s, ports[0], mark);
    }
    int links = 0;
    s.sealed = 0;
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) {
        if (mark[x][z] || !lmIsWalkableTag(s.tag[x][z])) continue;
        int size = lmFlood(s, x, z, mark, 3);
        bool join = size >= LM_KEEP_CELLS && links < LM_MAX_LINKS && lmLink(s, x, z, mark, gapX, gapZ);
        if (join) { links++; lmMarkMain(s, ports[0], mark); continue; }
        for (int ax = 0; ax < s.w; ax++) for (int az = 0; az < s.h; az++) if (mark[ax][az] == 3) mark[ax][az] = 2;
    }
    // A low room never opens straight onto a drop: it gets one cell of deck as a threshold.
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) {
        if (mark[x][z] != 1 || !lmIsLowShellTag(s.tag[x][z])) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h || !lmIsPitTag(s.tag[nx][nz])) continue;
            lmOpenCell(s, nx, nz);
            mark[nx][nz] = 5;
        }
    }
    // Drops that border reachable floor stay; everything else unreachable becomes solid.
    int head = 0, tail = 0;
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) {
        if (mark[x][z] != 1 && mark[x][z] != 5) continue;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h || mark[nx][nz] == 4 || !lmIsPitTag(s.tag[nx][nz])) continue;
            mark[nx][nz] = 4;
            queue[tail][0] = (short)nx; queue[tail][1] = (short)nz; tail++;
        }
    }
    while (head < tail) {
        int x = queue[head][0], z = queue[head][1]; head++;
        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d], nz = z + dz[d];
            if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h || mark[nx][nz] == 4 || !lmIsPitTag(s.tag[nx][nz])) continue;
            mark[nx][nz] = 4;
            queue[tail][0] = (short)nx; queue[tail][1] = (short)nz; tail++;
        }
    }
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) {
        if (mark[x][z] == 1 || mark[x][z] >= 4 || s.tag[x][z] == LT_WALL) continue;
        bool low = lmSlabIsVault(s) && lmIsLowShellTag(s.tag[x][z]);
        unsigned short mz = (low && (s.flag[x][z] & LF_MEZZ)) ? LF_MEZZ : 0u;
        if (lmIsWalkableTag(s.tag[x][z])) s.sealed++;
        s.tag[x][z] = LT_WALL;
        s.flag[x][z] = low ? (LF_LOW_WALL | mz) : 0u;
    }
    s.links = links;
    lmEnsureLoops(s, gapX, gapZ, h);
    return true;
}

// Builds broad tiled staircases descending from y=0 terraces into drained LT_BASIN pools
// so players can physically walk down onto the dry tiled basin floor!
inline void lmBuildBasinStairs(LimenSlab& s, u32 h) {
    s.walkBasins = 0;
    if (!lmSlabIsVault(s)) return;
    if (s.megaForm == MF_ZIGGURAT || s.megaForm == MF_PLATFORM || lmIsMegaUnstruct(s.megaForm)) return;
    static unsigned char seen[LM_MAX][LM_MAX];
    static short comp[LM_MAX * LM_MAX][2];
    // Rim directions: 0 = rim at -Z, 1 = rim at +Z, 2 = rim at -X, 3 = rim at +X
    static const int rdx[4] = {0, 0, -1, 1}, rdz[4] = {-1, 1, 0, 0};
    memset(seen, 0, sizeof(seen));

    for (int sx = 1; sx < s.w - 1; sx++) {
        for (int sz = 1; sz < s.h - 1; sz++) {
            if (s.tag[sx][sz] != LT_BASIN || seen[sx][sz]) continue;
            int head = 0, tail = 0;
            seen[sx][sz] = 1;
            comp[tail][0] = (short)sx; comp[tail][1] = (short)sz; tail++;
            while (head < tail) {
                int x = comp[head][0], z = comp[head][1]; head++;
                for (int d = 0; d < 4; d++) {
                    int nx = x + rdx[d], nz = z + rdz[d];
                    if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h) continue;
                    if (seen[nx][nz] || s.tag[nx][nz] != LT_BASIN) continue;
                    seen[nx][nz] = 1;
                    comp[tail][0] = (short)nx; comp[tail][1] = (short)nz; tail++;
                }
            }
            if (tail < 4) continue;
            // Pick a clean boundary cell whose rim side is LT_HALL/LT_CORR and opposite side is LT_BASIN
            int bestX = -1, bestZ = -1, bestDir = 0;
            u32 bestScore = 0;
            for (int k = 0; k < tail; k++) {
                int x = comp[k][0], z = comp[k][1];
                if (x < 1 || z < 1 || x >= s.w - 1 || z >= s.h - 1) continue;
                for (int d = 0; d < 4; d++) {
                    int rt = lmTagLocal(s, x + rdx[d], z + rdz[d]);
                    if (rt != LT_HALL && rt != LT_CORR) continue;
                    if (lmTagLocal(s, x - rdx[d], z - rdz[d]) != LT_BASIN) continue;
                    u32 sc = 1u + (lmHash(h, (u32)(x * 67 + z), (u32)d, 0x4253u) & 0xFFFFu);
                    if (sc > bestScore) { bestScore = sc; bestX = x; bestZ = z; bestDir = d; }
                }
            }
            if (bestX < 0) continue;
            s.flag[bestX][bestZ] = (unsigned short)(LF_STAIR_DESC | LF_WALK_BASIN | lmPackStairDir(bestDir));
            for (int k = 0; k < tail; k++) {
                int x = comp[k][0], z = comp[k][1];
                s.flag[x][z] |= LF_WALK_BASIN;
                s.walkBasins++;
            }
        }
    }
}

// Guarantees physical player passability across 0.26 m thin partitions (`LF_THIN_W/N`):
// if any walkable pocket is isolated by doorless drywall, punches a clean `LF_THIN_DOOR_*`.
inline void lmEnsureThinPassability(LimenSlab& s) {
    static unsigned char vis[LM_MAX][LM_MAX];
    static short q[LM_MAX * LM_MAX][2];
    static const int dx[4] = {-1, 1, 0, 0}, dz[4] = {0, 0, -1, 1};

    auto canStepThin = [&](int ax, int az, int bx, int bz) -> bool {
        if (!lmIsWalkableTag(s.tag[ax][az]) || !lmIsWalkableTag(s.tag[bx][bz])) return false;
        if (bx == ax - 1 && (s.flag[ax][az] & (LF_THIN_W | LF_THIN_DOOR_W)) == LF_THIN_W) return false;
        if (bx == ax + 1 && (s.flag[bx][bz] & (LF_THIN_W | LF_THIN_DOOR_W)) == LF_THIN_W) return false;
        if (bz == az - 1 && (s.flag[ax][az] & (LF_THIN_N | LF_THIN_DOOR_N)) == LF_THIN_N) return false;
        if (bz == az + 1 && (s.flag[bx][bz] & (LF_THIN_N | LF_THIN_DOOR_N)) == LF_THIN_N) return false;
        return true;
    };

    int sx = -1, sz = -1;
    for (int x = 0; x < s.w && sx < 0; x++)
        for (int z = 0; z < s.h && sx < 0; z++)
            if (lmIsWalkableTag(s.tag[x][z])) { sx = x; sz = z; }
    if (sx < 0) return;

    for (int guard = 0; guard < 64; guard++) {
        memset(vis, 0, sizeof(vis));
        int head = 0, tail = 0;
        vis[sx][sz] = 1;
        q[tail][0] = (short)sx; q[tail][1] = (short)sz; tail++;
        while (head < tail) {
            int x = q[head][0], z = q[head][1]; head++;
            for (int d = 0; d < 4; d++) {
                int nx = x + dx[d], nz = z + dz[d];
                if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h || vis[nx][nz]) continue;
                if (!canStepThin(x, z, nx, nz)) continue;
                vis[nx][nz] = 1;
                q[tail][0] = (short)nx; q[tail][1] = (short)nz; tail++;
            }
        }
        bool punched = false;
        for (int x = 0; x < s.w && !punched; x++) {
            for (int z = 0; z < s.h && !punched; z++) {
                if (vis[x][z] || !lmIsWalkableTag(s.tag[x][z])) continue;
                if (x > 0 && vis[x - 1][z] && (s.flag[x][z] & LF_THIN_W)) { s.flag[x][z] |= LF_THIN_DOOR_W; punched = true; }
                else if (x + 1 < s.w && vis[x + 1][z] && (s.flag[x + 1][z] & LF_THIN_W)) { s.flag[x + 1][z] |= LF_THIN_DOOR_W; punched = true; }
                else if (z > 0 && vis[x][z - 1] && (s.flag[x][z] & LF_THIN_N)) { s.flag[x][z] |= LF_THIN_DOOR_N; punched = true; }
                else if (z + 1 < s.h && vis[x][z + 1] && (s.flag[x][z + 1] & LF_THIN_N)) { s.flag[x][z + 1] |= LF_THIN_DOOR_N; punched = true; }
            }
        }
        if (!punched) break;
    }
}

// Enforces strict cell-vocabulary invariants and stamps multi-height human-scale architecture.
// Membership of a slab in an operator family: affinity over the decision threshold (families may overlap).
inline bool lmSlabHasFam(const LimenSlab& s, int k) { return lmWingAffinityAt(s.genome.g, s.rect.j, s.rect.i, k) >= 128; }

inline void lmFinalizeCellFacts(LimenSlab& s) {
    bool vault = lmSlabIsVault(s);
    bool skyOpen = lmHasQuirk(gLimenDNA, LQ_SKY);
    bool famCorr = lmSlabHasFam(s, LM_WING_CORRIDORS), famEnf = lmSlabHasFam(s, LM_WING_ENFILADE);
    bool famConc = lmSlabHasFam(s, LM_WING_CONCOURSE), famSvc = lmSlabHasFam(s, LM_WING_SERVICE);
    bool famPart = lmSlabHasFam(s, LM_WING_PARTITIONS);
    bool wantNarrow = (!vault && (s.genome.g[LG_GRAIN] < 190 || famCorr || famSvc)) ||
                      (vault && s.genome.g[LG_GRAIN] < 195);
    bool lowRoomWing = !vault && (famPart || famSvc ||
                                  (famCorr && s.genome.g[LG_HEIGHT] < 138) ||
                                  s.genome.g[LG_HEIGHT] < 64);
    bool stdRoomWing = !vault && !skyOpen && !lowRoomWing && (famCorr || famEnf || famConc);
    s.thinCount = 0;
    s.narrowCount = 0;

    for (int x = 0; x < s.w; x++) {
        for (int z = 0; z < s.h; z++) {
            int t = s.tag[x][z];
            unsigned short f = s.flag[x][z];
            if (t == LT_WALL) {
                unsigned short keep = 0;
                if (vault && (f & LF_LOW_WALL)) {
                    keep |= LF_LOW_WALL;
                    if ((f & LF_MEZZ) && s.ceilTiers >= 3) keep |= LF_MEZZ;
                }
                if (f & LF_CHAMFER) keep |= LF_CHAMFER;
                s.flag[x][z] = keep;
                s.zone[x][z] &= LZ_XENO;
                continue;
            }
            if (t == LT_BRIDGE) {
                s.flag[x][z] = (unsigned short)(f & (LF_OVER_VOID | LF_OVER_BASIN));
                s.zone[x][z] &= LZ_XENO;
                continue;
            }
            if (t == LT_VOID || t == LT_VITRINE) {
                s.flag[x][z] = 0;
                s.zone[x][z] &= LZ_XENO;
                continue;
            }
            if (t == LT_BASIN) {
                s.flag[x][z] = (unsigned short)(f & (LF_STAIR_DESC | LF_WALK_BASIN | (3u << LF_STAIR_DIR_SHIFT)));
                s.zone[x][z] &= LZ_XENO;
                continue;
            }
            // Walkable floor: LT_CORR, LT_ROOM, LT_HALL, LT_LOW
            unsigned short keep = 0;
            unsigned char zn = s.zone[x][z];
            bool inXeno = (zn & LZ_XENO) != 0 && s.xeno.active;
            if (vault || skyOpen || inXeno) zn &= ~(unsigned char)(LZ_STD_CEIL | LZ_TALL_WELL);

            if (vault && lmIsLowShellTag(t) && (f & LF_MEZZ) && s.ceilTiers >= 3) keep |= LF_MEZZ;
            if (inXeno && (!vault || lmIsLowShellTag(t))) {
                if (s.xeno.ceilMode == 0) { keep = (unsigned short)((keep & ~LF_MEZZ) | LF_CRAMPED); }
                else if (s.xeno.ceilMode == 2) { keep = (unsigned short)((keep & ~LF_CRAMPED) | LF_MEZZ); }
            }
            if ((f & LF_THIN_W) && x > 0 && lmIsWalkableTag(s.tag[x - 1][z])) {
                keep |= LF_THIN_W | (f & LF_THIN_DOOR_W);
                s.thinCount++;
            }
            if ((f & LF_THIN_N) && z > 0 && lmIsWalkableTag(s.tag[x][z - 1])) {
                keep |= LF_THIN_N | (f & LF_THIN_DOOR_N);
                s.thinCount++;
            }

            // Foreshadowing Halo border facing an adjacent megastructure attractor: compress to 2.72 m throat!
            bool inHaloBand = !vault && !inXeno &&
                ((s.haloDir == 0 && x < 4) || (s.haloDir == 1 && x >= s.w - 4) ||
                 (s.haloDir == 2 && z < 4) || (s.haloDir == 3 && z >= s.h - 4));
            if (inHaloBand) zn |= LZ_HALO;

            bool hasThinTouch = (keep & (LF_THIN_W | LF_THIN_N)) != 0 ||
                                (x + 1 < s.w && (s.flag[x + 1][z] & LF_THIN_W) != 0) ||
                                (z + 1 < s.h && (s.flag[x][z + 1] & LF_THIN_N) != 0);
            if ((wantNarrow || inHaloBand) && !hasThinTouch && (t == LT_LOW || (!vault && t == LT_CORR)) &&
                x > 0 && x < s.w - 1 && z > 0 && z < s.h - 1) {
                bool wN = (s.tag[x][z - 1] == LT_WALL), wS = (s.tag[x][z + 1] == LT_WALL);
                bool wW = (s.tag[x - 1][z] == LT_WALL), wE = (s.tag[x + 1][z] == LT_WALL);
                // Keep 1-cell thresholds between two rooms un-narrowed so framed doorways stand there cleanly!
                bool corrRunX = (s.tag[x - 1][z] == LT_CORR || s.tag[x + 1][z] == LT_CORR ||
                                 s.tag[x - 1][z] == LT_LOW  || s.tag[x + 1][z] == LT_LOW  || inHaloBand);
                bool corrRunZ = (s.tag[x][z - 1] == LT_CORR || s.tag[x][z + 1] == LT_CORR ||
                                 s.tag[x][z - 1] == LT_LOW  || s.tag[x][z + 1] == LT_LOW  || inHaloBand);
                if (wN && wS && !wW && !wE && corrRunX) { keep |= LF_NARROW_X; s.narrowCount++; }
                else if (wW && wE && !wN && !wS && corrRunZ) { keep |= LF_NARROW_Z; s.narrowCount++; }
            }

            if (!inXeno && (keep & LF_MEZZ) == 0) {
                if (vault) {
                    if ((f & LF_CRAMPED) != 0 && lmIsLowShellTag(t)) keep |= LF_CRAMPED;
                    else if (s.genome.g[LG_GRAIN] < 72 && s.genome.g[LG_DECAY] > 100 && t == LT_LOW &&
                             (keep & (LF_NARROW_X | LF_NARROW_Z)) != 0) keep |= LF_CRAMPED;
                } else {
                    if (inHaloBand || (f & LF_CRAMPED) != 0) {
                        keep |= LF_CRAMPED;
                        zn &= ~(unsigned char)(LZ_STD_CEIL | LZ_TALL_WELL);
                    } else if ((zn & LZ_TALL_WELL) != 0) {
                        zn &= ~(unsigned char)LZ_STD_CEIL;
                    } else if (lowRoomWing && (t == LT_CORR || t == LT_ROOM)) {
                        keep |= LF_CRAMPED;
                        zn &= ~(unsigned char)LZ_STD_CEIL;
                    } else if (famCorr && t == LT_CORR) {
                        keep |= LF_CRAMPED;
                        zn &= ~(unsigned char)LZ_STD_CEIL;
                    } else if (stdRoomWing && (t == LT_ROOM || t == LT_CORR || (t == LT_HALL && famEnf))) {
                        zn |= LZ_STD_CEIL;
                    }
                }
            }
            s.flag[x][z] = keep;
            s.zone[x][z] = zn;
        }
    }
    lmEnsureThinPassability(s);
}

inline void lmPlaceRailGap(LimenSlab& s, u32 h) {
    if (lmIsMegaUnstruct(s.megaForm)) return;
    int chance = s.genome.g[LG_DECAY] * 65 / 255;
    if ((int)(lmHash((u32)s.rect.j, (u32)s.rect.i, 4u, 0x414Eu) % 100u) >= chance) return;
    for (int pass = 0, count = 0, pick = 0; pass < 2; pass++) {
        if (pass == 1) { if (count < 4) return; pick = (int)((h >> 20) % (u32)count); }
        for (int x = 1; x < s.w - 1; x++) for (int z = 1; z < s.h - 1; z++) {
            if (!lmIsWalkableTag(s.tag[x][z]) || lmIsLowShellTag(s.tag[x][z]) || lmPitDegreeLocal(s, x, z) == 0) continue;
            if (pass == 0) count++;
            else if (pick-- == 0) { lmAddSite(s, LS_RAIL_GAP, x, z); return; }
        }
    }
}
