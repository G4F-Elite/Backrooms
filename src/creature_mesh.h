#pragma once
// Low-poly faceted mesh builder for creatures and player bodies.
// Geometry is pure data (no GL) and split into jointed parts so the renderer can pose limbs.
// Vertex layout matches the main shader: pos3, uv2, normal3.
#include <vector>
#include <cmath>
#include "math.h"

enum CreatureJoint {
    CJ_ROOT = 0,
    CJ_SPINE,
    CJ_HEAD,
    CJ_ARM_L,
    CJ_FOREARM_L,
    CJ_ARM_R,
    CJ_FOREARM_R,
    CJ_LEG_L,
    CJ_SHIN_L,
    CJ_LEG_R,
    CJ_SHIN_R,
    CJ_COUNT
};

struct CreaturePart {
    int first;   // first vertex
    int count;   // vertex count
    int parent;  // parent part index, -1 for the root
    int joint;   // CreatureJoint driving this part
    Vec3 pivot;  // rotation pivot in model space
};

struct CreaturePose {
    Vec3 rot[CJ_COUNT]; // euler radians per joint, applied as Rz * Ry * Rx around the pivot
    Vec3 rootOffset;    // model-space translation of the whole body
};

struct CreatureRing {
    Vec3 c;
    float rx, rz;
};

inline const int CREATURE_STRIDE = 8;

