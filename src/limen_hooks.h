#pragma once
// Glue between LIMEN and the unmodified game sources: chunk rasterization (`0/1/2` pit-safe
// cells), deterministic lighting, thin-wall / narrow-corridor / basin-stair collisions,
// and live `LimenSite` activation behind the player's back.
#include "limen_world.h"
#include "limen_narrator.h"

const int LM_VIEW_CHUNKS = 3;   // 7x7 chunks (560x560 m) so megastructures & fog vistas stay seamless
const float LM_NARROW_INSET = 1.25f;   // 5.0 m - 2 * 1.25 m = 2.50 m human-scale corridor width
const float LM_THIN_HALF = 0.13f;      // 0.26 m drywall partition thickness
const float LM_THIN_DOOR_HALF = 0.68f; // 1.36 m clear doorway opening in thin partitions

void lmPostBuildGeom();
int lmGatherLightsAndDrawMega(const std::vector<Light>& lights, const Vec3& camPos, float* outPos);
bool lmCollideVestiges(float x, float z, float PR);

// True if basin cell (bx, bz) has a staircase whose upper y=0 rim lies at (bx + rdx, bz + rdz).
inline bool lmBasinStairOpensToward(const LimenSlab& s, int bx, int bz, int rdx, int rdz) {
    if (lmTagLocal(s, bx, bz) != LT_BASIN) return false;
    unsigned short f = lmFlagLocal(s, bx, bz);
    if ((f & LF_STAIR_DESC) == 0) return false;
    int dir = lmStairRimDir(f); // 0=(0,-1), 1=(0,+1), 2=(-1,0), 3=(+1,0)
    static const int dx[4] = {0, 0, -1, 1}, dz[4] = {-1, 1, 0, 0};
    return dx[dir] == rdx && dz[dir] == rdz;
}

inline void generateChunk(int cx, int cz) {
    lmReset(worldSeed, gCurrentLevel);
    long long key = chunkKey(cx, cz);
    if (chunks.find(key) != chunks.end()) return;
    Chunk c{};
    c.cx = cx; c.cz = cz; c.gen = true;
    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            c.cells[lx][lz] = lmCell(cx * CHUNK_SIZE + lx, cz * CHUNK_SIZE + lz);
        }
    }
    if (!gCellOverrides.empty()) {
        for (int lx = 0; lx < CHUNK_SIZE; lx++) {
            for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                auto ov = gCellOverrides.find(cellKey(cx * CHUNK_SIZE + lx, cz * CHUNK_SIZE + lz));
                if (ov != gCellOverrides.end()) c.cells[lx][lz] = ov->second;
            }
        }
    }
    chunks[key] = c;
}

inline void updateVisibleChunks(float px, float pz) {
    lmReset(worldSeed, gCurrentLevel);
    if (gLmStorySync) gLmStorySync();
    int pcx = (int)floorf(px / (CS * CHUNK_SIZE)), pcz = (int)floorf(pz / (CS * CHUNK_SIZE));
    for (int dx = -LM_VIEW_CHUNKS; dx <= LM_VIEW_CHUNKS; dx++) {
        for (int dz = -LM_VIEW_CHUNKS; dz <= LM_VIEW_CHUNKS; dz++) generateChunk(pcx + dx, pcz + dz);
    }
    std::vector<long long> rm;
    for (auto& p : chunks) {
        int ax = abs(p.second.cx - pcx), az = abs(p.second.cz - pcz);
        if ((ax > az ? ax : az) > LM_VIEW_CHUNKS + 1) rm.push_back(p.first);
    }
    for (auto k : rm) chunks.erase(k);
    playerChunkX = pcx; playerChunkZ = pcz;
}

inline int chunkStyleAt(int cx, int cz) {
    lmReset(worldSeed, gCurrentLevel);
    LimenRect r = lmSlabAt(cx * CHUNK_SIZE + CHUNK_SIZE / 2, cz * CHUNK_SIZE + CHUNK_SIZE / 2);
    const LimenSlab& s = lmSlab(r.j, r.i);
    int roomy = s.share[LT_ROOM] + s.share[LT_LOW], open = s.share[LT_HALL];
    if (roomy >= 30) return CHUNK_STYLE_OFFICE;
    if (roomy >= 12) return CHUNK_STYLE_MIXED;
    if (open >= 60 && !lmSlabIsVault(s) && gLimenDNA.wallSurf.grain > 135 && s.genome.g[LG_MASS] > 110) return CHUNK_STYLE_PARKING;
    return CHUNK_STYLE_HALLS;
}

