#pragma once
// LIMEN 3D geometry synthesizer & per-slab GPU mesh cache:
//   - Immutable per-slab GPU VAO/VBO cache (`LmCachedSlabMesh`) with LRU eviction & frustum culling
//   - Human-scale 0.26 m thin partitions (`LF_THIN_*`) and 2.50 m narrowed corridors (`LF_NARROW_*`)
//   - Multi-tier ceilings: 2.65 m cramped service halls, 4.5 m storeys, 9.0 m mezzanine pavilions, 54 m vaults
//   - Walkable 10-step tiled staircases into drained `LT_BASIN` pools
#include "limen_envmap.h"
#include <chrono>

struct LmSlabPerfAccum {
    long long slabsBuilt = 0;
    long long preTotalVerts = 0;   // full slab structural verts BEFORE R1/R2 arch detail
    long long archAddedVerts = 0;  // vertices added by R1/R2 procedural arch detail
    double preBuildMs = 0.0;       // slab mesh CPU build time without arch detail
    double archBuildMs = 0.0;      // CPU build time of procedural arch detail
    double gpuUploadMs = 0.0;      // VBO/VAO upload time
    void reset() { slabsBuilt = 0; preTotalVerts = archAddedVerts = 0; preBuildMs = archBuildMs = gpuUploadMs = 0.0; }
};
inline LmSlabPerfAccum gLmPerf;

// Bench-only verification state (`LM_BENCH_HASH=1 limen_shots.exe bench`): every cached mesh is
// re-modelled and compared against what the cache holds.
#ifdef LIMEN_SHOTS
inline bool gLmBenchHashOn = false;
inline bool gLmBenchHashKeep = false;   // hash every mesh as it is built, verify only while gLmBenchHashOn
inline void (*gLmBenchStaleFn)(int j, int i) = nullptr;   // told about every slab whose cached mesh no longer matches a rebuild
inline unsigned long long gLmBenchGeomHash = 0;
inline unsigned long long gLmBenchPropVertHash = 0;
inline long long gLmBenchPropStale = 0, gLmBenchPropChunksBuilt = 0, gLmBenchSlabStale = 0;
#endif

// Geometry audit (`limen_shots.exe audit`): CPU copies of every structural buffer, tagged with
// the buffer it is drawn from and whether that draw is back-face culled.
#ifdef LIMEN_SHOTS
struct LmAuditTri { float p[9]; unsigned char buf, twoSided, flip; };
enum { LA_WALL = 0, LA_FLOOR, LA_CEIL, LA_PILLAR, LA_DECOR, LA_MEGA, LA_MEGA_ALT, LA_SLAB_WALL, LA_DADO,
       LA_SLAB_DECOR, LA_VIT_FLOOR, LA_VIT_CEIL, LA_X_WALL, LA_X_DADO, LA_X_FLOOR, LA_X_CEIL, LA_LIGHT, LA_PROP,
       LA_UNSTRUCT_ACC, LA_UNSTRUCT_SKY, LA_BUF_COUNT };
inline bool gLmAuditOn = false;
inline std::vector<LmAuditTri> gLmAuditWorld;
inline std::unordered_map<long long, std::vector<LmAuditTri>> gLmAuditSlab;
inline void lmAuditAppend(std::vector<LmAuditTri>& dst, const std::vector<float>& v, int buf, bool twoSided) {
    for (size_t k = 0; k + 24 <= v.size(); k += 24) {
        LmAuditTri t;
        for (int q = 0; q < 3; q++) { t.p[q*3] = v[k + q*8]; t.p[q*3+1] = v[k + q*8 + 1]; t.p[q*3+2] = v[k + q*8 + 2]; }
        t.buf = (unsigned char)buf; t.twoSided = twoSided ? 1 : 0;
        {   // a two-sided face is lit by its stored normal, which may disagree with its winding
            float e1[3] = {t.p[3]-t.p[0], t.p[4]-t.p[1], t.p[5]-t.p[2]}, e2[3] = {t.p[6]-t.p[0], t.p[7]-t.p[1], t.p[8]-t.p[2]};
            float g[3] = {e1[1]*e2[2] - e1[2]*e2[1], e1[2]*e2[0] - e1[0]*e2[2], e1[0]*e2[1] - e1[1]*e2[0]};
            t.flip = (twoSided && g[0]*v[k+5] + g[1]*v[k+6] + g[2]*v[k+7] < 0.0f) ? 1 : 0;
        }
        dst.push_back(t);
    }
}
// Glowing surfaces (5 floats per vertex) hide what is behind them just like any other surface.
inline void lmAuditAppendLights(std::vector<LmAuditTri>& dst, const std::vector<float>& v) {
    for (size_t k = 0; k + 15 <= v.size(); k += 15) {
        LmAuditTri t;
        for (int q = 0; q < 3; q++) { t.p[q*3] = v[k + q*5]; t.p[q*3+1] = v[k + q*5 + 1]; t.p[q*3+2] = v[k + q*5 + 2]; }
        t.buf = LA_LIGHT; t.twoSided = 1; t.flip = 0;
        dst.push_back(t);
    }
}
#endif

