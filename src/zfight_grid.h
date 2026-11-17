#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include "zfight_geom.h"

// ============================================================================
// Z-FIGHT GRID
// Broad phase of the z-fight guard (see zfight_guard.h): turns vertex buffers into
// triangles with a facing and a box, and sorts them into buckets so that only
// faces that could share a plane are ever compared.
// ============================================================================

struct ZfMesh {
    std::vector<float>* data;
    int stride;        // floats per vertex: 8 = pos uv normal, 5 = pos uv
    int rank;          // a higher rank stays on top of a lower one
    const char* name;
};

// Runs job(0) .. job(jobs - 1) on up to `threads` threads, the caller included.
template <class Job>
inline void zfParallel(int threads, int jobs, Job job) {
    if (threads > jobs) threads = jobs;
    std::atomic<int> next(0);
    auto work = [&]() {
        for (int i = next.fetch_add(1); i < jobs; i = next.fetch_add(1)) job(i);
    };
    std::vector<std::thread> pool;
    for (int t = 1; t < threads; t++) {
        try {
            pool.emplace_back(work);
        } catch (...) {
            break; // no more threads to be had: the caller does the rest
        }
    }
    work();
    for (auto& t : pool) t.join();
}

// Small meshes are done before a thread has even started.
inline int zfThreadsFor(int triangles, int threads) {
    return triangles >= 20000 ? threads : 1;
}

// Work memory that is kept between calls: a world mesh is checked again on every rebuild,
// and fresh megabytes from the system cost more than the check itself.
template <class T>
struct ZfPool {
    std::unique_ptr<T[]> data;
    size_t capacity = 0;

    // Room for n items, contents undefined.
    T* get(size_t n) {
        if (n > capacity) {
            capacity = n + n / 4 + 64;
            data.reset(new T[capacity]);
        }
        return data.get();
    }
};

// Builders emit a quad as (a, b, c) (a, c, d). Such halves lie in one plane by design and
// meet along a - c without sharing area, as long as b and d are on opposite sides of it.
inline bool zfQuadHalves(const ZfTri& first, const ZfTri& second) {
    const float* a = first.v;
    const float* b = first.v + first.stride;
    const float* c = first.v + 2 * first.stride;
    const float* a2 = second.v;
    const float* c2 = second.v + second.stride;
    const float* d = second.v + 2 * second.stride;
    for (int k = 0; k < 3; k++) {
        if (a[k] != a2[k] || c[k] != c2[k]) return false;
    }
    double e[3] = {(double)c[0] - a[0], (double)c[1] - a[1], (double)c[2] - a[2]};
    double n[3] = {first.n[0], first.n[1], first.n[2]};
    double in[3];
    zfCross(n, e, in);
    double sb = in[0] * ((double)b[0] - a[0]) + in[1] * ((double)b[1] - a[1]) + in[2] * ((double)b[2] - a[2]);
    double sd = in[0] * ((double)d[0] - a[0]) + in[1] * ((double)d[1] - a[1]) + in[2] * ((double)d[2] - a[2]);
    return sb * sd <= 0.0;
}

// Fills in one triangle. False for a collapsed one (the spare half of a fan slice):
// it draws nothing, so it stays out of the buckets (axes == 0).
inline bool zfLoadTri(ZfTri& t, const float* v, int mesh, int index, int stride, double pad) {
    t.v = v;
    t.mesh = mesh;
    t.index = index;
    t.stride = stride;
    t.mate = -1;
    t.axes = 0;
    double p[3][3], e1[3], e2[3], g[3];
    for (int k = 0; k < 3; k++) zfVertex(t, k, p[k]);
    for (int k = 0; k < 3; k++) {
        e1[k] = p[1][k] - p[0][k];
        e2[k] = p[2][k] - p[0][k];
    }
    zfCross(e1, e2, g);
    double len = sqrt(zfDot(g, g));
    if (len < 2.0e-9) return false;
    // Two-sided meshes are lit by their stored normal, culled ones by winding; the stored
    // normal decides when there is one.
    double sign = 1.0;
    if (stride >= 8 && g[0] * v[5] + g[1] * v[6] + g[2] * v[7] < 0.0) sign = -1.0;
    double big = 0.0;
    for (int k = 0; k < 3; k++) {
        t.n[k] = (float)(g[k] / len * sign);
        if (fabs(t.n[k]) > big) big = fabs(t.n[k]);
        t.lo[k] = (float)(std::min(p[0][k], std::min(p[1][k], p[2][k])) - pad);
        t.hi[k] = (float)(std::max(p[0][k], std::max(p[1][k], p[2][k])) + pad);
    }
    // A face goes into the bucket of every axis it faces almost as much as its main one,
    // so two faces with nearly the same normal always share a bucket.
    for (int k = 0; k < 3; k++) {
        if (fabs(t.n[k]) >= 0.9 * big) t.axes |= (unsigned char)(1 << (k * 2 + (t.n[k] < 0.0f ? 1 : 0)));
    }
    return true;
}

