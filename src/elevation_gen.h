#pragma once
// Elevation generation - creates full second floor with maze structure

inline void applyElevation(Chunk& c, std::mt19937& cr) {
    // 40% chance to have a second floor in this chunk
    if (cr() % 100 >= 40) return;

    // Create a full second floor covering most of the chunk
    // Leave border cells as ground for connectivity
    int x0 = 2, z0 = 2;
    int w = CHUNK_SIZE - 4, h = CHUNK_SIZE - 4;
    
    // Find valid open cells for the second floor base
    std::vector<std::pair<int,int>> openCells;
    for (int x = x0; x < x0 + w; x++) {
        for (int z = z0; z < z0 + h; z++) {
            if (c.cells[x][z] == 0) {
                openCells.push_back({x, z});
            }
        }
    }
    
    // Need at least 20 open cells to create a meaningful second floor
    if (openCells.size() < 20) return;
    
    // Mark all valid cells as elevated (second floor)
    for (auto& cell : openCells) {
        c.elev[cell.first][cell.second] = 1;
    }
    
    // Carve some "rooms" and "corridors" on the second floor by making walls
    // Similar to how the first floor has maze structure
    int numWalls = 3 + cr() % 4; // 3-6 wall segments
    for (int i = 0; i < numWalls; i++) {
        int wx = x0 + 2 + cr() % (w - 4);
        int wz = z0 + 2 + cr() % (h - 4);
        bool horizontal = cr() % 2 == 0;
        int len = 2 + cr() % 4;
        
        for (int j = 0; j < len; j++) {
            int cx = horizontal ? wx + j : wx;
            int cz = horizontal ? wz : wz + j;
            if (cx >= x0 && cx < x0 + w && cz >= z0 && cz < z0 + h) {
                // Only place walls on elevated cells, not on ramps
                if (c.elev[cx][cz] == 1 && c.cells[cx][cz] == 0) {
                    c.cells[cx][cz] = 1; // Wall
                }
            }
        }
    }
    
    // Add some "doors"/openings in walls
    int numDoors = 2 + cr() % 3;
    for (int i = 0; i < numDoors; i++) {
        int dx = x0 + 2 + cr() % (w - 4);
        int dz = z0 + 2 + cr() % (h - 4);
        if (c.cells[dx][dz] == 1 && c.elev[dx][dz] == 1) {
            c.cells[dx][dz] = 0; // Open
        }
    }
    
    // Find ramp candidates at edges facing open ground cells
    struct RC { int x, z; int8_t dir; };
    std::vector<RC> cands;
    for (int x = x0; x < x0 + w; x++) {
        for (int z = z0; z < z0 + h; z++) {
            if (c.elev[x][z] != 1) continue; // Only elevated cells
            bool edge = (x==x0 || x==x0+w-1 || z==z0 || z==z0+h-1);
            if (!edge) continue;
            
            // Check for ground-level neighbors
            if (x==x0 && x>1 && c.cells[x-1][z]==0 && c.elev[x-1][z]==0)
                cands.push_back({x, z, 2}); // rises +X toward interior
            if (x==x0+w-1 && x<CHUNK_SIZE-2 && c.cells[x+1][z]==0 && c.elev[x+1][z]==0)
                cands.push_back({x, z, 3}); // rises -X toward interior
            if (z==z0 && z>1 && c.cells[x][z-1]==0 && c.elev[x][z-1]==0)
                cands.push_back({x, z, 4}); // rises +Z toward interior
            if (z==z0+h-1 && z<CHUNK_SIZE-2 && c.cells[x][z+1]==0 && c.elev[x][z+1]==0)
                cands.push_back({x, z, 5}); // rises -Z toward interior
        }
    }
    
    // Place 2-4 ramps for better connectivity
    int nR = 2 + (cands.size() > 6 ? (int)(cr() % 3) : 0);
    for (int i = 0; i < nR && !cands.empty(); i++) {
        int idx = cr() % cands.size();
        c.elev[cands[idx].x][cands[idx].z] = cands[idx].dir;
        cands.erase(cands.begin() + idx);
    }
    
    // Ensure at least 2 ramps, otherwise cancel elevation
    int rampCount = 0;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            if (isRamp(c.elev[x][z])) rampCount++;
        }
    }
    if (rampCount < 2) {
        // Not enough ramps - revert
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                c.elev[x][z] = 0;
            }
        }
    }
}
