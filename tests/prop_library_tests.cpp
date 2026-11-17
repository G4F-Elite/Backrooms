#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "../src/world.h"
#include "../src/map_content.h"
#include "../src/prop_models.h"

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

const int STRIDE = 8;
const float EPS = 0.015f;

struct Bounds {
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f, minZ = 1e9f, maxZ = -1e9f;
};

std::vector<float> buildProp(int type, float x, float z, float yaw) {
    MapProp pr{};
    pr.pos = Vec3(x, 0.0f, z);
    pr.type = type;
    pr.scale = 1.0f;
    pr.yaw = yaw;
    std::vector<float> v;
    buildMapPropModel(v, pr);
    return v;
}

Bounds boundsOf(const std::vector<float>& v) {
    Bounds b;
    for (size_t i = 0; i < v.size(); i += STRIDE) {
        if (v[i] < b.minX) b.minX = v[i];
        if (v[i] > b.maxX) b.maxX = v[i];
        if (v[i + 1] < b.minY) b.minY = v[i + 1];
        if (v[i + 1] > b.maxY) b.maxY = v[i + 1];
        if (v[i + 2] < b.minZ) b.minZ = v[i + 2];
        if (v[i + 2] > b.maxZ) b.maxZ = v[i + 2];
    }
    return b;
}

void assertWellFormed(const std::vector<float>& v) {
    assert(!v.empty());
    assert(v.size() % (STRIDE * 3) == 0);
    for (size_t i = 0; i < v.size(); i += STRIDE) {
        assert(v[i + 1] >= -0.0001f);
        assert(v[i + 1] <= WH + 0.0001f);
        assert(v[i + 3] >= 0.0f && v[i + 3] <= 1.0f);
        assert(v[i + 4] >= 0.0f && v[i + 4] <= 1.0f);
        float nl = sqrtf(v[i + 5] * v[i + 5] + v[i + 6] * v[i + 6] + v[i + 7] * v[i + 7]);
        assert(fabsf(nl - 1.0f) < 0.01f);
    }
}

// `prop_library_tests --measure`: real mesh bounds next to the catalog row.
void printMeasurements() {
    printf("%-20s %-9s %6s %6s | %6s %6s %6s %6s | %5s %5s | tris\n",
           "prop", "place", "hx", "hz", "minX", "maxX", "minZ", "maxZ", "minY", "maxY");
    const char* places[5] = {"floor", "wall", "wallhigh", "ceiling", "flat"};
    for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
        std::vector<float> v = buildProp(type, 0.0f, 0.0f, 0.0f);
        Bounds b = boundsOf(v);
        const PropInfo& p = PROP_CATALOG[type];
        printf("%-20s %-9s %6.3f %6.3f | %6.3f %6.3f %6.3f %6.3f | %5.2f %5.2f | %d\n",
               p.name, places[p.placement], p.hx, p.hz, b.minX, b.maxX, b.minZ, b.maxZ, b.minY, b.maxY,
               (int)v.size() / (STRIDE * 3));
    }
}

void testCatalogRowsMatchTheEnum() {
    for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
        const PropInfo& p = PROP_CATALOG[type];
        assert(p.type == type);
        assert(p.name && p.name[0] != '\0');
        assert(p.themes != 0u && (p.themes & ~(unsigned)THEME_ALL) == 0u);
        assert(p.weight > 0);
        assert(p.placement >= PROP_PLACE_FLOOR && p.placement <= PROP_PLACE_FLAT);
        for (int other = 0; other < type; other++) assert(strcmp(PROP_CATALOG[other].name, p.name) != 0);
    }
    assert(propInfo(-1) == nullptr);
    assert(propInfo(MAP_PROP_TYPE_COUNT) == nullptr);
}

// The catalog describes the base props exactly like the level 0 / parking code does.
void testCatalogAgreesWithBaseProps() {
    for (int type = 0; type < MAP_PROP_LIBRARY_FIRST; type++) {
        const PropInfo& p = PROP_CATALOG[type];
        float hx = 0.0f, hz = 0.0f;
        mapPropBaseExtents(type, hx, hz);
        assert(fabsf(hx - p.hx) < 0.0001f && fabsf(hz - p.hz) < 0.0001f);
        assert(isMapPropWallType(type) == (p.placement == PROP_PLACE_WALL));
        assert(isMapPropPushableType(type) == ((p.flags & PROP_FLAG_PUSHABLE) != 0));
        assert(isPropSolidType(type) == (type != MAP_PROP_PUDDLE));
        assert(!isPropOverheadType(type));
    }
}

