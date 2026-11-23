#pragma once
// LIMEN world queries: cells, lamps, pillars, ground elevation, and LRU slab cache.
// Pure world queries are deterministic functions of (seed, level, cell); session-level
// `gLmActiveSites` holds optional behind-the-back triggered `LimenSite` IDs.
#include "limen_build.h"
#include "limen_prof.h"
#include <cmath>
#include <unordered_map>
#include <vector>
#include <algorithm>

inline std::unordered_map<long long, LimenSlab> gLimenSlabs;
inline std::unordered_map<long long, unsigned long long> gLimenSlabUse;
inline unsigned long long gLimenUseClock = 0;
// Cell queries come in long runs over one slab; the last slab answered skips both hash lookups.
inline const LimenSlab* gLimenLastSlab = nullptr;
inline long long gLimenLastSlabKey = 0;

// Session bitset/list of dynamically triggered LimenSite IDs (e.g. lamps extinguished behind player).
inline std::vector<u32> gLmActiveSites;
inline unsigned int gLmSiteEpoch = 1u;

// ---- what the game does to the layout ------------------------------------------------------
// The game stamps sets into the maze (the garage and its portal), bores the rig's lane through
// it cell by cell and lets players break walls. It only knows "cell open / cell solid"; LIMEN's
// cells carry much more (thin partitions, narrowed corridors, low ceilings, pits, vault mass,
// megastructures). So those edits are folded into the slab itself right after it is grown, and
// every consumer (meshers, colliders, props, sound) sees one consistent world:
//   edits       cells the game opened (0) or closed (1)
//   lane        the row of cells the rig will crawl along, known from the moment it is put down:
//               kept level, full height and bridged over pits before anybody gets to see it
//   clear       rectangles of a stamped set (same treatment)
// A slab the lane or a set runs through also gives up its megastructure and delirium set pieces.
struct LmStory {
    std::unordered_map<long long, int> edits;
    bool lane = false;
    int laneX = 0, laneZ = 0, laneDX = 0, laneDZ = 0;
    int nClear = 0;
    int clear[12][4] = {};  // x0, z0, x1, z1 (inclusive, world cells)
    int nSet = 0;           // the first nSet rectangles are walled, roofed sets with their own interior
};
inline LmStory gLmStory;
inline void (*gLmStorySync)() = nullptr;     // set by the game build: refreshes gLmStory from the game's state
inline std::unordered_map<long long, unsigned int> gLmStorySlabEpoch;   // bumped when a slab's edits change
inline unsigned int gLmStoryEpoch = 1u;                                  // bumped when lane / sets change

inline long long lmCellKey64(int wx, int wz) { return ((long long)wx << 32) | (wz & 0xFFFFFFFF); }
inline int lmStoryEdit(int wx, int wz) {
    if (gLmStory.edits.empty()) return -1;
    auto it = gLmStory.edits.find(lmCellKey64(wx, wz));
    return it == gLmStory.edits.end() ? -1 : it->second;
}
inline bool lmStoryLaneCell(int wx, int wz) {
    const LmStory& st = gLmStory;
    if (!st.lane) return false;
    int ox = wx - st.laneX, oz = wz - st.laneZ;
    if (ox * st.laneDZ - oz * st.laneDX != 0) return false;
    return ox * st.laneDX + oz * st.laneDZ >= -1;
}
inline bool lmStoryInClearRect(int wx, int wz) {
    const LmStory& st = gLmStory;
    for (int k = 0; k < st.nClear; k++)
        if (wx >= st.clear[k][0] && wx <= st.clear[k][2] && wz >= st.clear[k][1] && wz <= st.clear[k][3]) return true;
    return false;
}
inline bool lmStoryInSet(int wx, int wz) {
    const LmStory& st = gLmStory;
    for (int k = 0; k < st.nSet && k < st.nClear; k++)
        if (wx >= st.clear[k][0] && wx <= st.clear[k][2] && wz >= st.clear[k][1] && wz <= st.clear[k][3]) return true;
    return false;
}
inline bool lmStoryKeepClear(int wx, int wz) { return lmStoryLaneCell(wx, wz) || lmStoryInClearRect(wx, wz); }
inline unsigned int lmStoryEpochOf(long long slabKey) {
    auto it = gLmStorySlabEpoch.find(slabKey);
    return gLmStoryEpoch * 65536u + (it == gLmStorySlabEpoch.end() ? 0u : it->second);
}