// Coplanar faces that cover the same spot flicker through each other. The game's own mesher
// ran its z-fight guard over the whole view on every rebuild; LIMEN's meshes are cached per slab
// and per prop tile, so each is repaired once, when it is built: of two such faces the one
// emitted first (or of lower rank) loses the shared area.
inline double gLmZfMs = 0.0;
inline long long gLmZfPairs = 0, gLmZfRuns = 0;
#ifdef LIMEN_SHOTS
#include <map>
#include <string>
struct LmZfHist { long long pairs = 0; double area = 0; float ex[3] = {0, 0, 0}; float exArea = 0; };
inline std::map<std::string, LmZfHist> gLmZfHist;
inline bool gLmZfHistOn = false;
#endif
inline double gLmZfMsBy[3] = {0, 0, 0}; inline long long gLmZfPairsBy[3] = {0, 0, 0}, gLmZfRunsBy[3] = {0, 0, 0}, gLmZfTrisBy[3] = {0,0,0};
inline void lmResolveZfight(ZfMesh* meshes, int count) {
    struct Acc { int k; double ms0; long long p0; ~Acc() { gLmZfMsBy[k] += gLmZfMs - ms0; gLmZfPairsBy[k] += gLmZfPairs - p0; gLmZfRunsBy[k]++; } };
    Acc acc{count >= 11 ? 0 : (count == 1 ? 1 : 2), gLmZfMs, gLmZfPairs};
    for (int k = 0; k < count; k++) gLmZfTrisBy[acc.k] += (long long)meshes[k].data->size() / 24;
    auto t0 = std::chrono::high_resolution_clock::now();
    int n = 0;
    for (int k = 0; k < count; k++) if (!meshes[k].data->empty()) meshes[n++] = meshes[k];
#ifdef LIMEN_SHOTS
    if (n > 0 && gLmZfHistOn) {
        ZfConfig cfg; cfg.maxSamples = 2000000;
        ZfReport r = zfightScan(meshes, n, cfg);
        for (const ZfConflict& c : r.samples) {
            char key[200];
            const char* face = fabsf(c.normal[1]) > 0.9f ? (c.normal[1] > 0 ? "up" : "down") : (fabsf(c.normal[1]) < 0.1f ? "vert" : "slope");
            const char* yb = c.pos[1] < -0.05f ? "y<0" : (c.pos[1] < 2.6f ? "y0-2.6" : (c.pos[1] < 4.45f ? "y2.6-4.5" : (c.pos[1] < 4.6f ? "y~4.5" : "y>4.5")));
            snprintf(key, sizeof(key), "%s under %s / %s %s", meshes[c.meshA].name, meshes[c.meshB].name, face, yb);
            LmZfHist& h = gLmZfHist[key];
            h.pairs++; h.area += c.area;
            if (h.pairs == 1 || (h.pairs % 97) == 0) { h.ex[0] = c.pos[0]; h.ex[1] = c.pos[1]; h.ex[2] = c.pos[2]; h.exArea = c.area; }
        }
    }
#endif
    if (n > 0) {
        ZfReport r = zfightResolve(meshes, n);
        gLmZfPairs += r.pairs;
    }
    gLmZfRuns++;
    gLmZfMs += std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t0).count();
}

inline unsigned long long lmFnv64(const void* data, size_t n, unsigned long long h = 1469598103934665603ull) {
    const unsigned char* b = (const unsigned char*)data;
    for (size_t q = 0; q < n; q++) h = (h ^ b[q]) * 1099511628211ull;
    return h;
}

inline void lmPushQuad(std::vector<float>& v, Vec3 a, Vec3 b, Vec3 c, Vec3 d, float tile) {
    Vec3 e1 = b - a, e2 = d - a;
    Vec3 n = e1.cross(e2).norm();
    float u1 = e1.len() / tile, v1 = e2.len() / tile;
    float vv[] = {
        a.x, a.y, a.z, 0.0f, 0.0f, n.x, n.y, n.z,
        b.x, b.y, b.z, u1,   0.0f, n.x, n.y, n.z,
        c.x, c.y, c.z, u1,   v1,   n.x, n.y, n.z,
        a.x, a.y, a.z, 0.0f, 0.0f, n.x, n.y, n.z,
        c.x, c.y, c.z, u1,   v1,   n.x, n.y, n.z,
        d.x, d.y, d.z, 0.0f, v1,   n.x, n.y, n.z
    };
    v.insert(v.end(), vv, vv + 48);
}

inline void lmPushTri(std::vector<float>& v, Vec3 a, Vec3 b, Vec3 c, float tile) {
    Vec3 n = (b - a).cross(c - a).norm();
    float vv[] = {
        a.x, a.y, a.z, 0.0f, 0.0f, n.x, n.y, n.z,
        b.x, b.y, b.z, (b - a).len() / tile, 0.0f, n.x, n.y, n.z,
        c.x, c.y, c.z, 0.0f, (c - a).len() / tile, n.x, n.y, n.z
    };
    v.insert(v.end(), vv, vv + 24);
}

inline void lmPushBox(std::vector<float>& v, float x0, float y0, float z0, float x1, float y1, float z1, float tile) {
    lmPushQuad(v, Vec3(x0, y0, z1), Vec3(x1, y0, z1), Vec3(x1, y1, z1), Vec3(x0, y1, z1), tile);
    lmPushQuad(v, Vec3(x1, y0, z0), Vec3(x0, y0, z0), Vec3(x0, y1, z0), Vec3(x1, y1, z0), tile);
    lmPushQuad(v, Vec3(x1, y0, z1), Vec3(x1, y0, z0), Vec3(x1, y1, z0), Vec3(x1, y1, z1), tile);
    lmPushQuad(v, Vec3(x0, y0, z0), Vec3(x0, y0, z1), Vec3(x0, y1, z1), Vec3(x0, y1, z0), tile);
    lmPushQuad(v, Vec3(x0, y1, z1), Vec3(x1, y1, z1), Vec3(x1, y1, z0), Vec3(x0, y1, z0), tile);
    lmPushQuad(v, Vec3(x0, y0, z0), Vec3(x1, y0, z0), Vec3(x1, y0, z1), Vec3(x0, y0, z1), tile);
}

