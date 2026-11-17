#pragma once
#include <cstdio>
#include <cstdlib>
#include "zfight_grid.h"

// ============================================================================
// Z-FIGHT GUARD
// Two faces in the same plane, facing the same way and covering the same spot,
// get the same depth: which one wins changes per pixel and per frame, so the two
// textures flicker through each other. This module finds such pairs in finished
// vertex buffers (position first, any stride) and repairs them: the face that was
// emitted first loses the shared area, it is cut out of it. So the rule for content
// is "what is built later lies on top", like paint.
//
//   zfightScan()    - report only.
//   zfightResolve() - report and repair.
//   zfightRepair()  - what the engine calls on every mesh before it is uploaded.
//
// The build runs tools/zfight_check.cpp over all content and sample worlds and is
// refused when something flickers that the engine cannot repair by itself.
//
// Faces that touch back to back (a box standing on a table) are fine: the solids
// on both sides hide them. Faces 2 mm or more apart are left to the depth buffer.
// ============================================================================

struct ZfConflict {
    int meshA, triA;   // loses the spot
    int meshB, triB;   // stays
    float area;
    float pos[3];
    float normal[3];
};

struct ZfReport {
    int pairs = 0;
    double area = 0.0;   // m^2 that would flicker
    int trisCut = 0;
    int trisAdded = 0;
    std::vector<ZfConflict> samples;

    bool clean() const {
        return pairs == 0;
    }
};

struct ZfCut {
    int tri;
    ZfPoly poly;
};

inline bool zfOutranks(const ZfMesh* meshes, const ZfTri& a, const ZfTri& b) {
    if (meshes[a.mesh].rank != meshes[b.mesh].rank) return meshes[a.mesh].rank > meshes[b.mesh].rank;
    if (a.mesh != b.mesh) return a.mesh > b.mesh;
    return a.index > b.index;
}

inline void zfNote(ZfReport& r, const ZfTri& lose, const ZfTri& stay, const ZfFrame& f, const ZfPoly& poly, int maxSamples) {
    double area = zfSignedArea(poly);
    r.pairs++;
    r.area += area;
    if ((int)r.samples.size() >= maxSamples) return;
    double cx = 0.0, cy = 0.0;
    for (int i = 0; i < poly.n; i++) {
        cx += poly.p[i].x / poly.n;
        cy += poly.p[i].y / poly.n;
    }
    ZfConflict c;
    c.meshA = lose.mesh;
    c.triA = lose.index;
    c.meshB = stay.mesh;
    c.triB = stay.index;
    c.area = (float)area;
    for (int k = 0; k < 3; k++) {
        c.pos[k] = (float)(f.o[k] + f.u[k] * cx + f.w[k] * cy);
        c.normal[k] = lose.n[k];
    }
    r.samples.push_back(c);
}

struct ZfSearch {
    const ZfMesh* meshes;
    const ZfTri* tris;
    const ZfBuckets* buckets;
    ZfGrid grid;
    ZfConfig cfg;
    bool wantCuts;
};