inline void lmApplyStory(LimenSlab& s) {
    const LmStory& st = gLmStory;
    if (st.edits.empty() && !st.lane && st.nClear == 0) return;
    const bool vault = lmSlabIsVault(s);
    const unsigned short thinW = LF_THIN_W | LF_THIN_DOOR_W, thinN = LF_THIN_N | LF_THIN_DOOR_N;
    auto dropSites = [&](int lx, int lz) {
        int n = 0;
        for (int k = 0; k < s.siteCount; k++) {
            if (s.sites[k].lx == lx && s.sites[k].lz == lz) continue;
            s.sites[n++] = s.sites[k];
        }
        s.siteCount = n;
        if (s.specialLampX == lx && s.specialLampZ == lz) { s.specialLampX = s.specialLampZ = -1; }
    };
    // 1. the rig's lane and stamped sets: plain from the start
    bool touched = false;
    for (int lx = 0; lx < s.w; lx++) {
        for (int lz = 0; lz < s.h; lz++) {
            int wx = s.rect.x0 + 1 + lx, wz = s.rect.z0 + 1 + lz;
            bool lane = lmStoryLaneCell(wx, wz);
            if (!lane && !lmStoryInClearRect(wx, wz)) continue;
            touched = true;
            int t = s.tag[lx][lz];
            unsigned short& f = s.flag[lx][lz];
            if (lmIsPitTag(t)) {
                s.tag[lx][lz] = LT_BRIDGE;
                f = (t == LT_VOID) ? LF_OVER_VOID : LF_OVER_BASIN;
            } else if (lmIsWalkableTag(t)) {
                f &= (unsigned short)(LF_OVER_VOID | LF_OVER_BASIN | thinW | thinN);
                if (lane) f &= (unsigned short)~(st.laneDX != 0 ? thinW : thinN);   // nothing across the lane
            }
            s.zone[lx][lz] = (unsigned char)((s.zone[lx][lz] & LZ_XENO) | LZ_STORY);
            dropSites(lx, lz);
        }
    }
    if (touched && vault) {
        // A low room the lane cuts through loses its low ceiling as a whole: the slab mesher has
        // no beam for a ceiling step in the middle of a shell room (it left a sheet of wall there).
        static short qx[LM_MAX * LM_MAX], qz[LM_MAX * LM_MAX];
        int head = 0, tail = 0;
        for (int lx = 0; lx < s.w; lx++) for (int lz = 0; lz < s.h; lz++)
            if ((s.zone[lx][lz] & LZ_STORY) && lmIsLowShellTag(s.tag[lx][lz])) { qx[tail] = (short)lx; qz[tail] = (short)lz; tail++; }
        static const int ddx[4] = {1, -1, 0, 0}, ddz[4] = {0, 0, 1, -1};
        while (head < tail) {
            int x = qx[head], z = qz[head]; head++;
            for (int d = 0; d < 4; d++) {
                int nx = x + ddx[d], nz = z + ddz[d];
                if (nx < 0 || nz < 0 || nx >= s.w || nz >= s.h) continue;
                if (!lmIsLowShellTag(s.tag[nx][nz]) || s.tag[nx][nz] == LT_VITRINE || (s.flag[nx][nz] & LF_CRAMPED) == 0) continue;
                s.flag[nx][nz] &= (unsigned short)~LF_CRAMPED;
                qx[tail] = (short)nx; qz[tail] = (short)nz; tail++;
            }
        }
    }
    if (touched) {
        // (a megastructure the narrator laid out around this very road keeps standing, and so
        //  does a nested / spiral structure it stamped beside the road)
        if (s.megaForm > MF_CLASSIC && !s.roadThrough) { s.megaForm = MF_CLASSIC; s.megaSub = 0; }
        if (s.stampX0 < 0) { s.nestedRings = 0; s.spiralW = 0; s.spiralH = 0; s.deliriumStruct = 0; }
    }
    // 2. cells the game opened or closed
    for (const auto& kv : st.edits) {
        int wx = (int)(kv.first >> 32), wz = (int)(kv.first & 0xFFFFFFFF);
        int lx = wx - s.rect.x0 - 1, lz = wz - s.rect.z0 - 1;
        if (lx < 0 || lz < 0 || lx >= s.w || lz >= s.h) continue;
        int t = s.tag[lx][lz];
        unsigned short f = s.flag[lx][lz];
        if (kv.second == 0) {
            int nt = t;
            unsigned short nf = 0;
            if (t == LT_WALL || t == LT_VITRINE) nt = vault ? LT_LOW : LT_CORR;
            else if (lmIsPitTag(t)) { nt = LT_BRIDGE; nf = (t == LT_VOID) ? LF_OVER_VOID : LF_OVER_BASIN; }
            else if (t == LT_BRIDGE) nf = f & (LF_OVER_VOID | LF_OVER_BASIN);
            if (vault && lmStoryInSet(wx, wz)) { if (!lmIsLowShellTag(nt)) nt = LT_LOW; nf = 0; }   // a set is a roofed pavilion
            s.tag[lx][lz] = (unsigned char)nt;
            s.flag[lx][lz] = nf;
            // the wings of a narrowed corridor next door would stand across the new opening
            if (lx > 0)       s.flag[lx - 1][lz] &= (unsigned short)~LF_NARROW_Z;
            if (lx + 1 < s.w) s.flag[lx + 1][lz] &= (unsigned short)~(LF_NARROW_Z | thinW);
            if (lz > 0)       s.flag[lx][lz - 1] &= (unsigned short)~LF_NARROW_X;
            if (lz + 1 < s.h) s.flag[lx][lz + 1] &= (unsigned short)~(LF_NARROW_X | thinN);
        } else {
            if (!(t == LT_WALL && (f & LF_LOW_WALL) == 0)) {
                s.tag[lx][lz] = LT_WALL;
                s.flag[lx][lz] = vault ? (unsigned short)LF_LOW_WALL : (unsigned short)0;
            }
            if (lx + 1 < s.w) s.flag[lx + 1][lz] &= (unsigned short)~thinW;
            if (lz + 1 < s.h) s.flag[lx][lz + 1] &= (unsigned short)~thinN;
        }
        s.zone[lx][lz] = (unsigned char)((s.zone[lx][lz] & LZ_XENO) | LZ_STORY);
        dropSites(lx, lz);
    }
    // 3. set pieces grown from a site reach a cell or two beyond it (stairs into the ceiling,
    //    orphan frames, plugged corridors): none of them next to a story cell
    {
        int n = 0;
        for (int k = 0; k < s.siteCount; k++) {
            const LimenSite& q = s.sites[k];
            bool keep = true;
            if (q.type != LS_DEAD_LAMP && q.type != LS_DARK_SLAB && q.type != LS_DYN_LAMP && q.type != LS_XENOGRAFT) {
                for (int dx = -2; dx <= 2 && keep; dx++) for (int dz = -2; dz <= 2 && keep; dz++) {
                    int x = q.lx + dx, z = q.lz + dz;
                    if (x >= 0 && z >= 0 && x < s.w && z < s.h && (s.zone[x][z] & LZ_STORY)) keep = false;
                }
            }
            if (keep) s.sites[n++] = s.sites[k];
        }
        s.siteCount = n;
    }
}

