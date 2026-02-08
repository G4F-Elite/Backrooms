#pragma once

#include <vector>

#include "entity_types.h"

inline int computeEntityCap(float survivalTime) {
    if (survivalTime < 90.0f) return 1;
    if (survivalTime < 210.0f) return 2;
    if (survivalTime < 360.0f) return 3;
    return 4;
}

inline float computeEntitySpawnDelay(float survivalTime, int roll) {
    float base = 36.0f - survivalTime * 0.04f;
    if (base < 12.0f) base = 12.0f;
    int extra = roll % 10;
    if (extra < 0) extra += 10;
    return base + (float)extra;
}

inline bool hasEntityNearPos(const std::vector<Entity>& entities, const Vec3& pos, float minDist) {
    float minDistSq = minDist * minDist;
    for (const auto& e : entities) {
        if (!e.active) continue;
        Vec3 d = e.pos - pos;
        d.y = 0;
        float dsq = d.x * d.x + d.z * d.z;
        if (dsq < minDistSq) return true;
    }
    return false;
}

inline EntityType chooseSpawnEntityType(float survivalTime, int rollA, int rollB) {
    if (survivalTime > 260.0f && (rollA % 100) < 45) return ENTITY_SHADOW;
    if (survivalTime > 140.0f && (rollB % 100) < 45) return ENTITY_CRAWLER;
    return ENTITY_STALKER;
}
