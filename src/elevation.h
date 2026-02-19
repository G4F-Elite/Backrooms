#pragma once
#include <cstdint>
#include <random>
#include <vector>
#include "math.h"

extern const float CS, WH;

const float FLOOR2_Y = 4.5f;       // Second floor height (== WH)
const float FLOOR2_ROOM_H = 4.5f;  // Second floor room height (same as first floor)
const float FLOOR2_CEIL = 9.0f;    // FLOOR2_Y + FLOOR2_ROOM_H
const float RAILING_H = 1.0f;

// Elevation values: 0=ground, 1=second floor, 2=ramp+X, 3=ramp-X, 4=ramp+Z, 5=ramp-Z
inline bool isRamp(int8_t e) { return e >= 2 && e <= 5; }
inline bool isElevated(int8_t e) { return e == 1; }
inline bool isAboveGround(int8_t e) { return e >= 1 && e <= 5; }

inline float getFloorYFromElev(int8_t e) {
    return (e == 1) ? FLOOR2_Y : 0.0f;
}

inline float getRampY(int8_t e, float fracX, float fracZ) {
    switch (e) {
        case 2: return fracX * FLOOR2_Y;
        case 3: return (1.0f - fracX) * FLOOR2_Y;
        case 4: return fracZ * FLOOR2_Y;
        case 5: return (1.0f - fracZ) * FLOOR2_Y;
        default: return (e == 1) ? FLOOR2_Y : 0.0f;
    }
}

// Wall from y0 to y1 (generalized mkWall)
inline void mkWallAt(std::vector<float>& v, float x, float z,
                     float dx, float dz, float y0, float y1, float cs) {
    float wallLen = sqrtf(dx * dx + dz * dz);
    float tx = (wallLen / cs) * 1.8f;
    float ty = ((y1 - y0) / 4.5f) * 1.6f;
    auto pushFace = [&](float sx, float sz, float sdx, float sdz) {
        Vec3 n = Vec3(sdz, 0, -sdx).norm();
        float vv[] = {
            sx,y0,sz, 0,0, n.x,n.y,n.z, sx,y1,sz, 0,ty, n.x,n.y,n.z,
            sx+sdx,y1,sz+sdz, tx,ty, n.x,n.y,n.z, sx,y0,sz, 0,0, n.x,n.y,n.z,
            sx+sdx,y1,sz+sdz, tx,ty, n.x,n.y,n.z, sx+sdx,y0,sz+sdz, tx,0, n.x,n.y,n.z
        };
        v.insert(v.end(), vv, vv + 48);
    };
    pushFace(x, z, dx, dz);
    pushFace(x + dx, z + dz, -dx, -dz);
}

// Ramp slope surface from Y=0 to Y=FLOOR2_Y
inline void mkRamp(std::vector<float>& v, float px, float pz, int8_t elev, float cs) {
    float y1 = FLOOR2_Y;
    Vec3 n;
    if (elev == 2) {
        n = Vec3(-y1, cs, 0).norm();
        float d[] = {
            px,0,pz, 0,0, n.x,n.y,n.z, px,0,pz+cs, 0,1, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px,0,pz, 0,0, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px+cs,y1,pz, 1,0, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    } else if (elev == 3) {
        n = Vec3(y1, cs, 0).norm();
        float d[] = {
            px,y1,pz, 0,0, n.x,n.y,n.z, px,y1,pz+cs, 0,1, n.x,n.y,n.z,
            px+cs,0,pz+cs, 1,1, n.x,n.y,n.z, px,y1,pz, 0,0, n.x,n.y,n.z,
            px+cs,0,pz+cs, 1,1, n.x,n.y,n.z, px+cs,0,pz, 1,0, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    } else if (elev == 4) {
        n = Vec3(0, cs, -y1).norm();
        float d[] = {
            px,0,pz, 0,0, n.x,n.y,n.z, px+cs,0,pz, 1,0, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px,0,pz, 0,0, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px,y1,pz+cs, 0,1, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    } else if (elev == 5) {
        n = Vec3(0, cs, y1).norm();
        float d[] = {
            px,y1,pz, 0,0, n.x,n.y,n.z, px+cs,y1,pz, 1,0, n.x,n.y,n.z,
            px+cs,0,pz+cs, 1,1, n.x,n.y,n.z, px,y1,pz, 0,0, n.x,n.y,n.z,
            px+cs,0,pz+cs, 1,1, n.x,n.y,n.z, px,0,pz+cs, 0,1, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    }
}
