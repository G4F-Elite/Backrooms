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
std::vector<MapPoi> mapPois;
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

void installOpenChunk(int cx, int cz) {
    Chunk c{};
    c.cx = cx;
    c.cz = cz;
    c.gen = true;
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            c.cells[x][z] = (x == 0 || z == 0 || x == CHUNK_SIZE - 1 || z == CHUNK_SIZE - 1) ? 1 : 0;
        }
    }
    chunks[chunkKey(cx, cz)] = c;
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
    mapPois.clear();
    worldSeed = 424242;
    generateArea(0, 0);
    auto a = makeSnapshot();

    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    mapPois.clear();
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
    mapPois.clear();
    worldSeed = 1701;

    for (int cx = -1; cx <= 1; cx++) {
        for (int cz = -1; cz <= 1; cz++) {
            generateChunk(cx, cz);
        }
    }
    updateMapContent(0, 0);

    assert(mapProps.size() >= 8);
    bool seen[13] = {false, false, false, false, false, false, false, false, false, false, false, false, false};
    for (const auto& p : mapProps) {
        if (p.type >= 0 && p.type < 13) seen[p.type] = true;
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
    mapPois.clear();
    worldSeed = 8088;
    generateArea(0, 0);
    assert(!mapProps.empty());

    for (const auto& p : mapProps) {
        int wx = (int)floorf(p.pos.x / CS);
        int wz = (int)floorf(p.pos.z / CS);
        assert(getCellWorld(wx, wz) == 0);
    }
}

void testPropCollisionBlocksSolidPropsOnly() {
    mapProps.clear();
    MapProp solid{};
    solid.pos = Vec3(10.0f, 0.0f, 10.0f);
    solid.type = MAP_PROP_CRATE_STACK;
    solid.scale = 1.0f;
    mapProps.push_back(solid);
    assert(collideMapProps(10.0f, 10.0f, 0.2f));

    mapProps.clear();
    MapProp puddle{};
    puddle.pos = Vec3(10.0f, 0.0f, 10.0f);
    puddle.type = MAP_PROP_PUDDLE;
    puddle.scale = 1.0f;
    mapProps.push_back(puddle);
    assert(!collideMapProps(10.0f, 10.0f, 0.2f));
}

void testPoisDeterministicBySeed() {
    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    mapPois.clear();
    worldSeed = 777001;
    generateArea(0, 0);
    std::vector<int> first;
    for (const auto& poi : mapPois) {
        first.push_back((int)floorf(poi.pos.x * 10.0f));
        first.push_back((int)floorf(poi.pos.z * 10.0f));
        first.push_back(poi.type);
    }

    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    mapPois.clear();
    worldSeed = 777001;
    generateArea(0, 0);
    std::vector<int> second;
    for (const auto& poi : mapPois) {
        second.push_back((int)floorf(poi.pos.x * 10.0f));
        second.push_back((int)floorf(poi.pos.z * 10.0f));
        second.push_back(poi.type);
    }

    assert(first == second);
}

void testPoisNeverInsideWalls() {
    chunks.clear();
    lights.clear();
    pillars.clear();
    mapProps.clear();
    mapPois.clear();
    worldSeed = 129001;
    generateArea(0, 0);

    for (const auto& poi : mapPois) {
        int wx = (int)floorf(poi.pos.x / CS);
        int wz = (int)floorf(poi.pos.z / CS);
        assert(getCellWorld(wx, wz) == 0);
    }
}

void testPushablePropCanMove() {
    chunks.clear();
    installOpenChunk(0, 0);
    mapProps.clear();
    MapProp crate{};
    crate.pos = Vec3(10.0f, 0.0f, 10.0f);
    crate.type = MAP_PROP_CRATE_STACK;
    crate.scale = 1.0f;
    mapProps.push_back(crate);
    bool pushed = tryPushMapProps(10.0f, 10.0f, 0.3f, 1.0f, 0.0f);
    assert(pushed);
    assert(mapProps[0].pos.x > 10.0f);
}

void testNonPushablePropDoesNotMove() {
    chunks.clear();
    installOpenChunk(0, 0);
    mapProps.clear();
    MapProp locker{};
    locker.pos = Vec3(10.0f, 0.0f, 10.0f);
    locker.type = MAP_PROP_LOCKER_BANK;
    locker.scale = 1.0f;
    mapProps.push_back(locker);
    bool pushed = tryPushMapProps(10.0f, 10.0f, 0.3f, 1.0f, 0.0f);
    assert(!pushed);
    assert(mapProps[0].pos.x == 10.0f);
}

int main() {
    testDeterministicPlacementBySeed();
    testGeneratesVariedProps();
    testNoPropsInsideWalls();
    testPropCollisionBlocksSolidPropsOnly();
    testPoisDeterministicBySeed();
    testPoisNeverInsideWalls();
    testPushablePropCanMove();
    testNonPushablePropDoesNotMove();
    std::cout << "All map content tests passed.\n";
    return 0;
}
