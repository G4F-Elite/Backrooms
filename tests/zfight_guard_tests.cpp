// Z-fight guard: detection, repair and the rules content relies on.
//   g++ -std=c++17 -I deps/include tests/zfight_guard_tests.cpp -o build/zfight_guard_tests.exe
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "../src/math.h"
#include "../src/decor_builder.h"
#include "../src/zfight_guard.h"

const int STRIDE = 8;

// Flat quad at height y, lit from above or below.
void pushQuad(std::vector<float>& v, float x0, float z0, float x1, float z1, float y, float ny = 1.0f) {
    float q[] = {
        x0, y, z0, 0, 0, 0, ny, 0,
        x0, y, z1, 0, 1, 0, ny, 0,
        x1, y, z1, 1, 1, 0, ny, 0,
        x0, y, z0, 0, 0, 0, ny, 0,
        x1, y, z1, 1, 1, 0, ny, 0,
        x1, y, z0, 1, 0, 0, ny, 0
    };
    v.insert(v.end(), q, q + 48);
}

double areaOf(const std::vector<float>& v, int stride = STRIDE) {
    double sum = 0.0;
    for (size_t i = 0; i + (size_t)stride * 3 <= v.size(); i += (size_t)stride * 3) {
        Vec3 a(v[i], v[i + 1], v[i + 2]);
        Vec3 b(v[i + stride], v[i + stride + 1], v[i + stride + 2]);
        Vec3 c(v[i + 2 * stride], v[i + 2 * stride + 1], v[i + 2 * stride + 2]);
        sum += (double)(b - a).cross(c - a).len() * 0.5;
    }
    return sum;
}

bool about(double a, double b, double eps = 1e-4) {
    return std::fabs(a - b) < eps;
}

void testFindsSharedArea() {
    std::vector<float> v;
    pushQuad(v, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f);
    pushQuad(v, 1.0f, 1.0f, 3.0f, 3.0f, 1.0f);
    ZfMesh m = {&v, STRIDE, 0, "quads"};
    ZfReport r = zfightScan(&m, 1);
    assert(!r.clean());
    assert(about(r.area, 1.0));
    assert(!r.samples.empty());
    assert(r.samples[0].pos[0] > 1.0f && r.samples[0].pos[0] < 2.0f);
    assert(about(r.samples[0].pos[1], 1.0));
}

// The two halves of a quad and neighbouring tiles share a plane but no area.
void testTilesAndQuadHalvesAreNotConflicts() {
    std::vector<float> v;
    for (int x = 0; x < 4; x++) {
        for (int z = 0; z < 4; z++) pushQuad(v, (float)x * 5.0f, (float)z * 5.0f, (float)x * 5.0f + 5.0f, (float)z * 5.0f + 5.0f, 0.0f);
    }
    ZfMesh m = {&v, STRIDE, 0, "floor"};
    assert(zfightScan(&m, 1).clean());
}

void testLaterFaceKeepsTheSpot() {
    std::vector<float> v;
    pushQuad(v, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f);
    pushQuad(v, 1.0f, 1.0f, 3.0f, 3.0f, 1.0f);
    std::vector<float> second(v.begin() + 48, v.end());
    ZfMesh m = {&v, STRIDE, 0, "quads"};
    ZfReport r = zfightResolve(&m, 1);
    assert(r.trisCut == 2);
    assert(zfightScan(&m, 1).clean());
    assert(about(areaOf(v), 7.0));                                  // 4 + 4 - 1
    assert(std::equal(second.begin(), second.end(), v.begin() + 48)); // the later quad is untouched
    assert(v.size() % (STRIDE * 3) == 0);
}

void testHigherRankKeepsTheSpot() {
    std::vector<float> floor, decal;
    pushQuad(decal, 1.0f, 1.0f, 2.0f, 2.0f, 0.0f);
    pushQuad(floor, 0.0f, 0.0f, 5.0f, 5.0f, 0.0f);
    // the decal comes first in the list, but its rank is higher
    ZfMesh layers[2] = {{&decal, STRIDE, 2, "decal"}, {&floor, STRIDE, 0, "floor"}};
    zfightResolve(layers, 2);
    assert(about(areaOf(decal), 1.0));
    assert(about(areaOf(floor), 24.0));
    assert(zfightScan(layers, 2).clean());
}

// A face hidden completely keeps its place in the buffer as a collapsed triangle.
void testFullyCoveredFaceCollapses() {
    std::vector<float> v;
    pushQuad(v, 1.0f, 1.0f, 2.0f, 2.0f, 0.5f);
    pushQuad(v, 0.0f, 0.0f, 3.0f, 3.0f, 0.5f);
    ZfMesh m = {&v, STRIDE, 0, "quads"};
    zfightResolve(&m, 1);
    assert(v.size() == 96);
    assert(about(areaOf(v), 9.0));
    assert(zfightScan(&m, 1).clean());
}

