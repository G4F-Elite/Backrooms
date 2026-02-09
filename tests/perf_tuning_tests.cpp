#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "../src/math.h"
#include "../src/world.h"
#include "../src/perf_tuning.h"

// ============================================================================
// RENDER TARGET SIZE TESTS
// ============================================================================

void testComputeRenderTargetSize() {
    int outW, outH;
    
    // Normal case
    computeRenderTargetSize(1920, 1080, 0.75f, outW, outH);
    assert(outW == 1440);
    assert(outH == 810);
    
    // Min scale clamping
    computeRenderTargetSize(1920, 1080, 0.3f, outW, outH);
    assert(outW == 576);  // 0.3 scale is now valid
    assert(outH == 324);

    // New lower minimum scale
    computeRenderTargetSize(1920, 1080, 0.1f, outW, outH);
    assert(outW == 384);  // 0.2 min scale
    assert(outH == 216);
    
    // Max scale clamping
    computeRenderTargetSize(1920, 1080, 1.5f, outW, outH);
    assert(outW == 1920);  // 1.0 max scale
    assert(outH == 1080);
    
    // Minimum size clamping
    computeRenderTargetSize(400, 200, 0.2f, outW, outH);
    assert(outW == 160);  // Min width
    assert(outH == 90);   // Min height
    
    std::cout << "  [PASS] Render target size computation\n";
}

// ============================================================================
// LIGHT KEY TESTS
// ============================================================================

void testSceneLightKey() {
    Light l1, l2, l3;
    l1.pos = Vec3(10.5f, 2.8f, 15.3f);
    l2.pos = Vec3(10.5f, 2.8f, 15.3f);  // Same as l1
    l3.pos = Vec3(20.0f, 5.0f, 30.0f);  // Different
    
    int key1 = sceneLightKey(l1);
    int key2 = sceneLightKey(l2);
    int key3 = sceneLightKey(l3);
    
    // Same position should give same key
    assert(key1 == key2);
    
    // Different positions should (usually) give different keys
    assert(key1 != key3);
    
    std::cout << "  [PASS] Scene light key generation\n";
}

// ============================================================================
// LIGHT GATHERING TESTS
// ============================================================================

void testGatherNearestSceneLights() {
    std::vector<Light> lights;
    
    // Create some test lights
    for (int i = 0; i < 20; i++) {
        Light l;
        l.pos = Vec3((float)i * 5.0f, 2.8f, (float)i * 5.0f);
        l.on = true;
        lights.push_back(l);
    }
    
    Vec3 camPos(25.0f, 1.5f, 25.0f);  // Near middle
    float outPos[SCENE_LIGHT_LIMIT * 3];
    float outFade[SCENE_LIGHT_LIMIT];
    
    int count = gatherNearestSceneLights(lights, camPos, outPos, outFade, 0.0f);
    
    // Should get up to SCENE_LIGHT_LIMIT lights
    assert(count <= SCENE_LIGHT_LIMIT);
    assert(count > 0);
    
    std::cout << "  [PASS] Light gathering (count=" << count << ")\n";
}

void testGatherLightsDistanceOrdering() {
    std::vector<Light> lights;
    
    // Create lights at known distances
    Light near, mid, far;
    near.pos = Vec3(5.0f, 2.8f, 0.0f);
    near.on = true;
    mid.pos = Vec3(15.0f, 2.8f, 0.0f);
    mid.on = true;
    far.pos = Vec3(30.0f, 2.8f, 0.0f);
    far.on = true;
    
    lights.push_back(far);
    lights.push_back(near);
    lights.push_back(mid);
    
    Vec3 camPos(0.0f, 1.5f, 0.0f);
    float outPos[SCENE_LIGHT_LIMIT * 3];
    float outFade[SCENE_LIGHT_LIMIT];
    
    int count = gatherNearestSceneLights(lights, camPos, outPos, outFade, 0.0f);
    
    assert(count == 3);
    
    // First light should be the nearest
    assert(std::fabs(outPos[0] - 5.0f) < 0.1f);  // near.x
    // Second should be mid
    assert(std::fabs(outPos[3] - 15.0f) < 0.1f);  // mid.x
    // Third should be far
    assert(std::fabs(outPos[6] - 30.0f) < 0.1f);  // far.x
    
    std::cout << "  [PASS] Light distance ordering\n";
}

