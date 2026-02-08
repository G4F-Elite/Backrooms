#include <cassert>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <random>

#include "../src/world.h"
#include "../src/map_content.h"

const float CS = 5.0f;
const float WH = 4.5f;
std::mt19937 rng;
unsigned int worldSeed = 0;
std::unordered_map<long long, Chunk> chunks;
std::vector<Light> lights;
std::vector<Vec3> pillars;
std::vector<MapProp> mapProps;
int playerChunkX = 0;
int playerChunkZ = 0;

struct PropSnapshot {
    int x;
    int z;
    int type;
};

inline bool operator==(const PropSnapshot& a, const PropSnapshot& b) {
    return a.x == b.x && a.z == b.z && a.type == b.type;
}

std::vector<PropSnapshot> makeSnapshot() {
    std::vector<PropSnapshot> out;
    out.reserve(mapProps.size());
    for (const auto& p : mapProps) {
        out.push_back({(int)floorf(p.pos.x * 10.0f), (int)floorf(p.pos.z * 10.0f), p.type});
    }
    std::sort(out.begin(), out.end(), [](const PropSnapshot& a, const PropSnapshot& b) {
        if (a.x != b.x) return a.x < b.x;
        if (a.z != b.z) return a.z < b.z;
        return a.type < b.type;
    });
    return out;
}

void generateArea(int cx, int cz) {
    updateVisibleChunks(cx * CHUNK_SIZE * CS, cz * CHUNK_SIZE * CS);
    updateMapContent(cx, cz);
}

void testDeterministicPlacementBySeed() {
    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    worldSeed = 424242;
    generateArea(0, 0);
    auto a = makeSnapshot();

    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    worldSeed = 424242;
    generateArea(0, 0);
    auto b = makeSnapshot();

    assert(!a.empty());
    assert(a.size() == b.size());
    assert(a == b);
}

void testGeneratesVariedProps() {
    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    worldSeed = 1701;

    for (int cx = -1; cx <= 1; cx++) {
        for (int cz = -1; cz <= 1; cz++) {
            generateChunk(cx, cz);
        }
    }
    updateMapContent(0, 0);

    assert(mapProps.size() > 35);
    bool seen[6] = {false, false, false, false, false, false};
    for (const auto& p : mapProps) {
        if (p.type >= 0 && p.type < 6) seen[p.type] = true;
    }
    int seenCount = 0;
    for (bool v : seen) if (v) seenCount++;
    assert(seenCount >= 5);
}

void testNoPropsInsideWalls() {
    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    worldSeed = 8088;
    generateArea(0, 0);
    assert(!mapProps.empty());

    for (const auto& p : mapProps) {
        int wx = (int)floorf(p.pos.x / CS);
        int wz = (int)floorf(p.pos.z / CS);
        assert(getCellWorld(wx, wz) == 0);
    }
}

int main() {
    testDeterministicPlacementBySeed();
    testGeneratesVariedProps();
    testNoPropsInsideWalls();
    std::cout << "All map content tests passed.\n";
    return 0;
}