// Exact length/normalize: model building must not depend on the fast-math toggle.
inline float creatureLen(const Vec3& a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
inline Vec3 creatureNorm(const Vec3& a) {
    float l = creatureLen(a);
    return l > 1e-12f ? a * (1.0f / l) : Vec3(0, 1, 0);
}
inline const int CREATURE_MAX_PARTS = 16;

struct CreatureMesh {
    std::vector<float> v;
    std::vector<CreaturePart> parts;
    int strips = 4;       // texture is split into vertical material strips
    int strip = 0;        // strip used by the next primitives
    float vScale = 1.0f;  // texture repeats per meter along a tube

    int vertexCount() const { return (int)v.size() / CREATURE_STRIDE; }

    int beginPart(int joint, int parent, const Vec3& pivot) {
        closePart();
        CreaturePart p;
        p.first = vertexCount();
        p.count = 0;
        p.parent = parent;
        p.joint = joint;
        p.pivot = pivot;
        parts.push_back(p);
        return (int)parts.size() - 1;
    }

    void closePart() {
        if (parts.empty()) return;
        CreaturePart& p = parts.back();
        p.count = vertexCount() - p.first;
    }

    void finish() { closePart(); }

    float stripU(float t) const {
        // Mirrored wrap keeps a tube seamless without crossing into the neighbour strip.
        float m = 1.0f - fabsf(2.0f * t - 1.0f);
        return ((float)strip + 0.1f + 0.8f * m) / (float)strips;
    }

    void pushVertex(const Vec3& p, float u, float vv, const Vec3& n) {
        v.push_back(p.x); v.push_back(p.y); v.push_back(p.z);
        v.push_back(u); v.push_back(vv);
        v.push_back(n.x); v.push_back(n.y); v.push_back(n.z);
    }

    // Flat-shaded triangle wound so its face points away from `inside`.
    void tri(Vec3 a, Vec3 b, Vec3 c, float ua, float va, float ub, float vb, float uc, float vc, const Vec3& inside) {
        Vec3 n = (b - a).cross(c - a);
        float len = creatureLen(n);
        if (len < 1e-9f) return;
        n = n * (1.0f / len);
        Vec3 center = (a + b + c) * (1.0f / 3.0f);
        if (n.dot(center - inside) < 0.0f) {
            Vec3 tp = b; b = c; c = tp;
            float tu = ub; ub = uc; uc = tu;
            float tv = vb; vb = vc; vc = tv;
            n = n * -1.0f;
        }
        pushVertex(a, ua, va, n);
        pushVertex(b, ub, vb, n);
        pushVertex(c, uc, vc, n);
    }

    // Faceted tube through the rings. rx spreads along `wide`, rz along the perpendicular.
    void tube(const CreatureRing* rings, int n, int sides, bool capA = true, bool capB = true,
              Vec3 wide = Vec3(1, 0, 0), float phase = 0.0f) {
        if (n < 2 || sides < 3) return;
        std::vector<Vec3> pts((size_t)n * (size_t)sides);
        std::vector<float> vs((size_t)n);
        std::vector<Vec3> tans((size_t)n);
        float run = 0.0f;
        for (int i = 0; i < n; i++) {
            const Vec3& prev = rings[i > 0 ? i - 1 : i].c;
            const Vec3& next = rings[i < n - 1 ? i + 1 : i].c;
            Vec3 t = creatureNorm(next - prev);
            Vec3 s = wide - t * wide.dot(t);
            if (creatureLen(s) < 0.05f) s = Vec3(0, 0, 1) - t * t.z;
            s = creatureNorm(s);
            Vec3 f = t.cross(s);
            if (i > 0) run += creatureLen(rings[i].c - rings[i - 1].c);
            vs[i] = run * vScale;
            tans[i] = t;
            for (int k = 0; k < sides; k++) {
                float a = phase + 6.2831853f * (float)k / (float)sides;
                pts[(size_t)i * sides + k] = rings[i].c + s * (rings[i].rx * cosf(a)) + f * (rings[i].rz * sinf(a));
            }
        }
        for (int i = 0; i < n - 1; i++) {
            Vec3 inside = (rings[i].c + rings[i + 1].c) * 0.5f;
            for (int k = 0; k < sides; k++) {
                int k2 = (k + 1) % sides;
                float u0 = stripU((float)k / (float)sides);
                float u1 = stripU((float)(k + 1) / (float)sides);
                const Vec3& a = pts[(size_t)i * sides + k];
                const Vec3& b = pts[(size_t)i * sides + k2];
                const Vec3& c = pts[(size_t)(i + 1) * sides + k2];
                const Vec3& d = pts[(size_t)(i + 1) * sides + k];
                tri(a, b, c, u0, vs[i], u1, vs[i], u1, vs[i + 1], inside);
                tri(a, c, d, u0, vs[i], u1, vs[i + 1], u0, vs[i + 1], inside);
            }
        }
        for (int e = 0; e < 2; e++) {
            if ((e == 0 && !capA) || (e == 1 && !capB)) continue;
            int i = (e == 0) ? 0 : n - 1;
            float r = rings[i].rx < rings[i].rz ? rings[i].rx : rings[i].rz;
            Vec3 tip = rings[i].c + tans[i] * ((e == 0 ? -0.45f : 0.45f) * r);
            float vt = vs[i] + (e == 0 ? -0.45f : 0.45f) * r * vScale;
            for (int k = 0; k < sides; k++) {
                int k2 = (k + 1) % sides;
                float u0 = stripU((float)k / (float)sides);
                float u1 = stripU((float)(k + 1) / (float)sides);
                tri(pts[(size_t)i * sides + k], pts[(size_t)i * sides + k2], tip,
                    u0, vs[i], u1, vs[i], (u0 + u1) * 0.5f, vt, rings[i].c);
            }
        }
    }

    // Straight tapered segment.
    void limb(const Vec3& a, const Vec3& b, float ra, float rb, int sides = 6) {
        CreatureRing r[2] = { {a, ra, ra}, {b, rb, rb} };
        tube(r, 2, sides);
    }

    // Two-bone limb with a slightly thicker joint.
    void limb3(const Vec3& a, const Vec3& b, const Vec3& c, float ra, float rb, float rc, int sides = 6) {
        CreatureRing r[3] = { {a, ra, ra}, {b, rb, rb}, {c, rc, rc} };
        tube(r, 3, sides);
    }

    // Faceted egg between two poles; rx is the lateral radius, rz the front/back one.
    void ovoid(const Vec3& bottom, const Vec3& top, float rx, float rz, int sides = 7, float bulge = 0.56f) {
        const float ts[5] = { 0.0f, 0.2f, bulge, 0.86f, 1.0f };
        CreatureRing r[5];
        for (int i = 0; i < 5; i++) {
            // Profile peaks at `bulge` and closes down to blunt poles.
            float d = (ts[i] < bulge) ? (bulge - ts[i]) / bulge : (ts[i] - bulge) / (1.0f - bulge);
            float w = sqrtf(1.0f - 0.82f * d * d);
            r[i].c = bottom + (top - bottom) * ts[i];
            r[i].rx = rx * w;
            r[i].rz = rz * w;
        }
        tube(r, 5, sides);
    }

    // Axis-aligned box (hard edged props: shoes, flashlight).
    void box(const Vec3& c, float sx, float sy, float sz) {
        const float k = 0.70710678f;
        CreatureRing r[2] = { {Vec3(c.x, c.y - sy * 0.5f, c.z), sx * k, sz * k}, {Vec3(c.x, c.y + sy * 0.5f, c.z), sx * k, sz * k} };
        tube(r, 2, 4, false, false, Vec3(1, 0, 0), 0.78539816f);
        Vec3 lo(c.x, c.y - sy * 0.5f, c.z), hi(c.x, c.y + sy * 0.5f, c.z);
        float hx = sx * 0.5f, hz = sz * 0.5f;
        float u0 = stripU(0.0f), u1 = stripU(0.5f);
        for (int e = 0; e < 2; e++) {
            Vec3 m = e == 0 ? lo : hi;
            Vec3 a(m.x - hx, m.y, m.z - hz), b(m.x + hx, m.y, m.z - hz), d(m.x - hx, m.y, m.z + hz), cc(m.x + hx, m.y, m.z + hz);
            tri(a, b, cc, u0, 0, u1, 0, u1, sz * vScale, c);
            tri(a, cc, d, u0, 0, u1, sz * vScale, u0, sz * vScale, c);
        }
    }
};

// ---- Posing ----------------------------------------------------------------
// Column-major affine product in math order: result = a * b (b is applied first).
inline Mat4 creatureMul(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a.m[k * 4 + row] * b.m[col * 4 + k];
            r.m[col * 4 + row] = s;
        }
    }
    return r;
}

