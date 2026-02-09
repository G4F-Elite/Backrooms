#pragma once

#include <vector>
#include <unordered_map>
#include <cstring>

#include "math.h"
#include "world.h"

// ============================================================================
// PERFORMANCE TUNING CONSTANTS
// ============================================================================
inline constexpr int SCENE_LIGHT_LIMIT = 16;
inline constexpr float SCENE_LIGHT_MAX_DIST = 50.0f;  // Extended range for smoother transitions
inline constexpr float SCENE_LIGHT_FADE_START = 35.0f; // Start fading at this distance
inline constexpr float SCENE_RENDER_SCALE = 0.75f;
inline constexpr float LIGHT_FADE_SPEED = 3.0f; // How fast lights fade in/out per second

// ============================================================================
// RENDER TARGET UTILITIES
// ============================================================================
inline void computeRenderTargetSize(int winW, int winH, float scale, int& outW, int& outH) {
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 1.0f) scale = 1.0f;
    outW = fastFloor((float)winW * scale);
    outH = fastFloor((float)winH * scale);
    if (outW < 320) outW = 320;
    if (outH < 180) outH = 180;
}

// ============================================================================
// LIGHT DATA STRUCTURES
// ============================================================================

// Stores light position + fade factor
struct SceneLightData {
    float x, y, z;
    float fade; // 1.0 = full brightness, 0.0 = off
    int lightId; // For tracking across frames
};

// Temporal light state for smooth transitions
struct LightTemporalState {
    float currentFade = 0.0f;
    float targetFade = 0.0f;
    bool wasVisible = false;
};

// Global state for light temporal smoothing
inline std::unordered_map<int, LightTemporalState> g_lightStates;
inline float g_lastLightUpdateTime = 0.0f;

// ============================================================================
// LIGHT KEY GENERATION
// Uses fast floor for quantization
// ============================================================================
inline int sceneLightKey(const Light& l) {
    int qx = fastFloor(l.pos.x * 4.0f);
    int qy = fastFloor(l.pos.y * 4.0f);
    int qz = fastFloor(l.pos.z * 4.0f);
    return (qx * 73856093) ^ (qy * 19349663) ^ (qz * 83492791);
}

// ============================================================================
// TEMPORAL LIGHT STATE UPDATES
// ============================================================================
inline void updateLightTemporalStates(float currentTime) {
    float dt = currentTime - g_lastLightUpdateTime;
    if (dt <= 0.0f || dt > 0.5f) dt = 0.016f; // Cap at reasonable value
    g_lastLightUpdateTime = currentTime;
    
    for (auto& [id, state] : g_lightStates) {
        if (state.currentFade < state.targetFade) {
            state.currentFade += LIGHT_FADE_SPEED * dt;
            if (state.currentFade > state.targetFade) state.currentFade = state.targetFade;
        } else if (state.currentFade > state.targetFade) {
            state.currentFade -= LIGHT_FADE_SPEED * dt;
            if (state.currentFade < state.targetFade) state.currentFade = state.targetFade;
        }
    }
}

inline void cleanupOldLightStates() {
    // Remove lights that have been off for a while
    std::vector<int> toRemove;
    for (auto& [id, state] : g_lightStates) {
        if (!state.wasVisible && state.currentFade <= 0.001f) {
            toRemove.push_back(id);
        }
        state.wasVisible = false; // Reset for next frame
    }
    for (int id : toRemove) {
        g_lightStates.erase(id);
    }
}