// Mesh, footprint and placement rules of every library prop agree.
void testLibraryModelsFitTheirFootprint() {
    for (int type = MAP_PROP_LIBRARY_FIRST; type < MAP_PROP_TYPE_COUNT; type++) {
        const PropInfo& p = PROP_CATALOG[type];
        std::vector<float> v = buildProp(type, 0.0f, 0.0f, 0.0f);
        assertWellFormed(v);
        Bounds b = boundsOf(v);
        float hx = 0.0f, hz = 0.0f;
        mapPropBaseExtents(type, hx, hz);
        assert(hx == p.hx && hz == p.hz);

        // inside the footprint, and the footprint is not much bigger than the mesh
        assert(b.minX >= -hx - EPS && b.maxX <= hx + EPS);
        assert(b.minZ >= -hz - EPS && b.maxZ <= hz + EPS);
        assert(fmaxf(-b.minX, b.maxX) > hx - 0.03f);
        assert(fmaxf(-b.minZ, b.maxZ) > hz - 0.03f);
        assert(-b.minX > hx - 0.12f && b.maxX > hx - 0.12f);

        if (p.placement == PROP_PLACE_FLOOR) {
            assert(b.minY < 0.001f);
            assert(b.maxY <= PROP_OVERHEAD_Y - 0.1f);
            assert(-b.minZ > hz - 0.12f && b.maxZ > hz - 0.12f);
        } else if (p.placement == PROP_PLACE_WALL) {
            assert(b.maxY <= PROP_OVERHEAD_Y - 0.1f);
            assert(b.minZ < -hz + 0.012f);
        } else if (p.placement == PROP_PLACE_WALL_HIGH) {
            assert(b.minY >= PROP_OVERHEAD_Y);
            assert(b.minZ < -hz + 0.012f);
        } else if (p.placement == PROP_PLACE_CEILING) {
            assert(b.minY >= PROP_OVERHEAD_Y);
            assert(fabsf(b.maxY - WH) < 0.001f);
        } else {
            assert(b.minY < 0.01f);
            assert(b.maxY <= PROP_FLAT_MAX_Y);
        }
    }
}

void testRotatedFootprintCoversRotatedLibraryMesh() {
    const float yaws[3] = {1.5707963f, 3.1415927f, 0.65f};
    for (int type = MAP_PROP_LIBRARY_FIRST; type < MAP_PROP_TYPE_COUNT; type++) {
        for (float yaw : yaws) {
            MapProp pr{};
            pr.pos = Vec3(3.0f, 0.0f, -4.0f);
            pr.type = type;
            pr.scale = 1.0f;
            pr.yaw = yaw;
            float hx = 0.0f, hz = 0.0f;
            mapPropHalfExtents(pr, hx, hz);
            Bounds b = boundsOf(buildProp(type, pr.pos.x, pr.pos.z, yaw));
            assert(b.minX >= pr.pos.x - hx - EPS && b.maxX <= pr.pos.x + hx + EPS);
            assert(b.minZ >= pr.pos.z - hz - EPS && b.maxZ <= pr.pos.z + hz + EPS);
        }
    }
}

int countBits(unsigned v) {
    int n = 0;
    for (; v; v >>= 1) n += (int)(v & 1u);
    return n;
}

// Every theme can furnish a room on its own, and themes overlap through shared props.
void testThemesAreFurnishedAndOverlap() {
    for (int t = 0; t < THEME_COUNT; t++) {
        unsigned theme = 1u << t;
        assert(strcmp(propThemeName(theme), "mixed") != 0);
        int total = 0, signature = 0, shared = 0;
        int byPlace[5] = {0, 0, 0, 0, 0};
        for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
            const PropInfo& p = PROP_CATALOG[type];
            if ((p.themes & theme) == 0) continue;
            total++;
            byPlace[p.placement]++;
            if (countBits(p.themes) <= 2) signature++;
            if (countBits(p.themes) >= 3) shared++;
        }
        assert(total >= 16);
        assert(signature >= 4);
        assert(shared >= 6);
        assert(byPlace[PROP_PLACE_FLOOR] >= 5);
        assert(byPlace[PROP_PLACE_WALL] >= 4);
        assert(byPlace[PROP_PLACE_WALL_HIGH] + byPlace[PROP_PLACE_CEILING] >= 2);
        assert(byPlace[PROP_PLACE_FLAT] >= 1);
    }
}

