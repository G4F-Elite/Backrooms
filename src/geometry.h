#pragma once
#include <vector>
#include "math.h"

inline void mkWall(std::vector<float>& v, float x, float z, float dx, float dz, float h, float CS, float WH) {
    Vec3 n = Vec3(dz, 0, -dx).norm();
    float tx = sqrtf(dx * dx + dz * dz) / CS;
    float ty = h / WH;
    float vv[] = {
        x, 0, z, 0, 0, n.x, n.y, n.z,
        x, h, z, 0, ty, n.x, n.y, n.z,
        x + dx, h, z + dz, tx, ty, n.x, n.y, n.z,
        x, 0, z, 0, 0, n.x, n.y, n.z,
        x + dx, h, z + dz, tx, ty, n.x, n.y, n.z,
        x + dx, 0, z + dz, tx, 0, n.x, n.y, n.z
    };
    for (int i = 0; i < 48; i++) v.push_back(vv[i]);
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
        for (int i = 0; i < 48; i++) v.push_back(vv[i]);
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
    for (int i = 0; i < 30; i++) v.push_back(vv[i]);
}

// Note paper model - floating sheet with slight rotation
inline void mkNotePaper(std::vector<float>& v, Vec3 pos, float bob, float tm) {
    float y = 0.8f + sinf(bob) * 0.1f;
    float rot = tm * 0.5f;
    float s = 0.25f;
    float cr = cosf(rot), sr = sinf(rot);
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
inline void mkNoteGlow(std::vector<float>& v, Vec3 pos, float bob) {
    float y = 0.5f + sinf(bob) * 0.15f;
    float s = 0.4f;
    // Vertical quad facing camera (billboard approximation - just 4 directions)
    for(int dir = 0; dir < 4; dir++) {
        float ang = dir * 1.5708f;
        Vec3 n(sinf(ang), 0, cosf(ang));
        float dx = n.x * 0.01f, dz = n.z * 0.01f;
        float vv[] = {
            pos.x + dx - n.z*s, y, pos.z + dz + n.x*s, 0, 0, n.x, n.y, n.z,
            pos.x + dx + n.z*s, y, pos.z + dz - n.x*s, 1, 0, n.x, n.y, n.z,
            pos.x + dx + n.z*s, y + s*2, pos.z + dz - n.x*s, 1, 1, n.x, n.y, n.z,
            pos.x + dx - n.z*s, y, pos.z + dz + n.x*s, 0, 0, n.x, n.y, n.z,
            pos.x + dx + n.z*s, y + s*2, pos.z + dz - n.x*s, 1, 1, n.x, n.y, n.z,
            pos.x + dx - n.z*s, y + s*2, pos.z + dz + n.x*s, 0, 1, n.x, n.y, n.z
        };
        for (int i = 0; i < 48; i++) v.push_back(vv[i]);
    }
}