// Every fighting pair among the buckets of slots [slot0, slot1).
inline void zfSearchSlots(const ZfSearch& s, int slot0, int slot1, ZfReport& report, std::vector<ZfCut>& cuts) {
    // An overlap of the smallest width that counts reaches at least this far across any
    // edge and into the box of either face. Boxes are padded by planeEps on every side.
    double graze = s.cfg.minWidth * 0.2;
    float touch = (float)(s.cfg.planeEps * 2.0 + graze);
    float reach = (float)(s.cfg.planeEps * 1.05 + 1.0e-5);
    const ZfEntry* entries = s.buckets->entries;
    ZfPoly poly;
    for (int slot = slot0; slot < slot1; slot++) {
        int end = s.buckets->first[(size_t)slot + 1];
        for (int i = s.buckets->first[(size_t)slot]; i < end; i++) {
            const ZfEntry& ea = entries[i];
            float need[3] = {touch, touch, touch};
            need[(ea.key >> 61) & 3] = 0.0f;
            for (int j = i + 1; j < end; j++) {
                const ZfEntry& eb = entries[j];
                if (zfCannotFight(ea, eb, need)) continue;
                if (!zfPairBelongsHere(ea, eb, s.grid)) continue;
                const ZfTri& a = s.tris[ea.tri];
                const ZfTri& b = s.tris[eb.tri];
                if (!zfNearPlane(a, b, reach) || zfApart(a, b, graze)) continue;
                bool aStays = zfOutranks(s.meshes, a, b);
                const ZfTri& lose = aStays ? b : a;
                const ZfTri& stay = aStays ? a : b;
                ZfFrame f = zfFrameOf(lose);
                if (!f.ok || !zfOverlap(f, stay, s.cfg, poly)) continue;
                zfNote(report, lose, stay, f, poly, s.cfg.maxSamples);
                if (s.wantCuts) cuts.push_back({aStays ? eb.tri : ea.tri, poly});
            }
        }
    }
}

// Loads the meshes into the work memory and finds every fighting pair. The slots are
// split into a fixed number of jobs and the results joined in job order, so the outcome
// does not depend on how many threads did the work.
inline void zfFindPairs(const ZfMesh* meshes, int count, const ZfConfig& cfg, ZfReport& report, std::vector<ZfCut>* cuts) {
    ZfScratch& scratch = zfScratch();
    int triCount = zfLoad(meshes, count, cfg.planeEps, cfg.threads, scratch.tris);
    const ZfTri* tris = scratch.tris.data.get();
    ZfGrid grid(cfg.cell, cfg.slab);
    zfFill(tris, triCount, grid, cfg.threads, scratch.buckets);
    ZfSearch search = {meshes, tris, &scratch.buckets, grid, cfg, cuts != nullptr};
    const int jobs = 64;
    int slots = scratch.buckets.slots();
    std::vector<ZfReport> jobReports((size_t)jobs);
    std::vector<std::vector<ZfCut>> jobCuts((size_t)jobs);
    zfParallel(zfThreadsFor(triCount, cfg.threads), jobs, [&](int job) {
        int slot0 = (int)((long long)slots * job / jobs);
        int slot1 = (int)((long long)slots * (job + 1) / jobs);
        zfSearchSlots(search, slot0, slot1, jobReports[(size_t)job], jobCuts[(size_t)job]);
    });
    for (int job = 0; job < jobs; job++) {
        const ZfReport& r = jobReports[(size_t)job];
        report.pairs += r.pairs;
        report.area += r.area;
        for (const ZfConflict& c : r.samples) {
            if ((int)report.samples.size() < cfg.maxSamples) report.samples.push_back(c);
        }
        if (cuts) cuts->insert(cuts->end(), jobCuts[(size_t)job].begin(), jobCuts[(size_t)job].end());
    }
}

inline ZfReport zfightScan(const ZfMesh* meshes, int count, const ZfConfig& cfg = ZfConfig()) {
    ZfReport report;
    zfFindPairs(meshes, count, cfg, report, nullptr);
    return report;
}

struct ZfPatch {
    int mesh, index;
    std::vector<float> verts;   // the triangles that take the place of the cut one
};

// The first piece goes where the cut triangle was, the others to the end of the buffer.
// A triangle that lost everything is collapsed into a point.
inline void zfApply(ZfMesh* meshes, const std::vector<ZfPatch>& patches) {
    for (const ZfPatch& p : patches) {
        std::vector<float>& data = *meshes[p.mesh].data;
        size_t stride = (size_t)meshes[p.mesh].stride;
        size_t triSize = stride * 3;
        size_t at = (size_t)p.index * triSize;
        if (p.verts.empty()) {
            for (size_t k = stride; k < triSize; k++) data[at + k] = data[at + k % stride];
            continue;
        }
        for (size_t k = 0; k < triSize; k++) data[at + k] = p.verts[k];
        data.insert(data.end(), p.verts.begin() + (long long)triSize, p.verts.end());
    }
}