// The only way to empty the slab cache: `gLimenLastSlab` points into it.
inline void lmSlabCacheClear() {
    gLimenLastSlab = nullptr;
    gLimenSlabs.clear();
    gLimenSlabUse.clear();
}

inline bool lmIsSiteTriggered(u32 id) {
    for (u32 v : gLmActiveSites) if (v == id) return true;
    return false;
}

inline bool lmTriggerSite(u32 id) {
    if (lmIsSiteTriggered(id)) return false;
    gLmActiveSites.push_back(id);
    gLmSiteEpoch++;
    return true;
}

inline void lmReset(u32 seed, int level) {
    if (gLimen.dnaReady && gLimen.seed == seed && gLimen.level == level) return;
    lmApplyLevelDNA(seed, level);
    lmBeatsClear();
    lmSlabCacheClear();
    gLmActiveSites.clear();
    gLmSiteEpoch++;
}

inline bool lmLampLocal(const LimenSlab& s, int lx, int lz);

// Lighting deviations need the finished lamp lattice, so they are placed last.
// Also seeds up to 3 LS_DYN_LAMP candidates that `reshuffleBehind` can quietly kill.
inline void lmPlaceLampSites(LimenSlab& s) {
    if (s.fallback || lmIsMegaUnstruct(s.megaForm)) return;
    int decay = s.genome.g[LG_DECAY];
    u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, 3u, 0x414Eu);
    int count = 0;
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) if (lmLampLocal(s, x, z)) count++;
    if (count < 4) return;

    int deadChance = decay * 70 / 255 + (gLimenDNA.lightDensity < 80 ? 20 : 0);
    int darkChance = (int)gLimenDNA.darkSlabChance + decay / 28;
    if (lmDepth(s.rect.j, s.rect.i) == 0 && gLimenDNA.originCalmRings > 0) darkChance = 0;
    bool dark = (s.beatForce & LBF_DARK) != 0u || (int)(h % 100u) < darkChance;
    bool dead = !dark && (int)((h >> 8) % 100u) < deadChance;
    if (dark || dead) {
        int pick = (int)((h >> 16) % (u32)count);
        for (int x = 0; x < s.w && pick >= 0; x++) {
            for (int z = 0; z < s.h; z++) {
                if (!lmLampLocal(s, x, z)) continue;
                if (pick-- != 0) continue;
                s.specialLampX = x; s.specialLampZ = z;
                s.dark = dark;
                lmAddSite(s, dark ? LS_DARK_SLAB : LS_DEAD_LAMP, x, z);
                break;
            }
        }
    }

    if (!s.dark && count >= 6) {
        for (int k = 0; k < 3 && s.siteCount < LM_MAX_SITES; k++) {
            int pick = (int)(lmHash(h, (u32)k, 9u, 0x4459u) % (u32)count);
            for (int x = 0; x < s.w && pick >= 0; x++) {
                for (int z = 0; z < s.h; z++) {
                    if (!lmLampLocal(s, x, z)) continue;
                    if (pick-- != 0) continue;
                    if (x != s.specialLampX || z != s.specialLampZ) lmAddSite(s, LS_DYN_LAMP, x, z);
                    break;
                }
            }
        }
    }
}