// Exact underside soffit height of MF_HANGING's inverted stepped ceiling pyramid at (wxPos, wzPos).
inline float lmMegaHangingSoffitAt(const LimenSlab& s, float wxPos, float wzPos) {
    float ceilY = s.ceilTiers * WH;
    if (s.megaForm != MF_HANGING) return ceilY;
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float margin = (s.genome.g[LG_CELLS] > 165) ? (3.0f * CS) : (1.5f * CS);
    float px0 = x0 + margin, px1 = x1 - margin;
    float pz0 = z0 + margin, pz1 = z1 - margin;
    if (px1 - px0 < 20.0f || pz1 - pz0 < 20.0f) return ceilY;
    if (wxPos < px0 || wxPos > px1 || wzPos < pz0 || wzPos > pz1) return ceilY;
    int nSteps = lmClampInt(4 + s.ceilTiers / 2, 4, 8);
    float halfW = 0.5f * (px1 - px0), halfH = 0.5f * (pz1 - pz0);
    float midX = 0.5f * (px0 + px1), midZ = 0.5f * (pz0 + pz1);
    float nx = fabsf(wxPos - midX) / halfW;
    float nz = fabsf(wzPos - midZ) / halfH;
    float radFrac = nx > nz ? nx : nz; // 0 at center, 1 at outer step rim
    int stepIdx = (int)floorf((1.0f - radFrac) * (float)nSteps);
    if (stepIdx < 0) stepIdx = 0;
    if (stepIdx >= nSteps) stepIdx = nSteps - 1;
    const float nadirY = 2.65f;
    float topDrop = ceilY - 2.2f;
    float t = (nSteps > 1) ? ((float)stepIdx / (float)(nSteps - 1)) : 1.0f;
    return topDrop + (nadirY - topDrop) * t;
}

// Deterministic auxiliary vault pendants over sunken LT_BASIN pools and wide LT_VOID shafts
// (active only in non-dark vaults, giving sunken basins & chasms real pools of light and
// silhouettes while keeping core lmLampAt / limen_tool invariants unchanged).
inline int lmAuxVaultLampLocal(const LimenSlab& s, int slx, int slz, float* outY = nullptr) {
    if (!lmSlabIsVault(s) || s.dark || lmIsMegaUnstruct(s.megaForm)) return 0;
    int t = lmTagLocal(s, slx, slz);
    int p = s.lampPitch < 3 ? 3 : (s.lampPitch > 5 ? 5 : s.lampPitch);
    if (t == LT_BASIN) {
        if ((s.flag[slx][slz] & LF_STAIR_DESC) != 0) return 0;
        if (s.megaForm == MF_ZIGGURAT) return 0; // ziggurat/stair/amphitheatre uses its own step-lip lighting!
        int o = p / 2;
        if ((slx % p) != o || (slz % p) != o) return 0;
        u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(slx * 67 + slz), 0x424Cu);
        bool dead = ((int)(h % 256u) < (int)s.genome.g[LG_DECAY] / 4);
        float yL = 1.65f;
        if (s.megaForm == MF_ZIGGURAT) {
            yL = ((s.megaSub & 3) <= 1) ? fminf(s.ceilTiers * WH - 2.2f, 13.5f) : 4.20f;
        } else if (s.megaForm == MF_PLATFORM) {
            yL = 5.20f;
        }
        if (outY) *outY = yL;
        return dead ? 2 : 1;
    }
    if (t == LT_VOID) {
        int vp = p + 1;
        int vo = vp / 2;
        if ((slx % vp) != vo || (slz % vp) != vo) return 0;
        u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)(slx * 71 + slz), 0x564Cu);
        bool dead = ((int)(h % 256u) < (int)s.genome.g[LG_DECAY] / 4);
        if (outY) *outY = (s.megaForm == MF_CANYON || s.megaForm == MF_SILO) ? 6.40f : 3.85f;
        return dead ? 2 : 1;
    }
    return 0;
}