// Cuts the shared area out of the face that loses it.
inline ZfReport zfightResolve(ZfMesh* meshes, int count, const ZfConfig& scanCfg = ZfConfig()) {
    ZfConfig cfg = scanCfg;
    cfg.minArea *= cfg.repairMargin;
    cfg.minWidth *= cfg.repairMargin;
    ZfReport report;
    std::vector<ZfCut> cuts;
    zfFindPairs(meshes, count, cfg, report, &cuts);
    if (cuts.empty()) return report;
    std::stable_sort(cuts.begin(), cuts.end(), [](const ZfCut& a, const ZfCut& b) { return a.tri < b.tri; });

    const ZfTri* tris = zfScratch().tris.data.get();
    std::vector<ZfPatch> patches;
    std::vector<ZfPt> frags;
    for (size_t start = 0; start < cuts.size();) {
        size_t end = start;
        const ZfTri& t = tris[cuts[start].tri];
        ZfFrame f = zfFrameOf(t);
        frags.assign(f.a, f.a + 3);
        while (end < cuts.size() && cuts[end].tri == cuts[start].tri) zfSubtract(frags, cuts[end++].poly);
        patches.push_back({t.mesh, t.index, {}});
        zfEmit(t, f, frags, patches.back().verts);
        report.trisCut++;
        report.trisAdded += (int)frags.size() / 3;
        start = end;
    }
    // only now: the triangles above point into the buffers that change here
    zfApply(meshes, patches);
    return report;
}

inline void zfightPrint(FILE* to, const char* what, const ZfReport& r, const ZfMesh* meshes) {
    std::fprintf(to, "Z-FIGHT in %s: %d face pair(s) share a plane, %.1f cm2 flickering\n", what, r.pairs, r.area * 1.0e4);
    for (const ZfConflict& c : r.samples) {
        std::fprintf(to, "    %s #%d under %s #%d: %.1f cm2 at (%.3f, %.3f, %.3f), facing (%.2f, %.2f, %.2f)\n",
                     meshes[c.meshA].name, c.triA, meshes[c.meshB].name, c.triB, c.area * 1.0e4f,
                     c.pos[0], c.pos[1], c.pos[2], c.normal[0], c.normal[1], c.normal[2]);
    }
    if (r.pairs > (int)r.samples.size()) std::fprintf(to, "    ... and %d more\n", r.pairs - (int)r.samples.size());
}

// What the guard had to repair in the running game: the last world mesh and the total.
inline ZfReport gZfightWorld;
inline long long gZfightRepairs = 0;

// The engine's entry point: repairs the meshes and keeps count. To see what was repaired
// and where, run the game with BACKROOMS_ZFIGHT_LOG=<file>.
inline ZfReport zfightRepair(ZfMesh* meshes, int count, const char* what) {
    ZfConfig cfg;
    int cores = (int)std::thread::hardware_concurrency();
    cfg.threads = cores > 4 ? 4 : (cores < 1 ? 1 : cores);
    ZfReport r = zfightResolve(meshes, count, cfg);
    gZfightRepairs += r.pairs;
    static const char* logPath = std::getenv("BACKROOMS_ZFIGHT_LOG");
    if (r.clean() || !logPath || !logPath[0]) return r;
    if (FILE* f = std::fopen(logPath, "a")) {
        zfightPrint(f, what, r, meshes);
        std::fprintf(f, "    repaired: %d triangle(s) cut into %d\n", r.trisCut, r.trisAdded);
        std::fclose(f);
    }
    return r;
}

// One mesh that is drawn on its own (loot, the rig, hand-held devices).
inline ZfReport resolveModelMesh(std::vector<float>& v, const char* what, int stride = 8) {
    ZfMesh m = {&v, stride, 0, what};
    return zfightRepair(&m, 1, what);
}
