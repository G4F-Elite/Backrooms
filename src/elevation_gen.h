#pragma once
// Elevation generation - included from world.h after Chunk is defined

inline bool isCellOpenForElev(const Chunk& c, int x, int z) {
    if (x < 1 || x >= CHUNK_SIZE-1 || z < 1 || z >= CHUNK_SIZE-1) return false;
    return c.cells[x][z] == 0;
}

// Count how many of the 4 cardinal neighbors are open (cell==0)
inline int openNeighbors(const Chunk& c, int x, int z) {
    int n = 0;
    if (x > 0 && c.cells[x-1][z] == 0) n++;
    if (x < CHUNK_SIZE-1 && c.cells[x+1][z] == 0) n++;
    if (z > 0 && c.cells[x][z-1] == 0) n++;
    if (z < CHUNK_SIZE-1 && c.cells[x][z+1] == 0) n++;
    return n;
}

inline void applyElevation(Chunk& c, std::mt19937& cr) {
    if (cr() % 100 >= 28) return;

    // Find a good rectangular area of open cells for the platform
    int bestX0 = -1, bestZ0 = -1, bestW = 0, bestH = 0;
    for (int attempt = 0; attempt < 8; attempt++) {
        int x0 = 3 + cr() % (CHUNK_SIZE - 8);
        int z0 = 3 + cr() % (CHUNK_SIZE - 8);
        int w = 3 + cr() % 4;
        int h = 3 + cr() % 4;
        if (x0 + w >= CHUNK_SIZE - 2) w = CHUNK_SIZE - 3 - x0;
        if (z0 + h >= CHUNK_SIZE - 2) h = CHUNK_SIZE - 3 - z0;
        if (w < 2 || h < 2) continue;
        // Verify ALL cells in rectangle are open
        bool allOpen = true;
        for (int x = x0; x < x0 + w && allOpen; x++)
            for (int z = z0; z < z0 + h && allOpen; z++)
                if (c.cells[x][z] != 0) allOpen = false;
        if (!allOpen) continue;
        if (w * h > bestW * bestH) { bestX0 = x0; bestZ0 = z0; bestW = w; bestH = h; }
    }
    if (bestX0 < 0 || bestW * bestH < 4) return;

    int x0 = bestX0, z0 = bestZ0, w = bestW, h = bestH;

    // Mark all cells in the rectangle as elevated
    for (int x = x0; x < x0 + w; x++)
        for (int z = z0; z < z0 + h; z++)
            c.elev[x][z] = 1;

    // Find valid ramp positions: edge cells with open ground neighbor outside rect
    struct RampCandidate { int x, z; int8_t dir; };
    std::vector<RampCandidate> candidates;
    for (int x = x0; x < x0 + w; x++) {
        for (int z = z0; z < z0 + h; z++) {
            // Only edge cells of the rectangle
            bool isEdge = (x == x0 || x == x0+w-1 || z == z0 || z == z0+h-1);
            if (!isEdge) continue;
            // Check each outward direction for ground-level open cell
            // Ramp goes from elevated (platform interior) to ground (outside)
            // Type 2 (+X): rises with X. Type 3 (-X): falls with X.
            // Left edge: ground is at -X, platform at +X → rises toward platform = type 2
            if (x == x0 && x > 1 && c.cells[x-1][z] == 0)
                candidates.push_back({x, z, 2});
            // Right edge: ground is at +X, platform at -X → falls toward ground = type 3
            if (x == x0+w-1 && x < CHUNK_SIZE-2 && c.cells[x+1][z] == 0)
                candidates.push_back({x, z, 3});
            // Back edge: ground at -Z, platform at +Z → rises toward platform = type 4
            if (z == z0 && z > 1 && c.cells[x][z-1] == 0)
                candidates.push_back({x, z, 4});
            // Front edge: ground at +Z, platform at -Z → falls toward ground = type 5
            if (z == z0+h-1 && z < CHUNK_SIZE-2 && c.cells[x][z+1] == 0)
                candidates.push_back({x, z, 5});
        }
    }
    if (candidates.empty()) {
        // No valid ramp positions - cancel elevation
        for (int x = x0; x < x0 + w; x++)
            for (int z = z0; z < z0 + h; z++)
                c.elev[x][z] = 0;
        return;
    }

    // Place 1-2 ramps from candidates
    int nRamps = 1 + (candidates.size() > 3 ? cr() % 2 : 0);
    for (int i = 0; i < nRamps && !candidates.empty(); i++) {
        int idx = cr() % candidates.size();
        auto& rc = candidates[idx];
        c.elev[rc.x][rc.z] = rc.dir;
        candidates.erase(candidates.begin() + idx);
    }
}
