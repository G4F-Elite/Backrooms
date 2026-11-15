#pragma once
#include <vector>
#include <cmath>
#include "math.h"

// ============================================================================
// DECOR MESH BUILDER
// Structures are assembled from parts in LOCAL metres: +X = right, +Z = front,
// Y = up from the floor. The builder applies yaw + uniform scale and emits
// pos(3) uv(2) normal(3) vertices mapped into one tile of the decor atlas.
// Every part is a closed solid, so nothing looks hollow from below or behind.
// ============================================================================

enum DecorMat {
    DECOR_MAT_WOOD = 0,
    DECOR_MAT_CARDBOARD = 1,
    DECOR_MAT_METAL = 2,
    DECOR_MAT_DARK_METAL = 3,
    DECOR_MAT_ORANGE = 4,
    DECOR_MAT_PLASTER = 5,
    DECOR_MAT_RUBBER = 6,
    DECOR_MAT_WHITE = 7,
    DECOR_MAT_DRUM = 8,
    DECOR_MAT_FABRIC = 9,
    DECOR_MAT_SCREEN = 10,
    DECOR_MAT_CAUTION = 11,
    DECOR_MAT_PUDDLE = 12,
    DECOR_MAT_CEILTILE = 13,
    DECOR_MAT_DOOR = 14,
    DECOR_MAT_TRIM = 15
};

const int DECOR_ATLAS_GRID = 4;
const float DECOR_TEXEL_METERS = 1.6f;  // one tile covers this many metres on a face

struct DecorUv {
    float u0, v0, du, dv;
};

inline DecorUv decorTileUv(int mat) {
    const float inset = 0.035f;
    float cell = 1.0f / (float)DECOR_ATLAS_GRID;
    DecorUv t;
    t.u0 = ((float)(mat % DECOR_ATLAS_GRID) + inset) * cell;
    t.v0 = ((float)(mat / DECOR_ATLAS_GRID) + inset) * cell;
    t.du = (1.0f - inset * 2.0f) * cell;
    t.dv = t.du;
    return t;
}

struct DecorBuilder {
    std::vector<float>& v;
    float ox, oz;
    float cs, sn;
    float scale;
    bool fitTile;

    DecorBuilder(std::vector<float>& out, float x, float z, float yaw, float s)
        : v(out), ox(x), oz(z), cs(cosf(yaw)), sn(sinf(yaw)), scale(s), fitTile(false) {}

    Vec3 world(float lx, float y, float lz) const {
        float sx = lx * scale;
        float sz = lz * scale;
        return Vec3(ox + sx * cs + sz * sn, y * scale, oz - sx * sn + sz * cs);
    }

    Vec3 worldDir(float lx, float y, float lz) const {
        return Vec3(lx * cs + lz * sn, y, -lx * sn + lz * cs);
    }

    // Builder for a sub-assembly placed at a local offset with its own extra yaw.
    DecorBuilder sub(float lx, float lz, float yawAdd) const {
        Vec3 o = world(lx, 0.0f, lz);
        float yaw = atan2f(sn, cs) + yawAdd;
        return DecorBuilder(v, o.x, o.z, yaw, scale);
    }

    void vertex(const Vec3& p, float tu, float tv, const Vec3& n, const DecorUv& t) {
        float vv[] = {p.x, p.y, p.z, t.u0 + tu * t.du, t.v0 + tv * t.dv, n.x, n.y, n.z};
        v.insert(v.end(), vv, vv + 8);
    }

    // Quad a-b-c-d (a-b = U direction, a-d = V direction), all in local space.
    void quad(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& nLocal, int mat) {
        DecorUv t = decorTileUv(mat);
        float uSpan = 1.0f;
        float vSpan = 1.0f;
        if (!fitTile) {
            uSpan = (b - a).len() * scale / DECOR_TEXEL_METERS;
            vSpan = (d - a).len() * scale / DECOR_TEXEL_METERS;
            if (uSpan > 1.0f) uSpan = 1.0f;
            if (vSpan > 1.0f) vSpan = 1.0f;
            if (uSpan < 0.12f) uSpan = 0.12f;
            if (vSpan < 0.12f) vSpan = 0.12f;
        }
        Vec3 n = worldDir(nLocal.x, nLocal.y, nLocal.z);
        Vec3 wa = world(a.x, a.y, a.z);
        Vec3 wb = world(b.x, b.y, b.z);
        Vec3 wc = world(c.x, c.y, c.z);
        Vec3 wd = world(d.x, d.y, d.z);
        vertex(wa, 0.0f, 0.0f, n, t);
        vertex(wb, uSpan, 0.0f, n, t);
        vertex(wc, uSpan, vSpan, n, t);
        vertex(wa, 0.0f, 0.0f, n, t);
        vertex(wc, uSpan, vSpan, n, t);
        vertex(wd, 0.0f, vSpan, n, t);
    }

