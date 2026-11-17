#pragma once
#include <cmath>
#include <utility>
#include <vector>

// ============================================================================
// Z-FIGHT GEOMETRY
// Narrow phase of the z-fight guard (see zfight_guard.h): does one triangle lie
// in the plane of another and cover part of it, and if so, cut that part out.
// Everything is done in the 2D frame of the triangle that loses the spot.
// ============================================================================

struct ZfConfig {
    // Faces closer than this end up with the same depth value at range: 24 bit depth with
    // the near plane at 0.1 m resolves about 1 mm at 40 m. Content is authored in whole
    // millimetres, so the limit sits between two steps: a 1 mm step counts as the same
    // plane, a 2 mm step is left to the depth buffer.
    double planeEps = 0.0015;
    // Smaller overlaps are rounding noise where two parts merely touch.
    double minArea = 1.0e-5;
    double minWidth = 5.0e-4;
    // Repairing cuts overlaps down to this share of the two limits above, so that a mesh
    // that went through zfightResolve() also passes zfightScan() after rounding to floats.
    double repairMargin = 0.5;
    // Broad phase grid, metres: big cells across a face, thin slabs along its normal.
    double cell = 8.0;
    double slab = 0.05;
    int maxSamples = 24;
    // Worker threads for big meshes (the result is the same for any number).
    int threads = 1;
};

struct ZfTri {
    const float* v;  // first vertex in its mesh buffer
    float n[3];      // unit normal, pointing to the side the face is lit from
    float lo[3], hi[3];
    int mesh, index, stride;
    int mate;        // other half of the same quad, or -1
    unsigned char axes;
};

struct ZfPt {
    double x, y, h;
};

struct ZfPoly {
    ZfPt p[16];
    int n = 0;

    static bool same(const ZfPt& a, const ZfPt& b) {
        return fabs(a.x - b.x) < 1.0e-9 && fabs(a.y - b.y) < 1.0e-9;
    }

    // A corner lying exactly on a clip line would otherwise come out twice.
    void add(const ZfPt& q) {
        if (n > 0 && same(p[n - 1], q)) return;
        if (n < 16) p[n++] = q;
    }

    void close() {
        while (n > 1 && same(p[n - 1], p[0])) n--;
    }
};

// 2D frame of a triangle: origin at its first vertex, u along the first edge.
struct ZfFrame {
    double o[3], u[3], w[3], g[3];
    ZfPt a[3];
    bool ok = false;
};