// tris[i] is triangle i of all meshes laid end to end, so a quad's halves are neighbours.
// Returns the number of triangles.
inline int zfLoad(const ZfMesh* meshes, int count, double pad, int threads, ZfPool<ZfTri>& pool) {
    struct Range {
        int mesh, begin, end, base;
    };
    const int chunk = 4096; // even, so the halves of a quad stay in one range
    std::vector<Range> ranges;
    int total = 0;
    for (int m = 0; m < count; m++) {
        int triCount = (int)(meshes[m].data->size() / (size_t)(meshes[m].stride * 3));
        for (int begin = 0; begin < triCount; begin += chunk) {
            ranges.push_back({m, begin, std::min(begin + chunk, triCount), total});
        }
        total += triCount;
    }
    ZfTri* tris = pool.get((size_t)total);
    zfParallel(zfThreadsFor(total, threads), (int)ranges.size(), [&](int job) {
        const Range& r = ranges[(size_t)job];
        int stride = meshes[r.mesh].stride;
        const float* data = meshes[r.mesh].data->data();
        for (int i = r.begin; i < r.end; i++) {
            int at = r.base + i;
            ZfTri& t = tris[at];
            if (!zfLoadTri(t, data + (size_t)i * stride * 3, r.mesh, i, stride, pad)) continue;
            if (!(i & 1)) continue;
            ZfTri& prev = tris[at - 1];
            if (prev.axes == 0 || !zfQuadHalves(prev, t)) continue;
            t.mate = at - 1;
            prev.mate = at;
        }
    });
    return total;
}

// Faces that could fight point the same way, so they are sorted by the axis they mostly
// face; along that axis the grid is cut into thin slabs (coplanar faces share a slab),
// across it into big cells.
struct ZfGrid {
    double perCell, perSlab;

    ZfGrid(double cell, double slab) : perCell(1.0 / cell), perSlab(1.0 / slab) {}

    int at(int axis, int k, double x) const {
        double g = x * ((axis >> 1) == k ? perSlab : perCell) + 0.5;
        int i = (int)g;
        return i - (g < (double)i ? 1 : 0);
    }

    static uint64_t key(int axis, int cx, int cy, int cz) {
        const int bias = 1 << 19;
        uint64_t k = (uint64_t)axis;
        k = (k << 20) | (uint64_t)((cx + bias) & 0xFFFFF);
        k = (k << 20) | (uint64_t)((cy + bias) & 0xFFFFF);
        k = (k << 20) | (uint64_t)((cz + bias) & 0xFFFFF);
        return k;
    }
};

struct ZfRaw {
    uint64_t key;
    int tri;
};

// One face in one bucket. Box and normal are copied here so that walking a bucket reads
// memory in a straight line: nearly every pair is settled by these alone.
struct ZfEntry {
    uint64_t key;
    float lo[3], hi[3], n[3];
    int tri;
    int mate;   // the other half of the same quad: shares the plane but never the area
    int axes;
};

// Entries sorted by hash slot: slot s owns entries[first[s]] .. entries[first[s + 1]].
// A slot can hold more than one bucket, so the key is compared too.
struct ZfBuckets {
    ZfPool<ZfRaw> raw;
    ZfPool<ZfEntry> pool;
    const ZfEntry* entries = nullptr;
    std::vector<int> first, cursor;

    int slots() const {
        return (int)first.size() - 1;
    }

    static uint64_t mix(uint64_t k) {
        k ^= k >> 33;
        k *= 0xff51afd7ed558ccdULL;
        k ^= k >> 33;
        return k;
    }
};

// Calls each(key) for every bucket the face belongs to.
template <class Each>
inline void zfBucketsOf(const ZfTri& t, const ZfGrid& grid, Each each) {
    for (int axis = 0; axis < 6; axis++) {
        if (!(t.axes & (1 << axis))) continue;
        int c0[3], c1[3];
        for (int k = 0; k < 3; k++) {
            c0[k] = grid.at(axis, k, t.lo[k]);
            c1[k] = grid.at(axis, k, t.hi[k]);
        }
        for (int x = c0[0]; x <= c1[0]; x++) {
            for (int y = c0[1]; y <= c1[1]; y++) {
                for (int z = c0[2]; z <= c1[2]; z++) each(ZfGrid::key(axis, x, y, z));
            }
        }
    }
}