inline void lmPushShearedBox(std::vector<float>& v, float bx, float by, float bz,
                             float tx, float ty, float tz, float hb, float ht, float tile) {
    if (ty <= by + 0.01f) return;
    Vec3 b00(bx - hb, by, bz - hb), b10(bx + hb, by, bz - hb), b11(bx + hb, by, bz + hb), b01(bx - hb, by, bz + hb);
    Vec3 t00(tx - ht, ty, tz - ht), t10(tx + ht, ty, tz - ht), t11(tx + ht, ty, tz + ht), t01(tx - ht, ty, tz + ht);
    lmPushQuad(v, b01, b11, t11, t01, tile);
    lmPushQuad(v, b10, b00, t00, t10, tile);
    lmPushQuad(v, b11, b10, t10, t11, tile);
    lmPushQuad(v, b00, b01, t01, t00, tile);
    lmPushQuad(v, t01, t11, t10, t00, tile);
    lmPushQuad(v, b00, b10, b11, b01, tile);
}

// A six-sided solid from its bottom ring and the ring above it (corner order as in lmPushShearedBox).
inline void lmPushHexa(std::vector<float>& v, Vec3 b00, Vec3 b10, Vec3 b11, Vec3 b01,
                       Vec3 t00, Vec3 t10, Vec3 t11, Vec3 t01, float tile) {
    lmPushQuad(v, b01, b11, t11, t01, tile);
    lmPushQuad(v, b10, b00, t00, t10, tile);
    lmPushQuad(v, b11, b10, t10, t11, tile);
    lmPushQuad(v, b00, b01, t01, t00, tile);
    lmPushQuad(v, t01, t11, t10, t00, tile);
    lmPushQuad(v, b00, b10, b11, b01, tile);
}

inline void lmPushWallSpan(std::vector<float>& v, float x, float y0, float z, float dx, float dz, float y1) {
    if (y1 <= y0 + 0.01f) return;
    Vec3 n = Vec3(dz, 0.0f, -dx).norm();
    float len = sqrtf(dx * dx + dz * dz);
    float tx = (len / CS) * 1.8f;
    float ty0 = (y0 / WH) * 1.6f, ty1 = (y1 / WH) * 1.6f;
    float vv[] = {
        x, y0, z, 0, ty0, n.x, n.y, n.z,
        x, y1, z, 0, ty1, n.x, n.y, n.z,
        x + dx, y1, z + dz, tx, ty1, n.x, n.y, n.z,
        x, y0, z, 0, ty0, n.x, n.y, n.z,
        x + dx, y1, z + dz, tx, ty1, n.x, n.y, n.z,
        x + dx, y0, z + dz, tx, ty0, n.x, n.y, n.z
    };
    v.insert(v.end(), vv, vv + 48);
}

inline void lmPushLightQuad(std::vector<float>& lv, Vec3 a, Vec3 b, Vec3 c, Vec3 d, bool twoSided) {
    float q1[] = {a.x,a.y,a.z,0,0, b.x,b.y,b.z,1,0, c.x,c.y,c.z,1,1, a.x,a.y,a.z,0,0, c.x,c.y,c.z,1,1, d.x,d.y,d.z,0,1};
    lv.insert(lv.end(), q1, q1 + 30);
    if (twoSided) {
        float q2[] = {a.x,a.y,a.z,0,0, d.x,d.y,d.z,0,1, c.x,c.y,c.z,1,1, a.x,a.y,a.z,0,0, c.x,c.y,c.z,1,1, b.x,b.y,b.z,1,0};
        lv.insert(lv.end(), q2, q2 + 30);
    }
}

inline void lmPushLightBox(std::vector<float>& lv, float cx, float y0, float cz, float hx, float hy, float hz) {
    lmPushLightQuad(lv, Vec3(cx-hx,y0,cz+hz), Vec3(cx+hx,y0,cz+hz), Vec3(cx+hx,y0+hy,cz+hz), Vec3(cx-hx,y0+hy,cz+hz), true);
    lmPushLightQuad(lv, Vec3(cx+hx,y0,cz-hz), Vec3(cx-hx,y0,cz-hz), Vec3(cx-hx,y0+hy,cz-hz), Vec3(cx+hx,y0+hy,cz-hz), true);
    lmPushLightQuad(lv, Vec3(cx+hx,y0,cz+hz), Vec3(cx+hx,y0,cz-hz), Vec3(cx+hx,y0+hy,cz-hz), Vec3(cx+hx,y0+hy,cz+hz), true);
    lmPushLightQuad(lv, Vec3(cx-hx,y0,cz-hz), Vec3(cx-hx,y0,cz+hz), Vec3(cx-hx,y0+hy,cz+hz), Vec3(cx-hx,y0+hy,cz-hz), true);
    lmPushLightQuad(lv, Vec3(cx-hx,y0+hy,cz+hz), Vec3(cx+hx,y0+hy,cz+hz), Vec3(cx+hx,y0+hy,cz-hz), Vec3(cx-hx,y0+hy,cz-hz), true);
}

inline void lmPushDecorQuadRaw(std::vector<float>& dv, Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n, int mat);

inline bool lmIsGraftedPodWall(const LimenSlab& s, int nlx, int nlz) {
    if (nlx < 0 || nlz < 0 || nlx >= s.w || nlz >= s.h) return false;
    return s.tag[nlx][nlz] == LT_WALL && (s.flag[nlx][nlz] & LF_LOW_WALL) != 0;
}