inline double zfDot(const double* a, const double* b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

inline void zfCross(const double* a, const double* b, double* out) {
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

inline void zfVertex(const ZfTri& t, int i, double* out) {
    const float* q = t.v + i * t.stride;
    out[0] = q[0];
    out[1] = q[1];
    out[2] = q[2];
}

inline ZfPt zfProject(const ZfFrame& f, const double* p) {
    double d[3] = {p[0] - f.o[0], p[1] - f.o[1], p[2] - f.o[2]};
    return {zfDot(d, f.u), zfDot(d, f.w), zfDot(d, f.g)};
}

inline ZfFrame zfFrameOf(const ZfTri& t) {
    ZfFrame f;
    double p1[3], p2[3], e1[3], e2[3];
    zfVertex(t, 0, f.o);
    zfVertex(t, 1, p1);
    zfVertex(t, 2, p2);
    for (int k = 0; k < 3; k++) {
        e1[k] = p1[k] - f.o[k];
        e2[k] = p2[k] - f.o[k];
    }
    zfCross(e1, e2, f.g);
    double gl = sqrt(zfDot(f.g, f.g)), ul = sqrt(zfDot(e1, e1));
    if (gl < 1e-12 || ul < 1e-9) return f;
    for (int k = 0; k < 3; k++) {
        f.g[k] /= gl;
        f.u[k] = e1[k] / ul;
    }
    zfCross(f.g, f.u, f.w);
    f.a[0] = {0.0, 0.0, 0.0};
    f.a[1] = zfProject(f, p1);
    f.a[2] = zfProject(f, p2);
    f.ok = true;
    return f;
}

// Keeps the part of a convex polygon where ax*x + ay*y + ah*h + d >= 0.
inline void zfClip(const ZfPoly& in, double ax, double ay, double ah, double d, ZfPoly& out) {
    out.n = 0;
    for (int i = 0; i < in.n; i++) {
        const ZfPt& a = in.p[i];
        const ZfPt& b = in.p[(i + 1) % in.n];
        double sa = ax * a.x + ay * a.y + ah * a.h + d;
        double sb = ax * b.x + ay * b.y + ah * b.h + d;
        if (sa >= 0.0) out.add(a);
        if ((sa >= 0.0) != (sb >= 0.0)) {
            double t = sa / (sa - sb);
            out.add({a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.h + (b.h - a.h) * t});
        }
    }
    out.close();
}

inline double zfSignedArea(const ZfPoly& p) {
    double s = 0.0;
    for (int i = 0; i < p.n; i++) {
        const ZfPt& a = p.p[i];
        const ZfPt& b = p.p[(i + 1) % p.n];
        s += a.x * b.y - b.x * a.y;
    }
    return s * 0.5;
}

inline double zfPerimeter(const ZfPoly& p) {
    double s = 0.0;
    for (int i = 0; i < p.n; i++) {
        const ZfPt& a = p.p[i];
        const ZfPt& b = p.p[(i + 1) % p.n];
        s += sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
    }
    return s;
}

// Half-plane to the left of the edge a -> b (the inside of a counter-clockwise polygon).
inline void zfEdge(const ZfPt& a, const ZfPt& b, double& ax, double& ay, double& d) {
    ax = -(b.y - a.y);
    ay = b.x - a.x;
    d = -(ax * a.x + ay * a.y);
}

// Vertex coordinates are floats, so a clipped polygon can come out with corners a few
// hundredths of a millimetre apart or bent the wrong way. Cutting needs a clean convex
// counter-clockwise outline: near corners are merged, reflex ones dropped.
inline void zfTidy(ZfPoly& poly) {
    const double snap = 1.0e-4;
    auto together = [&](const ZfPt& a, const ZfPt& b) {
        return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) < snap * snap;
    };
    ZfPoly o;
    for (int i = 0; i < poly.n; i++) {
        if (o.n > 0 && together(o.p[o.n - 1], poly.p[i])) continue;
        o.p[o.n++] = poly.p[i];
    }
    while (o.n > 1 && together(o.p[o.n - 1], o.p[0])) o.n--;
    for (bool changed = true; changed && o.n >= 3;) {
        changed = false;
        for (int i = 0; i < o.n; i++) {
            const ZfPt& a = o.p[(i + o.n - 1) % o.n];
            const ZfPt& b = o.p[i];
            const ZfPt& c = o.p[(i + 1) % o.n];
            double turn = (b.x - a.x) * (c.y - b.y) - (b.y - a.y) * (c.x - b.x);
            if (turn > 1.0e-10) continue;
            for (int k = i; k + 1 < o.n; k++) o.p[k] = o.p[k + 1];
            o.n--;
            changed = true;
            break;
        }
    }
    poly = o;
}

// True when the faces cannot share any area worth noting: every corner of one lies beyond
// an edge of the other (or pokes less than `graze` across it).
inline bool zfApart(const ZfTri& a, const ZfTri& b, double graze) {
    double pa[3][3], pb[3][3];
    for (int i = 0; i < 3; i++) {
        zfVertex(a, i, pa[i]);
        zfVertex(b, i, pb[i]);
    }
    double n[3] = {a.n[0], a.n[1], a.n[2]};
    // The two halves of a quad meet along the edge 2 -> 0 of the first one: try it first.
    const int order[3] = {2, 0, 1};
    for (int side = 0; side < 2; side++) {
        const double (*p)[3] = side == 0 ? pa : pb;
        const double (*q)[3] = side == 0 ? pb : pa;
        for (int step = 0; step < 3; step++) {
            int i = order[step];
            const double* p0 = p[i];
            const double* p1 = p[(i + 1) % 3];
            const double* p2 = p[(i + 2) % 3];
            double e[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]};
            double in[3];
            zfCross(n, e, in);
            double d2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
            double sign = zfDot(in, d2) >= 0.0 ? 1.0 : -1.0;
            double limit2 = graze * graze * zfDot(e, e);
            bool allOut = true;
            for (int j = 0; j < 3 && allOut; j++) {
                double d[3] = {q[j][0] - p0[0], q[j][1] - p0[1], q[j][2] - p0[2]};
                double depth = sign * zfDot(in, d);
                if (depth > 0.0 && depth * depth > limit2) allOut = false;
            }
            if (allOut) return true;
        }
    }
    return false;
}

