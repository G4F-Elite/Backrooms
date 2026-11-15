#include <cassert>
#include <iostream>
#include <random>
#include <unordered_map>

#include "../src/world.h"

const float CS = 5.0f;
const float WH = 4.5f;
std::mt19937 rng;
unsigned int worldSeed = 1337;
std::unordered_map<long long, Chunk> chunks;
std::vector<Light> lights;
std::vector<Vec3> pillars;
int playerChunkX = 0;
int playerChunkZ = 0;

Chunk makeSolidChunk() {
    Chunk c{};
    c.cx = 0;
    c.cz = 0;
    c.gen = true;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            c.cells[x][z] = 1;
        }
    }
    return c;
}

void generateArea(int radius) {
    chunks.clear();
    for (int cx = -radius; cx <= radius; cx++) {
        for (int cz = -radius; cz <= radius; cz++) {
            generateChunk(cx, cz);
        }
    }
}

int wrapLocal(int v) {
    return ((v % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
}

void testRectHelpers() {
    Chunk c = makeSolidChunk();
    carveRect(c, 2, 2, 4, 4);
    assert(c.cells[2][2] == 0);
    assert(c.cells[5][5] == 0);
    fillRect(c, 3, 3, 2, 2);
    assert(c.cells[3][3] == 1);
}

void testChunkGenerationHasVariedDensity() {
    worldSeed = 1337;
    generateArea(1);
    assert(chunks.size() == 9);
    int minOpen = 9999;
    int maxOpen = 0;
    for (const auto& kv : chunks) {
        int open = 0;
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if (kv.second.cells[x][z] == 0) open++;
            }
        }
        assert(open > 90);
        assert(open < CHUNK_SIZE * CHUNK_SIZE);
        if (open < minOpen) minOpen = open;
        if (open > maxOpen) maxOpen = open;
    }
    assert(minOpen != maxOpen);
}

void testChunkGenerationIsDeterministic() {
    worldSeed = 4242;
    generateArea(0);
    Chunk a = chunks[chunkKey(0, 0)];
    generateArea(0);
    const Chunk& b = chunks[chunkKey(0, 0)];
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            assert(a.cells[x][z] == b.cells[x][z]);
        }
    }
}

// The old seed mix produced identical chunks at (cx, cz) and (-cx, -cz).
void testChunksAreNotMirrored() {
    worldSeed = 1337;
    generateArea(1);
    const Chunk& a = chunks[chunkKey(1, 1)];
    const Chunk& b = chunks[chunkKey(-1, -1)];
    int diff = 0;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            if (a.cells[x][z] != b.cells[x][z]) diff++;
        }
    }
    assert(diff > 8);
}

// Chunks own only their low walls, so a seam is a single wall line and every opening in it
// leads into open cells on both sides.
void testChunkSeamsJoin() {
    const unsigned int seeds[] = {1u, 77u, 1337u, 90210u, 3141592u};
    for (unsigned int seed : seeds) {
        for (int level = 0; level <= 1; level++) {
            gCurrentLevel = level;
            worldSeed = seed;
            generateArea(2);
            for (int cx = -1; cx <= 2; cx++) {
                for (int cz = -1; cz <= 2; cz++) {
                    int bx = cx * CHUNK_SIZE;
                    int bz = cz * CHUNK_SIZE;
                    int doorsX = 0;
                    int doorsZ = 0;
                    for (int t = 1; t < CHUNK_SIZE; t++) {
                        if (getCellWorld(bx, bz + t) == 0) {
                            doorsX++;
                            assert(getCellWorld(bx - 1, bz + t) == 0);
                            assert(getCellWorld(bx + 1, bz + t) == 0);
                        }
                        if (getCellWorld(bx + t, bz) == 0) {
                            doorsZ++;
                            assert(getCellWorld(bx + t, bz - 1) == 0);
                            assert(getCellWorld(bx + t, bz + 1) == 0);
                        }
                    }
                    assert(doorsX > 0);
                    assert(doorsZ > 0);
                    assert(getCellWorld(bx, bz) == 1);
                }
            }
        }
    }
    gCurrentLevel = 0;
}

// Every open cell of the inner 3x3 chunks must be reachable from every other one.
void testWholeAreaIsConnected() {
    const unsigned int seeds[] = {5u, 1337u, 424242u, 20260101u};
    for (unsigned int seed : seeds) {
        for (int level = 0; level <= 1; level++) {
            gCurrentLevel = level;
            worldSeed = seed;
            generateArea(2);
            const int lo = -2 * CHUNK_SIZE;
            const int side = 5 * CHUNK_SIZE;
            std::vector<char> seen(side * side, 0);
            std::vector<std::pair<int, int>> stack;
            for (int x = 0; x < CHUNK_SIZE && stack.empty(); x++) {
                for (int z = 0; z < CHUNK_SIZE && stack.empty(); z++) {
                    if (getCellWorld(x, z) == 0) stack.push_back({x, z});
                }
            }
            assert(!stack.empty());
            while (!stack.empty()) {
                auto cur = stack.back();
                stack.pop_back();
                int ix = cur.first - lo;
                int iz = cur.second - lo;
                if (ix < 0 || iz < 0 || ix >= side || iz >= side) continue;
                if (seen[ix * side + iz]) continue;
                if (getCellWorld(cur.first, cur.second) != 0) continue;
                seen[ix * side + iz] = 1;
                stack.push_back({cur.first + 1, cur.second});
                stack.push_back({cur.first - 1, cur.second});
                stack.push_back({cur.first, cur.second + 1});
                stack.push_back({cur.first, cur.second - 1});
            }
            for (int x = -CHUNK_SIZE; x < 2 * CHUNK_SIZE; x++) {
                for (int z = -CHUNK_SIZE; z < 2 * CHUNK_SIZE; z++) {
                    if (getCellWorld(x, z) != 0) continue;
                    assert(seen[(x - lo) * side + (z - lo)]);
                }
            }
        }
    }
    gCurrentLevel = 0;
}

