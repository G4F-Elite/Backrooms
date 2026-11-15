#pragma once
// ============================================================================
// CHUNK LAYOUT
// Rooms are built by recursive division, so every wall starts and ends on
// another wall and every room keeps at least one opening.
//
// Tiling rule: a chunk owns only its LOW walls (column lx == 0, row lz == 0).
// Its far sides (lx == 15, lz == 15) stay open and are closed by the low walls
// of the neighbouring chunk. Walls are therefore one cell thick everywhere and
// chunks join without a visible seam.
// ============================================================================

enum ChunkStyle {
    CHUNK_STYLE_HALLS = 0,
    CHUNK_STYLE_OFFICE = 1,
    CHUNK_STYLE_MAZE = 2,
    CHUNK_STYLE_MIXED = 3,
    CHUNK_STYLE_PARKING = 4
};

struct LayoutRules {
    int minRoom;        // smallest room side, in cells
    int doorMax;        // widest opening inside the chunk
    int keepArea;       // rooms up to this area may stay undivided
    int keepChance;     // ... with this chance (percent)
    int borderDoorMax;  // widest opening in the chunk-owned border walls
    int borderOpenRun;  // chance (percent) to remove a whole border wall run
    bool pillars;       // thin structural pillars in open halls
};

inline unsigned int hashCell(int x, int z, unsigned int salt) {
    unsigned int h = worldSeed ^ salt;
    h ^= (unsigned int)x * 0x9E3779B1u;
    h = (h ^ (h >> 15)) * 0x85EBCA6Bu;
    h ^= (unsigned int)z * 0xC2B2AE35u;
    h = (h ^ (h >> 13)) * 0x27D4EB2Fu;
    return h ^ (h >> 16);
}

inline int chunkStyleAt(int cx, int cz) {
    int p = (int)(hashCell(cx, cz, 0x51A7E5u) % 100u);
    if (isParkingLevel(gCurrentLevel)) {
        if (p < 62) return CHUNK_STYLE_PARKING;
        if (p < 82) return CHUNK_STYLE_MIXED;
        return CHUNK_STYLE_OFFICE;
    }
    if (p < 30) return CHUNK_STYLE_HALLS;
    if (p < 55) return CHUNK_STYLE_OFFICE;
    if (p < 72) return CHUNK_STYLE_MAZE;
    return CHUNK_STYLE_MIXED;
}

inline LayoutRules layoutRulesFor(int style) {
    if (style == CHUNK_STYLE_HALLS) return {4, 4, 64, 55, 4, 45, true};
    if (style == CHUNK_STYLE_OFFICE) return {2, 1, 9, 55, 2, 0, false};
    if (style == CHUNK_STYLE_MAZE) return {1, 1, 3, 30, 1, 0, false};
    if (style == CHUNK_STYLE_PARKING) return {6, 6, 120, 65, 6, 70, true};
    return {3, 2, 20, 45, 3, 15, false};
}

inline bool isLayoutInterior(int v) {
    return v >= 1 && v <= CHUNK_SIZE - 1;
}

// A new wall may only end on solid wall (or on a chunk border line, which is decided later).
inline bool wallEndOk(const Chunk& c, int x, int z) {
    if (!isLayoutInterior(x) || !isLayoutInterior(z)) return true;
    return c.cells[x][z] == 1;
}

inline void setWallCell(Chunk& c, bool alongZ, int line, int t, int val) {
    if (alongZ) c.cells[line][t] = val;
    else c.cells[t][line] = val;
}

// Builds a wall on `line` spanning t0..t1 and cuts its opening(s).
inline void buildDividerWall(Chunk& c, std::mt19937& r, bool alongZ, int line, int t0, int t1, const LayoutRules& rules) {
    int span = t1 - t0 + 1;
    for (int t = t0; t <= t1; t++) setWallCell(c, alongZ, line, t, 1);

    int len = 1 + (int)(r() % (unsigned)rules.doorMax);
    if (len > span - 1) len = span - 1;
    if (len < 1) len = 1;
    int start = t0 + (int)(r() % (unsigned)(span - len + 1));
    int edgeRoll = (int)(r() % 100u);
    if (edgeRoll < 18) start = t0;
    else if (edgeRoll < 36) start = t1 - len + 1;
    for (int t = start; t < start + len; t++) setWallCell(c, alongZ, line, t, 0);

    if (span >= 9 && (r() % 100u) < 55u) {
        int second = t0 + (int)(r() % (unsigned)span);
        if (second < start - 2 || second > start + len + 1) setWallCell(c, alongZ, line, second, 0);
    }
}

