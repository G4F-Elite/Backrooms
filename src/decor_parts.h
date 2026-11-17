#pragma once
#include "decor_builder.h"

// ============================================================================
// DECOR PARTS
// Pieces the box/cylinder builder cannot make on its own: bars between two
// arbitrary points (slanted legs, horizontal pipes, wheels on an axle), lathe
// shapes (cones, balls) and a few sub-assemblies shared by several prop files.
// All coordinates are LOCAL metres of the given builder.
// ============================================================================

// Quad whose winding is fixed up to face `outward`.
inline void decorFace(DecorBuilder& b, const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3,
                      const Vec3& outward, int mat) {
    Vec3 n = (p1 - p0).cross(p2 - p0);
    if (n.dot(outward) < 0.0f) b.quad(p0, p3, p2, p1, outward, mat);
    else b.quad(p0, p1, p2, p3, outward, mat);
}

// Two unit vectors perpendicular to `axis` (side is always horizontal).
inline void decorAxisFrame(const Vec3& axis, Vec3& side, Vec3& up) {
    if (fabsf(axis.y) > 0.99f) side = Vec3(1.0f, 0.0f, 0.0f);
    else side = axis.cross(Vec3(0.0f, 1.0f, 0.0f)).norm();
    up = side.cross(axis).norm();
}

// Rectangular bar from p0 to p1: `w` across (horizontal), `t` thick.
// flatEnds cuts both ends horizontally, so a slanted leg stands flat on the floor.
inline void decorBeam(DecorBuilder& b, const Vec3& p0, const Vec3& p1, float w, float t, int mat, bool flatEnds = false) {
    Vec3 axis = (p1 - p0).norm();
    Vec3 side, up;
    decorAxisFrame(axis, side, up);
    Vec3 sx = side * (w * 0.5f);
    Vec3 su = up * (t * 0.5f);
    Vec3 capN = axis;
    if (flatEnds && fabsf(axis.y) < 0.999f) {
        Vec3 flat = Vec3(axis.x, 0.0f, axis.z).norm();
        if (flat.dot(up) < 0.0f) flat = -flat;
        su = flat * (t * 0.5f);
        capN = Vec3(0.0f, axis.y >= 0.0f ? 1.0f : -1.0f, 0.0f);
    }
    Vec3 a[4] = {p0 - sx - su, p0 + sx - su, p0 + sx + su, p0 - sx + su};
    Vec3 c[4] = {p1 - sx - su, p1 + sx - su, p1 + sx + su, p1 - sx + su};
    decorFace(b, a[1], a[2], c[2], c[1], side, mat);
    decorFace(b, a[0], a[3], c[3], c[0], -side, mat);
    decorFace(b, a[3], a[2], c[2], c[3], up, mat);
    decorFace(b, a[0], a[1], c[1], c[0], -up, mat);
    decorFace(b, c[0], c[1], c[2], c[3], capN, mat);
    decorFace(b, a[0], a[1], a[2], a[3], -capN, mat);
}

// Closed round bar from p0 to p1 (horizontal pipes, axles, wheels, valve wheels).
inline void decorRod(DecorBuilder& b, const Vec3& p0, const Vec3& p1, float r, int mat, int sides = 8) {
    const float tau = 6.2831853f;
    Vec3 axis = (p1 - p0).norm();
    Vec3 side, up;
    decorAxisFrame(axis, side, up);
    for (int i = 0; i < sides; i++) {
        float a0 = tau * (float)i / (float)sides;
        float a1 = tau * (float)(i + 1) / (float)sides;
        float am = (a0 + a1) * 0.5f;
        Vec3 d0 = side * (cosf(a0) * r) + up * (sinf(a0) * r);
        Vec3 d1 = side * (cosf(a1) * r) + up * (sinf(a1) * r);
        Vec3 n = side * cosf(am) + up * sinf(am);
        decorFace(b, p0 + d0, p0 + d1, p1 + d1, p1 + d0, n, mat);
        decorFace(b, p1 + d0, p1 + d1, p1, p1, axis, mat);
        decorFace(b, p0 + d1, p0 + d0, p0, p0, -axis, mat);
    }
}

