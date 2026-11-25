#pragma once
#include "limen_world_mesh_scratch.h"

inline void lmPostBuildGeomImpl() {
    lmReset(worldSeed, gCurrentLevel);
    lmSyncStoryFromGame();
    int pcx = playerChunkX, pcz = playerChunkZ;
    { LM_PROF("post:envmap"); lmUpdateEnvMap(pcx, pcz); }

    lmEnsureLevelTextures();
    static LmWorldMeshScratch scratch;
    auto& wv = scratch.walls;
    auto& fv = scratch.floors;
    auto& cv = scratch.ceilings;
    auto& pv = scratch.pillars;
    auto& lv = scratch.lamps;
    auto& lvOff = scratch.darkLamps;
    auto& dv = scratch.decor;
    const bool skyOpen = lmHasQuirk(gLimenDNA, LQ_SKY);   // one-storey bays stand roofless
    gLmVisibleSlabKeys.clear();
    gLmVestigeColliders.clear();

    auto cellCeilH = [&](int wx, int wz) -> float {
        LimenRect r = lmSlabAt(wx, wz);
        if (wx == r.x0 || wz == r.z0) return WH;
        const LimenSlab& s = lmSlab(r.j, r.i);
        int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
        return lmShellCeilHeight(s.flag[lx][lz], s.ceilTiers, WH, s.zone[lx][lz]);
    };

    {LM_PROF("post:cells");
    for (int dcx = -LM_VIEW_CHUNKS; dcx <= LM_VIEW_CHUNKS; dcx++) {
        for (int dcz = -LM_VIEW_CHUNKS; dcz <= LM_VIEW_CHUNKS; dcz++) {
            auto it = chunks.find(chunkKey(pcx + dcx, pcz + dcz));
            if (it == chunks.end()) continue;
            for (int lx = 0; lx < CHUNK_SIZE; lx++) {
                for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                    int wx = (pcx + dcx) * CHUNK_SIZE + lx, wz = (pcz + dcz) * CHUNK_SIZE + lz;
                    LimenRect r = lmSlabAt(wx, wz);
                    bool onSeam = (wx == r.x0 || wz == r.z0);
                    if (!onSeam) {
                        long long skey = (long long)(((unsigned long long)(u32)r.j << 32) | (unsigned long long)(u32)r.i);
                        bool seen = false;
                        for (long long k : gLmVisibleSlabKeys) if (k == skey) { seen = true; break; }
                        if (!seen) gLmVisibleSlabKeys.push_back(skey);
                    }

                    // Skip walls & non-walkable drops (`1` and `2`) here; slab cache handles vault drops!
                    if (it->second.cells[lx][lz] != 0) continue;

                    const LimenSlab* sp = onSeam ? nullptr : &lmSlab(r.j, r.i);
                    bool isMega = sp && lmSlabIsVault(*sp);
                    int slx = onSeam ? -1 : (wx - r.x0 - 1);
                    int slz = onSeam ? -1 : (wz - r.z0 - 1);
                    int tag = onSeam ? LT_CORR : sp->tag[slx][slz];
                    if (isMega && !lmIsLowShellTag(tag)) continue;  // drawn from `LmCachedSlabMesh`!

                    unsigned short flg = onSeam ? 0u : sp->flag[slx][slz];
                    unsigned char zn   = onSeam ? 0u : sp->zone[slx][slz];
                    bool inXeno = !onSeam && sp->xeno.active && (zn & LZ_XENO) != 0;
                    float px = wx * CS, pz = wz * CS;
                    float cH = lmShellCeilHeight(flg, sp ? sp->ceilTiers : 1, WH, zn);

                    bool hasHole = isFloorHoleCell(wx, wz) || isAbyssCell(wx, wz);
                    // Warped floors and ceilings are pure functions of the lattice corner (they vanish
                    // by themselves next to seams, vaults, xeno cuts...), so every cell sharing a
                    // corner agrees on its height and the surfaces stay watertight.
                    bool warp = !hasHole && !inXeno && !isMega;
                    float yF00 = warp ? lmDrunkVertexFloorY(wx,     wz)     : 0.0f;
                    float yF01 = warp ? lmDrunkVertexFloorY(wx,     wz + 1) : 0.0f;
                    float yF11 = warp ? lmDrunkVertexFloorY(wx + 1, wz + 1) : 0.0f;
                    float yF10 = warp ? lmDrunkVertexFloorY(wx + 1, wz)     : 0.0f;

                    float yC00 = cH + (warp ? lmDrunkVertexCeilOffset(wx,     wz)     : 0.0f);
                    float yC01 = cH + (warp ? lmDrunkVertexCeilOffset(wx,     wz + 1) : 0.0f);
                    float yC11 = cH + (warp ? lmDrunkVertexCeilOffset(wx + 1, wz + 1) : 0.0f);
                    float yC10 = cH + (warp ? lmDrunkVertexCeilOffset(wx + 1, wz)     : 0.0f);

                    if (!hasHole) {
                        if (!inXeno) {
                            Vec3 nF = Vec3(yF00 - yF10, CS, yF00 - yF01).norm();
                            float fl[] = {
                                px,    yF00,pz,    0,0,nF.x,nF.y,nF.z, px,    yF01,pz+CS, 0,1,nF.x,nF.y,nF.z, px+CS,yF11,pz+CS, 1,1,nF.x,nF.y,nF.z,
                                px,    yF00,pz,    0,0,nF.x,nF.y,nF.z, px+CS,yF11,pz+CS, 1,1,nF.x,nF.y,nF.z, px+CS,yF10,pz,    1,0,nF.x,nF.y,nF.z
                            };
                            fv.insert(fv.end(), fl, fl + 48);
                        }
                    } else {
                        const float sd = 30.0f;
                        if (getCellWorld(wx-1,wz)==1 || (!isFloorHoleCell(wx-1,wz) && !isAbyssCell(wx-1,wz))) mkShaftWall(wv,px,pz,0,CS,0,sd,CS);
                        if (getCellWorld(wx+1,wz)==1 || (!isFloorHoleCell(wx+1,wz) && !isAbyssCell(wx+1,wz))) mkShaftWall(wv,px+CS,pz+CS,0,-CS,0,sd,CS);
                        if (getCellWorld(wx,wz-1)==1 || (!isFloorHoleCell(wx,wz-1) && !isAbyssCell(wx,wz-1))) mkShaftWall(wv,px+CS,pz,-CS,0,0,sd,CS);
                        if (getCellWorld(wx,wz+1)==1 || (!isFloorHoleCell(wx,wz+1) && !isAbyssCell(wx,wz+1))) mkShaftWall(wv,px,pz+CS,CS,0,0,sd,CS);
                    }

                    if (!inXeno && !(skyOpen && !isMega && cH >= WH - 0.01f)) {
                        Vec3 nC = Vec3(yC10 - yC00, -CS, yC01 - yC00).norm();
                        float cl[] = {
                            px,    yC00,pz,    0,0,nC.x,nC.y,nC.z, px,    yC01,pz+CS, 0,1,nC.x,nC.y,nC.z, px+CS,yC11,pz+CS, 1,1,nC.x,nC.y,nC.z,
                            px,    yC00,pz,    0,0,nC.x,nC.y,nC.z, px+CS,yC11,pz+CS, 1,1,nC.x,nC.y,nC.z, px+CS,yC10,pz,    1,0,nC.x,nC.y,nC.z
                        };
                        cv.insert(cv.end(), cl, cl + 48);
                    }

                    // Walls reach from under the lowest floor corner to above the highest ceiling corner
                    float minF = fminf(fminf(yF00, yF01), fminf(yF11, yF10));
                    float maxC = fmaxf(fmaxf(yC00, yC01), fmaxf(yC11, yC10));
                    float wallBot = (minF < -0.001f) ? (minF - 0.08f) : 0.0f;
                    float wallTop = (maxC > cH + 0.001f) ? (maxC + 0.10f) : cH;
                    if (!inXeno) {
                        auto emitHWall = [&](int nwx, int nwz, float bx, float bz, float wdx, float wdz) {
                            if (getCellWorld(nwx, nwz) == 1 || lmTagAt(nwx, nwz) == LT_VITRINE) {
                                if (lmIsVitrineWindowWorld(nwx, nwz)) lmEmitWindowedWall(wv, bx, bz, wdx, wdz, wallTop, lmVitrineSillY(nwx, nwz), wallBot);
                                else if (wallBot < -0.01f) lmPushWallSpan(wv, bx, wallBot, bz, wdx, wdz, wallTop);
                                else mkWall(wv, bx, bz, wdx, wdz, wallTop, CS, WH);
                            } else {
                                float nH = cellCeilH(nwx, nwz);
                                if (cH > nH + 0.05f) {
                                    // The wall above a lower neighbour's ceiling, and under it a real
                                    // 20 cm thick downstand beam (it used to be a sheet with no thickness).
                                    lmPushWallSpan(wv, bx, nH, bz, wdx, wdz, wallTop);
                                    // A tall shell standing in a vault hall shows this wall from outside
                                    // too (toward a lower SHELL the slab mesh draws that flank itself).
                                    bool tallOut = false;   // the beam below grows into a full 20 cm outer wall
                                    if (isMega && !lmIsLowShellTag(lmTagAt(nwx, nwz))) {
                                        // (where this cell is a narrowed corridor its wing ends are that wall)
                                        unsigned short sf = (zn & LZ_PLUG_CORR) ? (unsigned short)0 : flg;
                                        bool alongZ = fabsf(wdx) < 0.01f;
                                        bool ownAlong = alongZ ? (sf & LF_NARROW_Z) != 0 : (sf & LF_NARROW_X) != 0;
                                        bool ownEnds  = alongZ ? (sf & LF_NARROW_X) != 0 : (sf & LF_NARROW_Z) != 0;
                                        float q0 = ownEnds ? LM_NARROW_INSET / CS : 0.0f, q1 = 1.0f - q0;
                                        if (ownEnds)
                                            lmPushWallSpan(wv, bx + wdx * q1, nH, bz + wdz * q1, -wdx * (q1 - q0), -wdz * (q1 - q0), wallTop);
                                        else if (!ownAlong) tallOut = true;
                                    }
                                    const float bt = 0.20f, bh = 0.35f, in = LM_NARROW_INSET;
                                    float ex = wdx / CS, ez = wdz / CS;          // along the edge
                                    float mx = -ez, mz = ex;                       // into the low cell
                                    int eix = (int)lroundf(ex), eiz = (int)lroundf(ez);
                                    unsigned short nf = lmFlagAt(nwx, nwz);
                                    if (lmZoneAt(nwx, nwz) & LZ_PLUG_CORR) nf &= (unsigned short)~(LF_NARROW_X | LF_NARROW_Z);
                                    bool edgeAlongZ = (eix == 0);
                                    bool wingAlong = edgeAlongZ ? (nf & LF_NARROW_Z) != 0 : (nf & LF_NARROW_X) != 0;
                                    bool wingEnds  = edgeAlongZ ? (nf & LF_NARROW_X) != 0 : (nf & LF_NARROW_Z) != 0;
                                    bool thinHere = false;   // a partition stands in this very edge
                                    {
                                        int ownX = (nwx > wx || nwz > wz) ? nwx : wx, ownZ = (nwx > wx || nwz > wz) ? nwz : wz;
                                        unsigned short of = lmFlagAt(ownX, ownZ);
                                        thinHere = edgeAlongZ ? (of & LF_THIN_W) != 0 : (of & LF_THIN_N) != 0;
                                    }
                                    if (thinHere) {
                                        lmPushWallSpan(wv, bx, nH - bh, bz, wdx, wdz, nH);
                                        if (tallOut) lmPushWallSpan(wv, bx + wdx, nH, bz + wdz, -wdx, -wdz, wallTop);
                                    } else if (!wingAlong) {
                                        float a0 = 0.0f, a1 = CS;
                                        bool cap0 = true, cap1 = true;
                                        if (wingEnds) { a0 = in; a1 = CS - in; cap0 = cap1 = false; }
                                        else if (edgeAlongZ) {
                                            // where two beams meet in a corner of the low cell, this one stops short
                                            if (getCellWorld(nwx - eix, nwz - eiz) == 0 && cellCeilH(nwx - eix, nwz - eiz) > nH + 0.05f) { a0 = bt; cap0 = false; }
                                            if (getCellWorld(nwx + eix, nwz + eiz) == 0 && cellCeilH(nwx + eix, nwz + eiz) > nH + 0.05f) { a1 = CS - bt; cap1 = false; }
                                        }
                                        float y0 = nH - bh;
                                        float ax = bx + ex * a0, az = bz + ez * a0, cx2 = bx + ex * a1, cz2 = bz + ez * a1;
                                        float len = a1 - a0;
                                        float yT = tallOut ? wallTop : nH;
                                        lmPushWallSpan(wv, ax, y0, az, ex * len, ez * len, nH);
                                        lmPushWallSpan(wv, cx2 + mx * bt, y0, cz2 + mz * bt, -ex * len, -ez * len, yT);
                                        lmPushQuad(wv, Vec3(ax, y0, az), Vec3(cx2, y0, cz2), Vec3(cx2 + mx * bt, y0, cz2 + mz * bt),
                                                   Vec3(ax + mx * bt, y0, az + mz * bt), CS);
                                        if (cap0 || tallOut) lmPushWallSpan(wv, ax + mx * bt, y0, az + mz * bt, -mx * bt, -mz * bt, yT);
                                        if (cap1 || tallOut) lmPushWallSpan(wv, cx2, y0, cz2, mx * bt, mz * bt, yT);
                                        if (tallOut) lmPushQuad(wv, Vec3(ax + mx * bt, yT, az + mz * bt), Vec3(cx2 + mx * bt, yT, cz2 + mz * bt),
                                                                Vec3(cx2, yT, cz2), Vec3(ax, yT, az), CS);
                                    }
                                }
                            }
                        };
                        emitHWall(wx - 1, wz, px, pz, 0.0f, CS);
                        emitHWall(wx + 1, wz, px + CS, pz + CS, 0.0f, -CS);
                        emitHWall(wx, wz - 1, px + CS, pz, -CS, 0.0f);
                        emitHWall(wx, wz + 1, px, pz + CS, CS, 0.0f);
                        if (flg & LF_THIN_W) lmEmitThinPartition(wv, dv, px, pz, true, (flg & LF_THIN_DOOR_W) != 0, wallTop, wallBot);
                        if (flg & LF_THIN_N) lmEmitThinPartition(wv, dv, px, pz, false, (flg & LF_THIN_DOOR_N) != 0, wallTop, wallBot);
                        if ((zn & LZ_PLUG_CORR) == 0 && (flg & (LF_NARROW_X | LF_NARROW_Z))) lmEmitNarrowCorridor(wv, wx, wz, px, pz, flg, wallTop, wallBot);
                        lmEmitThinJoints(wv, wx, wz, wallBot, [&](int cx, int cz) { return fmaxf(wallTop, cellCeilH(cx, cz) + (wallTop - cH)); });
                    }

                    LmDoorway dw = lmDoorwayAt(wx, wz);   // same record the collider reads
                    if (dw.axis != 0) {
                        if (dw.drunk || fabsf(wallTop - WH) > 0.03f) lmBuildDrunkDoorway(dv, wx, wz, dw, wallTop);
                        else buildDoorwayModel(dv, wx, wz, dw.axis);
                    }
                }
            }
        }
    }

    }
    // Slabs the next chunk crossing (in any direction) will bring into view. Their meshes are
    // built one per frame by `lmRunFrameSlice`, so the crossing itself finds them ready.
    gLmPrewarmSlabKeys.clear();
    if (gLmStreaming) {
        int w0x = (pcx - LM_VIEW_CHUNKS - 1) * CHUNK_SIZE, w1x = (pcx + LM_VIEW_CHUNKS + 2) * CHUNK_SIZE - 1;
        int w0z = (pcz - LM_VIEW_CHUNKS - 1) * CHUNK_SIZE, w1z = (pcz + LM_VIEW_CHUNKS + 2) * CHUNK_SIZE - 1;
        auto consider = [&](int wx, int wz) {
            LimenRect r = lmSlabAt(wx, wz);
            long long skey = (long long)(((unsigned long long)(u32)r.j << 32) | (unsigned long long)(u32)r.i);
            for (long long k : gLmVisibleSlabKeys) if (k == skey) return;
            for (long long k : gLmPrewarmSlabKeys) if (k == skey) return;
            gLmPrewarmSlabKeys.push_back(skey);
        };
        const int step = 8;   // slabs are at least 20 cells across
        for (int wz = w0z; ; wz += step) {
            if (wz > w1z) wz = w1z;
            for (int wx = w0x; ; wx += step) {
                if (wx > w1x) wx = w1x;
                // only the ring outside the view; the view itself is in gLmVisibleSlabKeys
                bool inner = wx >= w0x + CHUNK_SIZE && wx <= w1x - CHUNK_SIZE && wz >= w0z + CHUNK_SIZE && wz <= w1z - CHUNK_SIZE;
                if (!inner) consider(wx, wz);
                if (wx == w1x) break;
            }
            if (wz == w1z) break;
        }
    }
    gLmSlabMeshesWanted = gLmVisibleSlabKeys.size() + gLmPrewarmSlabKeys.size() + 4;
    // Ensure GPU slab caches exist for all visible slabs & gather their light quads + colliders
    {LM_PROF("post:slabmesh");
    for (long long skey : gLmVisibleSlabKeys) {
        int sj = (int)(skey >> 32), si = (int)(skey & 0xFFFFFFFFull);
        const LmCachedSlabMesh& cm = lmEnsureSlabMesh(sj, si);
        if (!cm.lv.empty())    lv.insert(lv.end(), cm.lv.begin(), cm.lv.end());
        if (!cm.lvOff.empty()) lvOff.insert(lvOff.end(), cm.lvOff.begin(), cm.lvOff.end());
        if (!cm.cols.empty())  gLmVestigeColliders.insert(gLmVestigeColliders.end(), cm.cols.begin(), cm.cols.end());
    }

    }
    LM_PROF("post:tail");
    bool indTrim = (gLimenDNA.wallSurf.stripe > 155 && gLimenDNA.wallSurf.sheen < 120) ||
                   ((int)gLimenDNA.themeWeight[1] + (int)gLimenDNA.themeWeight[2] + (int)gLimenDNA.themeWeight[3] > 230);
    for (auto& p : pillars) {
        int pwx = (int)floorf(p.x / CS), pwz = (int)floorf(p.z / CS);
        float pCeilH = cellCeilH(pwx, pwz);
        int pDel = lmDeliriumCell255(pwx, pwz);
        if (pDel >= 80 && fabsf(pCeilH - WH) < 0.05f) {
            u32 hp = lmHash((u32)pwx, (u32)pwz, 13u, 0x504Cu);
            float amt = (float)(pDel - 80) / 175.0f;
            float lx = cosf((float)(hp % 64u) * 0.098f) * (0.35f + 0.65f * amt);
            float lz = sinf((float)(hp % 64u) * 0.098f) * (0.35f + 0.65f * amt);
            lmPushLeaningPillar(pv, p.x, p.z, lx, lz, PILLAR_SIZE, -0.35f, pCeilH + 0.85f);
        } else {
            mkPillar(pv, p.x, p.z, PILLAR_SIZE, pCeilH);
            if (fabsf(pCeilH - WH) < 0.05f) buildPillarTrimModel(dv, p, indTrim);
        }
    }
    {LM_PROF("post:propmodels");
    lmSyncChunkPropMeshes(LM_PROPSYNC_NEAR);   // props live in per-chunk VAOs, not in the shared decor buffer
#ifdef LIMEN_SHOTS
    if (gLmBenchHashOn) {
        std::vector<float> ref;
        for (const auto& pr : mapProps) lmAppendPropModel(ref, pr);
        for (const auto& poi : mapPois) lmAppendPoiModel(ref, poi);
        gLmBenchPropVertHash = lmFnv64(ref.data(), ref.size() * sizeof(float));
    }
#endif
    }
    if (coop.initialized) {
        buildHeistModels(dv);
        // the game's own sets rely on its z-fight guard (decals laid in the plane of their panels)
        ZfMesh zm[1] = {{&dv, 8, 2, "decor"}};
        lmResolveZfight(zm, 1);
    }
    LM_PROF("post:lamps");
    for (auto& l : lights) {
        int wx = (int)floorf(l.pos.x / CS), wz = (int)floorf(l.pos.z / CS);
        int ltag = lmTagAt(wx, wz);
        if (lmHighAt(wx, wz) || ltag == LT_VITRINE || lmIsPitTag(ltag)) continue;
        int lDel = lmDeliriumCell255(wx, wz);
        if (lDel >= 95 && l.on) {
            u32 hl = lmHash((u32)wx, (u32)wz, 19u, 0x4C44u);
            if (lDel >= 130 && (hl % 3u) != 2u) {
                // Chaotic cluster of 5 hanging/tilted lamps + 1 fallen lamp on the floor!
                float gy = lmGroundHeightAt(l.pos.x, l.pos.z, CS) + 0.03f;
                lmPushLightQuad(lv, Vec3(l.pos.x-0.6f, gy, l.pos.z-0.55f), Vec3(l.pos.x+0.6f, gy, l.pos.z-0.35f),
                                    Vec3(l.pos.x+0.5f, gy, l.pos.z+0.60f), Vec3(l.pos.x-0.7f, gy, l.pos.z+0.40f), true);
                float cy0 = l.pos.y + lmDrunkVertexCeilOffset(wx, wz) - 0.12f;
                for (int sw = 0; sw < 5; sw++) {
                    float ox = -1.3f + (float)(sw % 3) * 0.78f, oz = -1.1f + (float)(sw / 3) * 0.88f;
                    float ly = fmaxf(cy0 - (0.10f + (float)sw * 0.16f), 3.08f);
                    lmPushLightQuad(lv, Vec3(l.pos.x+ox-0.34f, ly, l.pos.z+oz-0.34f),
                                        Vec3(l.pos.x+ox+0.34f, ly + 0.12f, l.pos.z+oz-0.34f),
                                        Vec3(l.pos.x+ox+0.34f, ly + 0.12f, l.pos.z+oz+0.34f),
                                        Vec3(l.pos.x+ox-0.34f, ly, l.pos.z+oz+0.34f), true);
                }
                continue;
            }
            // Swimming / drifting lamp following pitched ceiling!
            float dx = (((float)(hl & 15u) - 7.5f) * 0.14f), dz = (((float)((hl >> 4) & 15u) - 7.5f) * 0.14f);
            float cy = l.pos.y + lmCeilWarpUnderPanel(l.pos.x + dx, l.pos.z + dz, l.sizeX, l.sizeZ) - 0.04f;
            mkLight(lv, Vec3(l.pos.x + dx, cy, l.pos.z + dz), l.sizeX, l.sizeZ);
            continue;
        }
        Vec3 lp = l.pos;
        if (lmCellCeilWarped(wx, wz)) lp.y += fminf(0.0f, lmCeilWarpUnderPanel(lp.x, lp.z, l.sizeX, l.sizeZ));
        if (l.on) mkLight(lv, lp, l.sizeX, l.sizeZ);
        else mkLight(lvOff, lp, l.sizeX, l.sizeZ);
    }

