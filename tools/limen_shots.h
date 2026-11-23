#pragma once
// Frame grabber for the LIMEN world synthesizer. Walks outward from the origin and
// photographs slabs with novel measured make-up, plus specific geometric motifs (severed
// spans with EXIT gantries, walkable basin staircases, human-scale corridors, mezzanines).
#include <direct.h>
#include "limen_trial_shot.h"

struct ShotCam { float x, z, yaw, pitch; bool ok; };

static const int shotDirX[4] = {0, 1, 0, -1}, shotDirZ[4] = {1, 0, -1, 0};

// True if world point (px, pz) hits a solid wall, sealed vitrine, thin drywall partition,
// narrowed corridor wing, or cached vestige AABB within radius `rad`.
static bool shotPointHitsSightBlocker(float px, float pz, float rad, const std::vector<LimenAabb2D>& cols) {
    int wx = (int)floorf(px / CS), wz = (int)floorf(pz / CS);
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            int cx = wx + dx, cz = wz + dz;
            int t = lmTagAt(cx, cz);
            float x0 = cx * CS, x1 = (cx + 1) * CS, z0 = cz * CS, z1 = (cz + 1) * CS;
            if ((t == LT_WALL || t == LT_VITRINE) && circleOverlapsAabb2D(px, pz, rad, x0, x1, z0, z1)) return true;
            if (lmHighAt(cx, cz)) continue;
            unsigned short f = lmFlagAt(cx, cz);
            if (f == 0) continue;
            if (f & LF_THIN_W) {
                if (f & LF_THIN_DOOR_W) {
                    float mid = z0 + CS * 0.5f;
                    if (circleOverlapsAabb2D(px, pz, rad, x0 - LM_THIN_HALF, x0 + LM_THIN_HALF, z0, mid - LM_THIN_DOOR_HALF)) return true;
                    if (circleOverlapsAabb2D(px, pz, rad, x0 - LM_THIN_HALF, x0 + LM_THIN_HALF, mid + LM_THIN_DOOR_HALF, z1)) return true;
                } else if (circleOverlapsAabb2D(px, pz, rad, x0 - LM_THIN_HALF, x0 + LM_THIN_HALF, z0, z1)) return true;
            }
            if (f & LF_THIN_N) {
                if (f & LF_THIN_DOOR_N) {
                    float mid = x0 + CS * 0.5f;
                    if (circleOverlapsAabb2D(px, pz, rad, x0, mid - LM_THIN_DOOR_HALF, z0 - LM_THIN_HALF, z0 + LM_THIN_HALF)) return true;
                    if (circleOverlapsAabb2D(px, pz, rad, mid + LM_THIN_DOOR_HALF, x1, z0 - LM_THIN_HALF, z0 + LM_THIN_HALF)) return true;
                } else if (circleOverlapsAabb2D(px, pz, rad, x0, x1, z0 - LM_THIN_HALF, z0 + LM_THIN_HALF)) return true;
            }
            if ((lmZoneAt(cx, cz) & LZ_PLUG_CORR) == 0) {
                if ((f & LF_NARROW_X) &&
                    (circleOverlapsAabb2D(px, pz, rad, x0, x1, z0, z0 + LM_NARROW_INSET) ||
                     circleOverlapsAabb2D(px, pz, rad, x0, x1, z1 - LM_NARROW_INSET, z1))) return true;
                if ((f & LF_NARROW_Z) &&
                    (circleOverlapsAabb2D(px, pz, rad, x0, x0 + LM_NARROW_INSET, z0, z1) ||
                     circleOverlapsAabb2D(px, pz, rad, x1 - LM_NARROW_INSET, x1, z0, z1))) return true;
            }
        }
    }
    for (const auto& cb : cols) {
        if (circleOverlapsAabb2D(px, pz, rad, cb.x0, cb.x1, cb.z0, cb.z1)) return true;
    }
    return false;
}

static float shotRayClearDist(float ox, float oz, float yaw, float maxDist, const std::vector<LimenAabb2D>& cols) {
    float sx = sinf(yaw), sz = cosf(yaw);
    for (float d = 0.28f; d <= maxDist; d += 0.28f) {
        if (shotPointHitsSightBlocker(ox + sx * d, oz + sz * d, 0.16f, cols)) return d - 0.28f;
    }
    return maxDist;
}

static ShotCam pickMegaFormVista(const LimenSlab& s, int reqForm = -1, int variant = 0) {
    if (!lmSlabIsVault(s)) return {0, 0, 0, 0, false};
    if (reqForm >= 0 && s.megaForm != reqForm) return {0, 0, 0, 0, false};
    const LimenRect& r = s.rect;
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(r.j, r.i);
    float midWX = (r.x0 + 1 + s.w * 0.5f) * CS, midWZ = (r.z0 + 1 + s.h * 0.5f) * CS;

    auto validStand = [&](int lx, int lz, float ox, float oz, float yaw, float minClear) -> bool {
        if (lx < 1 || lz < 1 || lx >= s.w - 1 || lz >= s.h - 1) return false;
        int t = s.tag[lx][lz];
        if (!lmIsWalkableTag(t) || lmIsLowShellTag(t)) return false;
        if ((s.zone[lx][lz] & LZ_XENO) != 0) return false; // never stand inside a foreign xeno cut!
        if (s.megaForm == MF_ZIGGURAT && t == LT_BRIDGE && (s.megaSub & 3) == 1) return false;
        if (shotPointHitsSightBlocker(ox, oz, 0.58f, sm.cols)) return false;
        return shotRayClearDist(ox, oz, yaw, 64.0f, sm.cols) >= minClear;
    };

    // Bounds for compact centered forms (MF_SILO, MF_CELLBLOCK) so camera is ALWAYS inside the tube/court!
    int xLo = 2, xHi = s.w - 3, zLo = 2, zHi = s.h - 3;
    if (s.megaForm == MF_SILO && s.w >= 11 && s.h >= 11) {
        int maxD = lmClampInt(11 + (int)s.genome.g[LG_GRAIN] / 64, 11, 15);
        int D = ((s.w - 4 < maxD) ? (s.w - 4) : maxD);
        if (s.h - 4 < D) D = s.h - 4;
        xLo = (s.w - D) / 2 + 1; xHi = xLo + D - 3;
        zLo = (s.h - D) / 2 + 1; zHi = zLo + D - 3;
    } else if (s.megaForm == MF_CELLBLOCK && s.w >= 13 && s.h >= 13) {
        int maxC = lmClampInt(12 + (int)s.genome.g[LG_GRAIN] / 48, 12, 18);
        int cW = (s.w - 4 < maxC) ? (s.w - 4) : maxC;
        int cH = (s.h - 4 < maxC) ? (s.h - 4) : maxC;
        xLo = (s.w - cW) / 2 + 1; xHi = xLo + cW - 3;
        zLo = (s.h - cH) / 2 + 1; zHi = zLo + cH - 3;
    }

    ShotCam best = {0, 0, 0, 0, false};
    float bestSc = -1e9f;
    for (int lx = xLo; lx <= xHi; lx++) {
        for (int lz = zLo; lz <= zHi; lz++) {
            float ox = (r.x0 + 1 + lx + 0.5f) * CS, oz = (r.z0 + 1 + lz + 0.5f) * CS;
            float toMidYaw = atan2f(midWX - ox, midWZ - oz);
            float distMid  = sqrtf((midWX - ox) * (midWX - ox) + (midWZ - oz) * (midWZ - oz));

            if (s.megaForm == MF_PLATFORM || s.megaForm == MF_CANYON) {
                int axis = s.megaSub & 1;
                if ((axis == 0 ? s.h : s.w) < 9) axis ^= 1;
                int pitTag = (s.megaForm == MF_PLATFORM) ? LT_BASIN : LT_VOID;
                bool isBr = (s.tag[lx][lz] == LT_BRIDGE);
                int rimDX = 0, rimDZ = 0;
                for (int d = 0; d < 4; d++) {
                    if (lmTagLocal(s, lx + shotDirX[d], lz + shotDirZ[d]) == pitTag) {
                        rimDX = shotDirX[d]; rimDZ = shotDirZ[d]; break;
                    }
                }
                if (!isBr && rimDX == 0 && rimDZ == 0) continue;
                float standX = ox + (isBr ? 0.0f : rimDX * 1.15f);
                float standZ = oz + (isBr ? 0.0f : rimDZ * 1.15f);
                for (int sgn = -1; sgn <= 1; sgn += 2) {
                    if ((variant & 1) && sgn < 0) continue;
                    float yaw = (axis == 0) ? (sgn > 0 ? 1.57079633f : -1.57079633f) : (sgn > 0 ? 0.0f : 3.14159265f);
                    if (!isBr) yaw += sinf(atan2f(midWX - standX, midWZ - standZ) - yaw) * 0.20f;
                    if (!validStand(lx, lz, standX, standZ, yaw, 12.0f)) continue;
                    float clr = shotRayClearDist(standX, standZ, yaw, 64.0f, sm.cols);
                    float sc = clr + (isBr && s.megaForm == MF_CANYON ? 80.0f : 0.0f);
                    float pitch = (s.megaForm == MF_CANYON) ? ((variant & 1) ? 0.28f : 0.18f) : 0.02f;
                    if (sc > bestSc) { bestSc = sc; best = {standX, standZ, yaw, pitch, true}; }
                }
            } else if (s.megaForm == MF_SILO) {
                if (lmLampLocal(s, lx, lz) && lmLampIsBollard(s, lx, lz)) continue;
                // Find a direct LT_VOID abyss edge so we look straight out over the shaft (never along a bridge deck!)
                int vDir = -1;
                for (int d = 0; d < 4; d++) {
                    if (lmTagLocal(s, lx + shotDirX[d], lz + shotDirZ[d]) == LT_VOID) {
                        int nbx = lx + shotDirX[d], nbz = lz + shotDirZ[d];
                        (void)nbx; (void)nbz;
                        vDir = d; break;
                    }
                }
                if (vDir < 0) continue;
                // Avoid standing right next to a bollard post on an adjacent rim cell
                if ((lmLampLocal(s, lx + 1, lz) && lmLampIsBollard(s, lx + 1, lz)) ||
                    (lmLampLocal(s, lx - 1, lz) && lmLampIsBollard(s, lx - 1, lz)) ||
                    (lmLampLocal(s, lx, lz + 1) && lmLampIsBollard(s, lx, lz + 1)) ||
                    (lmLampLocal(s, lx, lz - 1) && lmLampIsBollard(s, lx, lz - 1))) continue;
                float edgePush = (variant & 1) ? 2.15f : 1.15f; // push right to lip on downward shot so curb never blocks!
                float standX = ox + shotDirX[vDir] * edgePush;
                float standZ = oz + shotDirZ[vDir] * edgePush;
                float yaw = atan2f(midWX - standX, midWZ - standZ);
                if (!validStand(lx, lz, standX, standZ, yaw, 8.0f)) continue;
                float pitch = (variant & 1) ? -0.52f : 0.44f;
                float sc = distMid + (s.tag[lx][lz] == LT_BRIDGE && (variant & 1) ? 30.0f : 10.0f);
                if (sc > bestSc) { bestSc = sc; best = {standX, standZ, yaw, pitch, true}; }
            } else {
                float pitch = 0.14f;
                float standX = ox, standZ = oz;
                if (s.megaForm == MF_CELLBLOCK) pitch = (variant & 1) ? 0.30f : 0.22f;
                else if (s.megaForm == MF_HANGING) pitch = (variant & 1) ? 0.20f : 0.13f;
                else if (s.megaForm == MF_HYPOSTYLE) pitch = (variant & 1) ? 0.16f : 0.06f;
                else if (s.megaForm == MF_ZIGGURAT) {
                    int zSub = s.megaSub & 3;
                    int maxP = lmClampInt(9 + s.ceilTiers / 3, 9, 13);
                    int pCells = (s.w - 6 < maxP) ? (s.w - 6) : maxP;
                    if (s.h - 6 < pCells) pCells = s.h - 6;
                    float pyrHalfR = (float)pCells * 0.5f * CS;
                    if (zSub >= 2) {
                        if (s.tag[lx][lz] == LT_BRIDGE || lmPitDegreeLocal(s, lx, lz) == 0) continue;
                        standX += sinf(toMidYaw) * 2.25f;
                        standZ += cosf(toMidYaw) * 2.25f;
                        pitch = -0.52f;
                    } else if (zSub == 0) {
                        if (distMid < 18.0f || distMid > 52.0f) continue;
                        pitch = 0.25f;
                    } else {
                        // Stand 16..34 m back from the pyramid's outer base step (diagonal corner vista preferred!)
                        float distFromBase = distMid - pyrHalfR;
                        if (distFromBase < 15.0f || distFromBase > 36.0f) continue;
                        pitch = 0.18f;
                    }
                }
                float yaw = toMidYaw + ((variant & 1) ? 0.10f : 0.0f);
                if (s.megaForm == MF_ZIGGURAT && (s.megaSub & 3) == 0) {
                    int axis = (s.megaSub >> 2) & 1;
                    bool posEnd = ((s.megaSub >> 3) & 1) != 0;
                    yaw = (axis == 0) ? (posEnd ? 0.0f : 3.14159265f) : (posEnd ? 1.57079633f : -1.57079633f);
                }
                if (!validStand(lx, lz, standX, standZ, yaw, 9.0f)) continue;
                float clr = shotRayClearDist(standX, standZ, yaw, 64.0f, sm.cols);
                float sc = distMid * 0.9f + fminf(clr, 48.0f);
                if (s.megaForm == MF_ZIGGURAT) {
                    int maxP = lmClampInt(9 + s.ceilTiers / 3, 9, 13);
                    float pyrHalfR = (float)((s.w - 6 < maxP) ? (s.w - 6) : maxP) * 0.5f * CS;
                    float targetD = ((s.megaSub & 3) == 1) ? (pyrHalfR + 22.0f) : 36.0f;
                    sc = 80.0f - fabsf(distMid - targetD) + fminf(clr, 36.0f);
                }
                if (sc > bestSc) { bestSc = sc; best = {standX, standZ, yaw, pitch, true}; }
            }
        }
    }
    return best;
}