// Walls must be anchored: no free-floating single wall cubes in a fresh layout.
void testNoFloatingWallCells() {
    worldSeed = 271828;
    generateArea(2);
    for (int x = -CHUNK_SIZE; x < 2 * CHUNK_SIZE; x++) {
        for (int z = -CHUNK_SIZE; z < 2 * CHUNK_SIZE; z++) {
            if (getCellWorld(x, z) != 1) continue;
            int wallNeighbors = 0;
            if (getCellWorld(x - 1, z) == 1) wallNeighbors++;
            if (getCellWorld(x + 1, z) == 1) wallNeighbors++;
            if (getCellWorld(x, z - 1) == 1) wallNeighbors++;
            if (getCellWorld(x, z + 1) == 1) wallNeighbors++;
            bool chunkCorner = wrapLocal(x) == 0 && wrapLocal(z) == 0;
            assert(wallNeighbors > 0 || chunkCorner);
        }
    }
}

void testDoorwaysSitInWallHoles() {
    worldSeed = 1337;
    generateArea(1);
    int doorways = 0;
    for (int x = -CHUNK_SIZE + 1; x < 2 * CHUNK_SIZE - 1; x++) {
        for (int z = -CHUNK_SIZE + 1; z < 2 * CHUNK_SIZE - 1; z++) {
            int axis = doorwayAxisAt(x, z);
            if (axis == 0) continue;
            doorways++;
            assert(getCellWorld(x, z) == 0);
            if (axis == 1) {
                assert(getCellWorld(x - 1, z) == 1 && getCellWorld(x + 1, z) == 1);
                assert(getCellWorld(x, z - 1) == 0 && getCellWorld(x, z + 1) == 0);
            } else {
                assert(getCellWorld(x, z - 1) == 1 && getCellWorld(x, z + 1) == 1);
                assert(getCellWorld(x - 1, z) == 0 && getCellWorld(x + 1, z) == 0);
            }
            float cx = (x + 0.5f) * CS;
            float cz = (z + 0.5f) * CS;
            // Centre of the opening is walkable, the infill next to the wall is not.
            assert(!collideWorld(cx, cz, 0.3f));
            float off = CS * 0.4f;
            if (axis == 1) assert(collideDoorFrameAtCell(cx + off, cz, 0.05f, x, z));
            else assert(collideDoorFrameAtCell(cx, cz + off, 0.05f, x, z));
        }
    }
    assert(doorways > 0);
}

void testPillarsStandInOpenHalls() {
    lights.clear();
    pillars.clear();
    gCurrentLevel = 1;
    worldSeed = 777;
    generateArea(2);
    updateLightsAndPillars(0, 0);
    assert(!lights.empty());
    assert(!pillars.empty());
    for (const auto& p : pillars) {
        int wx = (int)floorf(p.x / CS);
        int wz = (int)floorf(p.z / CS);
        assert(isOpenHallCell(wx, wz));
        for (const auto& l : lights) {
            assert(fabsf(l.pos.x - p.x) > 1.0f || fabsf(l.pos.z - p.z) > 1.0f);
        }
    }
    gCurrentLevel = 0;
    pillars.clear();
    lights.clear();
}

void testFindSpawnPosAvoidsPillars() {
    chunks.clear();
    lights.clear();
    pillars.clear();
    worldSeed = 2026;
    generateChunk(0, 0);
    Vec3 around(8 * CS, 0, 8 * CS);
    Vec3 forced((int)floorf(around.x / CS) * CS + CS * 0.5f, 0, (int)floorf(around.z / CS) * CS + CS * 0.5f);
    pillars.push_back(forced);
    Vec3 sp = findSpawnPos(around, 0.0f);
    float dx = sp.x - forced.x;
    float dz = sp.z - forced.z;
    assert((dx * dx + dz * dz) > 0.25f);
}

int main() {
    testRectHelpers();
    testChunkGenerationHasVariedDensity();
    testChunkGenerationIsDeterministic();
    testChunksAreNotMirrored();
    testChunkSeamsJoin();
    testWholeAreaIsConnected();
    testNoFloatingWallCells();
    testDoorwaysSitInWallHoles();
    testPillarsStandInOpenHalls();
    testFindSpawnPosAvoidsPillars();
    std::cout << "All world variation tests passed.\n";
    return 0;
}