// One band of a vertical lathe: radius r0 at y0, r1 at y0 + h. Caps are optional so
// bands can be stacked into one closed shape.
inline void decorBand(DecorBuilder& b, float cx, float y0, float cz, float r0, float r1, float h, int mat,
                      int sides, bool capBottom, bool capTop) {
    const float tau = 6.2831853f;
    float y1 = y0 + h;
    for (int i = 0; i < sides; i++) {
        float a0 = tau * (float)i / (float)sides;
        float a1 = tau * (float)(i + 1) / (float)sides;
        float am = (a0 + a1) * 0.5f;
        Vec3 p0(cx + sinf(a0) * r0, y0, cz + cosf(a0) * r0);
        Vec3 p1(cx + sinf(a1) * r0, y0, cz + cosf(a1) * r0);
        Vec3 q0(cx + sinf(a0) * r1, y1, cz + cosf(a0) * r1);
        Vec3 q1(cx + sinf(a1) * r1, y1, cz + cosf(a1) * r1);
        b.quad(p0, p1, q1, q0, Vec3(sinf(am) * h, r0 - r1, cosf(am) * h).norm(), mat);
        Vec3 top(cx, y1, cz);
        Vec3 bottom(cx, y0, cz);
        if (capTop) b.quad(q0, q1, top, top, Vec3(0, 1, 0), mat);
        if (capBottom) b.quad(p1, p0, bottom, bottom, Vec3(0, -1, 0), mat);
    }
}

// Closed vertical frustum: plant pots, lamp shades, funnels, party hats (r1 = 0).
inline void decorCone(DecorBuilder& b, float cx, float y0, float cz, float r0, float r1, float h, int mat, int sides = 8) {
    decorBand(b, cx, y0, cz, r0, r1, h, mat, sides, true, true);
}

// Faceted ball centred at (cx, cy, cz): balloons, heads, play balls.
inline void decorBall(DecorBuilder& b, float cx, float cy, float cz, float r, int mat, int sides = 8) {
    // small balls get three bands instead of four
    const float fine[5] = {-1.0f, -0.62f, 0.0f, 0.62f, 1.0f};
    const float coarse[5] = {-1.0f, -0.45f, 0.45f, 1.0f, 1.0f};
    const float* lat = (sides > 6) ? fine : coarse;
    int bands = (sides > 6) ? 4 : 3;
    for (int i = 0; i < bands; i++) {
        float r0 = r * sqrtf(1.0f - lat[i] * lat[i]);
        float r1 = r * sqrtf(1.0f - lat[i + 1] * lat[i + 1]);
        decorBand(b, cx, cy + lat[i] * r, cz, r0, r1, (lat[i + 1] - lat[i]) * r, mat, sides, false, false);
    }
}

// Rubber wheel on a left-right axle, centre at (x, y, z). Small casters get no hub.
inline void decorWheel(DecorBuilder& b, float x, float y, float z, float r, float w) {
    float h = w * 0.5f;
    decorRod(b, Vec3(x - h, y, z), Vec3(x + h, y, z), r, DECOR_MAT_RUBBER, r > 0.2f ? 12 : 8);
    if (r < 0.09f) return;
    decorRod(b, Vec3(x - h - 0.005f, y, z), Vec3(x + h + 0.005f, y, z), r * 0.4f, DECOR_MAT_METAL, 6);
}

// Empty shipping pallet, 1.00 x 0.12 x 0.80, bottom at y.
inline void decorPallet(DecorBuilder& b, float cx, float y, float cz) {
    for (int i = -1; i <= 1; i++) {
        b.box(cx + 0.42f * (float)i, y, cz, 0.10f, 0.09f, 0.80f, DECOR_MAT_WOOD);
    }
    for (int i = -2; i <= 2; i++) {
        b.box(cx, y + 0.09f, cz + 0.17f * (float)i, 1.00f, 0.03f, 0.12f, DECOR_MAT_WOOD);
    }
}

const float DECOR_PALLET_H = 0.12f;