// ============================================================================
// MAIN LIGHT GATHERING FUNCTION
// Uses fast math for distance calculations when enabled
// ============================================================================
inline int gatherNearestSceneLights(const std::vector<Light>& lights, const Vec3& camPos, 
                                     float outPos[SCENE_LIGHT_LIMIT * 3], 
                                     float outFade[SCENE_LIGHT_LIMIT],
                                     float currentTime) {
    updateLightTemporalStates(currentTime);
    
    SceneLightData bestLights[SCENE_LIGHT_LIMIT];
    float bestDist2[SCENE_LIGHT_LIMIT];
    int count = 0;
    const float maxDist2 = SCENE_LIGHT_MAX_DIST * SCENE_LIGHT_MAX_DIST;

    // Precompute squared fade-start distance to avoid sqrt for close lights
    const float fadeStart2 = SCENE_LIGHT_FADE_START * SCENE_LIGHT_FADE_START;
    
    // First pass: mark all lights as not visible, calculate distances
    for (int i = 0; i < (int)lights.size(); i++) {
        const auto& l = lights[i];
        if (!l.on) continue;
        
        // Fast distance squared calculation (no sqrt needed for comparison)
        Vec3 d = l.pos - camPos;
        float dist2 = d.lenSq();
        
        // Early rejection: far beyond max distance with no existing state worth keeping
        int lightId = sceneLightKey(l);
        auto it = g_lightStates.find(lightId);
        if (dist2 > maxDist2 * 1.5f && (it == g_lightStates.end() || it->second.currentFade <= 0.001f)) continue;
        
        // Only compute sqrt if we need to calculate fade (light is beyond fade start)
        float distFade = 1.0f;
        if (dist2 > fadeStart2) {
            float dist = mSqrt(dist2);
            float t = (dist - SCENE_LIGHT_FADE_START) / (SCENE_LIGHT_MAX_DIST - SCENE_LIGHT_FADE_START);
            distFade = 1.0f - fastClamp01(t);
        }
        
        auto& state = g_lightStates[lightId];
        state.targetFade = distFade;
        state.wasVisible = true;
        
        // Only include lights that are or were recently visible
        if (dist2 > maxDist2 && state.currentFade <= 0.001f) continue;
        
        // Combine distance fade with temporal fade
        float finalFade = state.currentFade;
        
        SceneLightData ld = {l.pos.x, l.pos.y, l.pos.z, finalFade, lightId};

        if (count < SCENE_LIGHT_LIMIT) {
            int idx = count++;
            while (idx > 0 && bestDist2[idx - 1] > dist2) {
                bestDist2[idx] = bestDist2[idx - 1];
                bestLights[idx] = bestLights[idx - 1];
                idx--;
            }
            bestDist2[idx] = dist2;
            bestLights[idx] = ld;
        } else if (dist2 < bestDist2[count - 1]) {
            int idx = count - 1;
            while (idx > 0 && bestDist2[idx - 1] > dist2) {
                bestDist2[idx] = bestDist2[idx - 1];
                bestLights[idx] = bestLights[idx - 1];
                idx--;
            }
            bestDist2[idx] = dist2;
            bestLights[idx] = ld;
        }
    }

    // Mark lights not in view for fade out
    for (auto& [id, state] : g_lightStates) {
        if (!state.wasVisible) {
            state.targetFade = 0.0f;
        }
    }

    // Fill output arrays
    for (int i = 0; i < count; i++) {
        outPos[i * 3 + 0] = bestLights[i].x;
        outPos[i * 3 + 1] = bestLights[i].y;
        outPos[i * 3 + 2] = bestLights[i].z;
        outFade[i] = bestLights[i].fade;
    }
    
    // Periodic cleanup
    static int cleanupCounter = 0;
    if (++cleanupCounter > 60) {
        cleanupCounter = 0;
        cleanupOldLightStates();
    }
    
    return count;
}

// Legacy overload for compatibility
inline int gatherNearestSceneLights(const std::vector<Light>& lights, const Vec3& camPos, float outPos[SCENE_LIGHT_LIMIT * 3]) {
    float outFade[SCENE_LIGHT_LIMIT];
    return gatherNearestSceneLights(lights, camPos, outPos, outFade, 0.0f);
}

// ============================================================================
// FAST LIGHT ATTENUATION
// Various attenuation models optimized for performance
// ============================================================================

// Linear attenuation (fastest)
inline float lightAttenuationLinear(float dist, float maxDist) {
    if (dist >= maxDist) return 0.0f;
    return 1.0f - dist / maxDist;
}