// Part of `top` that shares the plane of `base` and lies inside it, in the frame of base.
// False when the faces do not fight: apart, facing away, or merely touching.
inline bool zfOverlap(const ZfFrame& f, const ZfTri& top, const ZfConfig& cfg, ZfPoly& out) {
    ZfPoly cur, next;
    bool above = true, below = true;
    for (int i = 0; i < 3; i++) {
        double p[3];
        zfVertex(top, i, p);
        ZfPt q = zfProject(f, p);
        if (q.h <= cfg.planeEps) above = false;
        if (q.h >= -cfg.planeEps) below = false;
        cur.add(q);
    }
    if (above || below) return false;
    for (int i = 0; i < 3; i++) {
        double ax, ay, d;
        zfEdge(f.a[i], f.a[(i + 1) % 3], ax, ay, d);
        zfClip(cur, ax, ay, 0.0, d, next);
        if (next.n < 3) return false;
        cur = next;
    }
    zfClip(cur, 0.0, 0.0, -1.0, cfg.planeEps, next);
    if (next.n < 3) return false;
    zfClip(next, 0.0, 0.0, 1.0, cfg.planeEps, cur);
    if (cur.n < 3) return false;
    if (zfSignedArea(cur) < 0.0) {
        for (int i = 0; i < cur.n / 2; i++) std::swap(cur.p[i], cur.p[cur.n - 1 - i]);
    }
    zfTidy(cur);
    if (cur.n < 3) return false;
    double area = zfSignedArea(cur);
    if (area < cfg.minArea) return false;
    if (2.0 * area / zfPerimeter(cur) < cfg.minWidth) return false;
    out = cur;
    return true;
}

// Removes the convex counter-clockwise polygon `cut` from every triangle of `frags`
// (triples of points in the same 2D frame).
inline void zfSubtract(std::vector<ZfPt>& frags, const ZfPoly& cut) {
    const double tiny = 1.0e-12;
    std::vector<ZfPt> result;
    result.reserve(frags.size() + 24);
    for (size_t t = 0; t + 2 < frags.size(); t += 3) {
        ZfPoly rest, part;
        rest.add(frags[t]);
        rest.add(frags[t + 1]);
        rest.add(frags[t + 2]);
        ZfPoly inside = rest;
        for (int e = 0; e < cut.n && inside.n >= 3; e++) {
            double ax, ay, d;
            zfEdge(cut.p[e], cut.p[(e + 1) % cut.n], ax, ay, d);
            zfClip(inside, ax, ay, 0.0, d, part);
            inside = part;
        }
        if (inside.n < 3 || fabs(zfSignedArea(inside)) < tiny) {
            result.insert(result.end(), frags.begin() + t, frags.begin() + t + 3);
            continue;
        }
        for (int e = 0; e < cut.n && rest.n >= 3; e++) {
            double ax, ay, d;
            zfEdge(cut.p[e], cut.p[(e + 1) % cut.n], ax, ay, d);
            zfClip(rest, -ax, -ay, 0.0, -d, part);
            if (part.n >= 3 && fabs(zfSignedArea(part)) > tiny) {
                for (int i = 1; i + 1 < part.n; i++) {
                    result.push_back(part.p[0]);
                    result.push_back(part.p[i]);
                    result.push_back(part.p[i + 1]);
                }
            }
            ZfPoly keep;
            zfClip(rest, ax, ay, 0.0, d, keep);
            rest = keep;
        }
    }
    frags.swap(result);
}

// Writes the triangles of `frags` as vertices of `t`: every attribute (position, uv,
// normal) is interpolated from the three original corners.
inline void zfEmit(const ZfTri& t, const ZfFrame& f, const std::vector<ZfPt>& frags, std::vector<float>& out) {
    double x1 = f.a[1].x, x2 = f.a[2].x, y2 = f.a[2].y;
    for (const ZfPt& q : frags) {
        double b2 = q.y / y2;
        double b1 = (q.x - b2 * x2) / x1;
        double b0 = 1.0 - b1 - b2;
        for (int k = 0; k < t.stride; k++) {
            out.push_back((float)(b0 * t.v[k] + b1 * t.v[t.stride + k] + b2 * t.v[2 * t.stride + k]));
        }
    }
}