    // Tapered box: sizes at the bottom (bx, bz) and at the top (tx, tz). 6 faces.
    void taper(float cx, float y0, float cz, float bx, float bz, float tx, float tz, float h, int mat) {
        float y1 = y0 + h;
        float hbx = bx * 0.5f, hbz = bz * 0.5f, htx = tx * 0.5f, htz = tz * 0.5f;
        Vec3 b00(cx - hbx, y0, cz - hbz), b10(cx + hbx, y0, cz - hbz);
        Vec3 b11(cx + hbx, y0, cz + hbz), b01(cx - hbx, y0, cz + hbz);
        Vec3 t00(cx - htx, y1, cz - htz), t10(cx + htx, y1, cz - htz);
        Vec3 t11(cx + htx, y1, cz + htz), t01(cx - htx, y1, cz + htz);
        float slopeX = (hbx - htx) / (h > 0.0001f ? h : 0.0001f);
        float slopeZ = (hbz - htz) / (h > 0.0001f ? h : 0.0001f);
        quad(b01, b11, t11, t01, Vec3(0, slopeZ, 1).norm(), mat);
        quad(b10, b00, t00, t10, Vec3(0, slopeZ, -1).norm(), mat);
        quad(b11, b10, t10, t11, Vec3(1, slopeX, 0).norm(), mat);
        quad(b00, b01, t01, t00, Vec3(-1, slopeX, 0).norm(), mat);
        quad(t01, t11, t10, t00, Vec3(0, 1, 0), mat);
        quad(b00, b10, b11, b01, Vec3(0, -1, 0), mat);
    }

    // Axis-aligned (in local space) closed box: centre (cx, cz), bottom y0, size (sx, sy, sz).
    void box(float cx, float y0, float cz, float sx, float sy, float sz, int mat) {
        taper(cx, y0, cz, sx, sz, sx, sz, sy, mat);
    }

    // Like box(), but every face shows the whole tile once (doors, signs, screens).
    void panel(float cx, float y0, float cz, float sx, float sy, float sz, int mat) {
        bool prev = fitTile;
        fitTile = true;
        box(cx, y0, cz, sx, sy, sz, mat);
        fitTile = prev;
    }

    // Vertical closed cylinder (prism) with `sides` faces.
    void cylinder(float cx, float y0, float cz, float r, float h, int mat, int sides = 10) {
        const float tau = 6.2831853f;
        float y1 = y0 + h;
        for (int i = 0; i < sides; i++) {
            float a0 = tau * (float)i / (float)sides;
            float a1 = tau * (float)(i + 1) / (float)sides;
            float am = (a0 + a1) * 0.5f;
            Vec3 p0(cx + sinf(a0) * r, y0, cz + cosf(a0) * r);
            Vec3 p1(cx + sinf(a1) * r, y0, cz + cosf(a1) * r);
            Vec3 q0(p0.x, y1, p0.z);
            Vec3 q1(p1.x, y1, p1.z);
            quad(p0, p1, q1, q0, Vec3(sinf(am), 0, cosf(am)), mat);
            Vec3 top(cx, y1, cz);
            Vec3 bottom(cx, y0, cz);
            quad(q0, q1, top, top, Vec3(0, 1, 0), mat);
            quad(p1, p0, bottom, bottom, Vec3(0, -1, 0), mat);
        }
    }

    // Flat floor decal (single quad, slightly above the floor).
    void decal(float cx, float y, float cz, float sx, float sz, int mat) {
        float hx = sx * 0.5f, hz = sz * 0.5f;
        bool prev = fitTile;
        fitTile = true;
        quad(Vec3(cx - hx, y, cz - hz), Vec3(cx - hx, y, cz + hz),
             Vec3(cx + hx, y, cz + hz), Vec3(cx + hx, y, cz - hz), Vec3(0, 1, 0), mat);
        fitTile = prev;
    }
};