void testBackToBackAndDistantFacesAreLeftAlone() {
    std::vector<float> v;
    pushQuad(v, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f, 1.0f);
    pushQuad(v, 0.0f, 0.0f, 2.0f, 2.0f, 1.0f, -1.0f);  // underside of something standing on it
    pushQuad(v, 0.0f, 0.0f, 2.0f, 2.0f, 1.002f, 1.0f); // 2 mm above: the depth buffer can tell
    ZfMesh m = {&v, STRIDE, 0, "quads"};
    assert(zfightScan(&m, 1).clean());

    pushQuad(v, 0.5f, 0.5f, 1.0f, 1.0f, 1.001f, 1.0f); // 1 mm above the first: cannot
    ZfReport r = zfightScan(&m, 1);
    assert(r.pairs > 0);
    // it also lies 1 mm under the 2 mm face
    assert(about(r.area, 0.5));
}

// Rotated solids far from the origin: float rounding must not hide or invent overlaps.
void testRotatedBoxesFarFromOrigin() {
    for (int i = 0; i < 12; i++) {
        std::vector<float> v;
        DecorBuilder b(v, 700.0f + 31.7f * (float)i, -512.3f, 0.37f * (float)i, 1.0f);
        b.box(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, DECOR_MAT_WOOD);
        b.box(0.25f, 0.0f, 0.5f, 1.0f, 1.0f, 1.0f, DECOR_MAT_METAL);
        ZfMesh m = {&v, STRIDE, 0, "boxes"};
        ZfReport found = zfightScan(&m, 1);
        // top and bottom share 0.75 x 0.5 each; the sides are in different planes
        assert(about(found.area, 0.75, 0.01));
        zfightResolve(&m, 1);
        assert(zfightScan(&m, 1).clean());
    }
}

void testLampPanelsUseTheirWinding() {
    // pos uv only: the facing comes from the winding
    std::vector<float> lamp = {
        0, 2, 0, 0, 0, 1, 2, 0, 1, 0, 1, 2, 1, 1, 1,
        0, 2, 0, 0, 0, 1, 2, 1, 1, 1, 0, 2, 1, 0, 1
    };
    std::vector<float> ceiling;
    pushQuad(ceiling, -2.0f, -2.0f, 3.0f, 3.0f, 2.0f, -1.0f);
    ZfMesh layers[2] = {{&ceiling, STRIDE, 0, "ceiling"}, {&lamp, 5, 3, "lamp"}};
    assert(about(zfightScan(layers, 2).area, 1.0));
    zfightResolve(layers, 2);
    assert(about(areaOf(ceiling), 24.0));
    assert(about(areaOf(lamp, 5), 1.0));
    assert(zfightScan(layers, 2).clean());
}

// Big meshes are checked on several threads; the mesh that comes out must be the same.
void testThreadsGiveTheSameMesh() {
    std::vector<float> v;
    for (int i = 0; i < 4000; i++) {
        DecorBuilder b(v, (float)(i % 60) * 1.7f, (float)(i / 60) * 1.9f, 0.11f * (float)i, 1.0f);
        b.box(0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f, DECOR_MAT_WOOD);
        b.box(0.3f, 0.0f, 0.2f, 1.0f, 0.5f, 1.0f, DECOR_MAT_METAL);
    }
    std::vector<float> a = v, b4 = v;
    ZfMesh ma = {&a, STRIDE, 0, "one"}, mb = {&b4, STRIDE, 0, "four"};
    ZfConfig one, four;
    four.threads = 4;
    ZfReport ra = zfightResolve(&ma, 1, one);
    ZfReport rb = zfightResolve(&mb, 1, four);
    assert(ra.pairs == rb.pairs && ra.pairs >= 8000);
    assert(a == b4);
    assert(zfightScan(&ma, 1).clean());
}

int main() {
    testFindsSharedArea();
    testTilesAndQuadHalvesAreNotConflicts();
    testLaterFaceKeepsTheSpot();
    testHigherRankKeepsTheSpot();
    testFullyCoveredFaceCollapses();
    testBackToBackAndDistantFacesAreLeftAlone();
    testRotatedBoxesFarFromOrigin();
    testLampPanelsUseTheirWinding();
    testThreadsGiveTheSameMesh();
    std::cout << "All z-fight guard tests passed.\n";
    return 0;
}
