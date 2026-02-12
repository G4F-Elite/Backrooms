#pragma once
// Elevation generation - included from world.h after Chunk is defined

inline void applyElevation(Chunk& c, std::mt19937& cr) {
    if (cr() % 100 >= 28) return;

    // Find a rectangle where ALL cells are open
    int bestX = -1, bestZ = -1, bestW = 0, bestH = 0;
    for (int att = 0; att < 10; att++) {
        int x0 = 3 + cr() % (CHUNK_SIZE - 8);
        int z0 = 3 + cr() % (CHUNK_SIZE - 8);
        int w = 3 + cr() % 4, h = 3 + cr() % 4;
        if (x0 + w >= CHUNK_SIZE - 2) w = CHUNK_SIZE - 3 - x0;
        if (z0 + h >= CHUNK_SIZE - 2) h = CHUNK_SIZE - 3 - z0;
        if (w < 3 || h < 3) continue;
        bool ok = true;
        for (int x = x0; x < x0+w && ok; x++)
            for (int z = z0; z < z0+h && ok; z++)
                if (c.cells[x][z] != 0) ok = false;
        if (!ok) continue;
        if (w * h > bestW * bestH) {
            bestX = x0; bestZ = z0; bestW = w; bestH = h;
        }
    }
    if (bestX < 0 || bestW * bestH < 9) return;
    int x0 = bestX, z0 = bestZ, w = bestW, h = bestH;

    // Mark all cells as elevated (second floor)
    for (int x = x0; x < x0+w; x++)
        for (int z = z0; z < z0+h; z++)
            c.elev[x][z] = 1;

    // Find ramp candidates at edges facing open ground cells
    struct RC { int x, z; int8_t dir; };
    std::vector<RC> cands;
    for (int x = x0; x < x0+w; x++) for (int z = z0; z < z0+h; z++) {
        bool edge = (x==x0 || x==x0+w-1 || z==z0 || z==z0+h-1);
        if (!edge) continue;
        // Ramp slopes: high side toward platform interior, low side toward ground
        if (x==x0 && x>1 && c.cells[x-1][z]==0)
            cands.push_back({x, z, 2}); // rises +X toward interior
        if (x==x0+w-1 && x<CHUNK_SIZE-2 && c.cells[x+1][z]==0)
            cands.push_back({x, z, 3}); // rises -X toward interior
        if (z==z0 && z>1 && c.cells[x][z-1]==0)
            cands.push_back({x, z, 4}); // rises +Z toward interior
        if (z==z0+h-1 && z<CHUNK_SIZE-2 && c.cells[x][z+1]==0)
            cands.push_back({x, z, 5}); // rises -Z toward interior
    }
    if (cands.empty()) {
        // No valid ramp spot - cancel
        for (int x = x0; x < x0+w; x++)
            for (int z = z0; z < z0+h; z++) c.elev[x][z] = 0;
        return;
    }
    // Place 1-2 ramps
    int nR = 1 + (cands.size() > 4 ? (int)(cr() % 2) : 0);
    for (int i = 0; i < nR && !cands.empty(); i++) {
        int idx = cr() % cands.size();
        c.elev[cands[idx].x][cands[idx].z] = cands[idx].dir;
        cands.erase(cands.begin() + idx);
    }
}
