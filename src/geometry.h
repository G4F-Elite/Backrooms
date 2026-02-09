#pragma once
#include <vector>
#include "math.h"

// ============================================================================
// GEOMETRY GENERATION UTILITIES
// Uses fast math when g_fastMathEnabled is true
// ============================================================================

inline void mkWall(std::vector<float>& v, float x, float z, float dx, float dz, float h, float CS, float WH) {
    Vec3 n = Vec3(dz, 0, -dx).norm();
    float wallLen = g_fastMathEnabled ? fastSqrt(dx * dx + dz * dz) : sqrtf(dx * dx + dz * dz);
    float tx = (wallLen / CS) * 1.8f;
    float ty = (h / WH) * 1.6f;
    float vv[] = {
        x, 0, z, 0, 0, n.x, n.y, n.z,
        x, h, z, 0, ty, n.x, n.y, n.z,
        x + dx, h, z + dz, tx, ty, n.x, n.y, n.z,
        x, 0, z, 0, 0, n.x, n.y, n.z,
        x + dx, h, z + dz, tx, ty, n.x, n.y, n.z,
        x + dx, 0, z + dz, tx, 0, n.x, n.y, n.z
    };
    v.insert(v.end(), vv, vv + 48);
}

inline void mkPillar(std::vector<float>& v, float cx, float cz, float s, float h) {
    float hs = s / 2;
    float ty = h / (s * 2.0f);
    if (ty < 1.0f) ty = 1.0f;

    auto pushFace = [&](float x0, float y0, float z0,
                        float x1, float y1, float z1,
                        float x2, float y2, float z2,
                        float x3, float y3, float z3,
                        float nx, float ny, float nz) {
        float vv[] = {
            x0, y0, z0, 0, 0, nx, ny, nz,
            x1, y1, z1, 1, 0, nx, ny, nz,
            x2, y2, z2, 1, ty, nx, ny, nz,
            x0, y0, z0, 0, 0, nx, ny, nz,
            x2, y2, z2, 1, ty, nx, ny, nz,
            x3, y3, z3, 0, ty, nx, ny, nz
        };
        v.insert(v.end(), vv, vv + 48);
    };

    // Front (+Z)
    pushFace(cx - hs, 0, cz + hs, cx + hs, 0, cz + hs, cx + hs, h, cz + hs, cx - hs, h, cz + hs, 0, 0, 1);
    // Back (-Z)
    pushFace(cx + hs, 0, cz - hs, cx - hs, 0, cz - hs, cx - hs, h, cz - hs, cx + hs, h, cz - hs, 0, 0, -1);
    // Right (+X)
    pushFace(cx + hs, 0, cz + hs, cx + hs, 0, cz - hs, cx + hs, h, cz - hs, cx + hs, h, cz + hs, 1, 0, 0);
    // Left (-X)
    pushFace(cx - hs, 0, cz - hs, cx - hs, 0, cz + hs, cx - hs, h, cz + hs, cx - hs, h, cz - hs, -1, 0, 0);
}

inline void mkLight(std::vector<float>& v, Vec3 pos, float sx, float sz) {
    float x = pos.x - sx / 2, z = pos.z - sz / 2, y = pos.y;
    float vv[] = {
        x, y, z, 0, 0,
        x + sx, y, z, 1, 0,
        x + sx, y, z + sz, 1, 1,
        x, y, z, 0, 0,
        x + sx, y, z + sz, 1, 1,
        x, y, z + sz, 0, 1
    };
    v.insert(v.end(), vv, vv + 30);
}

