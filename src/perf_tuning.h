#pragma once

#include <vector>
#include <cmath>
#include <unordered_map>

#include "world.h"

inline constexpr int SCENE_LIGHT_LIMIT = 16;
inline constexpr float SCENE_LIGHT_MAX_DIST = 50.0f;  // Extended range for smoother transitions
inline constexpr float SCENE_LIGHT_FADE_START = 35.0f; // Start fading at this distance
inline constexpr float SCENE_RENDER_SCALE = 0.75f;
inline constexpr float LIGHT_FADE_SPEED = 3.0f; // How fast lights fade in/out per second

inline void computeRenderTargetSize(int winW, int winH, float scale, int& outW, int& outH) {
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 1.0f) scale = 1.0f;
    outW = (int)floorf((float)winW * scale);
    outH = (int)floorf((float)winH * scale);
    if (outW < 320) outW = 320;
    if (outH < 180) outH = 180;
}

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

inline int sceneLightKey(const Light& l) {
    int qx = (int)floorf(l.pos.x * 4.0f);
    int qy = (int)floorf(l.pos.y * 4.0f);
    int qz = (int)floorf(l.pos.z * 4.0f);
    return (qx * 73856093) ^ (qy * 19349663) ^ (qz * 83492791);
}

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

// Returns count of lights, fills outPos[i*3+0..2] with positions, outFade[i] with fade factors
inline int gatherNearestSceneLights(const std::vector<Light>& lights, const Vec3& camPos, 
                                     float outPos[SCENE_LIGHT_LIMIT * 3], 
                                     float outFade[SCENE_LIGHT_LIMIT],
                                     float currentTime) {
    updateLightTemporalStates(currentTime);
    
    SceneLightData bestLights[SCENE_LIGHT_LIMIT];
    float bestDist2[SCENE_LIGHT_LIMIT];
    int count = 0;
    const float maxDist2 = SCENE_LIGHT_MAX_DIST * SCENE_LIGHT_MAX_DIST;

    // First pass: mark all lights as not visible, calculate distances
    for (int i = 0; i < (int)lights.size(); i++) {
        const auto& l = lights[i];
        if (!l.on) continue;
        
        Vec3 d = l.pos - camPos;
        float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
        
        // Calculate distance-based fade (before culling check)
        float dist = sqrtf(dist2);
        float distFade = 1.0f;
        if (dist > SCENE_LIGHT_FADE_START) {
            distFade = 1.0f - (dist - SCENE_LIGHT_FADE_START) / (SCENE_LIGHT_MAX_DIST - SCENE_LIGHT_FADE_START);
            if (distFade < 0.0f) distFade = 0.0f;
        }
        
        int lightId = sceneLightKey(l);
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