// Picks a standing point & yaw with maximal unobstructed raycast sightline (strictly >= 3.2 m to any wall!).
static ShotCam pickSlabCamera(const LimenSlab& s, u32 salt) {
    if (lmSlabIsVault(s) && s.megaForm > MF_CLASSIC) {
        ShotCam mc = pickMegaFormVista(s, -1);
        if (mc.ok) return mc;
    }
    const LimenRect& r = s.rect;
    ShotCam best = {0, 0, 0, 0, false};
    float bestScore = -1.0f;
    u32 bestH = 0;
    bool vault = lmSlabIsVault(s);
    int margin = vault ? 2 : 1;
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(r.j, r.i);
    for (int lx = margin; lx < s.w - margin; lx++) {
        for (int lz = margin; lz < s.h - margin; lz++) {
            int tag = s.tag[lx][lz];
            if (!lmIsWalkableTag(tag)) continue;
            if (lmLampLocal(s, lx, lz) && lmLampIsBollard(s, lx, lz)) continue;
            int wx = r.x0 + 1 + lx, wz = r.z0 + 1 + lz;
            bool high = vault && !lmIsLowShellTag(tag);
            u32 h = lmHash((u32)wx, (u32)wz, salt, 0x4341u);
            float baseTurn = ((float)(lmHash(h, 7u, 0u, 0x4342u) % 1000u) / 999.0f - 0.5f) *
                             ((s.flag[lx][lz] & (LF_NARROW_X | LF_NARROW_Z)) ? 0.10f : 0.28f);

            for (int d = 0; d < 4; d++) {
                int aheadTag = lmTagLocal(s, lx + shotDirX[d], lz + shotDirZ[d]);
                float camX = (wx + 0.5f) * CS, camZ = (wz + 0.5f) * CS;
                float pitch = high ? 0.08f : 0.0f;
                if (lmIsPitTag(aheadTag)) {
                    camX += shotDirX[d] * 1.05f; camZ += shotDirZ[d] * 1.05f;
                    pitch = aheadTag == LT_BASIN ? -0.12f : 0.03f;
                }
                if (shotPointHitsSightBlocker(camX, camZ, 0.55f, sm.cols)) continue;

                const float turns[2] = {baseTurn, 0.0f};
                for (int ti = 0; ti < 2; ti++) {
                    float yaw = d * 1.57079633f + turns[ti];
                    float dMid = shotRayClearDist(camX, camZ, yaw, 68.0f, sm.cols);
                    if (dMid < 4.5f) continue; // never stand nose-first near a wall or thin partition!
                    float dL = shotRayClearDist(camX, camZ, yaw - 0.20f, 36.0f, sm.cols);
                    float dR = shotRayClearDist(camX, camZ, yaw + 0.20f, 36.0f, sm.cols);
                    float dSide = dL < dR ? dL : dR;
                    if (dSide < 2.20f) continue;
                    float cappedMid = dMid > 48.0f ? 48.0f : dMid;
                    float score = cappedMid + dSide * 0.35f + (high ? 12.0f : 0.0f) +
                                  (lmIsPitTag(aheadTag) ? 18.0f : 0.0f) +
                                  (high && lmPitDegreeLocal(s, lx, lz) > 0 ? 6.0f : 0.0f);
                    if (score > bestScore || (fabsf(score - bestScore) < 0.01f && h < bestH)) {
                        best = {camX, camZ, yaw, pitch, true};
                        bestScore = score;
                        bestH = h;
                    }
                    break;
                }
            }
        }
    }
    return best;
}

