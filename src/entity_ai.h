#pragma once

#include <vector>

#include "math.h"
#include "entity_types.h"

// ============================================================================
// ENTITY CAP AND SPAWN CALCULATIONS
// ============================================================================

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

// ============================================================================
// PROXIMITY CHECKS
// Uses fast math for distance calculations (avoids sqrt when possible)
// ============================================================================

inline bool hasEntityNearPos(const std::vector<Entity>& entities, const Vec3& pos, float minDist) {
    float minDistSq = minDist * minDist;
    for (const auto& e : entities) {
        if (!e.active) continue;
        // Fast distance squared calculation (no sqrt needed for comparison)
        float dx = e.pos.x - pos.x;
        float dz = e.pos.z - pos.z;
        float dsq = dx * dx + dz * dz;
        if (dsq < minDistSq) return true;
    }
    return false;
}

// Find nearest entity to position, returns index or -1
inline int findNearestEntity(const std::vector<Entity>& entities, const Vec3& pos, float maxDist) {
    float maxDistSq = maxDist * maxDist;
    float nearestDistSq = maxDistSq;
    int nearestIdx = -1;
    
    for (int i = 0; i < (int)entities.size(); i++) {
        if (!entities[i].active) continue;
        float dx = entities[i].pos.x - pos.x;
        float dz = entities[i].pos.z - pos.z;
        float dsq = dx * dx + dz * dz;
        if (dsq < nearestDistSq) {
            nearestDistSq = dsq;
            nearestIdx = i;
        }
    }
    return nearestIdx;
}

// Get actual distance to nearest entity (uses fast sqrt when enabled)
inline float getDistanceToNearestEntity(const std::vector<Entity>& entities, const Vec3& pos) {
    float nearestDistSq = 1e30f;
    
    for (const auto& e : entities) {
        if (!e.active) continue;
        float dx = e.pos.x - pos.x;
        float dz = e.pos.z - pos.z;
        float dsq = dx * dx + dz * dz;
        if (dsq < nearestDistSq) {
            nearestDistSq = dsq;
        }
    }
    
    return mSqrt(nearestDistSq);
}

// ============================================================================
// SPAWN TYPE SELECTION
// ============================================================================

inline EntityType chooseSpawnEntityType(float survivalTime, int rollA, int rollB) {
    if (survivalTime > 220.0f && (rollA % 100) < 60) return ENTITY_SHADOW;
    if (survivalTime > 120.0f && (rollA % 100) < 35) return ENTITY_SHADOW;
    if (survivalTime > 140.0f && (rollB % 100) < 45) return ENTITY_CRAWLER;
    return ENTITY_STALKER;
}

// ============================================================================
// AI MOVEMENT CALCULATIONS
// Uses fast math for pathfinding and steering
// ============================================================================

// Calculate direction to target (normalized, XZ plane only)
inline Vec3 getDirectionToTarget(const Vec3& from, const Vec3& to) {
    Vec3 dir(to.x - from.x, 0.0f, to.z - from.z);
    float lenSq = dir.x * dir.x + dir.z * dir.z;
    if (lenSq > 0.0001f) {
        float invLen = mInvSqrt(lenSq);
        dir.x *= invLen;
        dir.z *= invLen;
    }
    return dir;
}

// Calculate angle from entity to target (radians)
inline float getAngleToTarget(const Vec3& entityPos, float entityYaw, const Vec3& targetPos) {
    float dx = targetPos.x - entityPos.x;
    float dz = targetPos.z - entityPos.z;
    float targetAngle = mAtan2(dx, dz);
    return fastAngleDiff(entityYaw, targetAngle);
}

// Smooth rotation towards target
inline float rotateTowardsTarget(float currentYaw, float targetYaw, float maxRotation) {
    float diff = fastAngleDiff(currentYaw, targetYaw);
    if (fastAbs(diff) <= maxRotation) {
        return targetYaw;
    }
    return currentYaw + (diff > 0 ? maxRotation : -maxRotation);
}

// ============================================================================
// STEERING BEHAVIORS
// ============================================================================

// Seek - move towards target
inline Vec3 steerSeek(const Vec3& pos, const Vec3& target, float maxSpeed) {
    Vec3 desired = getDirectionToTarget(pos, target);
    return desired * maxSpeed;
}

// Flee - move away from threat
inline Vec3 steerFlee(const Vec3& pos, const Vec3& threat, float maxSpeed) {
    Vec3 desired = getDirectionToTarget(threat, pos); // Reversed direction
    return desired * maxSpeed;
}

