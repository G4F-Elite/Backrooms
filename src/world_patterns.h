#pragma once
// Chunk architecture patterns split from world.h

inline void applyRampRoutePattern(Chunk& c, std::mt19937& cr) {
    carveRect(c, 1, 1, CHUNK_SIZE - 2, CHUNK_SIZE - 2);
    int laneA = 3 + (int)(cr() % 2);
    int laneB = CHUNK_SIZE - 4 - (int)(cr() % 2);
    for (int x = 2; x < CHUNK_SIZE - 2; x++) {
        c.cells[x][laneA] = 0;
        c.cells[x][laneB] = 0;
    }
    int r0 = 3 + (int)(cr() % 5);
    int r1 = r0 + 3;
    for (int x = 2; x < CHUNK_SIZE - 2; x++) {
        if ((x % 4) < 2) {
            c.cells[x][r0] = 0;
            c.cells[x][r1] = 0;
        }
    }
    for (int z = 2; z < CHUNK_SIZE - 2; z++) {
        if ((z % 5) == 0) {
            c.cells[3][z] = 0;
            c.cells[CHUNK_SIZE - 4][z] = 0;
        }
    }
}

inline void applyParkingPattern(Chunk& c, std::mt19937& cr) {
    carveRect(c, 1, 1, CHUNK_SIZE - 2, CHUNK_SIZE - 2);
    int laneL = 4 + (int)(cr() % 2);
    int laneR = CHUNK_SIZE - 5 - (int)(cr() % 2);
    for (int z = 2; z < CHUNK_SIZE - 2; z++) {
        c.cells[laneL][z] = 1;
        c.cells[laneR][z] = 1;
    }
    for (int z = 3; z < CHUNK_SIZE - 3; z += 2) {
        c.cells[laneL][z] = 0;
        c.cells[laneR][z] = 0;
    }
    for (int x = 2; x < CHUNK_SIZE - 2; x++) {
        if ((x % 3) == 0) {
            c.cells[x][2] = 1;
            c.cells[x][CHUNK_SIZE - 3] = 1;
        }
    }
}