// Collects up to `maxOut` interior cameras standing INSIDE small `LT_ROOM` bays (prioritizing
// rooms behind `LF_THIN_*` partitions), looking diagonally across the furnished room (>= 3.5 m ray!).
static int collectInteriorRoomShots(ShotCam* outCams, int maxOut) {
    std::vector<u32> usedRoomKeys;
    int found = 0;
    for (int pass = 0; pass < 2 && found < maxOut; pass++) {
        bool requireThin = (pass == 0);
        for (int ring = 0; ring <= 8 && found < maxOut; ring++) {
            for (int j = -ring; j <= ring && found < maxOut; j++) {
                for (int i = -ring; i <= ring && found < maxOut; i++) {
                    if (lmDepth(j, i) != ring) continue;
                    const LimenSlab s = lmSlab(j, i);
                    if (s.dark || s.fallback || s.share[LT_ROOM] < 6) continue;
                    if (requireThin && s.thinCount < 4) continue;
                    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(j, i);

                    ShotCam bestInSlab = {0, 0, 0, 0, false};
                    float bestScore = -1.0f;
                    u32 bestKey = 0;

                    for (int lx = 1; lx < s.w - 1; lx++) {
                        for (int lz = 1; lz < s.h - 1; lz++) {
                            if (s.tag[lx][lz] != LT_ROOM) continue;
                            int wx = s.rect.x0 + 1 + lx, wz = s.rect.z0 + 1 + lz;
                            unsigned short f0 = lmFlagAt(wx, wz);
                            unsigned short fE = lmFlagAt(wx + 1, wz);
                            unsigned short fS = lmFlagAt(wx, wz + 1);
                            bool touchesThin = ((f0 | fE) & LF_THIN_W) != 0 || ((f0 | fS) & LF_THIN_N) != 0;
                            if (requireThin && !touchesThin) continue;

                            u32 rKey = ((u32)(j & 255) << 24) | ((u32)(i & 255) << 16) | (u32)s.nodeKey[lx][lz];
                            if (std::find(usedRoomKeys.begin(), usedRoomKeys.end(), rKey) != usedRoomKeys.end()) continue;

                            bool wallW = (lmTagAt(wx - 1, wz) == LT_WALL) || ((f0 & LF_THIN_W) != 0);
                            bool wallE = (lmTagAt(wx + 1, wz) == LT_WALL) || ((fE & LF_THIN_W) != 0);
                            bool wallN = (lmTagAt(wx, wz - 1) == LT_WALL) || ((f0 & LF_THIN_N) != 0);
                            bool wallS = (lmTagAt(wx, wz + 1) == LT_WALL) || ((fS & LF_THIN_N) != 0);
                            int wallEdges = (wallW ? 1 : 0) + (wallE ? 1 : 0) + (wallN ? 1 : 0) + (wallS ? 1 : 0);
                            if (wallEdges < 2) continue; // truly enclosed small room bay, not a giant open hall!

                            // Test standing in each enclosed corner of the 5m x 5m room looking diagonally across it
                            float cx = (wx + 0.5f) * CS, cz = (wz + 0.5f) * CS;
                            for (int qx = -1; qx <= 1; qx += 2) {
                                for (int qz = -1; qz <= 1; qz += 2) {
                                    bool backEnclosed = ((qx < 0) ? wallW : wallE) && ((qz < 0) ? wallN : wallS);
                                    if (!backEnclosed) continue;
                                    float ox = cx + qx * 1.44f, oz = cz + qz * 1.44f;
                                    if (shotPointHitsSightBlocker(ox, oz, 0.48f, sm.cols)) continue;
                                    float yaw = atan2f((float)-qx, (float)-qz);
                                    float dMid = shotRayClearDist(ox, oz, yaw, 16.0f, sm.cols);
                                    if (dMid < 3.45f || dMid > 12.5f) continue;
                                    float dL = shotRayClearDist(ox, oz, yaw - 0.24f, 12.0f, sm.cols);
                                    float dR = shotRayClearDist(ox, oz, yaw + 0.24f, 12.0f, sm.cols);
                                    float dSide = dL < dR ? dL : dR;
                                    if (dSide < 2.02f) continue;
                                    float sweet = 8.0f - fabsf(dMid - 5.8f);
                                    float sc = sweet + dSide * 0.4f + (touchesThin ? 6.0f : 0.0f) + (float)wallEdges * 2.0f;
                                    if (sc > bestScore) {
                                        bestScore = sc;
                                        bestInSlab = {ox, oz, yaw, -0.10f, true};
                                        bestKey = rKey;
                                    }
                                }
                            }
                        }
                    }
                    if (bestInSlab.ok) {
                        usedRoomKeys.push_back(bestKey);
                        outCams[found++] = bestInSlab;
                    }
                }
            }
        }
    }
    return found;
}

template <class F> static ShotCam searchSlabs(F&& pick, std::vector<long long>* usedSlabs = nullptr) {
    for (int ring = 1; ring <= 14; ring++) {
        for (int j = -ring; j <= ring; j++) {
            for (int i = -ring; i <= ring; i++) {
                if (lmDepth(j, i) != ring) continue;
                long long skey = ((long long)j << 32) | (unsigned int)i;
                if (usedSlabs && std::find(usedSlabs->begin(), usedSlabs->end(), skey) != usedSlabs->end()) continue;
                const LimenSlab s = lmSlab(j, i);
                if (s.dark || s.fallback) continue;
                ShotCam c = pick(s);
                if (c.ok) {
                    if (usedSlabs) usedSlabs->push_back(skey);
                    return c;
                }
            }
        }
    }
    return {0, 0, 0, 0, false};
}

static ShotCam pickSeveredSpan(const LimenSlab& s) {
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            if (s.tag[lx][lz] != LT_BRIDGE || lmPitDegreeLocal(s, lx, lz) < 3) continue;
            for (int d = 0; d < 4; d++) {
                if (lmTagLocal(s, lx - shotDirX[d], lz - shotDirZ[d]) != LT_BRIDGE) continue;
                bool openAhead = true;
                for (int f = 1; f <= 6; f++) if (lmTagLocal(s, lx + f * shotDirX[d], lz + f * shotDirZ[d]) != LT_VOID) openAhead = false;
                if (!openAhead) continue;
                int wx = s.rect.x0 + 1 + lx - shotDirX[d], wz = s.rect.z0 + 1 + lz - shotDirZ[d];
                return {(wx + 0.5f + 0.25f * shotDirX[d]) * CS, (wz + 0.5f + 0.25f * shotDirZ[d]) * CS, d * 1.57079633f, -0.08f, true};
            }
        }
    }
    return {0, 0, 0, 0, false};
}

static ShotCam pickLowShell(const LimenSlab& s) {
    if (!lmSlabIsVault(s)) return {0, 0, 0, 0, false};
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            if (s.tag[lx][lz] != LT_LOW) continue;
            for (int d = 0; d < 4; d++) {
                bool clear = true;
                for (int f = 1; f <= 4; f++) if (lmTagLocal(s, lx + f * shotDirX[d], lz + f * shotDirZ[d]) != LT_HALL) clear = false;
                if (!clear) continue;
                int wx = s.rect.x0 + 1 + lx + 3 * shotDirX[d], wz = s.rect.z0 + 1 + lz + 3 * shotDirZ[d];
                return {(wx + 0.5f) * CS, (wz + 0.5f) * CS, atan2f((float)-shotDirX[d], (float)-shotDirZ[d]) + 0.18f, 0.14f, true};
            }
        }
    }
    return {0, 0, 0, 0, false};
}

static ShotCam pickLowToHigh(const LimenSlab& s) {
    if (!lmSlabIsVault(s)) return {0, 0, 0, 0, false};
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            if (s.tag[lx][lz] != LT_LOW) continue;
            for (int d = 0; d < 4; d++) {
                int fwd = lmTagLocal(s, lx + shotDirX[d], lz + shotDirZ[d]);
                if ((fwd != LT_HALL && fwd != LT_BRIDGE) || !lmIsLowShellTag(lmTagLocal(s, lx - shotDirX[d], lz - shotDirZ[d]))) continue;
                int run = 0;
                while (run < 24 && lmCell(s.rect.x0 + 1 + lx + (run + 1) * shotDirX[d], s.rect.z0 + 1 + lz + (run + 1) * shotDirZ[d]) != 1) run++;
                if (run < 9) continue;
                return {(s.rect.x0 + 1 + lx - 0.65f * shotDirX[d] + 0.5f) * CS, (s.rect.z0 + 1 + lz - 0.65f * shotDirZ[d] + 0.5f) * CS,
                        d * 1.57079633f, 0.06f, true};
            }
        }
    }
    return {0, 0, 0, 0, false};
}

static ShotCam pickThreshold(const LimenSlab& s) {
    if (!lmSlabIsVault(s) || lmPitShare(s) < 15) return {0, 0, 0, 0, false};
    const LimenRect& r = s.rect;
    for (int wz = r.z0 + 2; wz < r.z1 - 1; wz++) {
        if (lmCell(r.x0, wz) != 0 || lmVaultAt(r.x0 - 1, wz) || !lmWalkable(r.x0 - 1, wz)) continue;
        int run = 0;
        while (run < 30 && lmCell(r.x0 + 1 + run, wz) != 1) run++;
        if (run >= 11) return {(r.x0 - 0.35f) * CS, (wz + 0.5f) * CS, 1.57079633f, 0.02f, true};
    }
    for (int wx = r.x0 + 2; wx < r.x1 - 1; wx++) {
        if (lmCell(wx, r.z0) != 0 || lmVaultAt(wx, r.z0 - 1) || !lmWalkable(wx, r.z0 - 1)) continue;
        int run = 0;
        while (run < 30 && lmCell(wx, r.z0 + 1 + run) != 1) run++;
        if (run >= 11) return {(wx + 0.5f) * CS, (r.z0 - 0.35f) * CS, 0.0f, 0.02f, true};
    }
    return {0, 0, 0, 0, false};
}

// Standing down ON the walkable dry basin floor (-2.85 m) looking back at the 10-step tiled staircase!
static ShotCam pickBasinStair(const LimenSlab& s) {
    if (!lmSlabIsVault(s) || s.walkBasins < 8) return {0, 0, 0, 0, false};
    static const int rdx[4] = {0, 0, -1, 1}, rdz[4] = {-1, 1, 0, 0};
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            if (!(s.flag[lx][lz] & LF_STAIR_DESC)) continue;
            int dir = lmStairRimDir(s.flag[lx][lz]);
            int bx = lx - 2 * rdx[dir], bz = lz - 2 * rdz[dir];
            if (lmTagLocal(s, bx, bz) != LT_BASIN) continue;
            int wx = s.rect.x0 + 1 + bx, wz = s.rect.z0 + 1 + bz;
            float yaw = atan2f((float)rdx[dir], (float)rdz[dir]) + 0.22f;
            return {(wx + 0.5f) * CS, (wz + 0.5f) * CS, yaw, 0.10f, true};
        }
    }
    return {0, 0, 0, 0, false};
}

// Human-scale 2.50 m narrowed corridor alongside thin-partitioned clone rooms.
static ShotCam pickNarrowHumanCorridor(const LimenSlab& s) {
    if (lmSlabIsVault(s) || s.narrowCount < 10 || s.thinCount < 6) return {0, 0, 0, 0, false};
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            if (!(s.flag[lx][lz] & LF_NARROW_X)) continue;
            int run = 0;
            while (run < 14 && lx + run + 1 < s.w && lmIsWalkableTag(s.tag[lx + run + 1][lz])) run++;
            if (run < 6) continue;
            int wx = s.rect.x0 + 1 + lx, wz = s.rect.z0 + 1 + lz;
            return {(wx + 0.5f) * CS, (wz + 0.5f) * CS, 1.57079633f, -0.02f, true};
        }
    }
    return {0, 0, 0, 0, false};
}