// Vault slabs: is the space above this cell's 4.5 m line solid mass (true) or air (false)?
inline bool lmVaultMassAbove(const LimenSlab& s, int lx, int lz) {
    if (lx < 0 || lz < 0 || lx >= s.w || lz >= s.h) return true;
    int t = s.tag[lx][lz];
    if (t == LT_WALL) return (s.flag[lx][lz] & LF_LOW_WALL) == 0;
    if (!lmIsLowShellTag(t)) return false;
    auto fullWall = [&](int x, int z) {
        return x >= 0 && z >= 0 && x < s.w && z < s.h && s.tag[x][z] == LT_WALL && (s.flag[x][z] & LF_LOW_WALL) == 0;
    };
    // a doorway through a full-height wall line carries that wall above its lintel
    if ((fullWall(lx - 1, lz) && fullWall(lx + 1, lz)) || (fullWall(lx, lz - 1) && fullWall(lx, lz + 1))) return true;
    // what the game opened or stamped is a tunnel through mass (above) or else a roofed pavilion
    if (s.zone[lx][lz] & LZ_STORY) return false;
    return !lmIsGraftedPodWall(s, lx - 1, lz) && !lmIsGraftedPodWall(s, lx + 1, lz) &&
           !lmIsGraftedPodWall(s, lx, lz - 1) && !lmIsGraftedPodWall(s, lx, lz + 1);
}