inline void divideRegion(Chunk& c, std::mt19937& r, int x0, int z0, int x1, int z1, const LayoutRules& rules) {
    int w = x1 - x0 + 1;
    int h = z1 - z0 + 1;
    if (w * h <= rules.keepArea && (int)(r() % 100u) < rules.keepChance) return;

    bool canX = w >= rules.minRoom * 2 + 1;
    bool canZ = h >= rules.minRoom * 2 + 1;
    if (!canX && !canZ) return;
    bool splitX = canX && (!canZ || w > h || (w == h && (r() & 1u)));

    for (int attempt = 0; attempt < 2; attempt++) {
        std::vector<int> lines;
        if (splitX) {
            for (int k = x0 + rules.minRoom; k <= x1 - rules.minRoom; k++) {
                if (wallEndOk(c, k, z0 - 1) && wallEndOk(c, k, z1 + 1)) lines.push_back(k);
            }
        } else {
            for (int k = z0 + rules.minRoom; k <= z1 - rules.minRoom; k++) {
                if (wallEndOk(c, x0 - 1, k) && wallEndOk(c, x1 + 1, k)) lines.push_back(k);
            }
        }
        if (lines.empty()) {
            bool otherPossible = splitX ? canZ : canX;
            if (!otherPossible) return;
            splitX = !splitX;
            continue;
        }
        int k = lines[r() % lines.size()];
        if (splitX) {
            buildDividerWall(c, r, true, k, z0, z1, rules);
            divideRegion(c, r, x0, z0, k - 1, z1, rules);
            divideRegion(c, r, k + 1, z0, x1, z1, rules);
        } else {
            buildDividerWall(c, r, false, k, x0, x1, rules);
            divideRegion(c, r, x0, z0, x1, k - 1, rules);
            divideRegion(c, r, x0, k + 1, x1, z1, rules);
        }
        return;
    }
}

// Pure function of (seed, cx, cz): fills the interior (1..15) and leaves the low walls solid.
inline void buildChunkInterior(Chunk& c) {
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            c.cells[x][z] = (x == 0 || z == 0) ? 1 : 0;
        }
    }
    std::mt19937 r(hashCell(c.cx, c.cz, 0x1A7007u));
    LayoutRules rules = layoutRulesFor(chunkStyleAt(c.cx, c.cz));
    divideRegion(c, r, 1, 1, CHUNK_SIZE - 1, CHUNK_SIZE - 1, rules);
}

// Cuts openings into one chunk-owned border wall. `alongZ` selects the lx == 0 column
// (neighbour is cx - 1), otherwise the lz == 0 row (neighbour is cz - 1).
inline void openBorderWall(Chunk& c, const Chunk& nb, bool alongZ, std::mt19937& r, const LayoutRules& rules) {
    auto passable = [&](int t) {
        if (alongZ) return c.cells[1][t] == 0 && nb.cells[CHUNK_SIZE - 1][t] == 0;
        return c.cells[t][1] == 0 && nb.cells[t][CHUNK_SIZE - 1] == 0;
    };
    int opened = 0;
    int bestStart = -1;
    int bestLen = 0;
    int t = 1;
    while (t < CHUNK_SIZE) {
        if (!passable(t)) { t++; continue; }
        int a = t;
        while (t < CHUNK_SIZE && passable(t)) t++;
        int len = t - a;
        if (len > bestLen) { bestLen = len; bestStart = a; }

        int roll = (int)(r() % 100u);
        if (len >= 3 && roll < rules.borderOpenRun) {
            for (int k = a; k < a + len; k++) setWallCell(c, alongZ, 0, k, 0);
            opened++;
            continue;
        }
        if (roll >= 85) continue;
        int width = 1 + (int)(r() % (unsigned)rules.borderDoorMax);
        if (width > len) width = len;
        int start = a + (int)(r() % (unsigned)(len - width + 1));
        for (int k = start; k < start + width; k++) setWallCell(c, alongZ, 0, k, 0);
        opened++;
        if (len >= 8 && (r() % 100u) < 50u) {
            int second = a + (int)(r() % (unsigned)len);
            if (second < start - 2 || second > start + width + 1) setWallCell(c, alongZ, 0, second, 0);
        }
    }
    if (opened == 0 && bestStart >= 0) {
        setWallCell(c, alongZ, 0, bestStart + bestLen / 2, 0);
    }
}

inline void buildChunkLayout(Chunk& c) {
    buildChunkInterior(c);
    Chunk west{};
    west.cx = c.cx - 1;
    west.cz = c.cz;
    buildChunkInterior(west);
    Chunk north{};
    north.cx = c.cx;
    north.cz = c.cz - 1;
    buildChunkInterior(north);

    LayoutRules rules = layoutRulesFor(chunkStyleAt(c.cx, c.cz));
    std::mt19937 r(hashCell(c.cx, c.cz, 0xB04DE4u));
    openBorderWall(c, west, true, r, rules);
    openBorderWall(c, north, false, r, rules);
    c.cells[0][0] = 1;
}

// Pillar lattice uses odd world cells so it never collides with ceiling lamps (even cells).
inline bool isPillarLatticeCell(int wx, int wz) {
    int mx = ((wx % 4) + 4) % 4;
    int mz = ((wz % 4) + 4) % 4;
    return mx == 1 && mz == 1;
}