// LRU slab cache: evicts only the oldest quarter when full instead of dropping everything.
inline const LimenSlab& lmSlab(int j, int i) {
    long long key = (long long)(((unsigned long long)(u32)j << 32) | (unsigned long long)(u32)i);
    if (gLimenLastSlab && key == gLimenLastSlabKey) return *gLimenLastSlab;
    auto it = gLimenSlabs.find(key);
    if (it != gLimenSlabs.end()) {
        gLimenSlabUse[key] = ++gLimenUseClock;
        gLimenLastSlab = &it->second; gLimenLastSlabKey = key;
        return it->second;
    }
    gLimenLastSlab = nullptr;   // the build below recurses into neighbours and may evict
    if (gLimenSlabs.size() >= 224) {
        std::vector<std::pair<unsigned long long, long long>> ages;
        ages.reserve(gLimenSlabs.size());
        for (const auto& kv : gLimenSlabs) ages.push_back({gLimenSlabUse[kv.first], kv.first});
        std::nth_element(ages.begin(), ages.begin() + 56, ages.end());
        for (int k = 0; k < 56; k++) {
            gLimenSlabs.erase(ages[k].second);
            gLimenSlabUse.erase(ages[k].second);
        }
    }
    LimenSlab& s = gLimenSlabs[key];
    gLimenSlabUse[key] = ++gLimenUseClock;
    lmBuildSlab(s, j, i);
    lmPlaceLampSites(s);
    lmApplyStory(s);
    gLimenLastSlab = &s; gLimenLastSlabKey = key;
    return s;
}

inline int lmTagAt(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) {
        int e = lmStoryEdit(wx, wz);   // the rig bores through seam walls too
        if (e >= 0) return e == 0 ? LT_CORR : LT_WALL;
        return lmWallCellOpen(r, wx, wz) ? LT_CORR : LT_WALL;
    }
    return lmSlab(r.j, r.i).tag[wx - r.x0 - 1][wz - r.z0 - 1];
}

inline unsigned short lmFlagAt(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return 0;
    return lmSlab(r.j, r.i).flag[wx - r.x0 - 1][wz - r.z0 - 1];
}