inline void updateLightsAndPillars(int pcx, int pcz) {
    lmReset(worldSeed, gCurrentLevel);
    lights.clear();
    pillars.clear();
    for (int dcx = -LM_VIEW_CHUNKS; dcx <= LM_VIEW_CHUNKS; dcx++) {
        for (int dcz = -LM_VIEW_CHUNKS; dcz <= LM_VIEW_CHUNKS; dcz++) {
            auto it = chunks.find(chunkKey(pcx + dcx, pcz + dcz));
            if (it == chunks.end()) continue;
            for (int lx = 0; lx < CHUNK_SIZE; lx++) {
                for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                    int wx = (pcx + dcx) * CHUNK_SIZE + lx, wz = (pcz + dcz) * CHUNK_SIZE + lz;
                    int tag = lmTagAt(wx, wz);
                    if (tag == LT_VITRINE && lmLampAt(wx, wz) == 1) {
                        Light vl;
                        vl.pos = Vec3((wx + 0.5f) * CS, WH - 0.04f, (wz + 0.5f) * CS);
                        vl.sizeX = vl.sizeZ = 1.15f;
                        vl.intensity = 1.0f;
                        vl.on = true;
                        lights.push_back(vl);
                        continue;
                    }
                    if (lmIsPitTag(tag)) {
                        LimenRect pr = lmSlabAt(wx, wz);
                        if (wx != pr.x0 && wz != pr.z0) {
                            const LimenSlab& ps = lmSlab(pr.j, pr.i);
                            float ay = 3.0f;
                            int al = lmAuxVaultLampLocal(ps, wx - pr.x0 - 1, wz - pr.z0 - 1, &ay);
                            if (al != 0) {
                                Light bl;
                                bl.pos = Vec3((wx + 0.5f) * CS, ay, (wz + 0.5f) * CS);
                                bl.sizeX = bl.sizeZ = 1.45f;
                                bl.intensity = 1.0f;
                                bl.on = (al == 1);
                                lights.push_back(bl);
                            }
                        }
                        continue;
                    }
                    if (it->second.cells[lx][lz] != 0) continue;
                    if (!lmIsWalkableTag(tag)) continue;
                    int lamp = lmLampAt(wx, wz);
                    if (lamp != 0) {
                        unsigned short flg = lmFlagAt(wx, wz);
                        unsigned char zn = lmZoneAt(wx, wz);
                        LimenRect r = lmSlabAt(wx, wz);
                        const LimenSlab& s = lmSlab(r.j, r.i);
                        float lxPos = (wx + 0.5f) * CS, lzPos = (wz + 0.5f) * CS;
                        float ly = lmShellCeilHeight(flg, s.ceilTiers, WH, zn) - 0.02f;
                        float sz = (flg & (LF_NARROW_X | LF_NARROW_Z)) ? 0.76f : 1.2f;
                        if (lmHighAt(wx, wz)) {
                            int slx = wx - r.x0 - 1, slz = wz - r.z0 - 1;
                            if (lmLampIsBollard(s, slx, slz)) {
                                ly = 2.15f; sz = 0.65f;
                                if (lmIsPitTag(lmTagLocal(s, slx - 1, slz))) lxPos = wx * CS + 0.65f;
                                else if (lmIsPitTag(lmTagLocal(s, slx + 1, slz))) lxPos = (wx + 1) * CS - 0.65f;
                                else if (lmIsPitTag(lmTagLocal(s, slx, slz - 1))) lzPos = wz * CS + 0.65f;
                                else if (lmIsPitTag(lmTagLocal(s, slx, slz + 1))) lzPos = (wz + 1) * CS - 0.65f;
                            } else {
                                float soffit = lmMegaHangingSoffitAt(s, lxPos, lzPos);
                                ly = (s.megaForm == MF_PLATFORM) ? 3.72f : fminf(5.60f, soffit - 0.14f);
                                sz = (s.megaForm == MF_PLATFORM) ? 1.35f : 1.65f;
                            }
                        }
                        Light l;
                        l.pos = Vec3(lxPos, ly, lzPos);
                        l.sizeX = l.sizeZ = sz;
                        l.intensity = 1.0f;
                        l.on = (lamp == 1);
                        lights.push_back(l);
                    }
                    if (lmPillarAt(wx, wz) && isOpenHallCell(wx, wz)) {
                        pillars.push_back(Vec3((wx + 0.5f) * CS, 0, (wz + 0.5f) * CS));
                    }
                }
            }
        }
    }
}

