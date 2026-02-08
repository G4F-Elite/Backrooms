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

void testRectHelpers() {
    Chunk c = makeSolidChunk();
    carveRect(c, 2, 2, 4, 4);
    assert(c.cells[2][2] == 0);
    assert(c.cells[5][5] == 0);
    fillRect(c, 3, 3, 2, 2);
    assert(c.cells[3][3] == 1);
}

void testPatternsCreateOpenArea() {
    std::mt19937 cr(42);
    Chunk a = makeSolidChunk();
    applyAtriumPattern(a, cr);
    assert(countOpenCells(a) > 20);

    Chunk b = makeSolidChunk();
    applyOfficePattern(b, cr);
    assert(countOpenCells(b) > 20);

    Chunk d = makeSolidChunk();
    applyServicePattern(d, cr);
    assert(countOpenCells(d) > 20);
}

void testChunkGenerationHasVariedDensity() {
    chunks.clear();
    generateChunk(0, 0);
    generateChunk(1, 0);
    assert(chunks.size() == 2);
    const Chunk& c0 = chunks[chunkKey(0, 0)];
    const Chunk& c1 = chunks[chunkKey(1, 0)];
    int o0 = countOpenCells(c0);
    int o1 = countOpenCells(c1);
    assert(o0 > 35);
    assert(o1 > 35);
    assert(o0 != o1);
}

void testPillarsGenerated() {
    chunks.clear();
    lights.clear();
    pillars.clear();
    generateChunk(0, 0);
    updateLightsAndPillars(0, 0);
    assert(!lights.empty());
    assert(!pillars.empty());
}

int main() {
    testRectHelpers();
    testPatternsCreateOpenArea();
    testChunkGenerationHasVariedDensity();
    testPillarsGenerated();
    std::cout << "All world variation tests passed.\n";
    return 0;
}