// Host-side camera looking straight across the razor-sharp frameless `LZ_XENO` cut.
static ShotCam pickXenograftCut(const LimenSlab& s) {
    if (!s.xeno.active) return {0, 0, 0, 0, false};
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(s.rect.j, s.rect.i);
    for (int lx = 1; lx < s.w - 1; lx++) {
        for (int lz = 1; lz < s.h - 1; lz++) {
            if (!lmIsWalkableTag(s.tag[lx][lz]) || (s.zone[lx][lz] & LZ_XENO) != 0) continue;
            for (int d = 0; d < 4; d++) {
                int x1 = lx + shotDirX[d], z1 = lz + shotDirZ[d];
                int x2 = lx + 2 * shotDirX[d], z2 = lz + 2 * shotDirZ[d];
                if (!lmIsWalkableTag(lmTagLocal(s, x1, z1)) || (lmZoneLocal(s, x1, z1) & LZ_XENO) == 0) continue;
                if (!lmIsWalkableTag(lmTagLocal(s, x2, z2)) || (lmZoneLocal(s, x2, z2) & LZ_XENO) == 0) continue;
                int bx = lx, bz = lz;
                if (lmIsWalkableTag(lmTagLocal(s, lx - shotDirX[d], lz - shotDirZ[d])) &&
                    (lmZoneLocal(s, lx - shotDirX[d], lz - shotDirZ[d]) & LZ_XENO) == 0) {
                    bx = lx - shotDirX[d]; bz = lz - shotDirZ[d];
                }
                float cx = (s.rect.x0 + 1 + bx + 0.5f) * CS, cz = (s.rect.z0 + 1 + bz + 0.5f) * CS;
                bool hit = false;
                for (const auto& cb : sm.cols) if (circleOverlapsAabb2D(cx, cz, 1.4f, cb.x0, cb.x1, cb.z0, cb.z1)) hit = true;
                if (hit) continue;
                return {cx, cz, atan2f((float)shotDirX[d], (float)shotDirZ[d]) + 0.08f, 0.02f, true};
            }
        }
    }
    return {0, 0, 0, 0, false};
}

// Looking through a glazed `LS_VITRINE_WIN` window into a sealed lit furnished `LT_VITRINE` room.
static ShotCam pickVitrineWindow(const LimenSlab& s) {
    if (s.vitrineRooms <= 0) return {0, 0, 0, 0, false};
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(s.rect.j, s.rect.i);
    for (int k = 0; k < s.siteCount; k++) {
        if (s.sites[k].type != LS_VITRINE_WIN) continue;
        int wlx = s.sites[k].lx, wlz = s.sites[k].lz;
        for (int d = 0; d < 4; d++) {
            int flx = wlx - shotDirX[d], flz = wlz - shotDirZ[d];
            if (!lmIsWalkableTag(lmTagLocal(s, flx, flz))) continue;
            if (s.flag[flx][flz] & (LF_NARROW_X | LF_NARROW_Z)) continue;
            int px = shotDirZ[d], pz = shotDirX[d];
            if (lmTagLocal(s, flx - px, flz - pz) == LT_WALL && lmTagLocal(s, flx + px, flz + pz) == LT_WALL) continue;
            if (doorwayAxisAt(s.rect.x0 + 1 + flx, s.rect.z0 + 1 + flz) != 0) continue;
            float back = lmIsWalkableTag(lmTagLocal(s, flx - shotDirX[d], flz - shotDirZ[d])) ? 0.65f : 0.30f;
            float cx = (s.rect.x0 + 1 + flx + 0.5f - shotDirX[d] * back) * CS;
            float cz = (s.rect.z0 + 1 + flz + 0.5f - shotDirZ[d] * back) * CS;
            if (shotPointHitsSightBlocker(cx, cz, 0.48f, sm.cols)) continue;
            float yaw = atan2f((float)shotDirX[d], (float)shotDirZ[d]) + 0.06f;
            return {cx, cz, yaw, -0.02f, true};
        }
    }
    return {0, 0, 0, 0, false};
}

// Strong (>0.58) well-lit 1-storey drunken pocket: leaning walls, stepped floor, mismatched doors & ceiling props!
static ShotCam pickDrunkPocket(const LimenSlab& s, bool requireBright = true) {
    if (lmSlabIsVault(s) || s.deliriumPeak < 148) return {0, 0, 0, 0, false};
    if (requireBright && (s.genome.g[LG_LIGHT] < 85 || s.lampPitch > 4)) return {0, 0, 0, 0, false};
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(s.rect.j, s.rect.i);
    ShotCam best = {0, 0, 0, 0, false};
    float bestSc = -1.0f;
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            int t = s.tag[lx][lz];
            if (t != LT_CORR && t != LT_HALL) continue;
            if (s.flag[lx][lz] & (LF_NARROW_X | LF_NARROW_Z)) continue;
            if (lmLampLocal(s, lx, lz)) continue; // never stand directly under a sagging lamp cluster!
            int wx = s.rect.x0 + 1 + lx, wz = s.rect.z0 + 1 + lz;
            int del = lmDeliriumCell255(wx, wz);
            if (del < 145) continue;
            float ox = (wx + 0.5f) * CS, oz = (wz + 0.5f) * CS;
            if (shotPointHitsSightBlocker(ox, oz, 0.55f, sm.cols)) continue;
            for (int d = 0; d < 4; d++) {
                if (!lmLampLocal(s, lx + shotDirX[d], lz + shotDirZ[d]) &&
                    !lmLampLocal(s, lx + 2 * shotDirX[d], lz + 2 * shotDirZ[d])) continue; // ensure lit ahead!
                float yaw = d * 1.57079633f + 0.12f;
                float clr = shotRayClearDist(ox, oz, yaw, 42.0f, sm.cols);
                if (clr < 9.5f) continue;
                float sc = (float)del * 0.5f + (float)s.genome.g[LG_LIGHT] * 0.3f + fminf(clr, 26.0f);
                if (sc > bestSc) { bestSc = sc; best = {ox, oz, yaw, 0.02f, true}; } // pitch strictly +1.1 deg!
            }
        }
    }
    return best;
}

// Vault standing 14..24 m from the rare Escherian staircase knot looking straight up at it!
static ShotCam pickDrunkEscher(const LimenSlab& s) {
    if (!lmSlabHasEscherKnot(s)) return {0, 0, 0, 0, false};
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(s.rect.j, s.rect.i);
    float midWX = (s.rect.x0 + 1 + s.w * 0.5f) * CS, midWZ = (s.rect.z0 + 1 + s.h * 0.5f) * CS;
    ShotCam best = {0, 0, 0, 0, false};
    float bestSc = -1.0f;
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            int t = s.tag[lx][lz];
            if (t != LT_HALL && t != LT_BRIDGE) continue;
            float ox = (s.rect.x0 + 1 + lx + 0.5f) * CS, oz = (s.rect.z0 + 1 + lz + 0.5f) * CS;
            float dist = sqrtf((midWX - ox) * (midWX - ox) + (midWZ - oz) * (midWZ - oz));
            if (dist < 13.0f || dist > 28.0f) continue;
            if (shotPointHitsSightBlocker(ox, oz, 0.60f, sm.cols)) continue;
            float yaw = atan2f(midWX - ox, midWZ - oz);
            if (shotRayClearDist(ox, oz, yaw, 28.0f, sm.cols) < 11.0f) continue;
            float sc = 40.0f - fabsf(dist - 18.5f) + (float)s.deliriumPeak * 0.1f;
            if (sc > bestSc) { bestSc = sc; best = {ox, oz, yaw, 0.28f, true}; }
        }
    }
    return best;
}

// Telescoping sightline looking straight through 3..4 concentric Matryoshka rooms-within-rooms!
static ShotCam pickDrunkNested(const LimenSlab& s, bool requireBright = true) {
    if (s.nestedRings < 2 || s.nestedX < 0) return {0, 0, 0, 0, false};
    if (requireBright && s.genome.g[LG_LIGHT] < 75) return {0, 0, 0, 0, false};
    static const int sdx[4] = {1, -1, 0, 0}, sdz[4] = {0, 0, 1, -1};
    int dir = s.nestedDir & 3;
    int lx = s.nestedX, lz = s.nestedZ;
    if (!lmIsWalkableTag(lmTagLocal(s, lx, lz))) return {0, 0, 0, 0, false};
    int d1 = lmTagLocal(s, lx + sdx[dir], lz + sdz[dir]);
    int d2 = lmTagLocal(s, lx + sdx[dir] * 3, lz + sdz[dir] * 3);
    if (!lmIsWalkableTag(d1) || !lmIsWalkableTag(d2)) return {0, 0, 0, 0, false};
    float back = lmIsWalkableTag(lmTagLocal(s, lx - sdx[dir], lz - sdz[dir])) ? 0.55f : 0.0f;
    float cx = (s.rect.x0 + 1 + lx + 0.5f - sdx[dir] * back) * CS;
    float cz = (s.rect.z0 + 1 + lz + 0.5f - sdz[dir] * back) * CS;
    float yaw = atan2f((float)sdx[dir], (float)sdz[dir]);
    return {cx, cz, yaw, 0.0f, true};
}

// Standing INSIDE the outer lane of the Nautilus spiral seeing 2..3 curving, narrowing, dropping turns!
static ShotCam pickDrunkSpiral(const LimenSlab& s, bool requireBright = true) {
    if (s.spiralW < 7 || s.spiralH < 7 || s.spiralX < 0) return {0, 0, 0, 0, false};
    if (requireBright && s.genome.g[LG_LIGHT] < 75) return {0, 0, 0, 0, false};
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(s.rect.j, s.rect.i);
    float scx = (s.rect.x0 + 1 + s.spiralX + s.spiralW * 0.5f) * CS;
    float scz = (s.rect.z0 + 1 + s.spiralZ + s.spiralH * 0.5f) * CS;
    float R0  = fminf((float)s.spiralW, (float)s.spiralH) * 0.45f * CS;
    for (int step = 0; step < 6; step++) {
        float aCam = 0.55f + (float)step * 0.28f;
        float rCam = R0 * (0.78f - (float)step * 0.02f);
        float cx = scx + cosf(aCam) * rCam, cz = scz + sinf(aCam) * rCam;
        if (shotPointHitsSightBlocker(cx, cz, 0.32f, sm.cols)) continue;
        float tx = scx + cosf(aCam + 1.45f) * (R0 * 0.42f);
        float tz = scz + sinf(aCam + 1.45f) * (R0 * 0.42f);
        float yaw = atan2f(tx - cx, tz - cz);
        return {cx, cz, yaw, -0.06f, true};
    }
    return {0, 0, 0, 0, false};
}