// Form 1 (MF_ZIGGURAT): (0) slab-wide ceremonial stair climbing all the way to the vault,
// (1) 6..10-tier rising stepped ziggurat with lit rims & summit beacon, or (2+) deep -18m amphitheatre!
inline void lmBuildMegaZiggurat(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<float>& mav,
                                std::vector<float>& dv, std::vector<float>& lv, std::vector<LimenAabb2D>& outCols) {
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float ceilY = s.ceilTiers * WH;
    const unsigned char* g = s.genome.g;
    int subMode = s.megaSub & 3;

    for (float x = x0 + 15.0f; x <= x1 - 15.0f; x += 20.0f)
        lmPushBox(mv, x - 0.65f, ceilY - 2.0f, z0, x + 0.65f, ceilY, z1, T);
    for (float z = z0 + 15.0f; z <= z1 - 15.0f; z += 20.0f)
        lmPushBox(mv, x0, ceilY - 2.0f, z - 0.65f, x1, ceilY, z + 0.65f, T);

    int bMinX = s.w, bMaxX = -1, bMinZ = s.h, bMaxZ = -1;
    for (int lx = 0; lx < s.w; lx++) {
        for (int lz = 0; lz < s.h; lz++) {
            if (s.tag[lx][lz] == LT_BASIN || (s.tag[lx][lz] == LT_BRIDGE && (s.flag[lx][lz] & LF_OVER_BASIN))) {
                if (lx < bMinX) bMinX = lx;
                if (lx > bMaxX) bMaxX = lx;
                if (lz < bMinZ) bMinZ = lz;
                if (lz > bMaxZ) bMaxZ = lz;
            }
        }
    }
    if (bMaxX < bMinX || bMaxZ < bMinZ) return;
    float bx0 = (s.rect.x0 + 1 + bMinX) * CS, bx1 = (s.rect.x0 + 2 + bMaxX) * CS;
    float bz0 = (s.rect.z0 + 1 + bMinZ) * CS, bz1 = (s.rect.z0 + 2 + bMaxZ) * CS;

    if (subMode == 0) {
        // Grand ceremonial staircase spanning full width & climbing 94..96% of vault height into the end wall!
        int axis = (s.megaSub >> 2) & 1;
        bool posEnd = ((s.megaSub >> 3) & 1) != 0;
        float topY = ceilY - 1.6f;
        int nSteps = lmClampInt(10 + s.ceilTiers / 2, 11, 16);
        float runLen = (axis == 0) ? (bz1 - bz0) : (bx1 - bx0);
        float stepRun = runLen / (float)nSteps, stepRise = topY / (float)nSteps;
        float cLo = (axis == 0) ? bx0 : bz0, cHi = (axis == 0) ? bx1 : bz1;
        float cMid = 0.5f * (cLo + cHi);
        for (int k = 0; k < nSteps; k++) {
            float yTop = (float)(k + 1) * stepRise;
            float yBot = (k == 0) ? -0.35f : ((float)k * stepRise - 0.30f);
            float aStart = posEnd ? ((axis == 0 ? bz0 : bx0) + k * stepRun)
                                  : ((axis == 0 ? bz1 : bx1) - (k + 1) * stepRun);
            float aEnd = aStart + stepRun + 0.06f;
            std::vector<float>& dst = ((k & 1) == 0) ? mv : mav;
            if (axis == 0) lmPushBox(dst, cLo, yBot, aStart, cHi, yTop, aEnd, T);
            else           lmPushBox(dst, aStart, yBot, cLo, aEnd, yTop, cHi, T);
            for (int ss = 0; ss < 3; ss++) {
                float sy = (float)k * stepRise + (float)(ss + 1) * (stepRise / 3.0f) + 0.08f;
                float sa0 = posEnd ? (aStart + ss * (stepRun / 3.0f)) : (aEnd - (ss + 1) * (stepRun / 3.0f));
                float sa1 = sa0 + stepRun / 3.0f + 0.04f;
                if (axis == 0) lmPushBox(mv, cMid - 4.2f, yBot, sa0, cMid + 4.2f, sy, sa1, T);
                else           lmPushBox(mv, sa0, yBot, cMid - 4.2f, sa1, sy, cMid + 4.2f, T);
            }
            if (!s.dark) {
                float aLit = posEnd ? (aStart - 0.07f) : (aEnd + 0.07f);
                if (axis == 0) lmPushLightQuad(lv, Vec3(cLo + 1.0f, yTop - 0.30f, aLit), Vec3(cHi - 1.0f, yTop - 0.30f, aLit),
                                                   Vec3(cHi - 1.0f, yTop - 0.04f, aLit), Vec3(cLo + 1.0f, yTop - 0.04f, aLit), true);
                else           lmPushLightQuad(lv, Vec3(aLit, yTop - 0.30f, cLo + 1.0f), Vec3(aLit, yTop - 0.30f, cHi - 1.0f),
                                                   Vec3(aLit, yTop - 0.04f, cHi - 1.0f), Vec3(aLit, yTop - 0.04f, cLo + 1.0f), true);
            }
        }
        if (axis == 0) {
            lmPushBox(mav, cLo - 1.2f, 0.0f, bz0, cLo + 0.4f, ceilY, bz1, T);
            lmPushBox(mav, cHi - 0.4f, 0.0f, bz0, cHi + 1.2f, ceilY, bz1, T);
            // the cheek walls reach 1.2 m out over the plaza floor: solid there too
            outCols.push_back({cLo - 1.2f, bz0, cLo + 0.4f, bz1});
            outCols.push_back({cHi - 0.4f, bz0, cHi + 1.2f, bz1});
        } else {
            lmPushBox(mav, bx0, 0.0f, cLo - 1.2f, bx1, ceilY, cLo + 0.4f, T);
            lmPushBox(mav, bx0, 0.0f, cHi - 0.4f, bx1, ceilY, cHi + 1.2f, T);
            outCols.push_back({bx0, cLo - 1.2f, bx1, cLo + 0.4f});
            outCols.push_back({bx0, cHi - 0.4f, bx1, cHi + 1.2f});
        }
        float endA = posEnd ? (axis == 0 ? bz1 : bx1) : (axis == 0 ? bz0 : bx0);
        float dYaw = (axis == 0) ? (posEnd ? 3.14159265f : 0.0f) : (posEnd ? -1.57079633f : 1.57079633f);
        float dx = (axis == 0) ? cMid : (endA + (posEnd ? -0.06f : 0.06f));
        float dz = (axis == 0) ? (endA + (posEnd ? -0.06f : 0.06f)) : cMid;
        lmBuildClosedWallDoor(dx, topY - 2.35f, dz, dYaw, true, dv);
        if (!s.dark) lmPushLightBox(lv, dx, topY, dz, 1.4f, 1.0f, 1.4f);
        return;
    }

    if (subMode == 1) {
        // Steep 6..10-tier rising pyramid occupying 50..66% of vault height directly from y=0 plaza!
        int nTiers = lmClampInt(6 + (int)g[LG_MASS] * 4 / 256, 6, 10);
        float maxH = ceilY * (0.50f + 0.16f * ((float)g[LG_MASS] / 255.0f)); // 50%..66% of vault ceiling!
        float wAvail = bx1 - bx0, hAvail = bz1 - bz0;
        if (wAvail < 12.0f || hAvail < 12.0f) return;
        float mx = 0.5f * (bx0 + bx1), mz = 0.5f * (bz0 + bz1);
        for (int t = 0; t < nTiers; t++) {
            float f0 = (float)t / (float)nTiers * 0.84f;
            float tx0 = bx0 + wAvail * 0.5f * f0, tx1 = bx1 - wAvail * 0.5f * f0;
            float tz0 = bz0 + hAvail * 0.5f * f0, tz1 = bz1 - hAvail * 0.5f * f0;
            float y0 = (t == 0) ? -0.25f : (maxH * (float)t / (float)nTiers - 0.15f);
            float y1 = maxH * (float)(t + 1) / (float)nTiers;
            std::vector<float>& dst = ((t & 1) == 0) ? mv : mav;
            lmPushBox(dst, tx0, y0, tz0, tx1, y1, tz1, T);
            // Axial ceremonial stair rampart climbing all 4 faces!
            float sw = 2.4f;
            lmPushBox(mv, mx - sw, y0, tz0 - 0.85f, mx + sw, y1 - (y1 - y0) * 0.45f, tz1 + 0.85f, T);
            lmPushBox(mv, tx0 - 0.85f, y0, mz - sw, tx1 + 0.85f, y1 - (y1 - y0) * 0.45f, mz + sw, T);
            if (!s.dark) {
                lmPushLightQuad(lv, Vec3(tx0, y1 - 0.22f, tz0 - 0.06f), Vec3(tx1, y1 - 0.22f, tz0 - 0.06f), Vec3(tx1, y1, tz0 - 0.06f), Vec3(tx0, y1, tz0 - 0.06f), true);
                lmPushLightQuad(lv, Vec3(tx0, y1 - 0.22f, tz1 + 0.06f), Vec3(tx1, y1 - 0.22f, tz1 + 0.06f), Vec3(tx1, y1, tz1 + 0.06f), Vec3(tx0, y1, tz1 + 0.06f), true);
                lmPushLightQuad(lv, Vec3(tx0 - 0.06f, y1 - 0.22f, tz0), Vec3(tx0 - 0.06f, y1 - 0.22f, tz1), Vec3(tx0 - 0.06f, y1, tz1), Vec3(tx0 - 0.06f, y1, tz0), true);
                lmPushLightQuad(lv, Vec3(tx1 + 0.06f, y1 - 0.22f, tz0), Vec3(tx1 + 0.06f, y1 - 0.22f, tz1), Vec3(tx1 + 0.06f, y1, tz1), Vec3(tx1 + 0.06f, y1, tz0), true);
            }
        }
        // Monumental sanctum shrine + blazing crown at the pyramid apex!
        lmPushBox(mav, mx - 2.6f, maxH, mz - 2.6f, mx + 2.6f, maxH + 2.8f, mz + 2.6f, T);
        if (!s.dark) lmPushLightBox(lv, mx, maxH + 0.4f, mz, 1.55f, 2.55f, 1.55f);
        return;
    }

    // Sub-mode 2/3: Steep 7..9-ring sunken terraced amphitheatre (-16..-25 m deep) with lit risers & floor!
    int nRings = lmClampInt(7 + (int)g[LG_ABYSS] * 2 / 256, 7, 9);
    float dropStep = 2.30f + (float)g[LG_ABYSS] * 0.65f / 255.0f;
    float cw = bx1 - bx0, ch = bz1 - bz0;
    for (int r = 0; r < nRings; r++) {
        float f0 = (float)r / (float)(nRings + 1) * 0.92f;
        float f1 = (float)(r + 1) / (float)(nRings + 1) * 0.92f;
        float rx0 = bx0 + cw * 0.46f * f0, rx1 = bx1 - cw * 0.46f * f0;
        float rz0 = bz0 + ch * 0.46f * f0, rz1 = bz1 - ch * 0.46f * f0;
        float ix0 = bx0 + cw * 0.46f * f1, ix1 = bx1 - cw * 0.46f * f1;
        float iz0 = bz0 + ch * 0.46f * f1, iz1 = bz1 - ch * 0.46f * f1;
        float yTop = -(float)r * dropStep, yBot = -(float)(r + 1) * dropStep;
        std::vector<float>& dst = ((r & 1) == 0) ? mv : mav;
        if (r < nRings - 1) {
            lmPushBox(dst, rx0, yBot, rz0, rx1, yTop, iz0, T);
            lmPushBox(dst, rx0, yBot, iz1, rx1, yTop, rz1, T);
            lmPushBox(dst, rx0, yBot, iz0, ix0, yTop, iz1, T);
            lmPushBox(dst, ix1, yBot, iz0, rx1, yTop, iz1, T);
            if (!s.dark) {
                lmPushLightQuad(lv, Vec3(ix0, yTop - 0.28f, iz0 + 0.05f), Vec3(ix1, yTop - 0.28f, iz0 + 0.05f), Vec3(ix1, yTop - 0.04f, iz0 + 0.05f), Vec3(ix0, yTop - 0.04f, iz0 + 0.05f), true);
                lmPushLightQuad(lv, Vec3(ix0, yTop - 0.28f, iz1 - 0.05f), Vec3(ix1, yTop - 0.28f, iz1 - 0.05f), Vec3(ix1, yTop - 0.04f, iz1 - 0.05f), Vec3(ix0, yTop - 0.04f, iz1 - 0.05f), true);
                lmPushLightQuad(lv, Vec3(ix0 + 0.05f, yTop - 0.28f, iz0), Vec3(ix0 + 0.05f, yTop - 0.28f, iz1), Vec3(ix0 + 0.05f, yTop - 0.04f, iz1), Vec3(ix0 + 0.05f, yTop - 0.04f, iz0), true);
                lmPushLightQuad(lv, Vec3(ix1 - 0.05f, yTop - 0.28f, iz0), Vec3(ix1 - 0.05f, yTop - 0.28f, iz1), Vec3(ix1 - 0.05f, yTop - 0.04f, iz1), Vec3(ix1 - 0.05f, yTop - 0.04f, iz0), true);
            }
        } else {
            lmPushQuad(dst, Vec3(rx0, yBot, rz1), Vec3(rx1, yBot, rz1), Vec3(rx1, yBot, rz0), Vec3(rx0, yBot, rz0), T);
            lmPushQuad(dst, Vec3(rx0, yBot, rz0), Vec3(rx0, yTop, rz0), Vec3(rx0, yTop, rz1), Vec3(rx0, yBot, rz1), T);
            lmPushQuad(dst, Vec3(rx1, yBot, rz1), Vec3(rx1, yTop, rz1), Vec3(rx1, yTop, rz0), Vec3(rx1, yBot, rz0), T);
            lmPushQuad(dst, Vec3(rx1, yBot, rz0), Vec3(rx1, yTop, rz0), Vec3(rx0, yTop, rz0), Vec3(rx0, yBot, rz0), T);
            lmPushQuad(dst, Vec3(rx0, yBot, rz1), Vec3(rx0, yTop, rz1), Vec3(rx1, yTop, rz1), Vec3(rx1, yBot, rz1), T);
            if (!s.dark) lmPushLightQuad(lv, Vec3(rx0+0.8f, yBot+0.08f, rz0+0.8f), Vec3(rx1-0.8f, yBot+0.08f, rz0+0.8f),
                                             Vec3(rx1-0.8f, yBot+0.08f, rz1-0.8f), Vec3(rx0+0.8f, yBot+0.08f, rz1-0.8f), true);
        }
    }
}