inline void mkBox(std::vector<float>& v, float cx, float y0, float cz, float sx, float sy, float sz) {
    float hx = sx * 0.5f;
    float hz = sz * 0.5f;
    float y1 = y0 + sy;
    const float tile = 2.4f;
    float uvX = sx / tile;
    float uvY = sy / tile;
    float uvZ = sz / tile;
    if (uvX < 0.6f) uvX = 0.6f;
    if (uvY < 0.6f) uvY = 0.6f;
    if (uvZ < 0.6f) uvZ = 0.6f;

    auto pushQuad = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n, float uMax, float vMax) {
        float vv[] = {
            a.x, a.y, a.z, 0, 0, n.x, n.y, n.z,
            b.x, b.y, b.z, uMax, 0, n.x, n.y, n.z,
            c.x, c.y, c.z, uMax, vMax, n.x, n.y, n.z,
            a.x, a.y, a.z, 0, 0, n.x, n.y, n.z,
            c.x, c.y, c.z, uMax, vMax, n.x, n.y, n.z,
            d.x, d.y, d.z, 0, vMax, n.x, n.y, n.z
        };
        v.insert(v.end(), vv, vv + 48);
    };

    pushQuad(
        Vec3(cx - hx, y0, cz + hz), Vec3(cx + hx, y0, cz + hz),
        Vec3(cx + hx, y1, cz + hz), Vec3(cx - hx, y1, cz + hz),
        Vec3(0, 0, 1), uvX, uvY
    );
    pushQuad(
        Vec3(cx + hx, y0, cz - hz), Vec3(cx - hx, y0, cz - hz),
        Vec3(cx - hx, y1, cz - hz), Vec3(cx + hx, y1, cz - hz),
        Vec3(0, 0, -1), uvX, uvY
    );
    pushQuad(
        Vec3(cx + hx, y0, cz + hz), Vec3(cx + hx, y0, cz - hz),
        Vec3(cx + hx, y1, cz - hz), Vec3(cx + hx, y1, cz + hz),
        Vec3(1, 0, 0), uvZ, uvY
    );
    pushQuad(
        Vec3(cx - hx, y0, cz - hz), Vec3(cx - hx, y0, cz + hz),
        Vec3(cx - hx, y1, cz + hz), Vec3(cx - hx, y1, cz - hz),
        Vec3(-1, 0, 0), uvZ, uvY
    );
    pushQuad(
        Vec3(cx - hx, y1, cz + hz), Vec3(cx + hx, y1, cz + hz),
        Vec3(cx + hx, y1, cz - hz), Vec3(cx - hx, y1, cz - hz),
        Vec3(0, 1, 0), uvX, uvZ
    );
}

inline void mkShaftWall(std::vector<float>& v, float x, float z, float dx, float dz, float topY, float depth, float CS) {
    Vec3 n = Vec3(dz, 0, -dx).norm();
    float wallLen = sqrtf(dx * dx + dz * dz);
    float tx = (wallLen / CS) * 1.8f;
    float ty = (depth / 4.5f) * 3.0f;
    float botY = topY - depth;
    float vv[] = {
        x, botY, z, 0, 0, n.x, n.y, n.z,
        x, topY, z, 0, ty, n.x, n.y, n.z,
        x + dx, topY, z + dz, tx, ty, n.x, n.y, n.z,
        x, botY, z, 0, 0, n.x, n.y, n.z,
        x + dx, topY, z + dz, tx, ty, n.x, n.y, n.z,
        x + dx, botY, z + dz, tx, 0, n.x, n.y, n.z
    };
    for (int i = 0; i < 48; i++) v.push_back(vv[i]);
}

inline void mkFloorDecal(std::vector<float>& v, float cx, float y, float cz, float sx, float sz) {
    float hx = sx * 0.5f;
    float hz = sz * 0.5f;
    float vv[] = {
        cx - hx, y, cz - hz, 0, 0, 0, 1, 0,
        cx - hx, y, cz + hz, 0, 1, 0, 1, 0,
        cx + hx, y, cz + hz, 1, 1, 0, 1, 0,
        cx - hx, y, cz - hz, 0, 0, 0, 1, 0,
        cx + hx, y, cz + hz, 1, 1, 0, 1, 0,
        cx + hx, y, cz - hz, 1, 0, 0, 1, 0
    };
    for (int i = 0; i < 48; i++) v.push_back(vv[i]);
}

// Note paper model - floating sheet with slight rotation
// Uses fast sin/cos when enabled for bob animation
inline void mkNotePaper(std::vector<float>& v, Vec3 pos, float bob, float tm) {
    float y = 0.8f + mSin(bob) * 0.1f;
    float rot = tm * 0.5f;
    float s = 0.25f;
    float cr = mCos(rot), sr = mSin(rot);
    Vec3 corners[4] = {
        {-s * cr, y, -s * sr},
        {s * cr, y, s * sr},
        {s * cr, y + 0.35f, s * sr},
        {-s * cr, y + 0.35f, -s * sr}
    };
    Vec3 n(0, 0, 1);
    float vv[] = {
        pos.x + corners[0].x, corners[0].y, pos.z + corners[0].z, 0, 0, n.x, n.y, n.z,
        pos.x + corners[1].x, corners[1].y, pos.z + corners[1].z, 1, 0, n.x, n.y, n.z,
        pos.x + corners[2].x, corners[2].y, pos.z + corners[2].z, 1, 1, n.x, n.y, n.z,
        pos.x + corners[0].x, corners[0].y, pos.z + corners[0].z, 0, 0, n.x, n.y, n.z,
        pos.x + corners[2].x, corners[2].y, pos.z + corners[2].z, 1, 1, n.x, n.y, n.z,
        pos.x + corners[3].x, corners[3].y, pos.z + corners[3].z, 0, 1, n.x, n.y, n.z,
        // Back side
        pos.x + corners[1].x, corners[1].y, pos.z + corners[1].z, 0, 0, -n.x, -n.y, -n.z,
        pos.x + corners[0].x, corners[0].y, pos.z + corners[0].z, 1, 0, -n.x, -n.y, -n.z,
        pos.x + corners[3].x, corners[3].y, pos.z + corners[3].z, 1, 1, -n.x, -n.y, -n.z,
        pos.x + corners[1].x, corners[1].y, pos.z + corners[1].z, 0, 0, -n.x, -n.y, -n.z,
        pos.x + corners[3].x, corners[3].y, pos.z + corners[3].z, 1, 1, -n.x, -n.y, -n.z,
        pos.x + corners[2].x, corners[2].y, pos.z + corners[2].z, 0, 1, -n.x, -n.y, -n.z
    };
    for (int i = 0; i < 96; i++) v.push_back(vv[i]);
}