// Looking at a closed door (`LS_WALL_DOOR`) mounted flush in a solid blank wall.
static ShotCam pickWallDoor(const LimenSlab& s) {
    static const int ddx[4] = {0, 0, -1, 1}, ddz[4] = {-1, 1, 0, 0};
    for (int k = 0; k < s.siteCount; k++) {
        if (s.sites[k].type != LS_WALL_DOOR) continue;
        int lx = s.sites[k].lx, lz = s.sites[k].lz;
        if (!lmIsWalkableTag(lmTagLocal(s, lx, lz))) continue;
        u32 sh = s.sites[k].id;
        for (int t = 0; t < 4; t++) {
            int d = (int)((sh + (u32)t) & 3u);
            int nwx = lx + ddx[d], nwz = lz + ddz[d];
            if (lmTagLocal(s, nwx, nwz) != LT_WALL || lmSlabHasSite(s, nwx, nwz, LS_VITRINE_WIN)) continue;
            float back = lmIsWalkableTag(lmTagLocal(s, lx - ddx[d], lz - ddz[d])) ? 0.85f : 0.35f;
            float cx = (s.rect.x0 + 1 + lx + 0.5f - ddx[d] * back - ddz[d] * 0.18f) * CS;
            float cz = (s.rect.z0 + 1 + lz + 0.5f - ddz[d] * back + ddx[d] * 0.18f) * CS;
            float tx = (s.rect.x0 + 1 + lx + 0.5f + ddx[d] * 0.5f) * CS;
            float tz = (s.rect.z0 + 1 + lz + 0.5f + ddz[d] * 0.5f) * CS;
            return {cx, cz, atan2f(tx - cx, tz - cz), -0.02f, true};
        }
    }
    return {0, 0, 0, 0, false};
}

// Standing in the hall watching a `LS_CEIL_STAIR` staircase slam straight into the solid ceiling.
static ShotCam pickCeilStair(const LimenSlab& s) {
    const LmCachedSlabMesh& sm = lmEnsureSlabMesh(s.rect.j, s.rect.i);
    for (int k = 0; k < s.siteCount; k++) {
        if (s.sites[k].type != LS_CEIL_STAIR) continue;
        int lx = s.sites[k].lx, lz = s.sites[k].lz;
        if (lx + 1 >= s.w) continue;
        int wallSide = 0;
        if (lmTagLocal(s, lx, lz - 1) == LT_WALL && lmTagLocal(s, lx + 1, lz - 1) == LT_WALL) wallSide = -1;
        else if (lmTagLocal(s, lx, lz + 1) == LT_WALL && lmTagLocal(s, lx + 1, lz + 1) == LT_WALL) wallSide = 1;
        if (wallSide == 0) continue;
        int camLX = (lx >= 2 && lmIsWalkableTag(lmTagLocal(s, lx - 1, lz - wallSide))) ? (lx - 1) : lx;
        int camLZ = lz - wallSide;
        if (!lmIsWalkableTag(lmTagLocal(s, camLX, camLZ))) continue;
        float cx = (s.rect.x0 + 1 + camLX + 0.35f) * CS;
        float cz = (s.rect.z0 + 1 + camLZ + 0.5f) * CS;
        bool hit = false;
        for (const auto& cb : sm.cols) if (circleOverlapsAabb2D(cx, cz, 1.2f, cb.x0, cb.x1, cb.z0, cb.z1)) hit = true;
        if (hit) continue;
        float tx = (s.rect.x0 + 1 + lx) * CS + 4.2f;
        float tz = (s.rect.z0 + 1 + lz + (wallSide < 0 ? 0.18f : 0.82f)) * CS;
        return {cx, cz, atan2f(tx - cx, tz - cz), 0.14f, true};
    }
    return {0, 0, 0, 0, false};
}

// Looking down a `LZ_PLUG_CORR` corridor of numbered doors toward its dead-end concrete plug.
static ShotCam pickPluggedCorridor(const LimenSlab& s) {
    for (int k = 0; k < s.siteCount; k++) {
        if (s.sites[k].type != LS_PLUG_CORR) continue;
        int lx = s.sites[k].lx, lz = s.sites[k].lz;
        for (int d = 0; d < 4; d++) {
            int run = 0;
            while (run < 4 && lmIsWalkableTag(lmTagLocal(s, lx + (run + 1) * shotDirX[d], lz + (run + 1) * shotDirZ[d])) &&
                   (lmZoneLocal(s, lx + (run + 1) * shotDirX[d], lz + (run + 1) * shotDirZ[d]) & LZ_PLUG_CORR) != 0) {
                run++;
            }
            if (run < 2) continue;
            int bx = lx + run * shotDirX[d], bz = lz + run * shotDirZ[d];
            float cx = (s.rect.x0 + 1 + bx + 0.5f) * CS, cz = (s.rect.z0 + 1 + bz + 0.5f) * CS;
            return {cx, cz, atan2f((float)-shotDirX[d], (float)-shotDirZ[d]), 0.0f, true};
        }
    }
    return {0, 0, 0, 0, false};
}

extern "C" __declspec(dllimport) void __stdcall glFinish(void);

struct ShotMetrics { float meanLuma, hiPct, loPct, frameMs; };

static ShotMetrics shootAtCamera(u32 seed, const ShotCam& c, const char* path, bool clearNearLens = false, float eyeY = -999.0f) {
    float groundY = lmGroundHeightAt(c.x, c.z, CS);
    cam.pos = Vec3(c.x, eyeY > -900.0f ? eyeY : PH + groundY, c.z);
    cam.yaw = c.yaw;
    cam.pitch = c.pitch;
    chunks.clear();
    rng.seed(seed); srand(seed);
    updateVisibleChunks(c.x, c.z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    resetMapContent();
    updateMapContent(playerChunkX, playerChunkZ);
    if (clearNearLens) {
        // Keep a 0.38 m lens radius clear when standing in a cozy room corner so props never clip the near plane
        mapProps.erase(std::remove_if(mapProps.begin(), mapProps.end(), [&](const MapProp& p) {
            if (isPropOverheadType(p.type) || propPlacement(p.type) == PROP_PLACE_FLAT) return false;
            float hx = 0.0f, hz = 0.0f;
            mapPropHalfExtents(p, hx, hz);
            return fabsf(p.pos.x - c.x) < hx + 0.38f && fabsf(p.pos.z - c.z) < hz + 0.38f;
        }), mapProps.end());
    }
    buildGeom();
    glFinish();
    float meanLuma = trialShoot(path);
    // Measure pure GPU+CPU frame time across 2 warm passes (excluding disk BMP write!)
    auto tF0 = std::chrono::high_resolution_clock::now();
    for (int p = 0; p < 2; p++) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, renderW, renderH);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderScene();
        trialPostPass();
    }
    glFinish();
    auto tF1 = std::chrono::high_resolution_clock::now();
    float frameMs = (float)(std::chrono::duration<double, std::milli>(tF1 - tF0).count() * 0.5);

    static std::vector<unsigned char> px;
    px.resize((size_t)winW * (size_t)winH * 3);
    glBindFramebuffer(GL_FRAMEBUFFER, trialOutFbo);
    glPixelStorei(TRIAL_GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, winW, winH, GL_RGB, GL_UNSIGNED_BYTE, px.data());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    size_t hi = 0, lo = 0, tot = (size_t)winW * (size_t)winH;
    for (size_t i = 0; i < px.size(); i += 3) {
        float y = 0.299f * (float)px[i] + 0.587f * (float)px[i + 1] + 0.114f * (float)px[i + 2];
        if (y > 250.0f) hi++;
        if (y < 8.0f) lo++;
    }
    float inv = tot > 0 ? (100.0f / (float)tot) : 0.0f;
    return {meanLuma, (float)hi * inv, (float)lo * inv, frameMs};
}

static u32 shotSignature(const LimenSlab& s) {
    u32 sig = (u32)(s.ceilTiers > 1 ? (2 + (s.megaForm >= 0 ? s.megaForm : 0)) : 0);
    sig = sig * 5u + (u32)(s.share[LT_WALL] / 20);
    sig = sig * 4u + (u32)((s.share[LT_ROOM] + s.share[LT_LOW]) / 15 > 3 ? 3 : (s.share[LT_ROOM] + s.share[LT_LOW]) / 15);
    sig = sig * 3u + (u32)(s.share[LT_VOID] >= 10 ? 2 : (s.share[LT_BASIN] >= 8 ? 1 : 0));
    sig = sig * 2u + (u32)(s.share[LT_BRIDGE] >= 3 ? 1 : 0);
    return sig;
}

