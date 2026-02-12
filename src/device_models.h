#pragma once
#include <vector>
#include "math.h"

// Device models use a simple UV atlas so different parts can have different materials.
// Vertex format: pos(3), uv(2), normal(3) = 8 floats.

inline void pushQuadAtlas(std::vector<float>& v,
    const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
    const Vec3& n,
    float u0, float v0, float u1, float v1
){
    float vv[] = {
        a.x,a.y,a.z, u0,v0, n.x,n.y,n.z,
        b.x,b.y,b.z, u1,v0, n.x,n.y,n.z,
        c.x,c.y,c.z, u1,v1, n.x,n.y,n.z,
        a.x,a.y,a.z, u0,v0, n.x,n.y,n.z,
        c.x,c.y,c.z, u1,v1, n.x,n.y,n.z,
        d.x,d.y,d.z, u0,v1, n.x,n.y,n.z,
    };
    v.insert(v.end(), vv, vv + 48);
}

inline void mkBoxAtlas(std::vector<float>& v, float cx, float y0, float cz, float sx, float sy, float sz,
    float u0, float v0, float u1, float v1
){
    float hx = sx * 0.5f;
    float hz = sz * 0.5f;
    float y1 = y0 + sy;

    pushQuadAtlas(v,
        Vec3(cx - hx, y0, cz + hz), Vec3(cx + hx, y0, cz + hz),
        Vec3(cx + hx, y1, cz + hz), Vec3(cx - hx, y1, cz + hz),
        Vec3(0,0,1), u0,v0,u1,v1);
    pushQuadAtlas(v,
        Vec3(cx + hx, y0, cz - hz), Vec3(cx - hx, y0, cz - hz),
        Vec3(cx - hx, y1, cz - hz), Vec3(cx + hx, y1, cz - hz),
        Vec3(0,0,-1), u0,v0,u1,v1);
    pushQuadAtlas(v,
        Vec3(cx + hx, y0, cz + hz), Vec3(cx + hx, y0, cz - hz),
        Vec3(cx + hx, y1, cz - hz), Vec3(cx + hx, y1, cz + hz),
        Vec3(1,0,0), u0,v0,u1,v1);
    pushQuadAtlas(v,
        Vec3(cx - hx, y0, cz - hz), Vec3(cx - hx, y0, cz + hz),
        Vec3(cx - hx, y1, cz + hz), Vec3(cx - hx, y1, cz - hz),
        Vec3(-1,0,0), u0,v0,u1,v1);
    pushQuadAtlas(v,
        Vec3(cx - hx, y1, cz + hz), Vec3(cx + hx, y1, cz + hz),
        Vec3(cx + hx, y1, cz - hz), Vec3(cx - hx, y1, cz - hz),
        Vec3(0,1,0), u0,v0,u1,v1);
}

inline void mkClosedBoxAtlas(std::vector<float>& v, float cx, float y0, float cz, float sx, float sy, float sz,
    float u0, float v0, float u1, float v1
){
    mkBoxAtlas(v, cx, y0, cz, sx, sy, sz, u0, v0, u1, v1);
    float hx = sx * 0.5f;
    float hz = sz * 0.5f;
    pushQuadAtlas(v,
        Vec3(cx - hx, y0, cz - hz), Vec3(cx + hx, y0, cz - hz),
        Vec3(cx + hx, y0, cz + hz), Vec3(cx - hx, y0, cz + hz),
        Vec3(0,-1,0), u0,v0,u1,v1);
}

inline void buildFlashlightModel(std::vector<float>& v){
    // Atlas regions (u0,v0,u1,v1):
    // rubber: 0.00..0.50 , metal: 0.50..0.85 , glass/button: 0.85..1.00
    mkClosedBoxAtlas(v, 0.0f, 0.00f, 0.04f, 0.10f, 0.07f, 0.16f, 0.50f,0.00f,0.85f,1.00f); // metal tube
    mkClosedBoxAtlas(v, 0.0f, 0.02f, 0.22f, 0.12f, 0.08f, 0.36f, 0.00f,0.00f,0.50f,1.00f); // rubber body
    mkClosedBoxAtlas(v, 0.0f, 0.03f, 0.44f, 0.18f, 0.10f, 0.20f, 0.50f,0.00f,0.85f,1.00f); // metal head
    mkClosedBoxAtlas(v, 0.0f, 0.05f, 0.54f, 0.14f, 0.08f, 0.04f, 0.85f,0.00f,1.00f,0.60f); // glass lens plate
    mkClosedBoxAtlas(v, 0.0f, 0.09f, 0.16f, 0.05f, 0.02f, 0.07f, 0.85f,0.60f,1.00f,1.00f); // button
}