void testInactiveLightsIgnored() {
    std::vector<Light> lights;
    
    Light active, inactive;
    active.pos = Vec3(10.0f, 2.8f, 0.0f);
    active.on = true;
    inactive.pos = Vec3(5.0f, 2.8f, 0.0f);  // Closer but inactive
    inactive.on = false;
    
    lights.push_back(inactive);
    lights.push_back(active);
    
    Vec3 camPos(0.0f, 1.5f, 0.0f);
    float outPos[SCENE_LIGHT_LIMIT * 3];
    float outFade[SCENE_LIGHT_LIMIT];
    
    int count = gatherNearestSceneLights(lights, camPos, outPos, outFade, 0.0f);
    
    // Only active light should be included
    assert(count == 1);
    assert(std::fabs(outPos[0] - 10.0f) < 0.1f);
    
    std::cout << "  [PASS] Inactive lights ignored\n";
}

// ============================================================================
// LIGHT ATTENUATION TESTS
// ============================================================================

void testLightAttenuationLinear() {
    // At distance 0, full intensity
    assert(std::fabs(lightAttenuationLinear(0.0f, 10.0f) - 1.0f) < 0.001f);
    
    // At half distance, half intensity
    assert(std::fabs(lightAttenuationLinear(5.0f, 10.0f) - 0.5f) < 0.001f);
    
    // At max distance, zero intensity
    assert(std::fabs(lightAttenuationLinear(10.0f, 10.0f) - 0.0f) < 0.001f);
    
    // Beyond max distance, still zero
    assert(std::fabs(lightAttenuationLinear(15.0f, 10.0f) - 0.0f) < 0.001f);
    
    std::cout << "  [PASS] Linear light attenuation\n";
}

void testLightAttenuationQuadratic() {
    // At distance 0, full intensity
    assert(std::fabs(lightAttenuationQuadratic(0.0f, 10.0f) - 1.0f) < 0.001f);
    
    // Intensity decreases with distance
    float at5 = lightAttenuationQuadratic(5.0f, 10.0f);
    float at10 = lightAttenuationQuadratic(10.0f, 10.0f);
    assert(at5 > at10);
    
    // At radius distance, should be 0.5
    assert(std::fabs(lightAttenuationQuadratic(10.0f, 10.0f) - 0.5f) < 0.001f);
    
    std::cout << "  [PASS] Quadratic light attenuation\n";
}

void testLightAttenuationSmooth() {
    // Before inner radius, full intensity
    assert(std::fabs(lightAttenuationSmooth(2.0f, 5.0f, 15.0f) - 1.0f) < 0.001f);
    
    // After outer radius, zero intensity
    assert(std::fabs(lightAttenuationSmooth(20.0f, 5.0f, 15.0f) - 0.0f) < 0.001f);
    
    // In between, smooth falloff
    float mid = lightAttenuationSmooth(10.0f, 5.0f, 15.0f);
    assert(mid > 0.0f && mid < 1.0f);
    
    std::cout << "  [PASS] Smooth light attenuation\n";
}

void testLightAttenuationExp() {
    g_fastMathEnabled = true;
    
    // At distance 0, full intensity
    assert(std::fabs(lightAttenuationExp(0.0f, 0.1f) - 1.0f) < 0.001f);
    
    // Decreases exponentially
    float at1 = lightAttenuationExp(1.0f, 0.5f);
    float at2 = lightAttenuationExp(2.0f, 0.5f);
    assert(at1 > at2);
    
    // Fast falloff with higher falloff parameter
    float fast = lightAttenuationExp(5.0f, 1.0f);
    float slow = lightAttenuationExp(5.0f, 0.1f);
    assert(fast < slow);
    
    std::cout << "  [PASS] Exponential light attenuation\n";
    g_fastMathEnabled = false;
}

// ============================================================================
// FRUSTUM CULLING TESTS
// ============================================================================

void testIsInFrustum() {
    Vec3 camPos(0, 0, 0);
    Vec3 camFwd(0, 0, 1);  // Looking down +Z
    float nearZ = 0.1f;
    float farZ = 100.0f;
    float fovHalfTan = 0.577f;  // ~60 degree FOV
    
    // Object directly in front
    assert(isInFrustum(Vec3(0, 0, 10), 1.0f, camPos, camFwd, nearZ, farZ, fovHalfTan) == true);
    
    // Object behind camera
    assert(isInFrustum(Vec3(0, 0, -10), 1.0f, camPos, camFwd, nearZ, farZ, fovHalfTan) == false);
    
    // Object too far
    assert(isInFrustum(Vec3(0, 0, 150), 1.0f, camPos, camFwd, nearZ, farZ, fovHalfTan) == false);
    
    // Object far to the side
    assert(isInFrustum(Vec3(50, 0, 10), 1.0f, camPos, camFwd, nearZ, farZ, fovHalfTan) == false);
    
    std::cout << "  [PASS] Frustum culling\n";
}