// Quadratic attenuation (physically accurate)
inline float lightAttenuationQuadratic(float dist, float radius) {
    float d = dist / radius;
    float denom = 1.0f + d * d;
    return 1.0f / denom;
}

// Smoothstep attenuation (nice falloff, no hard edge)
inline float lightAttenuationSmooth(float dist, float innerRadius, float outerRadius) {
    if (dist <= innerRadius) return 1.0f;
    if (dist >= outerRadius) return 0.0f;
    float t = (dist - innerRadius) / (outerRadius - innerRadius);
    return fastSmoothstep(0.0f, 1.0f, 1.0f - t);
}

// Exponential attenuation (good for fog-like effects)
inline float lightAttenuationExp(float dist, float falloff) {
    return mExp(-dist * falloff);
}

// ============================================================================
// CULLING UTILITIES
// Fast object culling for performance
// ============================================================================

// Simple frustum culling (sphere test)
inline bool isInFrustum(const Vec3& objPos, float objRadius, 
                         const Vec3& camPos, const Vec3& camFwd, 
                         float nearZ, float farZ, float fovHalfTan) {
    Vec3 toObj = objPos - camPos;
    float depth = toObj.dot(camFwd);
    
    // Behind near plane or beyond far plane
    if (depth < nearZ - objRadius || depth > farZ + objRadius) return false;
    
    // Check against side planes (simplified cone test)
    float maxSide = depth * fovHalfTan + objRadius;
    Vec3 camRight = camFwd.cross(Vec3(0, 1, 0)).norm();
    Vec3 camUp = camRight.cross(camFwd);
    
    float sideX = fastAbs(toObj.dot(camRight));
    float sideY = fastAbs(toObj.dot(camUp));
    
    return sideX <= maxSide && sideY <= maxSide;
}

// Occlusion grid for simple spatial queries
struct OcclusionGrid {
    static constexpr int GRID_SIZE = 32;
    static constexpr float CELL_SIZE = 4.0f;
    
    bool occupied[GRID_SIZE][GRID_SIZE];
    
    void clear() {
        memset(occupied, 0, sizeof(occupied));
    }
    
    void markOccupied(float x, float z) {
        int gx = fastFloor(x / CELL_SIZE) + GRID_SIZE / 2;
        int gz = fastFloor(z / CELL_SIZE) + GRID_SIZE / 2;
        if (gx >= 0 && gx < GRID_SIZE && gz >= 0 && gz < GRID_SIZE) {
            occupied[gx][gz] = true;
        }
    }
    
    bool isOccupied(float x, float z) const {
        int gx = fastFloor(x / CELL_SIZE) + GRID_SIZE / 2;
        int gz = fastFloor(z / CELL_SIZE) + GRID_SIZE / 2;
        if (gx >= 0 && gx < GRID_SIZE && gz >= 0 && gz < GRID_SIZE) {
            return occupied[gx][gz];
        }
        return false;
    }
};

// ============================================================================
// LOD SYSTEM
// Level-of-detail calculations
// ============================================================================

// Calculate LOD level based on screen-space size
inline int calculateLodFromScreenSize(float objectSize, float distance, float screenHeight, float fov) {
    // Approximate screen-space height
    float screenSize = (objectSize * screenHeight) / (2.0f * distance * mTan(fov * 0.5f));
    
    if (screenSize > 200.0f) return 0; // Full detail
    if (screenSize > 100.0f) return 1;
    if (screenSize > 50.0f) return 2;
    if (screenSize > 25.0f) return 3;
    return 4; // Lowest detail or billboard
}

// Simple LOD blend factor for smooth transitions
inline float calculateLodBlendFactor(float distance, float lodStart, float lodEnd) {
    if (distance <= lodStart) return 0.0f;
    if (distance >= lodEnd) return 1.0f;
    return (distance - lodStart) / (lodEnd - lodStart);
}