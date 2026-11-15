#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

#include "../src/world.h"
#include "../src/map_content.h"
#include "../src/structure_models.h"

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

// Each face maps into exactly one atlas tile, so materials never bleed into each other.
void testBuilderKeepsUvInsideOneTile() {
    for (int mat = 0; mat < DECOR_ATLAS_GRID * DECOR_ATLAS_GRID; mat++) {
        std::vector<float> v;
        DecorBuilder b(v, 0.0f, 0.0f, 0.7f, 1.0f);
        b.box(0.0f, 0.2f, 0.0f, 3.0f, 0.4f, 0.05f, mat);
        float cell = 1.0f / (float)DECOR_ATLAS_GRID;
        float u0 = (float)(mat % DECOR_ATLAS_GRID) * cell;
        float v0 = (float)(mat / DECOR_ATLAS_GRID) * cell;
        for (size_t i = 0; i < v.size(); i += STRIDE) {
            assert(v[i + 3] > u0 && v[i + 3] < u0 + cell);
            assert(v[i + 4] > v0 && v[i + 4] < v0 + cell);
        }
    }
}

void testBuilderYawRotatesAroundOrigin() {
    std::vector<float> v;
    DecorBuilder b(v, 10.0f, 20.0f, 1.5707963f, 1.0f);
    // local +Z (front) must point along world +X after a quarter turn
    Vec3 p = b.world(0.0f, 0.0f, 1.0f);
    assert(fabsf(p.x - 11.0f) < 0.001f);
    assert(fabsf(p.z - 20.0f) < 0.001f);
}

// The mesh of every prop stays inside the footprint used for collision.
void testPropModelsFitTheirFootprint() {
    for (int type = MAP_PROP_CRATE_STACK; type <= MAP_PROP_LOCKER_BANK; type++) {
        MapProp pr{};
        pr.pos = Vec3(0.0f, 0.0f, 0.0f);
        pr.type = type;
        pr.scale = 1.0f;
        pr.yaw = 0.0f;
        std::vector<float> v;
        buildMapPropModel(v, pr);
        assertWellFormed(v);
        if (type == MAP_PROP_PUDDLE) continue;
        float hx = 0.0f, hz = 0.0f;
        mapPropBaseExtents(type, hx, hz);
        float maxX = 0.0f, maxZ = 0.0f;
        for (size_t i = 0; i < v.size(); i += STRIDE) {
            assert(fabsf(v[i]) <= hx + EPS);
            assert(fabsf(v[i + 2]) <= hz + EPS);
            if (fabsf(v[i]) > maxX) maxX = fabsf(v[i]);
            if (fabsf(v[i + 2]) > maxZ) maxZ = fabsf(v[i + 2]);
        }
        // ...and the footprint is not much bigger than the mesh (no invisible walls).
        assert(maxX > hx - 0.12f);
        assert(maxZ > hz - 0.12f);
    }
}

void testRotatedFootprintCoversRotatedMesh() {
    MapProp pr{};
    pr.pos = Vec3(3.0f, 0.0f, -4.0f);
    pr.type = MAP_PROP_LOCKER_BANK;
    pr.scale = 1.0f;
    pr.yaw = 1.5707963f;
    float hx = 0.0f, hz = 0.0f;
    mapPropHalfExtents(pr, hx, hz);
    assert(hz > hx);
    std::vector<float> v;
    buildMapPropModel(v, pr);
    for (size_t i = 0; i < v.size(); i += STRIDE) {
        assert(fabsf(v[i] - pr.pos.x) <= hx + EPS);
        assert(fabsf(v[i + 2] - pr.pos.z) <= hz + EPS);
    }
}

// POI sets start at the wall face (z = 0) and stay inside their declared size.
void testPoiSetsFitTheirFootprint() {
    for (int type = MAP_POI_OFFICE; type <= MAP_POI_RESTROOM; type++) {
        std::vector<float> v;
        DecorBuilder b(v, 0.0f, 0.0f, 0.0f, 1.0f);
        if (type == MAP_POI_OFFICE) buildPoiOfficeSet(b);
        else if (type == MAP_POI_SERVER) buildPoiServerSet(b);
        else if (type == MAP_POI_STORAGE) buildPoiStorageSet(b);
        else buildPoiRestroomSet(b);
        assertWellFormed(v);
        float width = 0.0f, depth = 0.0f;
        mapPoiSetSize(type, width, depth);
        float minZ = 1e9f;
        for (size_t i = 0; i < v.size(); i += STRIDE) {
            assert(fabsf(v[i]) <= width * 0.5f + EPS);
            assert(v[i + 2] >= -0.0001f);
            assert(v[i + 2] <= depth + EPS);
            if (v[i + 2] < minZ) minZ = v[i + 2];
        }
        assert(minZ < 0.06f);
    }
}

// The doorway infill closes the cell from wall to wall and from floor to ceiling,
// and leaves exactly the opening that collision reports as walkable.
void testDoorwayFillsWallToWall() {
    for (int axis = 1; axis <= 2; axis++) {
        std::vector<float> v;
        buildDoorwayModel(v, 0, 0, axis);
        assertWellFormed(v);
        float minA = 1e9f, maxA = -1e9f, maxY = 0.0f;
        for (size_t i = 0; i < v.size(); i += STRIDE) {
            float a = (axis == 1) ? v[i] : v[i + 2];
            if (a < minA) minA = a;
            if (a > maxA) maxA = a;
            if (v[i + 1] > maxY) maxY = v[i + 1];
        }
        assert(fabsf(minA - 0.0f) < 0.001f);
        assert(fabsf(maxA - CS) < 0.001f);
        assert(fabsf(maxY - WH) < 0.001f);

        DoorwayDims d = doorwayDims();
        float inner = d.openingHalf - d.postW * 0.5f;
        for (size_t i = 0; i < v.size(); i += STRIDE) {
            float a = ((axis == 1) ? v[i] : v[i + 2]) - CS * 0.5f;
            if (v[i + 1] < d.openingH - 0.001f) assert(fabsf(a) >= inner - 0.001f);
        }
    }
}

void testExitDoorLeafFillsTheOpening() {
    DoorwayDims d = doorwayDims();
    float inner = d.openingHalf - d.postW * 0.5f;
    std::vector<float> v;
    buildExitDoorModel(v, Vec3(0.0f, 0.0f, 0.0f), false);
    assertWellFormed(v);
    float minX = 1e9f, maxX = -1e9f, leafTop = 0.0f;
    for (size_t i = 0; i < v.size(); i += STRIDE) {
        if (v[i + 1] > d.openingH + 0.001f) continue;
        if (v[i] < minX) minX = v[i];
        if (v[i] > maxX) maxX = v[i];
        if (v[i + 1] > leafTop) leafTop = v[i + 1];
    }
    assert(fabsf(minX + inner) < 0.001f);
    assert(fabsf(maxX - inner) < 0.001f);
    assert(fabsf(leafTop - d.openingH) < 0.001f);
}

int main() {
    testBuilderKeepsUvInsideOneTile();
    testBuilderYawRotatesAroundOrigin();
    testPropModelsFitTheirFootprint();
    testRotatedFootprintCoversRotatedMesh();
    testPoiSetsFitTheirFootprint();
    testDoorwayFillsWallToWall();
    testExitDoorLeafFillsTheOpening();
    std::cout << "All decor model tests passed.\n";
    return 0;
}