#ifdef LIMEN_SHOTS
    if (gLmBenchHashOn || gLmBenchHashKeep) {
        unsigned long long h = 1469598103934665603ull;
        const std::vector<float>* vs[7] = {&wv, &fv, &cv, &pv, &dv, &lv, &lvOff};
        for (int k = 0; k < 7; k++) {
            const unsigned char* b = (const unsigned char*)vs[k]->data();
            size_t n = vs[k]->size() * sizeof(float);
            h = (h ^ (unsigned long long)n) * 1099511628211ull;
            for (size_t q = 0; q < n; q++) h = (h ^ b[q]) * 1099511628211ull;
        }
        for (const auto& c : gLmVestigeColliders) {
            const unsigned char* b = (const unsigned char*)&c;
            for (size_t q = 0; q < sizeof(c); q++) h = (h ^ b[q]) * 1099511628211ull;
        }
        gLmBenchGeomHash = h;
    }
#endif
#ifdef LIMEN_SHOTS
    if (gLmAuditOn) {
        gLmAuditWorld.clear();
        lmAuditAppend(gLmAuditWorld, wv, LA_WALL, false);  lmAuditAppend(gLmAuditWorld, fv, LA_FLOOR, false);
        lmAuditAppend(gLmAuditWorld, cv, LA_CEIL, true);   lmAuditAppend(gLmAuditWorld, pv, LA_PILLAR, false);
        lmAuditAppend(gLmAuditWorld, dv, LA_DECOR, true);
    }
#endif
    LM_PROF("post:upload");
    wallVC = (int)wv.size() / 8; floorVC = (int)fv.size() / 8; ceilVC = (int)cv.size() / 8;
    pillarVC = (int)pv.size() / 8; decorVC = (int)dv.size() / 8;
    lightVC = (int)lv.size() / 5; lightOffVC = (int)lvOff.size() / 5;

    setupVAO(wallVAO, wallVBO, wv, true);
    setupVAO(floorVAO, floorVBO, fv, true);
    setupVAO(ceilVAO, ceilVBO, cv, true);
    setupVAO(pillarVAO, pillarVBO, pv, true);
    if (decorVC > 0) setupVAO(decorVAO, decorVBO, dv, true);
    setupVAO(lightVAO, lightVBO, lv, false);
    if (!lvOff.empty()) setupVAO(lightOffVAO, lightOffVBO, lvOff, false);
    scratch.resetAfterUpload();
}