// Walk benchmark: reproduces the game's chunk-crossing sequence along a straight line and
// reports the hitch each crossing costs.  limen_shots.exe bench [seed] [level] [crossings]
inline double gLmBenchPostMs = 0.0;
static unsigned long long benchPropHash() {
    unsigned long long h = 1469598103934665603ull;
    auto mix = [&](const void* p, size_t n) { const unsigned char* b = (const unsigned char*)p; for (size_t q = 0; q < n; q++) h = (h ^ b[q]) * 1099511628211ull; };
    for (const auto& p : mapProps) { mix(&p.pos, sizeof(p.pos)); mix(&p.type, 4); mix(&p.scale, 4); mix(&p.yaw, 4); }
    for (const auto& p : mapPois) { mix(&p.pos, sizeof(p.pos)); mix(&p.type, 4); }
    for (const auto& l : lights) { mix(&l.pos, sizeof(l.pos)); h = (h ^ (l.on ? 1u : 0u)) * 1099511628211ull; }
    for (const auto& p : pillars) mix(&p, sizeof(p));
    return h;
}
static int runWalkBench(u32 seed, int level, int crossings) {
    gLmBenchHashOn = getenv("LM_BENCH_HASH") != nullptr;
    // LM_BENCH_HASH=1 verifies against the synchronous path; =2 verifies the caches with streaming on
    bool hashSync = gLmBenchHashOn && atoi(getenv("LM_BENCH_HASH")) != 2;
    gLmStreaming = !hashSync && getenv("LM_BENCH_SYNC") == nullptr;   // the game runs with streaming on
    unsigned long long runHash = 0;
    typedef std::chrono::high_resolution_clock Clk;
    auto ms = [](Clk::time_point a, Clk::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); };
    cam.pos = Vec3(8.0f * CS, PH, 8.0f * CS); cam.yaw = 0.0f; cam.pitch = 0.0f;
    chunks.clear(); rng.seed(seed); srand(seed);
    auto t0 = Clk::now();
    updateVisibleChunks(cam.pos.x, cam.pos.z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    resetMapContent();
    updateMapContent(playerChunkX, playerChunkZ);
    buildGeom(); glFinish();
    printf("initial load %.1f ms\n", ms(t0, Clk::now()));
    printf("%4s %8s %8s %8s %8s %8s %8s | %s\n", "#", "chunks", "lights", "content", "geom", "(limen)", "TOTAL", "slabs built");
    double worst = 0.0, sum = 0.0; std::vector<double> all;
    double worstSlice = 0.0; int maxSlices = 0;
    double sliceSum = 0.0; long long sliceCount = 0;
    auto pumpDeferred = [&]() {   // what the following frames pay inside the draw hook
        int n = 0;
        for (;;) {
            auto p0 = Clk::now();
            bool busy = lmRunFrameSlice(); glFinish();
            if (!busy) break;
            double t = ms(p0, Clk::now());
            if (t > worstSlice) worstSlice = t;
            sliceSum += t; sliceCount++;
            n++;
        }
        if (n > maxSlices) maxSlices = n;
    };
    pumpDeferred();
    lmProfReset();
    for (int k = 0; k < crossings; k++) {
        // diagonal-ish walk: mostly +Z, a step in +X every third crossing
        if (k % 3 == 2) cam.pos.x += CS * CHUNK_SIZE; else cam.pos.z += CS * CHUNK_SIZE;
        long long b0 = gLmPerf.slabsBuilt;
        auto a = Clk::now();
        updateVisibleChunks(cam.pos.x, cam.pos.z);
        auto b = Clk::now();
        updateLightsAndPillars(playerChunkX, playerChunkZ);
        auto c = Clk::now();
        updateMapContent(playerChunkX, playerChunkZ);
        auto d = Clk::now();
        gLmBenchPostMs = 0.0;
        buildGeom(); glFinish();
        auto e = Clk::now();
        double tot = ms(a, e);
        printf("%4d %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f | %lld\n", k, ms(a, b), ms(b, c), ms(c, d), ms(d, e), gLmBenchPostMs, tot, gLmPerf.slabsBuilt - b0);
        if (tot > worst) worst = tot;
        sum += tot; all.push_back(tot);
        pumpDeferred();
        if (getenv("LM_BENCH_PROPHASH")) printf("       settled prophash %016llx (%zu props, %zu pois)\n", benchPropHash(), mapProps.size(), mapPois.size());
        if (gLmBenchHashOn) {
            runHash = (runHash ^ benchPropHash()) * 1099511628211ull;
            runHash = (runHash ^ gLmBenchGeomHash) * 1099511628211ull;
            runHash = (runHash ^ gLmBenchPropVertHash) * 1099511628211ull;
            printf("       propvert %016llx\n", gLmBenchPropVertHash);
            printf("       props %zu pois %zu lights %zu  prophash %016llx geomhash %016llx\n", mapProps.size(), mapPois.size(), lights.size(), benchPropHash(), gLmBenchGeomHash);
        }
    }
    lmProfPrint((double)crossings);
    // lamp-behind-the-back event: what the game does when reshuffleBehind() fires
    double resh = 0.0; int reshN = 0;
    long long sb0 = gLmPerf.slabsBuilt;
    for (int k = 0; k < 64 && reshN < 4; k++) {
        auto a = Clk::now();
        if (!reshuffleBehind(cam.pos.x, cam.pos.z, (float)k * 0.785f)) {
            cam.pos.z -= 7.0f;   // look for the next lamp a little further back
            continue;
        }
        updateMapContent(playerChunkX, playerChunkZ);
        buildGeom(); glFinish();
        double t = ms(a, Clk::now());
        if (t > resh) resh = t;
        reshN++;
        pumpDeferred();
        if (gLmBenchHashOn) {
            runHash = (runHash ^ benchPropHash()) * 1099511628211ull;
            runHash = (runHash ^ gLmBenchGeomHash) * 1099511628211ull;
        }
    }
    printf("lamp events: %d fired, %lld slab meshes rebuilt, worst %.1f ms\n", reshN, gLmPerf.slabsBuilt - sb0, resh);
    if (gLmBenchHashOn) {
        runHash = (runHash ^ benchPropHash()) * 1099511628211ull;
        runHash = (runHash ^ gLmBenchGeomHash) * 1099511628211ull;
        printf("RUN HASH %016llx  (stale prop chunk meshes: %lld, stale slab meshes: %lld)\n", runHash, gLmBenchPropStale, gLmBenchSlabStale);
    }
    printf("slab meshes: %lld built, avg cpu %.2f ms + arch detail %.2f ms + upload %.2f ms\n", gLmPerf.slabsBuilt,
           gLmPerf.preBuildMs / (double)(gLmPerf.slabsBuilt ? gLmPerf.slabsBuilt : 1), gLmPerf.archBuildMs / (double)(gLmPerf.slabsBuilt ? gLmPerf.slabsBuilt : 1),
           gLmPerf.gpuUploadMs / (double)(gLmPerf.slabsBuilt ? gLmPerf.slabsBuilt : 1));
    std::sort(all.begin(), all.end());
    printf("crossing hitch: mean %.1f ms, median %.1f ms, worst %.1f ms | lamp event %.1f ms | streamed frame slices: mean %.1f ms, worst %.1f ms, up to %d frames per crossing | slab meshes cached %zu (in view %zu)\n",
           sum / (double)all.size(), all[all.size() / 2], worst, resh,
           sliceCount > 0 ? sliceSum / (double)sliceCount : 0.0, worstSlice, maxSlices, gLmSlabMeshes.size(), gLmVisibleSlabKeys.size());
    return 0;
}