// The one description of a framed doorway cell, shared by the mesher, the collider and the
// head-clearance test so they can never drift apart. axis: 0 = none, 1 = passage along Z
// (frame spans X), 2 = passage along X.
struct LmDoorway {
    int axis = 0;
    bool drunk = false;      // out-of-scale delirium portal standing on the warped floor
    bool giant = false;
    float openHalf = 0.0f;   // post centre line, from the cell centre
    float openH = 0.0f;      // clear height above yBot
    float postW = 0.0f;
    float frameT = 0.0f;     // frame depth along the passage
    float yBot = 0.0f;
};

inline LmDoorway lmDoorwayAt(int wx, int wz) {
    LmDoorway d;
    if (getCellWorld(wx, wz) != 0) return d;
    LimenRect r = lmSlabAt(wx, wz);
    bool onSeam = (wx == r.x0 || wz == r.z0);
    const LimenSlab* sp = onSeam ? nullptr : &lmSlab(r.j, r.i);
    bool isMega = sp && lmSlabIsVault(*sp);
    int slx = onSeam ? -1 : (wx - r.x0 - 1), slz = onSeam ? -1 : (wz - r.z0 - 1);
    int tag = onSeam ? LT_CORR : sp->tag[slx][slz];
    if (isMega && !lmIsLowShellTag(tag)) return d;       // vault floors carry no framed portals
    unsigned short flg = onSeam ? 0u : sp->flag[slx][slz];
    unsigned char zn = onSeam ? 0u : sp->zone[slx][slz];
    if (flg & (LF_NARROW_X | LF_NARROW_Z)) return d;
    if ((zn & LZ_PLUG_CORR) != 0) return d;
    if (((zn | lmZoneAt(wx - 1, wz) | lmZoneAt(wx + 1, wz) |
          lmZoneAt(wx, wz - 1) | lmZoneAt(wx, wz + 1)) & LZ_XENO) != 0) return d;
    int axis = doorwayAxisAt(wx, wz);
    if (axis == 0 && !(gDoorwayVeto && gDoorwayVeto(wx, wz)) && (zn & LZ_STORY) == 0) {
        bool wL = (getCellWorld(wx - 1, wz) == 1), wR = (getCellWorld(wx + 1, wz) == 1);
        bool wB = (getCellWorld(wx, wz - 1) == 1), wF = (getCellWorld(wx, wz + 1) == 1);
        bool roomZ = (lmTagAt(wx, wz - 1) == LT_ROOM || lmTagAt(wx, wz + 1) == LT_ROOM);
        bool roomX = (lmTagAt(wx - 1, wz) == LT_ROOM || lmTagAt(wx + 1, wz) == LT_ROOM);
        if (wL && wR && !wB && !wF && ((isMega && tag == LT_LOW) || roomZ)) axis = 1;
        else if (wB && wF && !wL && !wR && ((isMega && tag == LT_LOW) || roomX)) axis = 2;
    }
    if (axis == 0) return d;
    d.axis = axis;
    float cH = lmShellCeilHeight(flg, sp ? sp->ceilTiers : 1, WH, zn);
    if (lmDeliriumCell255(wx, wz) >= 88 && cH >= WH - 0.05f) {
        u32 h = lmHash((u32)wx, (u32)wz, 5u, 0x4457u);
        d.drunk = true;
        d.giant = (h & 1u) != 0u;
        d.openHalf = d.giant ? 0.92f : 0.45f;   // 0.45 * 2 = 0.90 m slit
        d.openH = d.giant ? fminf(3.95f, cH - 0.35f) : 1.92f;
        d.postW = 0.13f;
        d.frameT = 0.40f;
        d.yBot = lmGroundHeightAt((wx + 0.5f) * CS, (wz + 0.5f) * CS, CS);
    } else {
        DoorwayDims dd = doorwayDims();
        d.openHalf = dd.openingHalf;
        d.openH = fminf(dd.openingH, cH - 0.40f);
        d.postW = dd.postW;
        d.frameT = dd.frameT;
    }
    return d;
}

