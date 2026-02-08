#include <cassert>
#include <iostream>
#include <vector>
#include <random>
#include <unordered_map>

#include "../src/world.h"
const float CS = 5.0f;
const float WH = 4.5f;
std::mt19937 rng;
unsigned int worldSeed = 0;
std::unordered_map<long long, Chunk> chunks;
std::vector<Light> lights;
std::vector<Vec3> pillars;
int playerChunkX = 0;
int playerChunkZ = 0;

#include "../src/perf_tuning.h"

void testRenderTargetSizingClamp() {
    int w = 0, h = 0;
    computeRenderTargetSize(1280, 720, 0.2f, w, h);
    assert(w == 640);
    assert(h == 360);
}

void testRenderTargetSizingScaled() {
    int w = 0, h = 0;
    computeRenderTargetSize(1920, 1080, 0.75f, w, h);
    assert(w == 1440);
    assert(h == 810);
}

void testNearestLightsSelection() {
    std::vector<Light> src;
    for (int i = 0; i < 20; i++) {
        Light l;
        l.pos = Vec3((float)i, 0, 0);
        l.on = true;
        src.push_back(l);
    }
    src[3].on = false;

    float out[SCENE_LIGHT_LIMIT * 3] = {0};
    int count = gatherNearestSceneLights(src, Vec3(0, 0, 0), out);
    assert(count <= SCENE_LIGHT_LIMIT);
    assert(count > 0);
    // nearest enabled light should be x=0
    assert(out[0] == 0.0f);
}

void testDistanceCulling() {
    std::vector<Light> src;
    Light nearL; nearL.pos = Vec3(1, 0, 0); nearL.on = true; src.push_back(nearL);
    Light farL; farL.pos = Vec3(200, 0, 0); farL.on = true; src.push_back(farL);
    float out[SCENE_LIGHT_LIMIT * 3] = {0};
    int count = gatherNearestSceneLights(src, Vec3(0, 0, 0), out);
    assert(count == 1);
    assert(out[0] == 1.0f);
}

int main() {
    testRenderTargetSizingClamp();
    testRenderTargetSizingScaled();
    testNearestLightsSelection();
    testDistanceCulling();
    std::cout << "All perf tuning tests passed.\n";
    return 0;
}