// Glowing marker for notes - easy to spot
// Uses fast sin for bob animation
inline void mkNoteGlow(std::vector<float>& v, Vec3 pos, float bob) {
    float y = 0.5f + mSin(bob) * 0.15f;
    float s = 0.4f;
    // Vertical quad facing camera (billboard approximation - just 4 directions)
    for (int dir = 0; dir < 4; dir++) {
        float ang = dir * MATH_HALF_PI;
        float sn = mSin(ang), cn = mCos(ang);
        Vec3 n(sn, 0, cn);
        float dx = n.x * 0.01f, dz = n.z * 0.01f;
        float vv[] = {
            pos.x + dx - n.z * s, y, pos.z + dz + n.x * s, 0, 0, n.x, n.y, n.z,
            pos.x + dx + n.z * s, y, pos.z + dz - n.x * s, 1, 0, n.x, n.y, n.z,
            pos.x + dx + n.z * s, y + s * 2, pos.z + dz - n.x * s, 1, 1, n.x, n.y, n.z,
            pos.x + dx - n.z * s, y, pos.z + dz + n.x * s, 0, 0, n.x, n.y, n.z,
            pos.x + dx + n.z * s, y + s * 2, pos.z + dz - n.x * s, 1, 1, n.x, n.y, n.z,
            pos.x + dx - n.z * s, y + s * 2, pos.z + dz + n.x * s, 0, 1, n.x, n.y, n.z
        };
        for (int i = 0; i < 48; i++) v.push_back(vv[i]);
    }
}

// ============================================================================
// FAST GEOMETRY UTILITIES
// Distance-based culling and LOD helpers
// ============================================================================

// Check if point is within view frustum (simplified box check)
inline bool isInViewBox(const Vec3& point, const Vec3& camPos, const Vec3& camFwd, float maxDist, float fovHalf) {
    Vec3 toPoint = point - camPos;
    float dist = toPoint.len();
    if (dist > maxDist) return false;
    
    // Simplified cone check using dot product
    if (dist > 0.1f) {
        toPoint.normalize();
        float dot = toPoint.dot(camFwd);
        float minDot = mCos(fovHalf * 1.2f); // Some margin
        if (dot < minDot) return false;
    }
    return true;
}

// Fast distance-squared check for culling (avoids sqrt)
inline bool isInRange(const Vec3& a, const Vec3& b, float maxDist) {
    return a.distSqTo(b) <= maxDist * maxDist;
}

// Calculate LOD level based on distance (0 = highest detail)
inline int getLodLevel(float distSq, float lodDist0, float lodDist1, float lodDist2) {
    if (distSq <= lodDist0 * lodDist0) return 0;
    if (distSq <= lodDist1 * lodDist1) return 1;
    if (distSq <= lodDist2 * lodDist2) return 2;
    return 3;
}

// Fast billboard rotation facing camera (Y-axis only)
inline void getBillboardRotation(const Vec3& objPos, const Vec3& camPos, float& outAngle) {
    float dx = camPos.x - objPos.x;
    float dz = camPos.z - objPos.z;
    outAngle = mAtan2(dx, dz);
}

// Generate circular points for effects (uses fast sin/cos)
inline void generateCirclePoints(float cx, float cy, float radius, int segments, float* outX, float* outY) {
    float angleStep = MATH_TAU / (float)segments;
    for (int i = 0; i < segments; i++) {
        float angle = i * angleStep;
        outX[i] = cx + radius * mCos(angle);
        outY[i] = cy + radius * mSin(angle);
    }
}

// Generate spiral points (for effects, particle paths)
inline void generateSpiralPoints(float cx, float cy, float startRadius, float endRadius, 
                                  float startAngle, float turns, int points, 
                                  float* outX, float* outY) {
    float angleStep = turns * MATH_TAU / (float)(points - 1);
    float radiusStep = (endRadius - startRadius) / (float)(points - 1);
    
    for (int i = 0; i < points; i++) {
        float angle = startAngle + i * angleStep;
        float radius = startRadius + i * radiusStep;
        outX[i] = cx + radius * mCos(angle);
        outY[i] = cy + radius * mSin(angle);
    }
}