// Lowest underside of any door lintel the circle stands under (1e9 if none): framed doorway
// cells and the 2.26 m headers of thin-partition doors.
inline float lmLintelBottomAt(float x, float z, float PR) {
    lmReset(worldSeed, gCurrentLevel);
    int wx = (int)floorf(x / CS), wz = (int)floorf(z / CS);
    float best = 1e9f;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int cx = wx + dx, cz = wz + dz;
            float x0 = cx * CS, z0 = cz * CS, mx = x0 + CS * 0.5f, mz = z0 + CS * 0.5f;
            LmDoorway d = lmDoorwayAt(cx, cz);
            if (d.axis != 0) {
                float a = d.openHalf, b = d.frameT * 0.5f;
                bool hit = (d.axis == 1) ? circleOverlapsAabb2D(x, z, PR, mx - a, mx + a, mz - b, mz + b)
                                         : circleOverlapsAabb2D(x, z, PR, mx - b, mx + b, mz - a, mz + a);
                if (hit) best = fminf(best, d.yBot + d.openH);
            }
            unsigned short f = lmFlagAt(cx, cz);
            const float thinDoorH = 2.26f;
            if ((f & LF_THIN_W) && (f & LF_THIN_DOOR_W) &&
                circleOverlapsAabb2D(x, z, PR, x0 - LM_THIN_HALF, x0 + LM_THIN_HALF, mz - LM_THIN_DOOR_HALF, mz + LM_THIN_DOOR_HALF))
                best = fminf(best, thinDoorH);
            if ((f & LF_THIN_N) && (f & LF_THIN_DOOR_N) &&
                circleOverlapsAabb2D(x, z, PR, mx - LM_THIN_DOOR_HALF, mx + LM_THIN_DOOR_HALF, z0 - LM_THIN_HALF, z0 + LM_THIN_HALF))
                best = fminf(best, thinDoorH);
        }
    }
    return best;
}

// Defined next to the camera (game.cpp): true if this query is the player on foot about to
// walk head-first into a lintel. Monsters and placement probes never trip it.
bool lmPlayerHeadBlocked(float x, float z, float PR);

