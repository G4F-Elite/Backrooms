#pragma once
// Part of limen_mega.h: architectural detail of a slab.

inline void lmBuildSlabArchDetail(const LimenSlab& s, std::vector<float>& dadoV,
                                  std::vector<float>& dv, std::vector<float>& xDadoV) {
    const bool slabVault = lmSlabIsVault(s);
    const int decay = (int)s.genome.g[LG_DECAY];
    const int order = (int)s.genome.g[LG_ORDER];
    const int useG  = (int)s.genome.g[LG_USE];
    LmSlabArchSpec hostSpec = lmMakeSlabArchSpec(gLimenDNA, s.genome, slabVault);
    LimenLevelDNA xenoDNA{};
    LmSlabArchSpec xenoSpec = hostSpec;
    if (s.xeno.active) {
        xenoDNA = lmMakeLevelDNA(worldSeed, s.xeno.foreignLevel);
        LimenGenome xg = s.genome;
        xg.g[LG_HUE] = s.xeno.hue;
        xg.g[LG_USE] = s.xeno.useGene;
        xenoSpec = lmMakeSlabArchSpec(xenoDNA, xg, false);
    }

    auto cellCeilWorld = [&](int wx, int wz) -> float {
        LimenRect r = lmSlabAt(wx, wz);
        if (wx == r.x0 || wz == r.z0) return WH;
        const LimenSlab& ns = lmSlab(r.j, r.i);
        int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
        int nt = ns.tag[lx][lz];
        if (lmSlabIsVault(ns) && !lmIsLowShellTag(nt)) return ns.ceilTiers * WH;
        return lmShellCeilHeight(ns.flag[lx][lz], ns.ceilTiers, WH, ns.zone[lx][lz]);
    };

    bool leanAllowed = true;   // leaning panels need room: never in narrowed or cramped passages
    // Decorates a single exposed wall face along [0..spanLen] from (bx,y0,bz) along T=(tx,0,tz) facing N=(nx,0,nz).
    auto decorateWallFace = [&](const LmSlabArchSpec& sp, std::vector<float>& curDadoV,
                                int wx, int wz, int sideId, float bx, float y0, float bz,
                                float tx, float tz, float nx, float nz, float spanLen, float cH,
                                bool isWindow, bool hasCenterDoor, bool hasCeilingStair, bool isVaultWall) {
        if (cH <= y0 + 1.2f) return;
        float faceYaw = atan2f(nx, nz);
        float effDado = sp.hasDado ? fminf(sp.dadoH, (cH - y0) * 0.44f) : 0.0f;
        if (isWindow) {
            float wSill = lmVitrineSillY(wx - (int)nx, wz - (int)nz);
            float midX = bx + tx * (spanLen * 0.5f), midZ = bz + tz * (spanLen * 0.5f);
            DecorBuilder wb(dv, midX, midZ, faceYaw, 1.0f);
            wb.box(0.0f, wSill - 0.06f, 0.04f, 3.34f, 0.07f, 0.16f, sp.trimMat);
            wb.box(0.0f, 2.38f, 0.03f, 3.34f, 0.11f, 0.11f, sp.trimMat);
            wb.box(-1.60f, wSill, 0.03f, 0.11f, 2.38f - wSill, 0.10f, sp.trimMat);
            wb.box( 1.60f, wSill, 0.03f, 0.11f, 2.38f - wSill, 0.10f, sp.trimMat);
            if (effDado > wSill - 0.08f) effDado = fmaxf(0.0f, wSill - 0.08f);
        }

        // Merge the 2 sub-spans into 1 single 5 m span whenever neither bay is missing and there is no center door!
        u32 hBay0 = lmHash((u32)wx, (u32)wz, (u32)(sideId * 13 + 0), 0x4152u);
        u32 hBay1 = lmHash((u32)wx, (u32)wz, (u32)(sideId * 13 + 1), 0x4152u);
        int missThresh = decay * 88 / 255;
        bool miss0 = !isWindow && !hasCeilingStair && ((int)(hBay0 % 1000u) < missThresh);
        bool miss1 = !isWindow && !hasCeilingStair && ((int)(hBay1 % 1000u) < missThresh);
        int nSegs = (!hasCenterDoor && !miss0 && !miss1) ? 1 : 2;
        float segs[2][2] = {
            {0.0f, (nSegs == 1) ? spanLen : (hasCenterDoor ? (spanLen * 0.5f - 0.84f) : (spanLen * 0.5f))},
            {hasCenterDoor ? (spanLen * 0.5f + 0.84f) : (spanLen * 0.5f), spanLen}
        };
        bool segMiss[2] = {miss0, miss1};
        u32  segHash[2] = {hBay0, hBay1};

        for (int b = 0; b < nSegs; b++) {
            float u0 = segs[b][0], u1 = segs[b][1];
            if (u1 <= u0 + 0.15f) continue;
            u32 hBay = segHash[b];
            if (segMiss[b]) {
                float scarH = (effDado > 0.1f) ? (effDado * 0.82f) : (sp.plinthH * 1.3f);
                lmEmitWallMolding(dv, bx, bz, tx, tz, nx, nz, u0 + 0.18f, u1 - 0.18f,
                                  y0 + 0.03f, y0 + scarH, 0.008f, DECOR_MAT_CARDBOARD, false, false);
                if (((hBay >> 10) & 1u) == 0u && !hasCenterDoor && spanLen >= 4.0f) {
                    float midU = 0.5f * (u0 + u1);
                    float fx = bx + tx * midU + nx * 0.24f;
                    float fz = bz + tz * midU + nz * 0.24f;
                    float tiltYaw = faceYaw + (((hBay >> 12) & 1u) ? 0.19f : -0.19f);
                    DecorBuilder fb(dv, fx, fz, tiltYaw, 1.0f);
                    fb.box(0.0f, y0, 0.0f, (u1 - u0) * 0.62f, 0.035f,
                           effDado > 0.1f ? 0.32f : sp.plinthH, effDado > 0.1f ? sp.trimMat : sp.plinthMat);
                }
            } else {
                // Single-quad bevelled plinth + visibly leaning Caligari wall panels in strong (>0.55) pockets!
                int wDel = lmDeliriumCell255(wx, wz);
                // The wall stays plumb to above head height and only then cants into the room, so
                // nobody can ever stand between the leaning panel and its wall.
                const float leanFrom = 2.75f;
                if (wDel >= 135 && leanAllowed && cH - y0 >= 3.6f && !isWindow && !hasCenterDoor && !isVaultWall) {
                    float leanIn = 0.48f + (float)(wDel - 135) * 0.48f / 120.0f;
                    Vec3 Tvec(tx, 0.0f, tz), Nvec(nx, 0.0f, nz);
                    Vec3 b0 = Vec3(bx, y0 + leanFrom, bz) + Tvec * u0, b1 = Vec3(bx, y0 + leanFrom, bz) + Tvec * u1;
                    Vec3 w0 = Vec3(bx, cH + 0.65f, bz) + Tvec * u0,   w1 = Vec3(bx, cH + 0.65f, bz) + Tvec * u1;
                    Vec3 t0 = w0 + Nvec * leanIn, t1 = w1 + Nvec * leanIn;
                    lmPushQuad(curDadoV, b0, t0, t1, b1, CS * 0.6f);
                    lmPushTri(curDadoV, b0, w0, t0, CS * 0.6f);   // cheeks close the wedge at both ends
                    lmPushTri(curDadoV, b1, t1, w1, CS * 0.6f);
                }
                if (wDel >= 90 && !isWindow && !hasCenterDoor) {
                    float climb = 0.65f + (float)(wDel - 90) * 1.75f / 165.0f;
                    Vec3 Tvec(tx, 0.0f, tz), Nvec(nx, 0.0f, nz);
                    Vec3 p0 = Vec3(bx, 0, bz) + Tvec * u0 + Nvec * (sp.plinthD + 0.14f);
                    Vec3 p1 = Vec3(bx, 0, bz) + Tvec * u1 + Nvec * (sp.plinthD + 0.14f);
                    float ya = ((hBay & 2u) ? (y0 + climb) : y0), yb = ((hBay & 2u) ? y0 : (y0 + climb));
                    lmPushDecorQuadRaw(dv, Vec3(p0.x, ya, p0.z), Vec3(p1.x, yb, p1.z),
                                           Vec3(p1.x, yb + sp.plinthH * 1.3f, p1.z), Vec3(p0.x, ya + sp.plinthH * 1.3f, p0.z), Nvec, sp.plinthMat);
                } else {
                    lmEmitWallMolding(dv, bx, bz, tx, tz, nx, nz, u0, u1, y0, y0 + sp.plinthH, sp.plinthD, sp.plinthMat, false, false);
                }
                if (effDado > sp.plinthH + 0.12f) {
                    float ox = bx + tx * u0 + nx * 0.022f, oz = bz + tz * u0 + nz * 0.022f;
                    float dx = tx * (u1 - u0), dz = tz * (u1 - u0);
                    lmPushWallSpan(curDadoV, ox, y0 + sp.plinthH, oz, dx, dz, y0 + effDado);
                    // Single-quad bevelled chair-rail cap (6 verts across the entire 5m wall!)
                    lmEmitWallMolding(dv, bx, bz, tx, tz, nx, nz, u0, u1,
                                      y0 + effDado - 0.022f, y0 + effDado + 0.036f, 0.044f, sp.trimMat, false, false);
                    if (sp.verticalBattens && (u1 - u0) >= 4.2f && ((wx + wz) & 1) == 0) {
                        float su = 0.5f * (u0 + u1);
                        lmEmitWallMolding(dv, bx, bz, tx, tz, nx, nz, su - 0.026f, su + 0.026f,
                                          y0 + sp.plinthH, y0 + effDado - 0.022f, 0.018f, sp.trimMat, false, false);
                    }
                }
            }
        }

        // Wall-top cove cornice (1 bevelled quad = 6 verts)
        if (sp.corniceH > 0.04f) {
            float topRef = isVaultWall ? WH : cH;
            if (topRef > y0 + 2.1f && topRef <= cH + 0.05f) {
                u32 hc = lmHash((u32)wx, (u32)wz, (u32)(sideId + 41), 0x4352u);
                bool dropHalf = ((int)(hc % 1000u) < decay * 70 / 255);
                float cu0 = dropHalf && ((hc >> 10) & 1u) ? (spanLen * 0.48f) : 0.0f;
                float cu1 = dropHalf && !((hc >> 10) & 1u) ? (spanLen * 0.52f) : spanLen;
                lmEmitWallMolding(dv, bx, bz, tx, tz, nx, nz, cu0, cu1,
                                  topRef - sp.corniceH, topRef, sp.corniceH * 0.58f, sp.trimMat, false, false);
            }
        }

        // Lean single-quad wall hardware (6..12 verts instead of 108-vert closed boxes!): switches, outlets, HVAC grilles
        auto pushWallPatch = [&](float uMid, float yBot, float wSpan, float hSpan, float offN, int mat, bool fit = false) {
            Vec3 T(tx, 0.0f, tz), N(nx, 0.0f, nz);
            Vec3 p0 = Vec3(bx, 0.0f, bz) + T * (uMid - wSpan * 0.5f) + N * offN;
            Vec3 p1 = Vec3(bx, 0.0f, bz) + T * (uMid + wSpan * 0.5f) + N * offN;
            if (!fit) {
                lmPushDecorQuadRaw(dv, Vec3(p0.x, yBot, p0.z), Vec3(p1.x, yBot, p1.z),
                                       Vec3(p1.x, yBot + hSpan, p1.z), Vec3(p0.x, yBot + hSpan, p0.z), N, mat);
            } else {
                DecorUv tuv = decorTileUv(mat);
                float vv[] = {
                    p0.x,yBot,p0.z,       tuv.u0,        tuv.v0,        N.x,N.y,N.z,
                    p1.x,yBot,p1.z,       tuv.u0+tuv.du, tuv.v0,        N.x,N.y,N.z,
                    p1.x,yBot+hSpan,p1.z, tuv.u0+tuv.du, tuv.v0+tuv.dv, N.x,N.y,N.z,
                    p0.x,yBot,p0.z,       tuv.u0,        tuv.v0,        N.x,N.y,N.z,
                    p1.x,yBot+hSpan,p1.z, tuv.u0+tuv.du, tuv.v0+tuv.dv, N.x,N.y,N.z,
                    p0.x,yBot+hSpan,p0.z, tuv.u0,        tuv.v0+tuv.dv, N.x,N.y,N.z
                };
                dv.insert(dv.end(), vv, vv + 48);
            }
        };

        if (!isWindow && !hasCeilingStair && spanLen >= 2.4f) {
            u32 hf = lmHash((u32)wx, (u32)wz, (u32)(sideId + 73), 0x4658u);
            if (hasCenterDoor) {
                float su = spanLen * 0.5f + 1.02f;
                if (su + 0.15f < spanLen) {
                    pushWallPatch(su, y0 + 1.14f, 0.11f, 0.17f, 0.028f, sp.plateMat);
                    pushWallPatch(su, y0 + 1.19f, 0.036f, 0.06f, 0.033f, DECOR_MAT_DARK_METAL);
                }
            }
            int coordPhase = wx * 3 + wz * 5 + sideId;
            int jitterMod  = (order < 85) ? (int)(hf & 1u) : 0;
            bool outletBeat = (((coordPhase + jitterMod) % sp.fixPitch + sp.fixPitch) % sp.fixPitch) == 0;
            if (outletBeat && !hasCenterDoor) {
                float ou = spanLen * (order > 110 ? 0.50f : (0.34f + (float)(hf % 33u) * 0.01f));
                float oy = y0 + sp.plinthH + 0.16f;
                if ((int)((hf >> 8) % 420u) < decay) {
                    pushWallPatch(ou, oy, 0.13f, 0.19f, 0.026f, DECOR_MAT_RUBBER);
                } else {
                    pushWallPatch(ou, oy, 0.14f, 0.20f, 0.028f, sp.plateMat);
                    pushWallPatch(ou, oy + 0.04f, 0.06f, 0.11f, 0.033f, DECOR_MAT_RUBBER);
                }
            }
            int ventPitch = sp.fixPitch + 2;
            bool ventBeat = (((wx * 5 + wz * 3 + sideId * 2) % ventPitch + ventPitch) % ventPitch) == 0;
            if (ventBeat && (cH - y0) >= 2.55f) {
                float vu = hasCenterDoor ? (spanLen * 0.20f) : (spanLen * 0.50f);
                float vy = fminf(cH, WH) - 0.58f;
                if (vy > y0 + effDado + 0.35f) {
                    if ((int)((hf >> 14) % 360u) < decay) {
                        pushWallPatch(vu, vy + 0.03f, 0.58f, 0.30f, 0.012f, DECOR_MAT_RUBBER);
                    } else {
                        pushWallPatch(vu, vy, 0.64f, 0.36f, 0.016f, DECOR_MAT_METAL);
                        pushWallPatch(vu, vy + 0.04f, 0.54f, 0.28f, 0.024f, DECOR_MAT_DARK_METAL, true);
                    }
                }
                if (useG > 165 && !hasCenterDoor && (effDado + 0.55f < vy)) {
                    float rvy = y0 + (effDado > 0.2f ? (effDado + 0.18f) : 0.38f);
                    pushWallPatch(spanLen * 0.76f, rvy, 0.46f, 0.26f, 0.026f, DECOR_MAT_METAL);
                    pushWallPatch(spanLen * 0.76f, rvy + 0.03f, 0.38f, 0.20f, 0.031f, DECOR_MAT_DARK_METAL, true);
                }
            }
        }
    };

    // Walk all interior cells + this slab's owned seam portals (slx == -1 or slz == -1)
    for (int slx = -1; slx < s.w; slx++) {
        for (int slz = -1; slz < s.h; slz++) {
            bool onSeam = (slx < 0 || slz < 0);
            int wx = s.rect.x0 + 1 + slx, wz = s.rect.z0 + 1 + slz;
            int tag = onSeam ? lmTagAt(wx, wz) : (int)s.tag[slx][slz];
            unsigned short flg = onSeam ? 0u : s.flag[slx][slz];
            unsigned char  zn  = onSeam ? 0u : s.zone[slx][slz];
            bool walkBasin = (!onSeam && tag == LT_BASIN && (flg & LF_WALK_BASIN) != 0 && (flg & LF_STAIR_DESC) == 0);
            if (!lmIsWalkableTag(tag) && !walkBasin) continue;
            if (lmStoryInSet(wx, wz)) continue;   // a stamped set brings its own interior finish

            bool inXeno = !onSeam && s.xeno.active && (zn & LZ_XENO) != 0;
            const LmSlabArchSpec& sp = inXeno ? xenoSpec : hostSpec;
            std::vector<float>& curDado = inXeno ? xDadoV : dadoV;
            float px = wx * CS, pz = wz * CS;
            float y0 = walkBasin ? -2.85f : 0.0f;
            bool isHighCell = !onSeam && slabVault && !lmIsLowShellTag(tag);
            float cH = isHighCell ? (s.ceilTiers * WH) : lmShellCeilHeight(flg, onSeam ? 1 : s.ceilTiers, WH, zn);
            bool hasCeilStair = !onSeam && (lmSlabHasSite(s, slx, slz, LS_CEIL_STAIR) || lmSlabHasSite(s, slx - 1, slz, LS_CEIL_STAIR));

            // (nor on seams and in vaults: the panel's top would stick out into the hall above)
            leanAllowed = (flg & (LF_NARROW_X | LF_NARROW_Z | LF_CRAMPED)) == 0 && !onSeam && !slabVault && (zn & LZ_STORY) == 0;
            // 1. Narrowed corridor inset walls (only in non-high, non-plug cells) vs perimeter walls
            bool allowNarrow = !isHighCell && (zn & LZ_PLUG_CORR) == 0;
            if (allowNarrow && (flg & LF_NARROW_X)) {
                float in = LM_NARROW_INSET;
                decorateWallFace(sp, curDado, wx, wz, 2, px + CS, y0, pz + in, -1, 0, 0, 1, CS, cH, false, false, false, false);
                decorateWallFace(sp, curDado, wx, wz, 3, px, y0, pz + CS - in, 1, 0, 0, -1, CS, cH, false, false, false, false);
            } else if (allowNarrow && (flg & LF_NARROW_Z)) {
                float in = LM_NARROW_INSET;
                decorateWallFace(sp, curDado, wx, wz, 0, px + in, y0, pz, 0, 1, 1, 0, CS, cH, false, false, false, false);
                decorateWallFace(sp, curDado, wx, wz, 1, px + CS - in, y0, pz + CS, 0, -1, -1, 0, CS, cH, false, false, false, false);
            } else if (!walkBasin) {
                struct SideDef { int dx, dz; float bx, bz, tx, tz, nx, nz; };
                const SideDef sides[4] = {
                    {-1,  0, px,      pz,      0,  1,  1,  0},
                    { 1,  0, px + CS, pz + CS, 0, -1, -1,  0},
                    { 0, -1, px + CS, pz,     -1,  0,  0,  1},
                    { 0,  1, px,      pz + CS, 1,  0,  0, -1}
                };
                int dAxis = lmPureDoorAxisAt(wx, wz);
                for (int sd = 0; sd < 4; sd++) {
                    if (dAxis == 1 && (sd == 0 || sd == 1)) continue; // buildDoorwayModel already fills X walls!
                    if (dAxis == 2 && (sd == 2 || sd == 3)) continue; // buildDoorwayModel already fills Z walls!
                    int nwx = wx + sides[sd].dx, nwz = wz + sides[sd].dz;
                    int nslx = slx + sides[sd].dx, nslz = slz + sides[sd].dz;
                    int nTag = lmTagAt(nwx, nwz);
                    if (nTag == LT_WALL) {
                        bool isWin = lmIsVitrineWindowWorld(nwx, nwz);
                        bool cDoor = !onSeam && lmWallFaceHasCenteredDoor(s, slx, slz, nslx, nslz);
                        decorateWallFace(sp, curDado, wx, wz, sd, sides[sd].bx, y0, sides[sd].bz,
                                         sides[sd].tx, sides[sd].tz, sides[sd].nx, sides[sd].nz,
                                         CS, cH, isWin, cDoor, hasCeilStair, isHighCell);
                    } else if (lmIsWalkableTag(nTag) && (sd == 0 || sd == 2)) {
                        // Portal architrave & floor saddle at architectural space transitions!
                        float nCH = cellCeilWorld(nwx, nwz);
                        bool touchesXenoCut = inXeno || ((lmZoneAt(nwx, nwz) & LZ_XENO) != 0);
                        int pdx = sides[sd].dz, pdz = sides[sd].dx;
                        bool flankedLeft  = (lmTagAt(wx - pdx, wz - pdz) == LT_WALL) || (lmTagAt(nwx - pdx, nwz - pdz) == LT_WALL);
                        bool flankedRight = (lmTagAt(wx + pdx, wz + pdz) == LT_WALL) || (lmTagAt(nwx + pdx, nwz + pdz) == LT_WALL);
                        bool spaceStep = !touchesXenoCut && flankedLeft && flankedRight &&
                                         ((onSeam || nwx == s.rect.x0 || nwz == s.rect.z0) ||
                                          (fabsf(cH - nCH) > 0.25f) ||
                                          ((tag == LT_CORR || tag == LT_LOW) && (nTag == LT_HALL || nTag == LT_BRIDGE)) ||
                                          ((nTag == LT_CORR || nTag == LT_LOW) && (tag == LT_HALL || tag == LT_BRIDGE)));
                        if (spaceStep && lmPureDoorAxisAt(nwx, nwz) == 0 && !(lmFlagAt(nwx, nwz) & (LF_NARROW_X | LF_NARROW_Z))) {
                            float portH = fminf(fminf(cH, nCH), WH);
                            float midX = sides[sd].bx + sides[sd].tx * (CS * 0.5f);
                            float midZ = sides[sd].bz + sides[sd].tz * (CS * 0.5f);
                            float pYaw = atan2f(sides[sd].nx, sides[sd].nz);
                            u32 hp = lmHash((u32)wx, (u32)wz, (u32)(sd + 99), 0x5052u);
                            bool missLeft  = ((int)(hp % 1000u) < decay * 65 / 255);
                            bool missRight = !missLeft && ((int)((hp >> 10) % 1000u) < decay * 65 / 255);
                            DecorBuilder pb(dv, midX, midZ, pYaw, 1.0f);
                            float pw = sp.pilasterW;
                            bool hasStepBeam = (fabsf(cH - nCH) > 0.05f);
                            float pilH = hasStepBeam ? (portH - 0.355f) : (portH - 0.20f);
                            if (pilH > 0.40f) {
                                if (!missLeft)  pb.box(-CS * 0.5f + pw * 0.5f + 0.008f, 0.0f, 0.0f, pw, pilH, 0.19f, sp.trimMat);
                                if (!missRight) pb.box( CS * 0.5f - pw * 0.5f - 0.008f, 0.0f, 0.0f, pw, pilH, 0.19f, sp.trimMat);
                            }
                            if (!hasStepBeam) {
                                pb.box(0.0f, portH - 0.20f, 0.0f, CS - 0.016f, 0.195f, 0.21f, sp.trimMat);
                            }
                            // Flush threshold saddle strip on floor
                            pb.box(0.0f, 0.0f, 0.0f, CS - pw * 1.3f, 0.012f, 0.14f, DECOR_MAT_METAL);
                        }
                    }
                }
            }

            // 2. Thin drywall partitions (plinth & dado on both faces around thin doorways)
            if (!onSeam && !isHighCell && (flg & LF_THIN_W)) {
                bool hd = (flg & LF_THIN_DOOR_W) != 0;
                float ht = LM_THIN_HALF + 0.004f;
                decorateWallFace(sp, curDado, wx, wz, 4, px + ht, 0.0f, pz, 0, 1, 1, 0, CS, cH, false, hd, false, false);
                decorateWallFace(sp, curDado, wx, wz, 5, px - ht, 0.0f, pz + CS, 0, -1, -1, 0, CS, cH, false, hd, false, false);
            }
            if (!onSeam && !isHighCell && (flg & LF_THIN_N)) {
                bool hd = (flg & LF_THIN_DOOR_N) != 0;
                float ht = LM_THIN_HALF + 0.004f;
                decorateWallFace(sp, curDado, wx, wz, 6, px + CS, 0.0f, pz + ht, -1, 0, 0, 1, CS, cH, false, hd, false, false);
                decorateWallFace(sp, curDado, wx, wz, 7, px, 0.0f, pz - ht, 1, 0, 0, -1, CS, cH, false, hd, false, false);
            }

            // 3. Lamp collars & occasional missing/sagging ceiling tile (main T-bars run-merged below!)
            if (!onSeam && !isHighCell && !walkBasin && sp.hasCeilGrid && cH <= 9.1f && lmDeliriumCell255(wx, wz) < 72 && !lmCellCeilWarped(wx, wz)) {
                int nSub = sp.ceilSub;
                float step = CS / (float)nSub;
                bool hasLamp = (lmLampAt(wx, wz) != 0);
                if (hasLamp) {
                    float lHalf = ((flg & (LF_NARROW_X | LF_NARROW_Z)) ? 0.42f : 0.66f);
                    float midX = px + CS * 0.5f, midZ = pz + CS * 0.5f;
                    lmEmitCeilRunner(dv, midX - lHalf - 0.024f, midZ - lHalf - 0.024f, midX + lHalf + 0.024f, midZ - lHalf + 0.024f, cH, 0.016f, sp.gridMat);
                    lmEmitCeilRunner(dv, midX - lHalf - 0.024f, midZ + lHalf - 0.024f, midX + lHalf + 0.024f, midZ + lHalf + 0.024f, cH, 0.016f, sp.gridMat);
                } else if (decay >= 42 && !(flg & (LF_NARROW_X | LF_NARROW_Z))) {
                    u32 ht = lmHash((u32)wx, (u32)wz, 17u, 0x4354u);
                    if ((int)(ht % 1200u) < decay) {
                        int ix = (int)((ht >> 11) % (u32)nSub), iz = (int)((ht >> 14) % (u32)nSub);
                        float tx0 = px + ix * step + 0.05f, tz0 = pz + iz * step + 0.05f;
                        float tw = step - 0.10f;
                        lmPushDecorQuadRaw(dv, Vec3(tx0, cH - 0.008f, tz0), Vec3(tx0 + tw, cH - 0.008f, tz0),
                                               Vec3(tx0 + tw, cH - 0.008f, tz0 + tw), Vec3(tx0, cH - 0.008f, tz0 + tw),
                                               Vec3(0, -1, 0), DECOR_MAT_RUBBER);
                        lmPushDecorQuadRaw(dv, Vec3(tx0, cH - 0.02f, tz0), Vec3(tx0 + tw, cH - 0.02f, tz0),
                                               Vec3(tx0 + tw, cH - 0.34f, tz0 + tw * 0.92f), Vec3(tx0, cH - 0.34f, tz0 + tw * 0.92f),
                                               Vec3(0, -0.92f, -0.38f), DECOR_MAT_CEILTILE);
                    }
                }
            }
        }
    }

    // Run-merged Armstrong ceiling T-bars & floor expansion strips across maximal contiguous slab spans!
    auto cellGridCeilH = [&](int slx, int slz, int& outMat) -> float {
        int tag = s.tag[slx][slz];
        if (!lmIsWalkableTag(tag)) return -1.0f;
        if (slabVault && !lmIsLowShellTag(tag)) return -1.0f;
        if (lmDeliriumCell255(s.rect.x0 + 1 + slx, s.rect.z0 + 1 + slz) >= 72) return -1.0f;
        if (lmCellCeilWarped(s.rect.x0 + 1 + slx, s.rect.z0 + 1 + slz)) return -1.0f;
        if (lmStoryInSet(s.rect.x0 + 1 + slx, s.rect.z0 + 1 + slz)) return -1.0f;
        unsigned char zn = s.zone[slx][slz];
        const LmSlabArchSpec& sp = (s.xeno.active && (zn & LZ_XENO)) ? xenoSpec : hostSpec;
        if (!sp.hasCeilGrid) return -1.0f;
        outMat = sp.gridMat;
        float cH = lmShellCeilHeight(s.flag[slx][slz], s.ceilTiers, WH, zn);
        return (cH <= 9.1f) ? cH : -1.0f;
    };

    const int nSub = hostSpec.ceilSub;
    const float gStep = CS / (float)nSub, rw = 0.024f, lip = 0.016f;
    const float lipZ = lip - 0.004f;   // cross tees sit 4 mm above the main runners: crossings never share a plane
    // Merge X-running ceiling T-bars across slx = 0..s.w-1
    for (int slz = 0; slz < s.h; slz++) {
        float pz = (s.rect.z0 + 1 + slz) * CS;
        for (int k = 0; k < nSub; k++) {
            float rz = pz + k * gStep;
            int slx = 0;
            while (slx < s.w) {
                int mat = 0;
                float cH = cellGridCeilH(slx, slz, mat);
                unsigned short flg = s.flag[slx][slz];
                float cz0 = ((flg & LF_NARROW_X) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (pz + LM_NARROW_INSET) : pz;
                float cz1 = ((flg & LF_NARROW_X) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (pz + CS - LM_NARROW_INSET) : (pz + CS);
                if (cH < 0.0f || rz < cz0 - 0.01f || rz > cz1 + 0.01f) { slx++; continue; }
                int wx = s.rect.x0 + 1 + slx, wz = s.rect.z0 + 1 + slz;
                float px = wx * CS;
                bool lampCut = (lmLampAt(wx, wz) != 0) && fabsf(rz - (pz + CS * 0.5f)) < 0.66f;
                if (lampCut) {
                    float lHalf = (flg & (LF_NARROW_X | LF_NARROW_Z)) ? 0.42f : 0.66f;
                    float midX = px + CS * 0.5f;
                    lmEmitCeilRunner(dv, px, rz - rw, midX - lHalf, rz + rw, cH, lip, mat);
                    lmEmitCeilRunner(dv, midX + lHalf, rz - rw, px + CS, rz + rw, cH, lip, mat);
                    slx++; continue;
                }
                float runX0 = ((flg & LF_NARROW_Z) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (px + LM_NARROW_INSET) : px;
                float runX1 = ((flg & LF_NARROW_Z) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (px + CS - LM_NARROW_INSET) : (px + CS);
                int nx = slx + 1;
                while (nx < s.w && !(flg & LF_NARROW_Z)) {
                    int nMat = 0;
                    float nCH = cellGridCeilH(nx, slz, nMat);
                    unsigned short nFlg = s.flag[nx][slz];
                    if (fabsf(nCH - cH) > 0.02f || nMat != mat || (nFlg & (LF_NARROW_X | LF_NARROW_Z))) break;
                    int nwx = s.rect.x0 + 1 + nx;
                    if ((lmLampAt(nwx, wz) != 0) && fabsf(rz - (pz + CS * 0.5f)) < 0.66f) break;
                    runX1 = (nwx + 1) * CS;
                    nx++;
                }
                lmEmitCeilRunner(dv, runX0, rz - rw, runX1, rz + rw, cH, lip, mat);
                slx = nx;
            }
        }
    }
    // Merge Z-running ceiling T-bars across slz = 0..s.h-1
    for (int slx = 0; slx < s.w; slx++) {
        float px = (s.rect.x0 + 1 + slx) * CS;
        int wx = s.rect.x0 + 1 + slx;
        for (int k = 0; k < nSub; k++) {
            float rx = px + k * gStep;
            int slz = 0;
            while (slz < s.h) {
                int mat = 0;
                float cH = cellGridCeilH(slx, slz, mat);
                unsigned short flg = s.flag[slx][slz];
                float cx0 = ((flg & LF_NARROW_Z) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (px + LM_NARROW_INSET) : px;
                float cx1 = ((flg & LF_NARROW_Z) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (px + CS - LM_NARROW_INSET) : (px + CS);
                if (cH < 0.0f || rx < cx0 - 0.01f || rx > cx1 + 0.01f) { slz++; continue; }
                int wz = s.rect.z0 + 1 + slz;
                float pz = wz * CS;
                bool lampCut = (lmLampAt(wx, wz) != 0) && fabsf(rx - (px + CS * 0.5f)) < 0.66f;
                if (lampCut) {
                    float lHalf = (flg & (LF_NARROW_X | LF_NARROW_Z)) ? 0.42f : 0.66f;
                    float midZ = pz + CS * 0.5f;
                    lmEmitCeilRunner(dv, rx - rw, pz, rx + rw, midZ - lHalf, cH, lipZ, mat);
                    lmEmitCeilRunner(dv, rx - rw, midZ + lHalf, rx + rw, pz + CS, cH, lipZ, mat);
                    slz++; continue;
                }
                float runZ0 = ((flg & LF_NARROW_X) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (pz + LM_NARROW_INSET) : pz;
                float runZ1 = ((flg & LF_NARROW_X) && (s.zone[slx][slz] & LZ_PLUG_CORR) == 0) ? (pz + CS - LM_NARROW_INSET) : (pz + CS);
                int nz = slz + 1;
                while (nz < s.h && !(flg & LF_NARROW_X)) {
                    int nMat = 0;
                    float nCH = cellGridCeilH(slx, nz, nMat);
                    unsigned short nFlg = s.flag[slx][nz];
                    if (fabsf(nCH - cH) > 0.02f || nMat != mat || (nFlg & (LF_NARROW_X | LF_NARROW_Z))) break;
                    int nwz = s.rect.z0 + 1 + nz;
                    if ((lmLampAt(wx, nwz) != 0) && fabsf(rx - (px + CS * 0.5f)) < 0.66f) break;
                    runZ1 = (nwz + 1) * CS;
                    nz++;
                }
                lmEmitCeilRunner(dv, rx - rw, runZ0, rx + rw, runZ1, cH, lipZ, mat);
                slz = nz;
            }
        }
    }
    // Run-merged floor expansion strips every lampPitch cells (skipping warped drunken floors!)
    int fpStep = s.lampPitch < 2 ? 2 : s.lampPitch;
    if (s.deliriumPeak < 72) {
        for (int slx = fpStep; slx < s.w; slx += fpStep) {
            float px = (s.rect.x0 + 1 + slx) * CS;
            int slz = 0;
            while (slz < s.h) {
                int t = s.tag[slx][slz];
                if ((t != LT_HALL && t != LT_CORR) || lmTagLocal(s, slx - 1, slz) != t ||
                    (s.flag[slx][slz] & (LF_NARROW_X | LF_NARROW_Z))) { slz++; continue; }
                int nz = slz + 1;
                while (nz < s.h && s.tag[slx][nz] == t && lmTagLocal(s, slx - 1, nz) == t &&
                       !(s.flag[slx][nz] & (LF_NARROW_X | LF_NARROW_Z))) nz++;
                float z0 = (s.rect.z0 + 1 + slz) * CS, z1 = (s.rect.z0 + 1 + nz) * CS;
                lmPushDecorQuadRaw(dv, Vec3(px - 0.026f, 0.005f, z0), Vec3(px - 0.026f, 0.005f, z1),
                                       Vec3(px + 0.026f, 0.005f, z1), Vec3(px + 0.026f, 0.005f, z0),
                                       Vec3(0, 1, 0), hostSpec.plinthMat);
                slz = nz;
            }
        }
        for (int slz = fpStep; slz < s.h; slz += fpStep) {
            float pz = (s.rect.z0 + 1 + slz) * CS;
            int slx = 0;
            while (slx < s.w) {
                int t = s.tag[slx][slz];
                if ((t != LT_HALL && t != LT_CORR) || lmTagLocal(s, slx, slz - 1) != t ||
                    (s.flag[slx][slz] & (LF_NARROW_X | LF_NARROW_Z))) { slx++; continue; }
                int nx = slx + 1;
                while (nx < s.w && s.tag[nx][slz] == t && lmTagLocal(s, nx, slz - 1) == t &&
                       !(s.flag[nx][slz] & (LF_NARROW_X | LF_NARROW_Z))) nx++;
                float x0 = (s.rect.x0 + 1 + slx) * CS, x1 = (s.rect.x0 + 1 + nx) * CS;
                lmPushDecorQuadRaw(dv, Vec3(x0, 0.005f, pz - 0.026f), Vec3(x0, 0.005f, pz + 0.026f),
                                       Vec3(x1, 0.005f, pz + 0.026f), Vec3(x1, 0.005f, pz - 0.026f),
                                       Vec3(0, 1, 0), hostSpec.plinthMat);
                slx = nx;
            }
        }
    }
}

// Counts the baseline 1-storey structural vertices (floor + ceiling + walls + thin/narrow)
// owned by slab `s` in `lmPostBuildGeom` so full-slab pre-decor vs post-decor ratios are exact.
inline int lmCountSlabOneStoreyBaseVerts(const LimenSlab& s) {
    bool isMega = lmSlabIsVault(s);
    int quads = 0;
    for (int slx = 0; slx < s.w; slx++) {
        for (int slz = 0; slz < s.h; slz++) {
            int tag = s.tag[slx][slz];
            if (!lmIsWalkableTag(tag)) continue;
            if (isMega && !lmIsLowShellTag(tag)) continue;
            if (s.xeno.active && (s.zone[slx][slz] & LZ_XENO)) continue;
            quads += 2; // floor + ceiling quad
            int wx = s.rect.x0 + 1 + slx, wz = s.rect.z0 + 1 + slz;
            if (lmTagAt(wx - 1, wz) == LT_WALL) quads++;
            if (lmTagAt(wx + 1, wz) == LT_WALL) quads++;
            if (lmTagAt(wx, wz - 1) == LT_WALL) quads++;
            if (lmTagAt(wx, wz + 1) == LT_WALL) quads++;
            unsigned short f = s.flag[slx][slz];
            if (f & LF_THIN_W) quads += 2;
            if (f & LF_THIN_N) quads += 2;
            if (f & (LF_NARROW_X | LF_NARROW_Z)) quads += 8;
        }
    }
    return quads * 6;
}

// Session state a slab mesh depends on: the triggered/untriggered state of its own sites.
inline unsigned int lmSlabSiteSignature(const LimenSlab& s) {
    unsigned int sig = 0x9E3779B9u;
    if (gLmActiveSites.empty()) return sig;
    for (int k = 0; k < s.siteCount; k++) {
        if (lmIsSiteTriggered(s.sites[k].id)) sig = (sig ^ (unsigned int)s.sites[k].id) * 16777619u + (unsigned int)k;
    }
    return sig;
}