// The ready-made level mixes only pair themes that have plenty of props in common,
// and between them they use every theme there is.
void testThemeBlendsJoinSoftly() {
    unsigned used = 0u;
    for (unsigned blend : THEME_BLENDS) {
        assert(countBits(blend) >= 2 && (blend & ~(unsigned)THEME_ALL) == 0u);
        used |= blend;
        for (int a = 0; a < THEME_COUNT; a++) {
            for (int b = a + 1; b < THEME_COUNT; b++) {
                unsigned pair = (1u << a) | (1u << b);
                if ((blend & pair) != pair) continue;
                int common = 0, solid = 0, overhead = 0;
                for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
                    if ((PROP_CATALOG[type].themes & pair) != pair) continue;
                    common++;
                    if (isPropSolidType(type)) solid++;
                    if (isPropOverheadType(type)) overhead++;
                }
                assert(common >= 10);
                assert(solid >= 5);
                assert(overhead >= 2);
            }
        }
    }
    assert(used == (unsigned)THEME_ALL);
}

// Props are batched per chunk, so a single one must stay cheap.
void testLibraryModelsStayLight() {
    for (int type = MAP_PROP_LIBRARY_FIRST; type < MAP_PROP_TYPE_COUNT; type++) {
        size_t tris = buildProp(type, 0.0f, 0.0f, 0.0f).size() / (STRIDE * 3);
        assert(tris >= 12 && tris <= 1000);
    }
}

// decorCone / decorBall: closed, inside their radius, normals pointing outwards.
void testLatheParts() {
    std::vector<float> v;
    DecorBuilder b(v, 0.0f, 0.0f, 0.0f, 1.0f);
    decorCone(b, 0.0f, 0.5f, 0.0f, 0.30f, 0.10f, 0.40f, DECOR_MAT_ORANGE, 8);
    assertWellFormed(v);
    Bounds cone = boundsOf(v);
    assert(fabsf(cone.minY - 0.5f) < 0.0001f && fabsf(cone.maxY - 0.9f) < 0.0001f);
    assert(cone.maxX <= 0.3001f && cone.maxZ <= 0.3001f && cone.maxZ > 0.29f);

    for (int sides = 6; sides <= 8; sides += 2) {
        v.clear();
        decorBall(b, 1.0f, 2.0f, -1.0f, 0.25f, DECOR_MAT_WHITE, sides);
        assertWellFormed(v);
        Bounds ball = boundsOf(v);
        assert(fabsf(ball.minY - 1.75f) < 0.0001f && fabsf(ball.maxY - 2.25f) < 0.0001f);
        for (size_t i = 0; i < v.size(); i += STRIDE) {
            Vec3 fromCentre(v[i] - 1.0f, v[i + 1] - 2.0f, v[i + 2] + 1.0f);
            assert(fromCentre.len() <= 0.2501f);
            Vec3 n(v[i + 5], v[i + 6], v[i + 7]);
            assert(fromCentre.len() < 0.001f || n.dot(fromCentre) > 0.0f);
        }
    }
}

void testThemedPickIsWeightedAndStaysInTheme() {
    const unsigned overhead = (1u << PROP_PLACE_WALL_HIGH) | (1u << PROP_PLACE_CEILING);
    for (int t = 0; t < THEME_COUNT; t++) {
        unsigned theme = 1u << t;
        int total = propThemeWeight(theme, 0xFFu);
        assert(total > 0);
        std::vector<int> hits(MAP_PROP_TYPE_COUNT, 0);
        for (int roll = 0; roll < total; roll++) {
            int type = pickThemedPropType(theme, (unsigned)roll);
            assert(type >= 0 && type < MAP_PROP_TYPE_COUNT);
            assert(PROP_CATALOG[type].themes & theme);
            hits[type]++;
        }
        for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
            int expect = (PROP_CATALOG[type].themes & theme) ? PROP_CATALOG[type].weight : 0;
            assert(hits[type] == expect);
        }
        for (unsigned roll = 0; roll < 200u; roll++) {
            int type = pickThemedPropType(theme, roll * 7919u, overhead);
            assert(type >= 0 && isPropOverheadType(type));
            assert(pickThemedPropType(theme, roll) == pickThemedPropType(theme, roll + (unsigned)total));
        }
    }
    assert(pickThemedPropType(0u, 5u) == -1);
    assert(pickThemedPropType(THEME_OFFICE, 5u, 0u) == -1);
}