// Rotation Rz * Ry * Rx around `pivot`.
inline Mat4 creatureJointMatrix(const Vec3& pivot, const Vec3& rot) {
    Mat4 rx, ry, rz;
    float c = cosf(rot.x), s = sinf(rot.x);
    rx.m[5] = c; rx.m[6] = s; rx.m[9] = -s; rx.m[10] = c;
    c = cosf(rot.y); s = sinf(rot.y);
    ry.m[0] = c; ry.m[2] = -s; ry.m[8] = s; ry.m[10] = c;
    c = cosf(rot.z); s = sinf(rot.z);
    rz.m[0] = c; rz.m[1] = s; rz.m[4] = -s; rz.m[5] = c;
    Mat4 r = creatureMul(rz, creatureMul(ry, rx));
    Mat4 out = creatureMul(Mat4::translate(pivot.x, pivot.y, pivot.z), creatureMul(r, Mat4::translate(-pivot.x, -pivot.y, -pivot.z)));
    return out;
}

// World matrix per part. Parents always precede their children in `parts`.
inline void creaturePartMatrices(const std::vector<CreaturePart>& parts, const CreaturePose& pose,
                                 const Mat4& world, Mat4* out) {
    int n = (int)parts.size();
    if (n > CREATURE_MAX_PARTS) n = CREATURE_MAX_PARTS;
    for (int i = 0; i < n; i++) {
        const CreaturePart& p = parts[i];
        Mat4 local = creatureJointMatrix(p.pivot, pose.rot[p.joint]);
        if (p.parent < 0 || p.parent >= i) {
            Mat4 base = creatureMul(world, Mat4::translate(pose.rootOffset.x, pose.rootOffset.y, pose.rootOffset.z));
            out[i] = creatureMul(base, local);
        } else {
            out[i] = creatureMul(out[p.parent], local);
        }
    }
}

// Rotation around Y that turns model +Z towards (sin yaw, cos yaw), placed at `pos`.
inline Mat4 creatureWorldMatrix(const Vec3& pos, float yaw) {
    Mat4 m;
    float c = cosf(yaw), s = sinf(yaw);
    m.m[0] = c; m.m[2] = -s; m.m[8] = s; m.m[10] = c;
    m.m[12] = pos.x; m.m[13] = pos.y; m.m[14] = pos.z;
    return m;
}

// Stateless walk-cycle phase: distance travelled along the facing direction.
// Works for network clients too, where entities are rebuilt from snapshots.
inline float creatureGaitPhase(const Vec3& pos, float yaw, float stride) {
    float along = pos.x * sinf(yaw) + pos.z * cosf(yaw);
    return along * 3.14159265f / stride;
}
