#pragma once
#include <cstdint>
#include <random>
#include <vector>
#include "math.h"

extern const float CS, WH;

const float FLOOR2_Y = 2.25f;
const float RAILING_H = 0.9f;

// Elevation values per cell:
// 0 = ground (Y=0), 1 = elevated (Y=FLOOR2_Y)
// 2 = ramp +X, 3 = ramp -X, 4 = ramp +Z, 5 = ramp -Z

inline float getFloorYFromElev(int8_t e) {
    if (e == 1) return FLOOR2_Y;
    return 0.0f;
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

inline bool isRamp(int8_t e) { return e >= 2 && e <= 5; }
inline bool isElevated(int8_t e) { return e == 1; }
inline bool isAboveGround(int8_t e) { return e >= 1 && e <= 5; }

inline void mkRamp(std::vector<float>& v, float px, float pz, int8_t elev, float cs) {
    float y0 = 0.0f, y1 = FLOOR2_Y;
    Vec3 n;
    if (elev == 2) {
        n = Vec3(-FLOOR2_Y, cs, 0).norm();
        float d[] = {
            px,y0,pz, 0,0, n.x,n.y,n.z, px,y0,pz+cs, 0,1, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px,y0,pz, 0,0, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px+cs,y1,pz, 1,0, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    } else if (elev == 3) {
        n = Vec3(FLOOR2_Y, cs, 0).norm();
        float d[] = {
            px,y1,pz, 0,0, n.x,n.y,n.z, px,y1,pz+cs, 0,1, n.x,n.y,n.z,
            px+cs,y0,pz+cs, 1,1, n.x,n.y,n.z, px,y1,pz, 0,0, n.x,n.y,n.z,
            px+cs,y0,pz+cs, 1,1, n.x,n.y,n.z, px+cs,y0,pz, 1,0, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    } else if (elev == 4) {
        n = Vec3(0, cs, -FLOOR2_Y).norm();
        float d[] = {
            px,y0,pz, 0,0, n.x,n.y,n.z, px+cs,y0,pz, 1,0, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px,y0,pz, 0,0, n.x,n.y,n.z,
            px+cs,y1,pz+cs, 1,1, n.x,n.y,n.z, px,y1,pz+cs, 0,1, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    } else if (elev == 5) {
        n = Vec3(0, cs, FLOOR2_Y).norm();
        float d[] = {
            px,y1,pz, 0,0, n.x,n.y,n.z, px+cs,y1,pz, 1,0, n.x,n.y,n.z,
            px+cs,y0,pz+cs, 1,1, n.x,n.y,n.z, px,y1,pz, 0,0, n.x,n.y,n.z,
            px+cs,y0,pz+cs, 1,1, n.x,n.y,n.z, px,y0,pz+cs, 0,1, n.x,n.y,n.z
        };
        v.insert(v.end(), d, d+48);
    }
}