// Each chunk of a multi-theme level settles on one of the level's themes.
void testChunkThemeComesFromTheLevelThemes() {
    worldSeed = 99;
    assert(chunkDecorTheme(0u, 3, 4) == 0u);
    assert(chunkDecorTheme(THEME_POOL, -2, 7) == (unsigned)THEME_POOL);
    unsigned level = THEME_HOTEL | THEME_POOL | THEME_MAINT;
    unsigned seen = 0u;
    for (int cx = -6; cx <= 6; cx++) {
        for (int cz = -6; cz <= 6; cz++) {
            unsigned theme = chunkDecorTheme(level, cx, cz);
            assert(countBits(theme) == 1 && (theme & level) != 0u);
            assert(theme == chunkDecorTheme(level, cx, cz));
            seen |= theme;
        }
    }
    assert(seen == level);
}

void testCompanionsShareAThemeWithTheirHost() {
    for (const PropCompanion& c : PROP_COMPANIONS) {
        assert(propInfo(c.host) && propInfo(c.buddy));
        assert((PROP_CATALOG[c.host].themes & PROP_CATALOG[c.buddy].themes) != 0u);
        assert(c.chance > 0 && c.chance <= 100);
        assert(!isPropOverheadType(c.host) && !isPropOverheadType(c.buddy));
        assert(propCompanionFor(c.host) == &c);
    }
}

// Only floor and wall props block the way; pipes, ducts and hatches do not.
void testOnlySolidPropsCollide() {
    mapProps.clear();
    mapPois.clear();
    for (int type = 0; type < MAP_PROP_TYPE_COUNT; type++) {
        MapProp pr{};
        pr.pos = Vec3(100.0f, 0.0f, 100.0f);
        pr.type = type;
        pr.scale = 1.0f;
        mapProps.assign(1, pr);
        bool blocks = collideMapProps(100.0f, 100.0f, 0.05f);
        assert(blocks == isPropSolidType(type));
    }
    mapProps.clear();
}

void installAreaChunks() {
    chunks.clear();
    pillars.clear();
    lights.clear();
    resetMapContent();
    for (int cx = -1; cx <= 1; cx++) {
        for (int cz = -1; cz <= 1; cz++) generateChunk(cx, cz);
    }
}

int spawnThemeOverArea(unsigned theme) {
    mapProps.clear();
    mapPois.clear();
    int placed = 0;
    for (int cx = -1; cx <= 1; cx++) {
        for (int cz = -1; cz <= 1; cz++) {
            std::mt19937 r(hashCell(cx, cz, 0x5EEDu));
            placed += spawnThemedProps(chunks[chunkKey(cx, cz)], r, theme, 9, 3);
        }
    }
    return placed;
}

void assertPropStandsClear(size_t index) {
    const MapProp& p = mapProps[index];
    float hx = 0.0f, hz = 0.0f;
    mapPropHalfExtents(p, hx, hz);
    int x0 = (int)floorf((p.pos.x - hx) / CS), x1 = (int)floorf((p.pos.x + hx) / CS);
    int z0 = (int)floorf((p.pos.z - hz) / CS), z1 = (int)floorf((p.pos.z + hz) / CS);
    for (int wx = x0; wx <= x1; wx++) {
        for (int wz = z0; wz <= z1; wz++) assert(getCellWorld(wx, wz) == 0);
    }
    if (propPlacement(p.type) == PROP_PLACE_CEILING) {
        assert(!isLampLatticeCell((int)floorf(p.pos.x / CS), (int)floorf(p.pos.z / CS)));
    }
    for (size_t j = index + 1; j < mapProps.size(); j++) {
        const MapProp& q = mapProps[j];
        if (isPropOverheadType(p.type) != isPropOverheadType(q.type)) continue;
        if (p.type == MAP_PROP_PUDDLE || q.type == MAP_PROP_PUDDLE) continue;
        float qx = 0.0f, qz = 0.0f;
        mapPropHalfExtents(q, qx, qz);
        bool overlap = fabsf(p.pos.x - q.pos.x) < hx + qx && fabsf(p.pos.z - q.pos.z) < hz + qz;
        assert(!overlap);
    }
}