// ============================================================================
// OCCLUSION GRID TESTS
// ============================================================================

void testOcclusionGrid() {
    OcclusionGrid grid;
    grid.clear();
    
    // Initially all cells should be unoccupied
    assert(grid.isOccupied(0.0f, 0.0f) == false);
    assert(grid.isOccupied(10.0f, 10.0f) == false);
    
    // Mark a cell
    grid.markOccupied(10.0f, 10.0f);
    assert(grid.isOccupied(10.0f, 10.0f) == true);
    
    // Nearby but different cell should still be unoccupied
    assert(grid.isOccupied(0.0f, 0.0f) == false);
    
    // Clear and verify
    grid.clear();
    assert(grid.isOccupied(10.0f, 10.0f) == false);
    
    std::cout << "  [PASS] Occlusion grid\n";
}

// ============================================================================
// LOD CALCULATION TESTS
// ============================================================================

void testCalculateLodFromScreenSize() {
    g_fastMathEnabled = true;
    
    // Large object close = high detail (LOD 0)
    int lod1 = calculateLodFromScreenSize(5.0f, 5.0f, 1080.0f, 1.0f);
    assert(lod1 == 0 || lod1 == 1);
    
    // Same object far away = low detail
    int lod2 = calculateLodFromScreenSize(5.0f, 100.0f, 1080.0f, 1.0f);
    assert(lod2 > lod1);
    
    // Small object close = lower detail than large object
    int lod3 = calculateLodFromScreenSize(0.5f, 5.0f, 1080.0f, 1.0f);
    assert(lod3 >= lod1);
    
    std::cout << "  [PASS] LOD calculation from screen size\n";
    g_fastMathEnabled = false;
}

void testCalculateLodBlendFactor() {
    // Before start = 0
    assert(std::fabs(calculateLodBlendFactor(5.0f, 10.0f, 20.0f) - 0.0f) < 0.001f);
    
    // After end = 1
    assert(std::fabs(calculateLodBlendFactor(25.0f, 10.0f, 20.0f) - 1.0f) < 0.001f);
    
    // Middle = 0.5
    assert(std::fabs(calculateLodBlendFactor(15.0f, 10.0f, 20.0f) - 0.5f) < 0.001f);
    
    std::cout << "  [PASS] LOD blend factor\n";
}

// ============================================================================
// FAST MATH INTEGRATION TESTS
// ============================================================================

void testFastMathIntegration() {
    g_fastMathEnabled = true;
    
    // Test that light gathering still works with fast math
    std::vector<Light> lights;
    for (int i = 0; i < 10; i++) {
        Light l;
        l.pos = Vec3((float)i * 3.0f, 2.8f, 0.0f);
        l.on = true;
        lights.push_back(l);
    }
    
    Vec3 camPos(15.0f, 1.5f, 0.0f);
    float outPos[SCENE_LIGHT_LIMIT * 3];
    float outFade[SCENE_LIGHT_LIMIT];
    
    int count = gatherNearestSceneLights(lights, camPos, outPos, outFade, 0.0f);
    assert(count > 0);
    
    // Test attenuation functions
    float atten1 = lightAttenuationExp(5.0f, 0.2f);
    assert(atten1 > 0.0f && atten1 < 1.0f);
    
    float atten2 = lightAttenuationSmooth(7.5f, 5.0f, 10.0f);
    assert(atten2 > 0.0f && atten2 < 1.0f);
    
    std::cout << "  [PASS] Fast math integration in perf_tuning\n";
    g_fastMathEnabled = false;
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    std::cout << "=== Performance Tuning Tests ===\n\n";
    
    std::cout << "Render Target:\n";
    testComputeRenderTargetSize();
    
    std::cout << "\nLight System:\n";
    testSceneLightKey();
    testGatherNearestSceneLights();
    testGatherLightsDistanceOrdering();
    testInactiveLightsIgnored();
    
    std::cout << "\nLight Attenuation:\n";
    testLightAttenuationLinear();
    testLightAttenuationQuadratic();
    testLightAttenuationSmooth();
    testLightAttenuationExp();
    
    std::cout << "\nCulling:\n";
    testIsInFrustum();
    testOcclusionGrid();
    
    std::cout << "\nLOD System:\n";
    testCalculateLodFromScreenSize();
    testCalculateLodBlendFactor();
    
    std::cout << "\nFast Math Integration:\n";
    testFastMathIntegration();
    
    std::cout << "\n=== All performance tuning tests passed! ===\n";
    return 0;
}