// Narrator walk check: a player on foot with the narrator rewriting the unseen ahead, run
// through the game's own path (chunk cells, lights, furniture, slab and prop meshes).
//   limen_shots.exe narwalk <seed> <minutes> <level>
// Every so often the slab cache is thrown away, as the game does whenever the rig bores a cell
// or a wall is broken. Whatever is then rebuilt must be what was meshed before: a cached mesh
// or a chunk cell that differs is a piece of the old world left standing in the new one.
static int runNarWalk(u32 seed, int level, int minutes) {
    gLmBenchHashOn = false; gLmBenchHashKeep = true; gLmStreaming = true;     // (the self-check re-models what it checks: on only at the checkpoints)
    gLmNarBooted = true;
    gLmBenchStaleFn = [](int j, int i) {
        const LimenSlab& s = lmSlab(j, i);
        LimenRect r = s.rect;
        printf("  STALE MESH slab (%d,%d) x %d..%d z %d..%d: mega %d tiers %d halo %d, seen %d, t=%.1f (last: %s)\n", j, i, r.x0, r.x1, r.z0, r.z1,
               s.megaForm, s.ceilTiers, s.haloDir, (int)lmNarSlabSeen(j, i), (double)gNar.time, gNar.logCount ? gNar.log[(gNar.logCount - 1) % 10].text : "-");
    };
    gNar.busy = lmNarBusy; gNar.enabled = true; gNar.adaptive = true; gNar.savePath.clear();
    gNar.mem = LmNarMemory();
    const bool noRemesh = getenv("LM_NARWALK_NOREMESH") != nullptr;   // A/B: leave the neighbours of a rewritten slab as they were
    int cx = 8, cz = 8;
    for (int r = 0; r < 40 && !lmWalkable(cx, cz); r++) for (int dx = -r; dx <= r; dx++) for (int dz = -r; dz <= r; dz++)
        if (!lmWalkable(cx, cz) && lmWalkable(8 + dx, 8 + dz)) { cx = 8 + dx; cz = 8 + dz; }
    float fx = (cx + 0.5f) * CS, fz = (cz + 0.5f) * CS;
    cam.pos = Vec3(fx, PH, fz); cam.yaw = 0.0f; cam.pitch = 0.0f;
    chunks.clear(); rng.seed(seed); srand(seed);
    updateVisibleChunks(cam.pos.x, cam.pos.z);
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    resetMapContent();
    updateMapContent(playerChunkX, playerChunkZ);
    buildGeom(); glFinish();
    auto settle = [&]() { for (int k = 0; k < 400 && lmRunFrameSlice(); k++) {} };
    settle();
    const int R = 40, W = 2 * R + 1;
    std::vector<int> prev((size_t)W * W);
    std::vector<std::pair<int, int>> path;
    size_t pathPos = 0;
    float heading = 0.6f;
    int steps = (int)((float)minutes * 60.0f / 1.25f), rewrites = 0, drops = 0, stuck = 0;
    long long badCells = 0, crossings = 0;
    static const int ddx[4] = {1, -1, 0, 0}, ddz[4] = {0, 0, 1, -1};
    auto checkCells = [&]() {
        long long bad = 0;
        for (auto& p : chunks) {
            const Chunk& c = p.second;
            for (int lx = 0; lx < CHUNK_SIZE; lx++) for (int lz = 0; lz < CHUNK_SIZE; lz++)
                if (c.cells[lx][lz] != lmCell(c.cx * CHUNK_SIZE + lx, c.cz * CHUNK_SIZE + lz)) bad++;
        }
        return bad;
    };
    for (int s = 0; s < steps; s++) {
        if ((s % 150) == 149) heading += ((float)(lmHash((u32)s, seed, 3u, 0x524Du) % 200u) / 100.0f - 1.0f) * 1.1f;
        if (pathPos >= path.size()) {
            std::fill(prev.begin(), prev.end(), -2);
            std::vector<int> queue;
            prev[(size_t)R * W + R] = -1; queue.push_back(R * W + R);
            int best = R * W + R; float bestS = -1e9f;
            for (size_t head = 0; head < queue.size(); head++) {
                int c = queue[head], lx = c % W - R, lz = c / W - R;
                float sc = (float)lx * sinf(heading) + (float)lz * cosf(heading);
                if (sc > bestS) { bestS = sc; best = c; }
                for (int d = 0; d < 4; d++) {
                    int nx = lx + ddx[d], nz = lz + ddz[d];
                    if (nx < -R || nz < -R || nx > R || nz > R) continue;
                    int n = (nz + R) * W + nx + R;
                    if (prev[n] != -2 || !lmWalkable(cx + nx, cz + nz)) continue;
                    prev[n] = c; queue.push_back(n);
                }
            }
            path.clear(); pathPos = 0;
            for (int c = best; c >= 0 && prev[c] != -2; c = prev[c]) { path.push_back({cx + c % W - R, cz + c / W - R}); if (prev[c] == -1) break; }
            std::reverse(path.begin(), path.end());
            if (path.size() < 3) { heading += 1.3f; stuck++; }
            if (path.size() > 26) path.resize(26);
            pathPos = path.size() > 1 ? 1 : path.size();
            if (pathPos >= path.size()) continue;
        }
        int nx = path[pathPos].first, nz = path[pathPos].second;
        pathPos++;
        if (!lmWalkable(nx, nz)) { path.clear(); pathPos = 0; stuck++; continue; }
        float tx = (nx + 0.5f) * CS, tz = (nz + 0.5f) * CS;
        float yaw = atan2f(tx - fx, tz - fz);
        for (int q = 0; q < 5; q++) {
            fx += (tx - fx) / (float)(5 - q); fz += (tz - fz) / (float)(5 - q);
            cam.pos.x = fx; cam.pos.z = fz; cam.yaw = yaw + 0.5f * sinf((float)(s * 5 + q) * 0.35f);
            int n = lmNarTick(0.25f, fx, fz, cam.yaw, CS, 1.10f);
            if (n > 0) { rewrites += n; lmNarApplyDirty(); }
            else lmNarRemeshStep();     // (one stale neighbour a frame, as lmNarFrame does)
            if (noRemesh) { gLmNarRemesh.clear(); gLmNarRemeshRegather = false; }
        }
        cx = nx; cz = nz;
        int ccx = (int)floorf(fx / (CS * CHUNK_SIZE)), ccz = (int)floorf(fz / (CS * CHUNK_SIZE));
        if (ccx != playerChunkX || ccz != playerChunkZ) {
            crossings++;
            updateVisibleChunks(cam.pos.x, cam.pos.z);
            updateLightsAndPillars(playerChunkX, playerChunkZ);
            updateMapContent(playerChunkX, playerChunkZ);
            buildGeom();
            settle();
        }
        if ((s % 24) == 23) {
            // checkpoint: first the world as it stands, then again with the slab cache thrown away
            while (lmNarRemeshStep()) {}
            settle();
            long long s0 = gLmBenchSlabStale, p0 = gLmBenchPropStale;
            gLmBenchHashOn = true;
            buildGeom();
            long long s1 = gLmBenchSlabStale;
            lmSlabCacheClear();
            drops++;
            long long bad = checkCells();
            badCells += bad;
            updateLightsAndPillars(playerChunkX, playerChunkZ);
            buildGeom();
            gLmBenchHashOn = false;
            if (bad || gLmBenchSlabStale != s0 || gLmBenchPropStale != p0)
                printf("  t=%6.1f s cell (%d,%d): chunk cells off %lld, slab meshes stale %lld as it stands + %lld after the cache drop, prop meshes stale %lld\n", (double)gNar.time, cx, cz, bad,
                       s1 - s0, gLmBenchSlabStale - s1, gLmBenchPropStale - p0);
        }
    }
    printf("NARWALK seed %u L%d, %d min: %lld chunk crossings, %d slabs rewritten, %d cache drops | chunk cells off %lld, stale slab meshes %lld, stale prop meshes %lld | dead ends %d\n",
           seed, level, minutes, crossings, rewrites, drops, badCells, gLmBenchSlabStale, gLmBenchPropStale, stuck);
    return (badCells || gLmBenchSlabStale || gLmBenchPropStale) ? 2 : 0;
}

#include "limen_audit.h"