// Form 2 (MF_CANYON): narrow 7..12 storey gorge (20..25m cliff-to-cliff) with two-tone painted
// lower walls, 5..10 tiers of stacked balcony galleries with glowing portals, and multi-height bridges!
inline void lmBuildMegaCanyon(const LimenSlab& s, float T, std::vector<float>& mv, std::vector<float>& mav,
                              std::vector<float>& dv, std::vector<float>& lv) {
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float ceilY = s.ceilTiers * WH;
    const unsigned char* g = s.genome.g;
    int axis = (s.megaSub & 1);
    if ((axis == 0 ? s.h : s.w) < 9) axis ^= 1;

    int vMinC = 999, vMaxC = -1, vMinA = 999, vMaxA = -1;
    for (int lx = 0; lx < s.w; lx++) {
        for (int lz = 0; lz < s.h; lz++) {
            if (s.tag[lx][lz] == LT_VOID || (s.tag[lx][lz] == LT_BRIDGE && (s.flag[lx][lz] & LF_OVER_VOID))) {
                int a = (axis == 0) ? lx : lz, c = (axis == 0) ? lz : lx;
                if (c < vMinC) vMinC = c;
                if (c > vMaxC) vMaxC = c;
                if (a < vMinA) vMinA = a;
                if (a > vMaxA) vMaxA = a;
            }
        }
    }
    if (vMaxC < vMinC) return;
    float aLo = ((axis == 0 ? s.rect.x0 : s.rect.z0) + 1 + vMinA) * CS;
    float aHi = ((axis == 0 ? s.rect.x0 : s.rect.z0) + 2 + vMaxA) * CS;
    float cliffLo = ((axis == 0 ? s.rect.z0 : s.rect.x0) + vMinC) * CS;
    float cliffHi = ((axis == 0 ? s.rect.z0 : s.rect.x0) + 3 + vMaxC) * CS;
    if (cliffLo < (axis == 0 ? z0 : x0)) cliffLo = (axis == 0 ? z0 : x0);
    if (cliffHi > (axis == 0 ? z1 : x1)) cliffHi = (axis == 0 ? z1 : x1);

    // Two-tone painted lower cliff cladding (WH .. ceilY * 0.46) in contrasting mav pole!
    float bandTop = fmaxf(WH * 3.0f, ceilY * 0.45f);
    if (axis == 0) {
        lmPushQuad(mav, Vec3(aHi, WH, cliffLo + 0.06f), Vec3(aHi, bandTop, cliffLo + 0.06f), Vec3(aLo, bandTop, cliffLo + 0.06f), Vec3(aLo, WH, cliffLo + 0.06f), T);
        lmPushQuad(mav, Vec3(aLo, WH, cliffHi - 0.06f), Vec3(aLo, bandTop, cliffHi - 0.06f), Vec3(aHi, bandTop, cliffHi - 0.06f), Vec3(aHi, WH, cliffHi - 0.06f), T);
    } else {
        lmPushQuad(mav, Vec3(cliffLo + 0.06f, WH, aLo), Vec3(cliffLo + 0.06f, bandTop, aLo), Vec3(cliffLo + 0.06f, bandTop, aHi), Vec3(cliffLo + 0.06f, WH, aHi), T);
        lmPushQuad(mav, Vec3(cliffHi - 0.06f, WH, aHi), Vec3(cliffHi - 0.06f, bandTop, aHi), Vec3(cliffHi - 0.06f, bandTop, aLo), Vec3(cliffHi - 0.06f, WH, aLo), T);
    }

    {   // the cladding stands 6 cm proud of the cliff: close that standoff at both ends
        auto ret = [&](float ax, float az, float bx, float bz) {
            lmPushQuad(mav, Vec3(ax, WH, az), Vec3(ax, bandTop, az), Vec3(bx, bandTop, bz), Vec3(bx, WH, bz), T);
        };
        if (axis == 0) {
            ret(aLo, cliffLo + 0.06f, aLo, cliffLo); ret(aHi, cliffLo, aHi, cliffLo + 0.06f);
            ret(aLo, cliffHi, aLo, cliffHi - 0.06f); ret(aHi, cliffHi - 0.06f, aHi, cliffHi);
        } else {
            ret(cliffLo, aLo, cliffLo + 0.06f, aLo); ret(cliffLo + 0.06f, aHi, cliffLo, aHi);
            ret(cliffHi - 0.06f, aLo, cliffHi, aLo); ret(cliffHi, aHi, cliffHi - 0.06f, aHi);
        }
    }

    float cMid = 0.5f * (cliffLo + cliffHi);
    for (float a = aLo + 10.0f; a <= aHi - 8.0f; a += 14.0f) {
        if (axis == 0) lmPushBox(mv, a - 0.55f, ceilY - 2.4f, cliffLo, a + 0.55f, ceilY, cliffHi, T);
        else           lmPushBox(mv, cliffLo, ceilY - 2.4f, a - 0.55f, cliffHi, ceilY, a + 0.55f, T);
    }
    if (!s.dark) {
        if (axis == 0) lmPushLightQuad(lv, Vec3(aLo + 3.0f, ceilY - 0.08f, cMid - 1.6f), Vec3(aHi - 3.0f, ceilY - 0.08f, cMid - 1.6f),
                                           Vec3(aHi - 3.0f, ceilY - 0.08f, cMid + 1.6f), Vec3(aLo + 3.0f, ceilY - 0.08f, cMid + 1.6f), true);
        else           lmPushLightQuad(lv, Vec3(cMid - 1.6f, ceilY - 0.08f, aLo + 3.0f), Vec3(cMid + 1.6f, ceilY - 0.08f, aLo + 3.0f),
                                           Vec3(cMid + 1.6f, ceilY - 0.08f, aHi - 3.0f), Vec3(cMid - 1.6f, ceilY - 0.08f, aHi - 3.0f), true);
    }

    // Stacked cliffside balcony galleries (-3..ceilTiers-1) with rhythmic glowing portals!
    float balProj = 1.65f;
    int litChance = s.dark ? 15 : (115 + (int)g[LG_LIGHT] / 2);
    for (int st = -3; st < s.ceilTiers; st++) {
        if (st == 0) continue;
        float y = (float)st * WH;
        if (y > ceilY - 3.6f) break;
        for (int side = 0; side < 2; side++) {
            u32 hs = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)((st + 8) * 3 + side), 0x4342u);
            float cWall = (side == 0) ? cliffLo : cliffHi;
            float dirIn = (side == 0) ? 1.0f : -1.0f;
            float cEdge = cWall + dirIn * balProj;
            float cMin = cWall < cEdge ? cWall : cEdge, cMax = cWall > cEdge ? cWall : cEdge;
            bool severed = ((int)(hs % 500u) < (int)g[LG_DECAY]);
            float segHi = severed ? (aLo + (aHi - aLo) * 0.65f) : aHi;
            if (axis == 0) {
                lmPushBox(mv, aLo, y - 0.34f, cMin, segHi, y, cMax, T);
                lmPushBox(mav, aLo, y, cEdge - 0.14f, segHi, y + 0.92f, cEdge + 0.14f, T);
            } else {
                lmPushBox(mv, cMin, y - 0.34f, aLo, cMax, y, segHi, T);
                lmPushBox(mav, cEdge - 0.14f, y, aLo, cEdge + 0.14f, y + 0.92f, segHi, T);
            }
            if (st >= 1) {
                int idx = 0;
                for (float a = aLo + 4.5f; a <= segHi - 4.5f; a += 7.5f, idx++) {
                    u32 hp = lmHash(hs, (u32)idx, 5u, 0x4350u);
                    bool lit = ((int)(hp % 256u) < litChance);
                    float wOff = cWall + dirIn * 0.10f;
                    if (lit) {
                        if (axis == 0) lmPushLightQuad(lv, Vec3(a - 0.72f, y + 0.95f, wOff), Vec3(a + 0.72f, y + 0.95f, wOff), Vec3(a + 0.72f, y + 2.65f, wOff), Vec3(a - 0.72f, y + 2.65f, wOff), true);
                        else           lmPushLightQuad(lv, Vec3(wOff, y + 0.95f, a - 0.72f), Vec3(wOff, y + 0.95f, a + 0.72f), Vec3(wOff, y + 2.65f, a + 0.72f), Vec3(wOff, y + 2.65f, a - 0.72f), true);
                    } else {
                        if (axis == 0) lmPushDecorQuadRaw(dv, Vec3(a - 0.72f, y + 0.10f, wOff), Vec3(a + 0.72f, y + 0.10f, wOff), Vec3(a + 0.72f, y + 2.55f, wOff), Vec3(a - 0.72f, y + 2.55f, wOff), Vec3(0, 0, dirIn), DECOR_MAT_DOOR);
                        else           lmPushDecorQuadRaw(dv, Vec3(wOff, y + 0.10f, a - 0.72f), Vec3(wOff, y + 0.10f, a + 0.72f), Vec3(wOff, y + 2.55f, a + 0.72f), Vec3(wOff, y + 2.55f, a - 0.72f), Vec3(dirIn, 0, 0), DECOR_MAT_DOOR);
                    }
                }
            }
        }
    }

    // 5..8 staggered overhead bridges at distinct heights (+5.8m, +11.2m, +17.0m, +23.5m, +30.0m, -8.5m)
    const float bridgeHeights[6] = {5.8f, 11.2f, 17.0f, 23.5f, 30.0f, -8.5f};
    for (int k = 0; k < 6; k++) {
        float by = bridgeHeights[k];
        if (by > ceilY - 4.0f) continue;
        u32 hb = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(50 + k), 0x4343u);
        float frac = 0.16f + 0.14f * (float)k + ((float)(hb % 20u) - 10.0f) * 0.005f;
        if (frac > 0.86f) frac -= 0.70f;
        float ba = aLo + (aHi - aLo) * frac;
        float cEnd = (k >= 2 && (int)((hb >> 14) % 360u) < (int)g[LG_DECAY]) ? cMid : cliffHi;
        if (axis == 0) {
            lmPushBox(mav, ba - 1.35f, by - 0.75f, cliffLo, ba + 1.35f, by, cEnd, T);
            lmPushBox(mv,  ba - 1.35f, by, cliffLo, ba - 1.12f, by + 0.95f, cEnd, T);
            lmPushBox(mv,  ba + 1.12f, by, cliffLo, ba + 1.35f, by + 0.95f, cEnd, T);
            if (!s.dark && by > 0.0f) lmPushLightQuad(lv, Vec3(ba-0.35f, by-0.80f, cliffLo+2.0f), Vec3(ba+0.35f, by-0.80f, cliffLo+2.0f), Vec3(ba+0.35f, by-0.80f, cEnd-2.0f), Vec3(ba-0.35f, by-0.80f, cEnd-2.0f), true);
        } else {
            lmPushBox(mav, cliffLo, by - 0.75f, ba - 1.35f, cEnd, by, ba + 1.35f, T);
            lmPushBox(mv,  cliffLo, by, ba - 1.35f, cEnd, by + 0.95f, ba - 1.12f, T);
            lmPushBox(mv,  cliffLo, by, ba + 1.12f, cEnd, by + 0.95f, ba + 1.35f, T);
            if (!s.dark && by > 0.0f) lmPushLightQuad(lv, Vec3(cliffLo+2.0f, by-0.80f, ba-0.35f), Vec3(cEnd-2.0f, by-0.80f, ba-0.35f), Vec3(cEnd-2.0f, by-0.80f, ba+0.35f), Vec3(cliffLo+2.0f, by-0.80f, ba+0.35f), true);
        }
    }
}