// Walls, pillars and door frames as they are actually drawn (the game's own frame collider
// assumes one fixed portal in every single-cell wall gap, including where LIMEN draws none or
// an out-of-scale one), plus thin 0.26 m partitions, narrow 2.5 m corridor insets, vestige
// stairs/frames, vault parapets (except at LS_RAIL_GAP and basin stair mouths), sunken basin
// walls and door lintels at head height.
inline bool collideWorld(float x, float z, float PR) {
    lmReset(worldSeed, gCurrentLevel);
    {
        int wx = (int)floorf(x / CS), wz = (int)floorf(z / CS);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dz = -1; dz <= 1; dz++) {
                int cx = wx + dx, cz = wz + dz;
                float x0 = cx * CS, x1 = (cx + 1) * CS, z0 = cz * CS, z1 = (cz + 1) * CS;
                if (getCellWorld(cx, cz) == 1) {
                    if (circleOverlapsAabb2D(x, z, PR, x0, x1, z0, z1)) return true;
                    continue;
                }
                if (isForcedDoorway(cx, cz)) {
                    if (collideDoorFrameAtCell(x, z, PR, cx, cz)) return true;
                    continue;
                }
                LmDoorway d = lmDoorwayAt(cx, cz);
                if (d.axis == 0) continue;
                float mx = (x0 + x1) * 0.5f, mz = (z0 + z1) * 0.5f;
                float a = (d.axis == 1) ? (x - mx) : (z - mz);
                float b = (d.axis == 1) ? (z - mz) : (x - mx);
                // wide bodies (monsters) always fit: only the player has to thread a 0.9 m slit
                float inner = fmaxf(d.openHalf - d.postW * 0.5f, PR + 0.06f);
                float half = CS * 0.5f, fh = d.frameT * 0.5f;
                if (circleOverlapsAabb2D(a, b, PR, -half, -inner, -fh, fh)) return true;
                if (circleOverlapsAabb2D(a, b, PR, inner, half, -fh, fh)) return true;
            }
        }
        for (const auto& p : pillars)
            if (fabsf(x - p.x) < PILLAR_SIZE * 0.5f + PR && fabsf(z - p.z) < PILLAR_SIZE * 0.5f + PR) return true;
    }
    if (lmCollideVestiges(x, z, PR)) return true;
    if (lmPlayerHeadBlocked(x, z, PR)) return true;
    int wx = (int)floorf(x / CS), wz = (int)floorf(z / CS);

    // 1. Thin partitions, narrowed corridors & sealed LT_VITRINE boundary in 3x3 neighbourhood
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int cx = wx + dx, cz = wz + dz;
            if (lmTagAt(cx, cz) == LT_VITRINE &&
                circleOverlapsAabb2D(x, z, PR, cx * CS, (cx + 1) * CS, cz * CS, (cz + 1) * CS)) return true;
            unsigned short f = lmFlagAt(cx, cz);
            if (f == 0) continue;
            float x0 = cx * CS, x1 = (cx + 1) * CS, z0 = cz * CS, z1 = (cz + 1) * CS;
            if (f & LF_THIN_W) {
                if (f & LF_THIN_DOOR_W) {
                    float mid = z0 + CS * 0.5f;
                    if (circleOverlapsAabb2D(x, z, PR, x0 - LM_THIN_HALF, x0 + LM_THIN_HALF, z0, mid - LM_THIN_DOOR_HALF)) return true;
                    if (circleOverlapsAabb2D(x, z, PR, x0 - LM_THIN_HALF, x0 + LM_THIN_HALF, mid + LM_THIN_DOOR_HALF, z1)) return true;
                } else if (circleOverlapsAabb2D(x, z, PR, x0 - LM_THIN_HALF, x0 + LM_THIN_HALF, z0, z1)) return true;
            }
            if (f & LF_THIN_N) {
                if (f & LF_THIN_DOOR_N) {
                    float mid = x0 + CS * 0.5f;
                    if (circleOverlapsAabb2D(x, z, PR, x0, mid - LM_THIN_DOOR_HALF, z0 - LM_THIN_HALF, z0 + LM_THIN_HALF)) return true;
                    if (circleOverlapsAabb2D(x, z, PR, mid + LM_THIN_DOOR_HALF, x1, z0 - LM_THIN_HALF, z0 + LM_THIN_HALF)) return true;
                } else if (circleOverlapsAabb2D(x, z, PR, x0, x1, z0 - LM_THIN_HALF, z0 + LM_THIN_HALF)) return true;
            }
            if (f & LF_NARROW_X) {
                if (circleOverlapsAabb2D(x, z, PR, x0, x1, z0, z0 + LM_NARROW_INSET)) return true;
                if (circleOverlapsAabb2D(x, z, PR, x0, x1, z1 - LM_NARROW_INSET, z1)) return true;
            }
            if (f & LF_NARROW_Z) {
                if (circleOverlapsAabb2D(x, z, PR, x0, x0 + LM_NARROW_INSET, z0, z1)) return true;
                if (circleOverlapsAabb2D(x, z, PR, x1 - LM_NARROW_INSET, x1, z0, z1)) return true;
            }
        }
    }

    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return false;
    const LimenSlab& s = lmSlab(r.j, r.i);
    if (!lmSlabIsVault(s)) return false;
    int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
    int tag = s.tag[lx][lz];
    float x0 = wx * CS, x1 = (wx + 1) * CS, z0 = wz * CS, z1 = (wz + 1) * CS;

    // 2. Inside a walkable dry basin: keep player bounded by sheer 2.85 m basin walls except at stairs!
    if (tag == LT_BASIN && (s.flag[lx][lz] & LF_WALK_BASIN)) {
        const float margin = 0.24f;
        if (lmTagLocal(s, lx - 1, lz) != LT_BASIN && !lmBasinStairOpensToward(s, lx, lz, -1, 0) &&
            circleOverlapsAabb2D(x, z, PR, x0 - margin, x0 + margin, z0, z1)) return true;
        if (lmTagLocal(s, lx + 1, lz) != LT_BASIN && !lmBasinStairOpensToward(s, lx, lz, 1, 0) &&
            circleOverlapsAabb2D(x, z, PR, x1 - margin, x1 + margin, z0, z1)) return true;
        if (lmTagLocal(s, lx, lz - 1) != LT_BASIN && !lmBasinStairOpensToward(s, lx, lz, 0, -1) &&
            circleOverlapsAabb2D(x, z, PR, x0, x1, z0 - margin, z0 + margin)) return true;
        if (lmTagLocal(s, lx, lz + 1) != LT_BASIN && !lmBasinStairOpensToward(s, lx, lz, 0, 1) &&
            circleOverlapsAabb2D(x, z, PR, x0, x1, z1 - margin, z1 + margin)) return true;
        return false;
    }

    // 3. Parapets on y=0 walkable rims (open at LS_RAIL_GAP, walkable basin stair mouths, and natural unstructures!)
    if (!lmIsWalkableTag(tag) || lmIsMegaUnstruct(s.megaForm) || lmSlabHasSite(s, lx, lz, LS_RAIL_GAP)) return false;
    const float railT = 0.42f;
    if (lmIsPitTag(lmTagLocal(s, lx - 1, lz)) && !lmBasinStairOpensToward(s, lx - 1, lz, 1, 0) &&
        circleOverlapsAabb2D(x, z, PR, x0, x0 + railT, z0, z1)) return true;
    if (lmIsPitTag(lmTagLocal(s, lx + 1, lz)) && !lmBasinStairOpensToward(s, lx + 1, lz, -1, 0) &&
        circleOverlapsAabb2D(x, z, PR, x1 - railT, x1, z0, z1)) return true;
    if (lmIsPitTag(lmTagLocal(s, lx, lz - 1)) && !lmBasinStairOpensToward(s, lx, lz - 1, 0, 1) &&
        circleOverlapsAabb2D(x, z, PR, x0, x1, z0, z0 + railT)) return true;
    if (lmIsPitTag(lmTagLocal(s, lx, lz + 1)) && !lmBasinStairOpensToward(s, lx, lz + 1, 0, -1) &&
        circleOverlapsAabb2D(x, z, PR, x0, x1, z1 - railT, z1)) return true;
    return false;
}

