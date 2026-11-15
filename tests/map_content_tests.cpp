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

void resetAll(unsigned int seed) {
    chunks.clear();
    lights.clear();
    pillars.clear();
    resetMapContent();
    worldSeed = seed;
}

// Solid props stand on open floor only and never intersect each other.
void testPropsDoNotOverlapWallsOrEachOther() {
    const unsigned int seeds[] = {11u, 1337u, 8088u, 555555u};
    for (unsigned int seed : seeds) {
        resetAll(seed);
        generateArea(0, 0);
        for (size_t i = 0; i < mapProps.size(); i++) {
            const MapProp& a = mapProps[i];
            if (a.type == MAP_PROP_PUDDLE) continue;
            float ahx = 0.0f, ahz = 0.0f;
            mapPropHalfExtents(a, ahx, ahz);
            const float corners[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
            for (const auto& c : corners) {
                int wx = (int)floorf((a.pos.x + c[0] * ahx) / CS);
                int wz = (int)floorf((a.pos.z + c[1] * ahz) / CS);
                assert(getCellWorld(wx, wz) == 0);
            }
            assert(doorwayAxisAt((int)floorf(a.pos.x / CS), (int)floorf(a.pos.z / CS)) == 0);
            for (size_t j = i + 1; j < mapProps.size(); j++) {
                const MapProp& b = mapProps[j];
                if (b.type == MAP_PROP_PUDDLE) continue;
                float bhx = 0.0f, bhz = 0.0f;
                mapPropHalfExtents(b, bhx, bhz);
                bool overlap = fabsf(a.pos.x - b.pos.x) < ahx + bhx && fabsf(a.pos.z - b.pos.z) < ahz + bhz;
                assert(!overlap);
            }
        }
    }
}

// Lockers, cabinets and desks stand with their back flush against a wall.
void testWallPropsStandFlushAgainstWall() {
    const unsigned int seeds[] = {3u, 1337u, 424242u, 90210u};
    int checked = 0;
    for (unsigned int seed : seeds) {
        resetAll(seed);
        generateArea(0, 0);
        for (const auto& p : mapProps) {
            if (!isMapPropWallType(p.type)) continue;
            float bx = 0.0f, bz = 0.0f;
            mapPropBaseExtents(p.type, bx, bz);
            float backX = -sinf(p.yaw);
            float backZ = -cosf(p.yaw);
            float reach = bz * p.scale;
            Vec3 justBehind(p.pos.x + backX * (reach + 0.08f), 0.0f, p.pos.z + backZ * (reach + 0.08f));
            Vec3 backFace(p.pos.x + backX * (reach - 0.01f), 0.0f, p.pos.z + backZ * (reach - 0.01f));
            assert(getCellWorld((int)floorf(justBehind.x / CS), (int)floorf(justBehind.z / CS)) == 1);
            assert(getCellWorld((int)floorf(backFace.x / CS), (int)floorf(backFace.z / CS)) == 0);
            checked++;
        }
    }
    assert(checked > 5);
}

// POI sets back onto a wall, block the player, and leave the cell centre free for loot.
void testPoiSetsBackOntoWall() {
    const unsigned int seeds[] = {7u, 1337u, 777001u, 129001u, 31337u};
    int checked = 0;
    for (unsigned int seed : seeds) {
        resetAll(seed);
        generateArea(0, 0);
        for (const auto& poi : mapPois) {
            int dx = 0, dz = 0;
            mapPoiWallDir(poi, dx, dz);
            Vec3 anchor = mapPoiAnchor(poi);
            int wallX = (int)floorf((anchor.x + dx * 0.1f) / CS);
            int wallZ = (int)floorf((anchor.z + dz * 0.1f) / CS);
            assert(getCellWorld(wallX, wallZ) == 1);
            float minX, maxX, minZ, maxZ;
            mapPoiFootprint(poi, minX, maxX, minZ, maxZ);
            assert(collideMapProps((minX + maxX) * 0.5f, (minZ + maxZ) * 0.5f, 0.1f));
            assert(!collideMapPois(poi.pos.x, poi.pos.z, 0.3f));
            for (const auto& p : mapProps) {
                if (p.type == MAP_PROP_PUDDLE) continue;
                float hx = 0.0f, hz = 0.0f;
                mapPropHalfExtents(p, hx, hz);
                bool overlap = p.pos.x + hx > minX && p.pos.x - hx < maxX && p.pos.z + hz > minZ && p.pos.z - hz < maxZ;
                assert(!overlap);
            }
            checked++;
        }
    }
    assert(checked > 3);
}

// Refreshing content (or leaving and coming back) must not stack duplicates.
void testRepopulationDoesNotDuplicate() {
    resetAll(606060);
    generateArea(0, 0);
    auto first = makeSnapshot();
    size_t pois = mapPois.size();
    updateMapContent(0, 0);
    updateMapContent(0, 0);
    assert(makeSnapshot() == first);
    assert(mapPois.size() == pois);

    generateArea(4, 0);
    generateArea(0, 0);
    assert(makeSnapshot() == first);
    assert(mapPois.size() == pois);
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
    testPropsDoNotOverlapWallsOrEachOther();
    testWallPropsStandFlushAgainstWall();
    testPoiSetsBackOntoWall();
    testRepopulationDoesNotDuplicate();
    std::cout << "All map content tests passed.\n";
    return 0;
}