// Wander - random movement (uses fast sin/cos)
inline Vec3 steerWander(const Vec3& forward, float wanderRadius, float wanderDistance, 
                         float& wanderAngle, float wanderJitter, float maxSpeed) {
    // Add random jitter to wander angle
    wanderAngle += (((float)(rand() % 1000) / 500.0f) - 1.0f) * wanderJitter;
    
    // Calculate wander target
    Vec3 circleCenter = forward * wanderDistance;
    Vec3 displacement(mCos(wanderAngle) * wanderRadius, 0, mSin(wanderAngle) * wanderRadius);
    
    Vec3 wanderForce = circleCenter + displacement;
    float lenSq = wanderForce.lenSq();
    if (lenSq > 0.0001f) {
        float invLen = mInvSqrt(lenSq);
        wanderForce = wanderForce * (invLen * maxSpeed);
    }
    return wanderForce;
}

// Arrive - slow down as approaching target
inline Vec3 steerArrive(const Vec3& pos, const Vec3& target, float maxSpeed, float slowRadius) {
    Vec3 toTarget = target - pos;
    toTarget.y = 0;
    float distSq = toTarget.lenSq();
    
    if (distSq < 0.0001f) return Vec3();
    
    float dist = mSqrt(distSq);
    float invDist = 1.0f / dist;
    
    float speed = maxSpeed;
    if (dist < slowRadius) {
        speed = maxSpeed * (dist / slowRadius);
    }
    
    return Vec3(toTarget.x * invDist * speed, 0, toTarget.z * invDist * speed);
}

// Pursuit - predict target position
inline Vec3 steerPursuit(const Vec3& pos, const Vec3& vel, 
                          const Vec3& targetPos, const Vec3& targetVel, 
                          float maxSpeed) {
    Vec3 toTarget = targetPos - pos;
    toTarget.y = 0;
    float dist = mSqrt(toTarget.lenSq());
    
    // Predict where target will be
    float lookAhead = dist / maxSpeed;
    Vec3 predictedPos = targetPos + targetVel * lookAhead;
    
    return steerSeek(pos, predictedPos, maxSpeed);
}

// Evade - predict and avoid target
inline Vec3 steerEvade(const Vec3& pos, const Vec3& vel,
                        const Vec3& threatPos, const Vec3& threatVel,
                        float maxSpeed) {
    Vec3 toThreat = threatPos - pos;
    toThreat.y = 0;
    float dist = mSqrt(toThreat.lenSq());
    
    float lookAhead = dist / maxSpeed;
    Vec3 predictedPos = threatPos + threatVel * lookAhead;
    
    return steerFlee(pos, predictedPos, maxSpeed);
}

// ============================================================================
// VISIBILITY AND LINE OF SIGHT
// ============================================================================

// Simple cone-based visibility check
inline bool canSeeTarget(const Vec3& pos, float yaw, const Vec3& target, 
                          float maxDist, float fovHalf) {
    Vec3 toTarget = target - pos;
    toTarget.y = 0;
    
    float distSq = toTarget.lenSq();
    if (distSq > maxDist * maxDist) return false;
    
    // Get angle to target
    float dist = mSqrt(distSq);
    if (dist < 0.1f) return true; // Very close
    
    float invDist = 1.0f / dist;
    float targetAngle = mAtan2(toTarget.x * invDist, toTarget.z * invDist);
    float angleDiff = fastAbs(fastAngleDiff(yaw, targetAngle));
    
    return angleDiff <= fovHalf;
}

// Calculate awareness level based on distance and angle
inline float calculateAwareness(const Vec3& entityPos, float entityYaw,
                                 const Vec3& playerPos, float maxDist, float fovHalf) {
    Vec3 toPlayer = playerPos - entityPos;
    toPlayer.y = 0;
    
    float distSq = toPlayer.lenSq();
    if (distSq > maxDist * maxDist) return 0.0f;
    
    float dist = mSqrt(distSq);
    float distFactor = 1.0f - (dist / maxDist);
    
    float targetAngle = mAtan2(toPlayer.x, toPlayer.z);
    float angleDiff = fastAbs(fastAngleDiff(entityYaw, targetAngle));
    float angleFactor = 1.0f - fastClamp01(angleDiff / fovHalf);
    
    return distFactor * angleFactor;
}

// ============================================================================
// PATHFINDING HELPERS
// ============================================================================

// Simple grid-based A* heuristic (Manhattan with fast sqrt for diagonal)
inline float astarHeuristic(int x1, int y1, int x2, int y2) {
    int dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);
    int dy = (y1 > y2) ? (y1 - y2) : (y2 - y1);
    // Octile distance approximation
    int minD = (dx < dy) ? dx : dy;
    int maxD = (dx > dy) ? dx : dy;
    return (float)maxD + 0.414f * (float)minD;
}

// Fast check if path between two points is roughly clear (ray march)
inline bool isPathClear(const Vec3& from, const Vec3& to, 
                         bool (*isBlocked)(float, float), int steps) {
    Vec3 dir = to - from;
    float stepX = dir.x / (float)steps;
    float stepZ = dir.z / (float)steps;
    
    for (int i = 1; i < steps; i++) {
        float x = from.x + stepX * i;
        float z = from.z + stepZ * i;
        if (isBlocked(x, z)) return false;
    }
    return true;
}