// Live dynamic anomaly activation behind the player's back using stable `LimenSite::id`!
inline bool reshuffleBehind(float px, float pz, float yaw) {
    lmReset(worldSeed, gCurrentLevel);
    float fwdX = sinf(yaw), fwdZ = cosf(yaw);
    int wx = (int)floorf(px / CS), wz = (int)floorf(pz / CS);
    LimenRect cur = lmSlabAt(wx, wz);

    for (int dj = -1; dj <= 1; dj++) {
        for (int di = -1; di <= 1; di++) {
            const LimenSlab& s = lmSlab(cur.j + dj, cur.i + di);
            for (int k = 0; k < s.siteCount; k++) {
                const LimenSite& site = s.sites[k];
                if (site.type != LS_DYN_LAMP || lmIsSiteTriggered(site.id)) continue;
                float sx = (s.rect.x0 + 1 + site.lx + 0.5f) * CS;
                float sz = (s.rect.z0 + 1 + site.lz + 0.5f) * CS;
                float dx = sx - px, dz = sz - pz;
                float d2 = dx * dx + dz * dz;
                if (d2 < 144.0f || d2 > 1764.0f) continue;   // 12 m .. 42 m away
                float invD = 1.0f / sqrtf(d2);
                if (dx * invD * fwdX + dz * invD * fwdZ > -0.35f) continue;   // strictly behind view cone
                if (!lmTriggerSite(site.id)) continue;
                updateLightsAndPillars(playerChunkX, playerChunkZ);
                return true;
            }
        }
    }
    return false;
}