inline void zfFill(const ZfTri* tris, int triCount, const ZfGrid& grid, int threads, ZfBuckets& b) {
    // (bucket, face) pairs in face order: counted per chunk first, so that every chunk
    // knows where to write and the chunks can be done side by side
    const int chunk = 4096;
    int jobs = (triCount + chunk - 1) / chunk;
    std::vector<size_t> start((size_t)jobs + 1, 0);
    threads = zfThreadsFor(triCount, threads);
    zfParallel(threads, jobs, [&](int job) {
        size_t n = 0;
        int end = std::min(triCount, (job + 1) * chunk);
        for (int i = job * chunk; i < end; i++) zfBucketsOf(tris[i], grid, [&](uint64_t) { n++; });
        start[(size_t)job + 1] = n;
    });
    for (int job = 0; job < jobs; job++) start[(size_t)job + 1] += start[(size_t)job];
    size_t total = start[(size_t)jobs];
    ZfRaw* raw = b.raw.get(total);
    zfParallel(threads, jobs, [&](int job) {
        size_t n = start[(size_t)job];
        int end = std::min(triCount, (job + 1) * chunk);
        for (int i = job * chunk; i < end; i++) zfBucketsOf(tris[i], grid, [&](uint64_t key) { raw[n++] = {key, i}; });
    });

    size_t slots = 1024;
    while (slots < total) slots <<= 1;
    uint64_t mask = slots - 1;
    b.first.assign(slots + 1, 0);
    for (size_t r = 0; r < total; r++) b.first[(size_t)(ZfBuckets::mix(raw[r].key) & mask) + 1]++;
    for (size_t s = 0; s < slots; s++) b.first[s + 1] += b.first[s];
    b.cursor.assign(b.first.begin(), b.first.end() - 1);
    ZfEntry* entries = b.pool.get(total);
    for (size_t r = 0; r < total; r++) {
        ZfEntry& e = entries[b.cursor[(size_t)(ZfBuckets::mix(raw[r].key) & mask)]++];
        const ZfTri& t = tris[raw[r].tri];
        e.key = raw[r].key;
        e.tri = raw[r].tri;
        e.mate = t.mate;
        e.axes = t.axes;
        for (int k = 0; k < 3; k++) {
            e.lo[k] = t.lo[k];
            e.hi[k] = t.hi[k];
            e.n[k] = t.n[k];
        }
    }
    b.entries = entries;
}

// One set of work memory for the whole program: the guard runs on the thread that builds
// the meshes, one call at a time.
struct ZfScratch {
    ZfPool<ZfTri> tris;
    ZfBuckets buckets;
};

inline ZfScratch& zfScratch() {
    static ZfScratch scratch;
    return scratch;
}

// Most pairs of a bucket end here, so no early outs: one branch instead of many.
// need[k]: how far the boxes must reach into each other along axis k. Boxes are padded,
// so across the facing axis a pair that merely touches (neighbouring tiles, the slices
// of a round cap) still overlaps by the padding and is dropped by this.
inline bool zfCannotFight(const ZfEntry& a, const ZfEntry& b, const float* need) {
    float s0 = std::min(a.hi[0], b.hi[0]) - std::max(a.lo[0], b.lo[0]);
    float s1 = std::min(a.hi[1], b.hi[1]) - std::max(a.lo[1], b.lo[1]);
    float s2 = std::min(a.hi[2], b.hi[2]) - std::max(a.lo[2], b.lo[2]);
    float facing = a.n[0] * b.n[0] + a.n[1] * b.n[1] + a.n[2] * b.n[2];
    return (a.key != b.key) | (a.mate == b.tri) | (s0 < need[0]) | (s1 < need[1]) | (s2 < need[2]) | (facing < 0.9999f);
}

// A pair shows up in every bucket both faces are in; it is handled in one of them only:
// the first axis both face, and the cell where their boxes start to overlap.
inline bool zfPairBelongsHere(const ZfEntry& a, const ZfEntry& b, const ZfGrid& grid) {
    int common = a.axes & b.axes;
    int axis = 0;
    while (!(common & (1 << axis))) axis++;
    if (axis != (int)(a.key >> 60)) return false;
    int c[3];
    for (int k = 0; k < 3; k++) c[k] = grid.at(axis, k, std::max(a.lo[k], b.lo[k]));
    return a.key == ZfGrid::key(axis, c[0], c[1], c[2]);
}

// Cheap test with the stored normal: is any corner of b within reach of the plane of a?
inline bool zfNearPlane(const ZfTri& a, const ZfTri& b, float eps) {
    bool above = true, below = true;
    for (int i = 0; i < 3; i++) {
        const float* q = b.v + i * b.stride;
        float h = a.n[0] * (q[0] - a.v[0]) + a.n[1] * (q[1] - a.v[1]) + a.n[2] * (q[2] - a.v[2]);
        if (h <= eps) above = false;
        if (h >= -eps) below = false;
    }
    return !above && !below;
}