// A wall prop has the wall right behind its back.
void assertPropBacksOntoWall(const MapProp& p) {
    float bx = 0.0f, bz = 0.0f;
    mapPropBaseExtents(p.type, bx, bz);
    Vec3 front(sinf(p.yaw), 0.0f, cosf(p.yaw));
    Vec3 behind = p.pos - front * (bz + 0.05f);
    Vec3 back = p.pos - front * (bz - 0.01f);
    assert(getCellWorld((int)floorf(behind.x / CS), (int)floorf(behind.z / CS)) == 1);
    assert(getCellWorld((int)floorf(back.x / CS), (int)floorf(back.z / CS)) == 0);
}

// Themed generation on real chunk layouts: props of the theme only, flush with their wall,
// clear of each other, of walls and of the lamps.
void testThemedSpawnFurnishesRealChunks() {
    worldSeed = 1701;
    installAreaChunks();
    for (int t = 0; t < THEME_COUNT; t++) {
        unsigned theme = 1u << t;
        int placed = spawnThemeOverArea(theme);
        assert(placed == (int)mapProps.size());
        assert(placed >= 9 * 6);
        std::vector<MapProp> first = mapProps;

        bool seenType[MAP_PROP_TYPE_COUNT] = {false};
        int overhead = 0, wallProps = 0;
        for (size_t i = 0; i < mapProps.size(); i++) {
            const MapProp& p = mapProps[i];
            assert(PROP_CATALOG[p.type].themes & theme);
            seenType[p.type] = true;
            assertPropStandsClear(i);
            if (isPropOverheadType(p.type)) overhead++;
            if (isMapPropWallType(p.type)) {
                assertPropBacksOntoWall(p);
                wallProps++;
            }
        }
        int variety = 0;
        for (bool s : seenType) if (s) variety++;
        assert(variety >= 12);
        assert(overhead >= 6);
        assert(wallProps >= 6);

        // same seed, same rooms
        spawnThemeOverArea(theme);
        assert(mapProps.size() == first.size());
        for (size_t i = 0; i < first.size(); i++) {
            assert(mapProps[i].type == first[i].type);
            assert(mapProps[i].pos.x == first[i].pos.x && mapProps[i].pos.z == first[i].pos.z);
        }
    }
}

// The two floors under the garage keep their hand-tuned prop sets: no library prop appears
// there. Every floor the drill reaches after that is dressed with one of the theme mixes.
void testExistingLevelsStayUntouched() {
    for (int level = 0; level <= 1; level++) assert(levelDecorThemes(level) == 0u);
    for (int level = 2; level <= 20; level++) {
        assert(levelDecorThemes(level) != 0u);
        assert(levelDecorThemes(level) != levelDecorThemes(level + 1)); // neighbours differ
    }
    for (int level = 0; level <= 1; level++) {
        gCurrentLevel = level;
        worldSeed = 4242;
        installAreaChunks();
        updateMapContent(0, 0);
        assert(!mapProps.empty());
        for (const MapProp& p : mapProps) assert(!isLibraryPropType(p.type));
    }
    gCurrentLevel = 0;
}

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "--measure") == 0) {
        printMeasurements();
        return 0;
    }
    testCatalogRowsMatchTheEnum();
    testCatalogAgreesWithBaseProps();
    testLibraryModelsFitTheirFootprint();
    testRotatedFootprintCoversRotatedLibraryMesh();
    testThemesAreFurnishedAndOverlap();
    testThemeBlendsJoinSoftly();
    testLibraryModelsStayLight();
    testLatheParts();
    testThemedPickIsWeightedAndStaysInTheme();
    testChunkThemeComesFromTheLevelThemes();
    testCompanionsShareAThemeWithTheirHost();
    testOnlySolidPropsCollide();
    testThemedSpawnFurnishesRealChunks();
    testExistingLevelsStayUntouched();
    std::cout << "All prop library tests passed.\n";
    return 0;
}