inline void buildScannerModel(std::vector<float>& v){
    // plastic: 0..0.70, metal: 0.70..0.88, screen: 0.88..1.00
    mkBoxAtlas(v, 0.0f, -0.05f, 0.12f, 0.22f, 0.08f, 0.34f, 0.00f,0.00f,0.70f,1.00f);
    mkBoxAtlas(v, 0.0f, 0.03f, 0.22f, 0.18f, 0.10f, 0.18f, 0.88f,0.00f,1.00f,0.60f); // screen
    mkBoxAtlas(v, 0.07f, 0.12f, 0.34f, 0.03f, 0.20f, 0.03f, 0.70f,0.00f,0.88f,1.00f); // antenna metal
    mkBoxAtlas(v, -0.09f, -0.02f, 0.10f, 0.06f, 0.16f, 0.12f, 0.00f,0.00f,0.70f,1.00f);
}

inline void buildPlushToyModel(std::vector<float>& v){
    // fur: 0..0.80, face/eyes: 0.80..1.00
    mkBoxAtlas(v, 0.0f, -0.06f, 0.0f, 0.18f, 0.14f, 0.14f, 0.00f,0.00f,0.80f,1.00f);
    mkBoxAtlas(v, 0.0f, 0.08f, 0.02f, 0.14f, 0.14f, 0.14f, 0.00f,0.00f,0.80f,1.00f);
    mkBoxAtlas(v, -0.08f, 0.18f, 0.02f, 0.06f, 0.06f, 0.06f, 0.00f,0.00f,0.80f,1.00f);
    mkBoxAtlas(v, 0.08f, 0.18f, 0.02f, 0.06f, 0.06f, 0.06f, 0.00f,0.00f,0.80f,1.00f);
    mkBoxAtlas(v, -0.11f, 0.02f, 0.00f, 0.06f, 0.08f, 0.06f, 0.00f,0.00f,0.80f,1.00f);
    mkBoxAtlas(v, 0.11f, 0.02f, 0.00f, 0.06f, 0.08f, 0.06f, 0.00f,0.00f,0.80f,1.00f);
    // face patch
    mkBoxAtlas(v, 0.0f, 0.10f, 0.10f, 0.08f, 0.06f, 0.04f, 0.80f,0.00f,1.00f,1.00f);
}

inline void buildBatteryModel(std::vector<float>& v){
    // label: 0..0.82, metal top: 0.82..1.00
    mkBoxAtlas(v, 0.0f, -0.06f, 0.0f, 0.10f, 0.16f, 0.10f, 0.00f,0.00f,0.82f,1.00f);
    mkBoxAtlas(v, 0.0f, 0.10f, 0.0f, 0.06f, 0.05f, 0.06f, 0.82f,0.00f,1.00f,0.70f);
}

inline void buildMedSprayModel(std::vector<float>& v){
    // body: 0..0.75, cap/nozzle: 0.75..1.00
    mkClosedBoxAtlas(v, 0.0f, -0.10f, 0.0f, 0.12f, 0.26f, 0.12f, 0.00f,0.00f,0.75f,1.00f);
    mkClosedBoxAtlas(v, 0.0f, 0.16f, 0.0f, 0.10f, 0.06f, 0.10f, 0.75f,0.00f,1.00f,0.55f);
    mkClosedBoxAtlas(v, 0.0f, 0.22f, 0.03f, 0.06f, 0.05f, 0.12f, 0.75f,0.55f,1.00f,1.00f);
    mkClosedBoxAtlas(v, 0.0f, -0.02f, 0.07f, 0.06f, 0.10f, 0.03f, 0.75f,0.00f,1.00f,0.55f);
}
