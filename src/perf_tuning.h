#pragma once

#include <vector>
#include <cmath>

#include "world.h"

inline constexpr int SCENE_LIGHT_LIMIT = 16;
inline constexpr float SCENE_LIGHT_MAX_DIST = 22.0f;

inline void computeRenderTargetSize(int winW, int winH, float scale, int& outW, int& outH) {
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 1.0f) scale = 1.0f;
    outW = (int)floorf((float)winW * scale);
    outH = (int)floorf((float)winH * scale);
    if (outW < 320) outW = 320;
    if (outH < 180) outH = 180;
}

inline int gatherNearestSceneLights(const std::vector<Light>& lights, const Vec3& camPos, float outPos[SCENE_LIGHT_LIMIT * 3]) {
    float bestDist2[SCENE_LIGHT_LIMIT];
    Vec3 bestPos[SCENE_LIGHT_LIMIT];
    int count = 0;
    const float maxDist2 = SCENE_LIGHT_MAX_DIST * SCENE_LIGHT_MAX_DIST;

    for (const auto& l : lights) {
        if (!l.on) continue;
        Vec3 d = l.pos - camPos;
        float dist2 = d.x * d.x + d.y * d.y + d.z * d.z;
        if (dist2 > maxDist2) continue;

        if (count < SCENE_LIGHT_LIMIT) {
            int idx = count++;
            while (idx > 0 && bestDist2[idx - 1] > dist2) {
                bestDist2[idx] = bestDist2[idx - 1];
                bestPos[idx] = bestPos[idx - 1];
                idx--;
            }
            bestDist2[idx] = dist2;
            bestPos[idx] = l.pos;
        } else if (dist2 < bestDist2[count - 1]) {
            int idx = count - 1;
            while (idx > 0 && bestDist2[idx - 1] > dist2) {
                bestDist2[idx] = bestDist2[idx - 1];
                bestPos[idx] = bestPos[idx - 1];
                idx--;
            }
            bestDist2[idx] = dist2;
            bestPos[idx] = l.pos;
        }
    }

    for (int i = 0; i < count; i++) {
        outPos[i * 3 + 0] = bestPos[i].x;
        outPos[i * 3 + 1] = bestPos[i].y;
        outPos[i * 3 + 2] = bestPos[i].z;
    }
    return count;
}