int main(int argc, char** argv) {
    bool bench = argc >= 2 && strcmp(argv[1], "bench") == 0;
    if (bench) { argv++; argc--; }
    bool narwalk = argc >= 2 && strcmp(argv[1], "narwalk") == 0;   // narwalk <seed> <minutes> <level>
    if (narwalk) { argv++; argc--; }
    bool audit = argc >= 2 && strcmp(argv[1], "audit") == 0;
    if (audit) { argv++; argc--; gLmAuditOn = true; }
    bool ray = argc >= 9 && strcmp(argv[1], "ray") == 0;   // ray <seed> <n> <level> <x> <z> <yaw> <pitch> [eyeY]
    if (ray) { argv++; argc--; gLmAuditOn = true; }
    bool at = argc >= 10 && strcmp(argv[1], "at") == 0;   // at <seed> <n> <level> <x> <z> <yaw> <pitch> <out.bmp> [eyeY]
    if (at) { argv++; argc--; }
    u32 seed = argc >= 2 ? (u32)strtoul(argv[1], 0, 10) : 1337u;
    int want = argc >= 3 ? atoi(argv[2]) : 24;
    int shotLevel = argc >= 4 ? atoi(argv[3]) : 0;
    char outDir[128], csvPath[260];
    snprintf(outDir, sizeof(outDir), "out/shots_L%d", shotLevel);
    snprintf(csvPath, sizeof(csvPath), "%s/shots.csv", outDir);
    _mkdir("out"); _mkdir(outDir);
    settings.aaMode = AA_MODE_FXAA;
    gCurrentLevel = shotLevel;
    if (getenv("LM_QUIRK")) gLmQuirkForce = atoi(getenv("LM_QUIRK"));
    if (getenv("LM_QUIRKVAR")) gLmQuirkVarForce = atoi(getenv("LM_QUIRKVAR"));   // force the level's main quirk (see LmQuirk)
    if (!trialInitGl()) { printf("GL init failed\n"); return 1; }
    trialFreezeState();
    worldSeed = seed;
    lmReset(seed, shotLevel);
    gLmPerf.reset();
    if (narwalk) { int r = runNarWalk(seed, shotLevel, want > 0 ? want : 6); glfwTerminate(); return r; }
    if (bench) { int r = runWalkBench(seed, shotLevel, want > 0 ? want : 24); glfwTerminate(); return r; }
    if (getenv("LM_STORY")) { gLaStory = atoi(getenv("LM_STORY")); gLmAuditOn = true; lmStoryBegin(seed, shotLevel, gLaStory); }
    if (argc >= 5 && strcmp(argv[4], "storyreport") == 0) {   // <seed> <n> <level> storyreport [verbose]
        for (int pass = 0; pass < 2; pass++) {   // around the rig, then around where it started
            Vec3 at = pass == 0 ? heist.drillPos : heist.rigStart;
            chunks.clear(); updateVisibleChunks(at.x, at.z);
            updateLightsAndPillars(playerChunkX, playerChunkZ); resetMapContent(); updateMapContent(playerChunkX, playerChunkZ);
            buildGeom();
            lmStoryReport(argc >= 6);
        }
        glfwTerminate(); return 0;
    }
    if (audit) { int r = lmRunGeomAudit(seed, shotLevel, want > 0 ? want : 9, argc >= 5 ? atoi(argv[4]) : 0); glfwTerminate(); return r; }
    if (ray) {
        int r = lmRunAuditRay(seed, (float)atof(argv[4]), (float)atof(argv[5]), (float)atof(argv[6]), (float)atof(argv[7]), argc >= 9 ? (float)atof(argv[8]) : -999.0f);
        glfwTerminate(); return r;
    }
    if (at) {
        ShotCam c = {(float)atof(argv[4]), (float)atof(argv[5]), (float)atof(argv[6]), (float)atof(argv[7]), true};
        shootAtCamera(seed, c, argv[8], false, argc >= 10 ? (float)atof(argv[9]) : -999.0f);
        glfwTerminate(); return 0;
    }
    FILE* log = fopen(csvPath, "w");
    if (log) {
        fprintf(log, "file,slab_j,slab_i,storeys,palette,wall,corr,room,hall,void,bridge,basin,low,vitrine");
        for (int k = 0; k < LG_COUNT; k++) fprintf(log, ",%s", lmGeneName(k));
        fprintf(log, ",cam_x,cam_z,yaw,pitch,mean_luma,hi_250_pct,lo_8_pct,frame_ms\n");
    }
    std::vector<u32> seen;
    int taken = 0, totalShots = 0;
    float maxHi = 0.0f, sumHi = 0.0f, maxLo = 0.0f, sumLo = 0.0f, sumFrameMs = 0.0f;
    auto accMetrics = [&](const ShotMetrics& m) {
        totalShots++;
        sumHi += m.hiPct; sumLo += m.loPct; sumFrameMs += m.frameMs;
        if (m.hiPct > maxHi) maxHi = m.hiPct;
        if (m.loPct > maxLo) maxLo = m.loPct;
    };
    for (int ring = 0; ring <= 10 && taken < want; ring++) {
        for (int j = -ring; j <= ring && taken < want; j++) {
            for (int i = -ring; i <= ring && taken < want; i++) {
                if (lmDepth(j, i) != ring) continue;
                const LimenSlab s = lmSlab(j, i);
                if (s.dark || s.fallback) continue;
                u32 sig = shotSignature(s);
                if (std::find(seen.begin(), seen.end(), sig) != seen.end()) continue;
                ShotCam c = pickSlabCamera(s, 0u);
                if (!c.ok) continue;
                seen.push_back(sig);
                char path[260];
                snprintf(path, sizeof(path), "%s/%02d_slab_%d_%d.bmp", outDir, taken, j, i);
                ShotMetrics m = shootAtCamera(seed, c, path);
                accMetrics(m);
                printf("%s  storeys %d  drop %d%%  rooms %d%%  luma %.1f  hi(>250) %.2f%%  lo(<8) %.2f%%  %.2fms\n",
                       path, s.ceilTiers, lmPitShare(s), s.share[LT_ROOM] + s.share[LT_LOW],
                       m.meanLuma, m.hiPct, m.loPct, m.frameMs);
                if (log) {
                    fprintf(log, "%s,%d,%d,%d,%d", path, j, i, s.ceilTiers, s.palette);
                    for (int t = 0; t < LT_TAG_COUNT; t++) fprintf(log, ",%d", s.share[t]);
                    for (int k = 0; k < LG_COUNT; k++) fprintf(log, ",%d", s.genome.g[k]);
                    fprintf(log, ",%.2f,%.2f,%.4f,%.4f,%.1f,%.2f,%.2f,%.2f\n",
                            c.x, c.z, c.yaw, c.pitch, m.meanLuma, m.hiPct, m.loPct, m.frameMs);
                }
                taken++;
            }
        }
    }

    // 5 dedicated interior shots INSIDE small LT_ROOM bays (including behind thin drywall partitions)
    ShotCam roomCams[5];
    int nRooms = collectInteriorRoomShots(roomCams, 5);
    for (int rk = 0; rk < nRooms; rk++) {
        char path[260];
        snprintf(path, sizeof(path), "%s/room_%02d.bmp", outDir, rk);
        ShotMetrics m = shootAtCamera(seed, roomCams[rk], path, true);
        accMetrics(m);
        printf("%s  luma %.1f  hi(>250) %.2f%%  lo(<8) %.2f%%  %.2fms\n", path, m.meanLuma, m.hiPct, m.loPct, m.frameMs);
    }

    std::vector<long long> usedMotifSlabs;
    auto pickMegaPair = [&](int mf, int var) -> ShotCam {
        // First prefer pure mega-slabs without an Escher knot so mega motifs stay uncluttered!
        for (int allowEscher = 0; allowEscher < 2; allowEscher++) {
            if (mf == MF_ZIGGURAT) {
                ShotCam cp = searchSlabs([&](const LimenSlab& s) {
                    if (s.megaForm != MF_ZIGGURAT) return ShotCam{0,0,0,0,false};
                    if (!allowEscher && lmSlabHasEscherKnot(s)) return ShotCam{0,0,0,0,false};
                    if (var == 0 && (s.megaSub & 3) != 1) return ShotCam{0,0,0,0,false};
                    if (var == 1 && (s.megaSub & 3) == 1) return ShotCam{0,0,0,0,false};
                    return pickMegaFormVista(s, MF_ZIGGURAT, var);
                }, &usedMotifSlabs);
                if (cp.ok) return cp;
            }
            ShotCam c = searchSlabs([&](const LimenSlab& s) {
                if (!allowEscher && lmSlabHasEscherKnot(s)) return ShotCam{0,0,0,0,false};
                return pickMegaFormVista(s, mf, var);
            }, (mf == MF_SILO && var == 1) ? nullptr : &usedMotifSlabs);
            if (c.ok) return c;
        }
        if (var == 1) return searchSlabs([&](const LimenSlab& s) { return pickMegaFormVista(s, mf, 1); }, nullptr);
        return {0,0,0,0,false};
    };

    auto searchBrightDrunk = [&](auto&& fn) -> ShotCam {
        ShotCam c = searchSlabs([&](const LimenSlab& s) { return fn(s, true); }, &usedMotifSlabs);
        if (!c.ok) c = searchSlabs([&](const LimenSlab& s) { return fn(s, false); }, &usedMotifSlabs);
        return c;
    };

    struct FeatureShot { const char* name; ShotCam cam; };
    FeatureShot feats[] = {
        {"mega_ziggurat_a",  pickMegaPair(MF_ZIGGURAT, 0)},
        {"mega_ziggurat_b",  pickMegaPair(MF_ZIGGURAT, 1)},
        {"mega_canyon_a",    pickMegaPair(MF_CANYON, 0)},
        {"mega_canyon_b",    pickMegaPair(MF_CANYON, 1)},
        {"mega_hypostyle_a", pickMegaPair(MF_HYPOSTYLE, 0)},
        {"mega_hypostyle_b", pickMegaPair(MF_HYPOSTYLE, 1)},
        {"mega_silo_a",      pickMegaPair(MF_SILO, 0)},
        {"mega_silo_b",      pickMegaPair(MF_SILO, 1)},
        {"mega_hanging_a",   pickMegaPair(MF_HANGING, 0)},
        {"mega_hanging_b",   pickMegaPair(MF_HANGING, 1)},
        {"mega_cellblock_a", pickMegaPair(MF_CELLBLOCK, 0)},
        {"mega_cellblock_b", pickMegaPair(MF_CELLBLOCK, 1)},
        {"mega_platform_a",  pickMegaPair(MF_PLATFORM, 0)},
        {"mega_platform_b",  pickMegaPair(MF_PLATFORM, 1)},
        {"mega_monolith_a",  pickMegaPair(MF_MONOLITH, 0)},
        {"mega_monolith_b",  pickMegaPair(MF_MONOLITH, 1)},
        {"mega_spires_a",    pickMegaPair(MF_SPIRES, 0)},
        {"mega_spires_b",    pickMegaPair(MF_SPIRES, 1)},
        {"mega_grotto_a",    pickMegaPair(MF_GROTTO, 0)},
        {"mega_grotto_b",    pickMegaPair(MF_GROTTO, 1)},
        {"mega_rootfall_a",  pickMegaPair(MF_TENDRILS, 0)},
        {"mega_rootfall_b",  pickMegaPair(MF_TENDRILS, 1)},
        {"drunk_pocket",     searchBrightDrunk(pickDrunkPocket)},
        {"drunk_escher",     searchSlabs(pickDrunkEscher, &usedMotifSlabs)},
        {"drunk_nested",     searchBrightDrunk(pickDrunkNested)},
        {"drunk_spiral",     searchBrightDrunk(pickDrunkSpiral)},
        {"severed_span",       searchSlabs(pickSeveredSpan)},
        {"low_shell_in_vault", searchSlabs(pickLowShell)},
        {"low_to_high",        searchSlabs(pickLowToHigh)},
        {"threshold_to_drop",  searchSlabs(pickThreshold)},
        {"basin_stair",        searchSlabs(pickBasinStair)},
        {"narrow_corridor",    searchSlabs(pickNarrowHumanCorridor)},
        {"xenograft_cut",      searchSlabs(pickXenograftCut)},
        {"vitrine_window",     searchSlabs(pickVitrineWindow)},
        {"wall_door",          searchSlabs(pickWallDoor)},
        {"ceil_stair",         searchSlabs(pickCeilStair)},
        {"plug_corridor",      searchSlabs(pickPluggedCorridor)}
    };
    const int nFeats = (int)(sizeof(feats) / sizeof(feats[0]));
    for (int k = 0; k < nFeats; k++) {
        if (!feats[k].cam.ok) { printf("motif %s: not found near the origin\n", feats[k].name); continue; }
        char path[260];
        snprintf(path, sizeof(path), "%s/motif_%s.bmp", outDir, feats[k].name);
        ShotMetrics m = shootAtCamera(seed, feats[k].cam, path, true);
        accMetrics(m);
        printf("%s  luma %.1f  hi(>250) %.2f%%  lo(<8) %.2f%%  %.2fms\n", path, m.meanLuma, m.hiPct, m.loPct, m.frameMs);
    }
    if (totalShots > 0) {
        printf("exposure summary (%d shots): max hi(>250)=%.2f%% mean hi=%.2f%% | max lo(<8)=%.2f%% mean lo=%.2f%%\n",
               totalShots, maxHi, sumHi / totalShots, maxLo, sumLo / totalShots);
        double ns = gLmPerf.slabsBuilt > 0 ? (double)gLmPerf.slabsBuilt : 1.0;
        double preV = gLmPerf.preTotalVerts / ns;
        double addV = gLmPerf.archAddedVerts / ns;
        double postV = preV + addV;
        double preMs = gLmPerf.preBuildMs / ns;
        double archMs = gLmPerf.archBuildMs / ns;
        double upMs = gLmPerf.gpuUploadMs / ns;
        printf("perf summary (%lld slabs, %d shots): verts/slab pre=%.0f -> post=%.0f (+%.0f arch, %.2fx) | mesh build pre=%.2fms + arch=%.2fms + vbo=%.2fms = %.2fms | render %.2f ms/frame\n",
               gLmPerf.slabsBuilt, totalShots, preV, postV, addV, preV > 0.0 ? (postV / preV) : 1.0,
               preMs, archMs, upMs, preMs + archMs + upMs, sumFrameMs / totalShots);
    }

    if (log) fclose(log);
    glfwTerminate();
    return 0;
}
