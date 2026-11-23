#pragma once
// Part of limen_mega.h: lmBuildSlabMesh.

inline void lmBuildSlabMesh(const LimenSlab& s, LmCachedSlabMesh& out) {
    out.destroyGL();
    out.siteSig = lmSlabSiteSignature(s);
    out.rect = s.rect;
    out.palette = s.palette;
    out.altPalette = (s.megaForm == MF_SILO) ? 2 : ((s.megaForm == MF_CELLBLOCK) ? 1 : ((s.palette + 1) % LM_MEGA_POLES));
    if (out.altPalette == out.palette) out.altPalette = (out.palette + 1) % LM_MEGA_POLES;
    out.xeno = s.xeno;
    out.builtSeed = worldSeed;
    out.builtLevel = gCurrentLevel;
    out.builtEpoch = gLmSiteEpoch;
    out.storyEpoch = lmStoryEpochOf((long long)(((unsigned long long)(u32)s.rect.j << 32) | (unsigned long long)(u32)s.rect.i));
    out.lv.clear(); out.lvOff.clear(); out.cols.clear();
    const bool unstruct = lmIsMegaUnstruct(s.megaForm);
    out.unstruct = lmUnstructKind(s.megaForm);
    out.uinfo[0] = (s.rect.x0 + 1 + s.w * 0.5f) * CS;
    out.uinfo[1] = s.ceilTiers * WH;
    out.uinfo[2] = (s.rect.z0 + 1 + s.h * 0.5f) * CS;

    auto tPre0 = std::chrono::high_resolution_clock::now();
    std::vector<float> mv, mav, wv, dadoV, dv, vfv, vcv, xwv, xDadoV, xfv, xcv, skyV;
    lmBuildSlabDoorScarsAndPlates(s, dv);
    if (!unstruct) {   // rooms, doors and vitrines only exist in architecture
        lmBuildSlabDeadThresholds(s, wv, dv, out.lv, out.cols);
        lmBuildSlabVitrines(s, wv, vfv, vcv, dv, out.lv);
        lmBuildSlabDeliriumProps(s, wv, dadoV, dv, out.lv, out.cols);
    }

    // Build foreign-textured geometry for `LZ_XENO` (`opXenograft`) slices in 1-storey slabs & low shells
    if (s.xeno.active) {
        auto slabCeilH = [&](int wx, int wz) -> float {
            LimenRect r = lmSlabAt(wx, wz);
            if (wx == r.x0 || wz == r.z0) return WH;
            const LimenSlab& ns = lmSlab(r.j, r.i);
            int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
            return lmShellCeilHeight(ns.flag[lx][lz], ns.ceilTiers, WH, ns.zone[lx][lz]);
        };
        for (int slx = s.xeno.x0; slx <= s.xeno.x1; slx++) {
            for (int slz = s.xeno.z0; slz <= s.xeno.z1; slz++) {
                if (slx < 0 || slz < 0 || slx >= s.w || slz >= s.h) continue;
                if ((s.zone[slx][slz] & LZ_XENO) == 0) continue;
                int tag = s.tag[slx][slz];
                if (!lmIsWalkableTag(tag)) continue;
                if (lmSlabIsVault(s) && !lmIsLowShellTag(tag)) continue; // vault floor/walls handled in vault loop below!
                int wx = s.rect.x0 + 1 + slx, wz = s.rect.z0 + 1 + slz;
                float px = wx * CS, pz = wz * CS;
                float cH = lmShellCeilHeight(s.flag[slx][slz], s.ceilTiers, WH, s.zone[slx][slz]);
                float fl[] = {px,0,pz,0,0,0,1,0, px,0,pz+CS,0,1,0,1,0, px+CS,0,pz+CS,1,1,0,1,0,
                              px,0,pz,0,0,0,1,0, px+CS,0,pz+CS,1,1,0,1,0, px+CS,0,pz,1,0,0,1,0};
                xfv.insert(xfv.end(), fl, fl + 48);
                float cl[] = {px,cH,pz,0,0,0,-1,0, px,cH,pz+CS,0,1,0,-1,0, px+CS,cH,pz+CS,1,1,0,-1,0,
                              px,cH,pz,0,0,0,-1,0, px+CS,cH,pz+CS,1,1,0,-1,0, px+CS,cH,pz,1,0,0,-1,0};
                xcv.insert(xcv.end(), cl, cl + 48);

                auto emitXWall = [&](int nwx, int nwz, float bx, float bz, float wdx, float wdz) {
                    if (lmSolidFromOutsideAt(nwx, nwz)) {
                        if (lmIsVitrineWindowWorld(nwx, nwz)) lmEmitWindowedWall(xwv, bx, bz, wdx, wdz, cH);
                        else lmPushWallSpan(xwv, bx, 0.0f, bz, wdx, wdz, cH);
                    } else {
                        float nH = slabCeilH(nwx, nwz);
                        if (cH > nH + 0.05f) {
                            lmPushWallSpan(xwv, bx, nH, bz, wdx, wdz, cH);
                            lmPushWallSpan(xwv, bx + wdx, nH, bz + wdz, -wdx, -wdz, cH);
                        }
                    }
                };
                emitXWall(wx - 1, wz, px, pz, 0.0f, CS);
                emitXWall(wx + 1, wz, px + CS, pz + CS, 0.0f, -CS);
                emitXWall(wx, wz - 1, px + CS, pz, -CS, 0.0f);
                emitXWall(wx, wz + 1, px, pz + CS, CS, 0.0f);
                unsigned short xflg = s.flag[slx][slz];
                if (xflg & LF_THIN_W) lmEmitThinPartition(xwv, dv, px, pz, true, (xflg & LF_THIN_DOOR_W) != 0, cH);
                if (xflg & LF_THIN_N) lmEmitThinPartition(xwv, dv, px, pz, false, (xflg & LF_THIN_DOOR_N) != 0, cH);
                if (xflg & (LF_NARROW_X | LF_NARROW_Z)) lmEmitNarrowCorridor(xwv, wx, wz, px, pz, xflg, cH);
                lmEmitThinJoints(xwv, wx, wz, 0.0f, [&](int cx, int cz) { return fmaxf(cH, slabCeilH(cx, cz)); });
            }
        }
    }

    if (lmSlabIsVault(s)) {
        const float T = 5.6f + (float)(255 - gLimenDNA.megaPoles[s.palette % LM_MEGA_POLES].seamFreqX) * 2.8f / 255.0f;
        const float shaftBot = -56.0f, basinBot = -2.85f;
        float ceilY = s.ceilTiers * WH;
        lmBuildSlabSuperstructure(s, mv, mav, wv, dadoV, dv, out.lv, out.cols);

        auto isVoidSubAir = [&](int slx, int slz) -> bool {
            // an open phenomenon has no wall at its slab seams: its void simply goes on
            if (slx < 0 || slz < 0 || slx >= s.w || slz >= s.h) return lmIsMegaUnstruct(s.megaForm);
            int t = s.tag[slx][slz];
            return t == LT_VOID || (t == LT_BRIDGE && (s.flag[slx][slz] & LF_OVER_VOID));
        };
        auto isBasinSubAir = [&](int slx, int slz) -> bool {
            if (slx < 0 || slz < 0 || slx >= s.w || slz >= s.h) return false;
            int t = s.tag[slx][slz];
            return t == LT_BASIN || (t == LT_BRIDGE && (s.flag[slx][slz] & LF_OVER_BASIN));
        };

        for (int slx = 0; slx < s.w; slx++) {
            for (int slz = 0; slz < s.h; slz++) {
                int wx = s.rect.x0 + 1 + slx, wz = s.rect.z0 + 1 + slz;
                float px = wx * CS, pz = wz * CS;
                int tag = s.tag[slx][slz];
                unsigned short flg = s.flag[slx][slz];

                // Above the 4.5 m line a vault cell is either air or mass. Mass: full-height walls and
                // low passages cut through them (low cells with no pod wall beside them). Air: the high
                // floor, and everything over the roofs of free-standing low shells and their pod walls.
                // Wherever mass meets the air over a low shell it needs a face from the roof line up
                // (the faces toward the high floor are emitted from the floor cell's side below).
                if (tag == LT_WALL || lmIsLowShellTag(tag)) {
                    bool pod = lmIsGraftedPodWall(s, slx, slz);
                    if (pod) {
                        float shH = lmShellCeilHeight(flg, s.ceilTiers, WH);
                        lmPushBox(mv, px, shH, pz, px + CS, shH + 0.62f, pz + CS, T);
                        // A pod wall taller than the shell beside it (a 2.65 m service ceiling, a lower
                        // pod) shows its flank above that neighbour's roof line.
                        auto podFlank = [&](int dx, int dz, Vec3 bl, Vec3 br) {
                            int nlx = slx + dx, nlz = slz + dz;
                            if (nlx < 0 || nlz < 0 || nlx >= s.w || nlz >= s.h) return;
                            int nt = s.tag[nlx][nlz];
                            float hN;
                            if (lmIsGraftedPodWall(s, nlx, nlz)) hN = lmShellCeilHeight(s.flag[nlx][nlz], s.ceilTiers, WH);
                            else if (lmIsLowShellTag(nt)) hN = lmShellCeilHeight(s.flag[nlx][nlz], s.ceilTiers, WH, s.zone[nlx][nlz]);
                            else return;
                            if (hN >= shH - 0.05f) return;
                            lmPushWallSpan(wv, bl.x, hN, bl.z, br.x - bl.x, br.z - bl.z, shH);
                        };
                        podFlank( 1, 0, Vec3(px + CS, 0, pz), Vec3(px + CS, 0, pz + CS));
                        podFlank(-1, 0, Vec3(px, 0, pz + CS), Vec3(px, 0, pz));
                        podFlank( 0, 1, Vec3(px + CS, 0, pz + CS), Vec3(px, 0, pz + CS));
                        podFlank( 0,-1, Vec3(px, 0, pz), Vec3(px + CS, 0, pz));
                    } else if (tag != LT_WALL) {
                        // Roof slab. Its sides start exactly at the ceiling line (no 2 cm slit under the
                        // roof) and it has no underside: the room's own ceiling is drawn there.
                        float shH = lmShellCeilHeight(flg, s.ceilTiers, WH, s.zone[slx][slz]);
                        float rt = shH + 0.56f;
                        bool selfMass = lmVaultMassAbove(s, slx, slz);
                        // A side of the roof slab is only there where it can be seen: not against
                        // mass, not against the roof of a level neighbour, and not against a taller
                        // shell (that room's own wall stands in this very plane). Toward a LOWER
                        // shell this one shows its flank above the neighbour's roof line instead.
                        auto roofSide = [&](int dx, int dz, Vec3 bl, Vec3 br) {
                            int nlx = slx + dx, nlz = slz + dz;
                            bool side = true;
                            if (nlx >= 0 && nlz >= 0 && nlx < s.w && nlz < s.h) {
                                int nt = s.tag[nlx][nlz];
                                bool nPod = lmIsGraftedPodWall(s, nlx, nlz);
                                if (nt == LT_WALL && !nPod) side = false;
                                else if (nPod) {
                                    float hN = lmShellCeilHeight(s.flag[nlx][nlz], s.ceilTiers, WH);
                                    side = hN + 0.62f < rt - 0.01f;
                                    if (!selfMass && hN < shH - 0.05f)
                                        lmPushQuad(mv, Vec3(bl.x, hN, bl.z), Vec3(bl.x, shH, bl.z), Vec3(br.x, shH, br.z), Vec3(br.x, hN, br.z), T);
                                } else if (lmIsLowShellTag(nt) && nt != LT_VITRINE) {
                                    float hN = lmShellCeilHeight(s.flag[nlx][nlz], s.ceilTiers, WH, s.zone[nlx][nlz]);
                                    side = !lmVaultMassAbove(s, nlx, nlz) && hN < shH - 0.05f;
                                    if (!selfMass && hN < shH - 0.05f)
                                        lmPushQuad(mv, Vec3(bl.x, hN, bl.z), Vec3(bl.x, shH, bl.z), Vec3(br.x, shH, br.z), Vec3(br.x, hN, br.z), T);
                                }
                            }
                            if (side) lmPushQuad(mv, Vec3(bl.x, shH, bl.z), Vec3(bl.x, rt, bl.z), Vec3(br.x, rt, br.z), Vec3(br.x, shH, br.z), T);
                        };
                        roofSide( 1, 0, Vec3(px + CS, 0, pz), Vec3(px + CS, 0, pz + CS));
                        roofSide(-1, 0, Vec3(px, 0, pz + CS), Vec3(px, 0, pz));
                        roofSide( 0, 1, Vec3(px + CS, 0, pz + CS), Vec3(px, 0, pz + CS));
                        roofSide( 0,-1, Vec3(px, 0, pz), Vec3(px + CS, 0, pz));
                        lmPushQuad(mv, Vec3(px, rt, pz + CS), Vec3(px + CS, rt, pz + CS), Vec3(px + CS, rt, pz), Vec3(px, rt, pz), T);
                    }
                    if (!pod && lmVaultMassAbove(s, slx, slz)) {
                        auto upperFace = [&](int dx, int dz, Vec3 bl, Vec3 br) {
                            int nlx = slx + dx, nlz = slz + dz;
                            if (nlx < 0 || nlz < 0 || nlx >= s.w || nlz >= s.h) return;
                            int nt = s.tag[nlx][nlz];
                            if (!(lmIsLowShellTag(nt) || lmIsGraftedPodWall(s, nlx, nlz))) return;
                            if (lmVaultMassAbove(s, nlx, nlz)) return;
                            float yb = lmShellCeilHeight(s.flag[nlx][nlz], s.ceilTiers, WH, s.zone[nlx][nlz]);
                            if (yb >= ceilY - 0.05f) return;
                            lmPushQuad(mv, Vec3(bl.x, yb, bl.z), Vec3(bl.x, ceilY, bl.z), Vec3(br.x, ceilY, br.z), Vec3(br.x, yb, br.z), T);
                        };
                        upperFace( 1, 0, Vec3(px + CS, 0, pz), Vec3(px + CS, 0, pz + CS));
                        upperFace(-1, 0, Vec3(px, 0, pz + CS), Vec3(px, 0, pz));
                        upperFace( 0, 1, Vec3(px + CS, 0, pz + CS), Vec3(px, 0, pz + CS));
                        upperFace( 0,-1, Vec3(px, 0, pz), Vec3(px + CS, 0, pz));
                    }
                    continue;
                }

                // Thin partitions of low shells end where the shell opens onto this high cell
                lmEmitThinJoints(wv, wx, wz, 0.0f, [&](int cx, int cz) {
                    return lmShellCeilHeight(lmFlagAt(cx, cz), s.ceilTiers, WH, lmZoneAt(cx, cz));
                });

                // Megastructure floor, chasm shaft, or drained basin (with walkable stairs!)
                if (tag == LT_VOID) {
                    lmPushQuad(mv, Vec3(px, shaftBot, pz + CS), Vec3(px + CS, shaftBot, pz + CS),
                               Vec3(px + CS, shaftBot, pz), Vec3(px, shaftBot, pz), T);
                } else if (tag == LT_BASIN) {
                    if (s.megaForm == MF_PLATFORM) {
                        const float trackBot = -1.42f;
                        lmPushQuad(mv, Vec3(px, trackBot, pz + CS), Vec3(px + CS, trackBot, pz + CS),
                                   Vec3(px + CS, trackBot, pz), Vec3(px, trackBot, pz), T);
                    } else if (s.megaForm != MF_ZIGGURAT) {
                        lmPushQuad(mv, Vec3(px, basinBot, pz + CS), Vec3(px + CS, basinBot, pz + CS),
                                   Vec3(px + CS, basinBot, pz), Vec3(px, basinBot, pz), T);
                        lmBuildBasinDetails(s, slx, slz, px, pz, basinBot, T, mv, dv, out.cols);
                    }
                } else if ((s.zone[slx][slz] & LZ_XENO) != 0 && s.xeno.active) {
                    float fl[] = {px,0,pz,0,0,0,1,0, px,0,pz+CS,0,1,0,1,0, px+CS,0,pz+CS,1,1,0,1,0,
                                  px,0,pz,0,0,0,1,0, px+CS,0,pz+CS,1,1,0,1,0, px+CS,0,pz,1,0,0,1,0};
                    xfv.insert(xfv.end(), fl, fl + 48);
                } else if (lmUnstructHasTerrain(s.megaForm)) {
                    lmPushTerrainCell(s, slx, slz, px, pz, s.megaForm == MF_GROTTO ? mav : mv);
                } else {
                    lmPushQuad(mv, Vec3(px, 0.0f, pz + CS), Vec3(px + CS, 0.0f, pz + CS),
                               Vec3(px + CS, 0.0f, pz), Vec3(px, 0.0f, pz), T);
                }

                // Bridges, parapets (skipping walkable basin stair mouths!), and severed tips
                if (lmIsWalkableTag(tag)) {
                    bool isBridge = (tag == LT_BRIDGE);
                    bool overBasin = isBridge && isBasinSubAir(slx, slz);
                    float deckBot = overBasin ? -0.65f : -1.8f;
                    if (isBridge) {
                        lmPushQuad(mv, Vec3(px, deckBot, pz), Vec3(px + CS, deckBot, pz),
                                   Vec3(px + CS, deckBot, pz + CS), Vec3(px, deckBot, pz + CS), T);
                        if (!overBasin) {
                            lmPushBox(mv, px + 1.5f, -3.1f, pz + 1.5f, px + CS - 1.5f, deckBot, pz + CS - 1.5f, T);
                            // the shaft has a bottom under its bridges too
                            lmPushQuad(mv, Vec3(px, shaftBot, pz + CS), Vec3(px + CS, shaftBot, pz + CS),
                                       Vec3(px + CS, shaftBot, pz), Vec3(px, shaftBot, pz), T);
                        }
                        else if (s.megaForm != MF_ZIGGURAT) {
                            float bBot = (s.megaForm == MF_PLATFORM) ? -1.42f : basinBot;
                            lmPushQuad(mv, Vec3(px, bBot, pz + CS), Vec3(px + CS, bBot, pz + CS),
                                       Vec3(px + CS, bBot, pz), Vec3(px, bBot, pz), T);
                        }
                    }
                    int pitDeg = lmPitDegreeLocal(s, slx, slz);
                    bool railGap = lmSlabHasSite(s, slx, slz, LS_RAIL_GAP);
                    auto buildPitEdge = [&](int dx, int dz) {
                        int nt = lmTagLocal(s, slx + dx, slz + dz);
                        if (!lmIsPitTag(nt)) return;
                        // Leave parapet completely open where a walkable basin staircase meets the rim!
                        if (lmBasinStairOpensToward(s, slx + dx, slz + dz, -dx, -dz)) return;
                        bool isSeveredTip = isBridge && pitDeg >= 3 && !lmIsPitTag(lmTagLocal(s, slx - dx, slz - dz));
                        if (isBridge) {
                            if (dx == -1) lmPushQuad(mv, Vec3(px, deckBot, pz), Vec3(px, deckBot, pz+CS), Vec3(px, 0, pz+CS), Vec3(px, 0, pz), T);
                            else if (dx == 1) lmPushQuad(mv, Vec3(px+CS, deckBot, pz+CS), Vec3(px+CS, deckBot, pz), Vec3(px+CS, 0, pz), Vec3(px+CS, 0, pz+CS), T);
                            else if (dz == -1) lmPushQuad(mv, Vec3(px+CS, deckBot, pz), Vec3(px, deckBot, pz), Vec3(px, 0, pz), Vec3(px+CS, 0, pz), T);
                            else lmPushQuad(mv, Vec3(px, deckBot, pz+CS), Vec3(px+CS, deckBot, pz+CS), Vec3(px+CS, 0, pz+CS), Vec3(px, 0, pz+CS), T);
                        }
                        if (lmIsMegaUnstruct(s.megaForm)) return;   // open phenomena: no railings, only the deck edge
                        if (isSeveredTip) {
                            lmBuildSeveredSpanTip(px, pz, dx, dz, deckBot, s.dark, dv, out.lv);
                            return;
                        }
                        if (!isBridge && s.megaForm == MF_PLATFORM && nt == LT_BASIN) {
                            // Platform safety yellow line & coping instead of a tall guardrail!
                            if (dx == -1)      lmPushDecorQuadRaw(dv, Vec3(px+0.12f,0.008f,pz), Vec3(px+0.12f,0.008f,pz+CS), Vec3(px+0.38f,0.008f,pz+CS), Vec3(px+0.38f,0.008f,pz), Vec3(0,1,0), DECOR_MAT_CAUTION);
                            else if (dx == 1)  lmPushDecorQuadRaw(dv, Vec3(px+CS-0.38f,0.008f,pz), Vec3(px+CS-0.38f,0.008f,pz+CS), Vec3(px+CS-0.12f,0.008f,pz+CS), Vec3(px+CS-0.12f,0.008f,pz), Vec3(0,1,0), DECOR_MAT_CAUTION);
                            else if (dz == -1) lmPushDecorQuadRaw(dv, Vec3(px,0.008f,pz+0.12f), Vec3(px,0.008f,pz+0.38f), Vec3(px+CS,0.008f,pz+0.38f), Vec3(px+CS,0.008f,pz+0.12f), Vec3(0,1,0), DECOR_MAT_CAUTION);
                            else               lmPushDecorQuadRaw(dv, Vec3(px,0.008f,pz+CS-0.38f), Vec3(px,0.008f,pz+CS-0.12f), Vec3(px+CS,0.008f,pz+CS-0.12f), Vec3(px+CS,0.008f,pz+CS-0.38f), Vec3(0,1,0), DECOR_MAT_CAUTION);
                            return;
                        }
                        if (!isBridge && s.megaForm == MF_ZIGGURAT && nt == LT_BASIN) {
                            return; // Rising ziggurat, grand staircase, and flush amphitheatre rim need no guardrail!
                        }
                        float hCurb = railGap ? 0.14f : (nt == LT_BASIN ? 0.42f : 0.54f);
                        float hRail = (nt == LT_BASIN) ? 0.86f : 1.08f;
                        if (dx == -1) {
                            lmPushBox(mv, px, 0.0f, pz, px + 0.36f, hCurb, pz + CS, T);
                            if (!railGap) {
                                lmPushBox(mv, px, hCurb, pz + 0.2f, px + 0.34f, hRail, pz + 0.75f, T);
                                lmPushBox(mv, px, hCurb, pz + CS - 0.75f, px + 0.34f, hRail, pz + CS - 0.2f, T);
                                DecorBuilder rb(dv, px + 0.18f, pz + CS * 0.5f, 0.0f, 1.0f);
                                rb.box(0.0f, hRail - 0.12f, 0.0f, 0.12f, 0.09f, CS, DECOR_MAT_DARK_METAL);
                            }
                        } else if (dx == 1) {
                            lmPushBox(mv, px + CS - 0.36f, 0.0f, pz, px + CS, hCurb, pz + CS, T);
                            if (!railGap) {
                                lmPushBox(mv, px + CS - 0.34f, hCurb, pz + 0.2f, px + CS, hRail, pz + 0.75f, T);
                                lmPushBox(mv, px + CS - 0.34f, hCurb, pz + CS - 0.75f, px + CS, hRail, pz + CS - 0.2f, T);
                                DecorBuilder rb(dv, px + CS - 0.18f, pz + CS * 0.5f, 0.0f, 1.0f);
                                rb.box(0.0f, hRail - 0.12f, 0.0f, 0.12f, 0.09f, CS, DECOR_MAT_DARK_METAL);
                            }
                        } else if (dz == -1) {
                            lmPushBox(mv, px, 0.0f, pz, px + CS, hCurb, pz + 0.36f, T);
                            if (!railGap) {
                                lmPushBox(mv, px + 0.2f, hCurb, pz, px + 0.75f, hRail, pz + 0.34f, T);
                                lmPushBox(mv, px + CS - 0.75f, hCurb, pz, px + CS - 0.2f, hRail, pz + 0.34f, T);
                                DecorBuilder rb(dv, px + CS * 0.5f, pz + 0.18f, 0.0f, 1.0f);
                                rb.box(0.0f, hRail - 0.12f, 0.0f, CS, 0.09f, 0.12f, DECOR_MAT_DARK_METAL);
                            }
                        } else {
                            lmPushBox(mv, px, 0.0f, pz + CS - 0.36f, px + CS, hCurb, pz + CS, T);
                            if (!railGap) {
                                lmPushBox(mv, px + 0.2f, hCurb, pz + CS - 0.34f, px + 0.75f, hRail, pz + CS, T);
                                lmPushBox(mv, px + CS - 0.75f, hCurb, pz + CS - 0.34f, px + CS - 0.2f, hRail, pz + CS, T);
                                DecorBuilder rb(dv, px + CS * 0.5f, pz + CS - 0.18f, 0.0f, 1.0f);
                                rb.box(0.0f, hRail - 0.12f, 0.0f, CS, 0.09f, 0.12f, DECOR_MAT_DARK_METAL);
                            }
                        }
                    };
                    buildPitEdge(-1, 0); buildPitEdge(1, 0); buildPitEdge(0, -1); buildPitEdge(0, 1);
                }

                // Subfloor chasm & basin walls
                if (isVoidSubAir(slx, slz)) {
                    float topSub = (tag == LT_VOID) ? 0.0f : -1.8f;
                    if (!isVoidSubAir(slx - 1, slz)) lmPushQuad(mv, Vec3(px, shaftBot, pz), Vec3(px, topSub, pz), Vec3(px, topSub, pz + CS), Vec3(px, shaftBot, pz + CS), T);
                    if (!isVoidSubAir(slx + 1, slz)) lmPushQuad(mv, Vec3(px + CS, shaftBot, pz + CS), Vec3(px + CS, topSub, pz + CS), Vec3(px + CS, topSub, pz), Vec3(px + CS, shaftBot, pz), T);
                    if (!isVoidSubAir(slx, slz - 1)) lmPushQuad(mv, Vec3(px + CS, shaftBot, pz), Vec3(px + CS, topSub, pz), Vec3(px, topSub, pz), Vec3(px, shaftBot, pz), T);
                    if (!isVoidSubAir(slx, slz + 1)) lmPushQuad(mv, Vec3(px, shaftBot, pz + CS), Vec3(px, topSub, pz + CS), Vec3(px + CS, topSub, pz + CS), Vec3(px + CS, shaftBot, pz + CS), T);
                } else if (isBasinSubAir(slx, slz) && s.megaForm != MF_ZIGGURAT) {
                    float bBot = (s.megaForm == MF_PLATFORM) ? -1.42f : basinBot;
                    float topSub = (tag == LT_BASIN) ? 0.0f : -0.65f;
                    if (!isBasinSubAir(slx - 1, slz)) lmPushQuad(mv, Vec3(px, bBot, pz), Vec3(px, topSub, pz), Vec3(px, topSub, pz + CS), Vec3(px, bBot, pz + CS), T);
                    if (!isBasinSubAir(slx + 1, slz)) lmPushQuad(mv, Vec3(px + CS, bBot, pz + CS), Vec3(px + CS, topSub, pz + CS), Vec3(px + CS, topSub, pz), Vec3(px + CS, bBot, pz), T);
                    if (!isBasinSubAir(slx, slz - 1)) lmPushQuad(mv, Vec3(px + CS, bBot, pz), Vec3(px + CS, topSub, pz), Vec3(px, topSub, pz), Vec3(px, bBot, pz), T);
                    if (!isBasinSubAir(slx, slz + 1)) lmPushQuad(mv, Vec3(px, bBot, pz + CS), Vec3(px, topSub, pz + CS), Vec3(px + CS, topSub, pz + CS), Vec3(px + CS, bBot, pz + CS), T);
                }

                // Above-floor walls (using pure `lmTagAt`, never chunk-dependent `getCellWorld`!)
                auto emitNeighborFace = [&](int nwx, int nwz, int nslx, int nslz, bool border, Vec3 bl, Vec3 br, float wdx, float wdz) {
                    bool solid = lmSolidFromOutsideAt(nwx, nwz);
                    if (border && unstruct) {
                        // A phenomenon ends in sky, not in a wall: round the slab stands a curtain
                        // drawn as the very sky behind it (so nothing of the neighbours shows over
                        // the horizon), and a way out is a framed doorway standing in that sky.
                        // The two roofed kinds keep their rock up to the roof line instead.
                        const float skyTop = 420.0f;
                        bool openSky = (s.megaForm == MF_SPIRES || s.megaForm == MF_TENDRILS);
                        float skyBot = openSky ? ((s.megaForm == MF_TENDRILS) ? shaftBot : 0.0f) : ceilY;
                        auto span = [&](std::vector<float>& v, float y0, float y1, float inset) {
                            if (y1 <= y0 + 0.01f) return;
                            float ix = wdz / CS * inset, iz = -wdx / CS * inset;
                            lmPushQuad(v, Vec3(bl.x + ix, y0, bl.z + iz), Vec3(bl.x + ix, y1, bl.z + iz),
                                       Vec3(br.x + ix, y1, br.z + iz), Vec3(br.x + ix, y0, br.z + iz), T);
                        };
                        std::vector<float>& frame = (s.megaForm == MF_SPIRES) ? mav : mv;
                        // (the frame is 6 cm of stuff astride the plane of the sky, seen from either side)
                        auto frameBox = [&](float t0, float t1, float y0, float y1) {   // t along bl -> br, 0..1
                            float xa = bl.x + (br.x - bl.x) * t0, xb = bl.x + (br.x - bl.x) * t1;
                            float za = bl.z + (br.z - bl.z) * t0, zb = bl.z + (br.z - bl.z) * t1;
                            float ex = (wdx == 0.0f) ? 0.03f : 0.0f, ez = (wdx == 0.0f) ? 0.0f : 0.03f;
                            lmPushBox(frame, fminf(xa, xb) - ex, y0, fminf(za, zb) - ez, fmaxf(xa, xb) + ex, y1, fmaxf(za, zb) + ez, T);
                        };
                        if (solid) {
                            span(skyV, skyBot, skyTop, 0.0f);
                            if (!openSky) span(mv, 0.0f, ceilY, 0.0f);
                        } else {
                            span(skyV, fmaxf(skyBot, WH), skyTop, 0.0f);
                            if (skyBot < 0.0f) span(skyV, skyBot, 0.0f, 0.0f);
                            if (!openSky) span(mv, WH, ceilY, 0.0f);
                            else {
                                int ax = (wdx > 0.0f) - (wdx < 0.0f), az = (wdz > 0.0f) - (wdz < 0.0f);
                                const float pw = 0.45f / CS;
                                frameBox(0.0f, 1.0f, WH - 0.03f, WH + 0.45f);
                                if (lmSolidFromOutsideAt(nwx - ax, nwz - az)) frameBox(0.0f, pw, 0.0f, WH);
                                if (lmSolidFromOutsideAt(nwx + ax, nwz + az)) frameBox(1.0f - pw, 1.0f, 0.0f, WH);
                            }
                        }
                        return;
                    }
                    if (border) {
                        if (solid) lmPushQuad(mv, Vec3(bl.x, 0, bl.z), Vec3(bl.x, WH, bl.z), Vec3(br.x, WH, br.z), Vec3(br.x, 0, br.z), T);
                        return;
                    }
                    if (solid) {
                        bool cellXeno = (s.zone[slx][slz] & LZ_XENO) != 0 && s.xeno.active;
                        if (nslx >= 0 && nslz >= 0 && nslx < s.w && nslz < s.h && lmSlabHasSite(s, nslx, nslz, LS_VITRINE_WIN)) {
                            lmEmitWindowedWall(cellXeno ? xwv : wv, bl.x, bl.z, wdx, wdz, ceilY);
                        } else if (lmIsGraftedPodWall(s, nslx, nslz)) {
                            float shH = lmShellCeilHeight(lmFlagLocal(s, nslx, nslz), s.ceilTiers, WH, lmZoneLocal(s, nslx, nslz));
                            mkWall(cellXeno ? xwv : wv, bl.x, bl.z, wdx, wdz, shH, CS, WH);
                        } else if (cellXeno) {
                            lmPushWallSpan(xwv, bl.x, 0.0f, bl.z, wdx, wdz, ceilY);
                        } else {
                            lmPushQuad(mv, Vec3(bl.x, 0, bl.z), Vec3(bl.x, ceilY, bl.z), Vec3(br.x, ceilY, br.z), Vec3(br.x, 0, br.z), T);
                        }
                    } else if (lmIsLowShellTag(lmTagLocal(s, nslx, nslz)) && lmTagLocal(s, nslx, nslz) != LT_VITRINE) {
                        float shH = lmShellCeilHeight(lmFlagLocal(s, nslx, nslz), s.ceilTiers, WH, lmZoneLocal(s, nslx, nslz));
                        if (lmVaultMassAbove(s, nslx, nslz)) {
                            lmPushQuad(mv, Vec3(bl.x, shH, bl.z), Vec3(bl.x, ceilY, bl.z), Vec3(br.x, ceilY, br.z), Vec3(br.x, shH, br.z), T);
                        }
                    }
                };
                emitNeighborFace(wx - 1, wz, slx - 1, slz, slx == 0, Vec3(px, 0, pz), Vec3(px, 0, pz + CS), 0, CS);
                emitNeighborFace(wx + 1, wz, slx + 1, slz, slx == s.w - 1, Vec3(px + CS, 0, pz + CS), Vec3(px + CS, 0, pz), 0, -CS);
                emitNeighborFace(wx, wz - 1, slx, slz - 1, slz == 0, Vec3(px + CS, 0, pz), Vec3(px, 0, pz), -CS, 0);
                emitNeighborFace(wx, wz + 1, slx, slz + 1, slz == s.h - 1, Vec3(px, 0, pz + CS), Vec3(px + CS, 0, pz + CS), CS, 0);

                // Bollard, hall pendant, and suspended basin/chasm pendant fixtures
                float auxY = 0.0f;
                int lamp = lmLampAt(wx, wz);
                int auxLamp = (lamp == 0) ? lmAuxVaultLampLocal(s, slx, slz, &auxY) : 0;
                if (lamp != 0) {
                    float cx = px + CS * 0.5f, cz = pz + CS * 0.5f;
                    if (lmLampIsBollard(s, slx, slz)) {
                        if (lmIsPitTag(lmTagLocal(s, slx - 1, slz))) cx = px + 0.22f;
                        else if (lmIsPitTag(lmTagLocal(s, slx + 1, slz))) cx = px + CS - 0.22f;
                        else if (lmIsPitTag(lmTagLocal(s, slx, slz - 1))) cz = pz + 0.22f;
                        else if (lmIsPitTag(lmTagLocal(s, slx, slz + 1))) cz = pz + CS - 0.22f;
                        lmPushBox(mv, cx - 0.20f, 0.0f, cz - 0.20f, cx + 0.20f, 1.70f, cz + 0.20f, T);
                        DecorBuilder lb(dv, cx, cz, 0.0f, 1.0f);
                        lb.box(0.0f, 1.70f, 0.0f, 0.54f, 0.08f, 0.54f, DECOR_MAT_DARK_METAL);
                        lb.box(0.0f, 2.14f, 0.0f, 0.62f, 0.10f, 0.62f, DECOR_MAT_DARK_METAL);
                        lmPushLightBox(lamp == 1 ? out.lv : out.lvOff, cx, 1.78f, cz, 0.20f, 0.35f, 0.20f);
                    } else {
                        float soffitY = lmMegaHangingSoffitAt(s, cx, cz);
                        float pendY = (s.megaForm == MF_PLATFORM) ? 3.72f : fminf(5.60f, soffitY - 0.14f);
                        if (soffitY - pendY > 0.35f)
                            lmPushBox(mv, cx - 0.07f, pendY + 0.10f, cz - 0.07f, cx + 0.07f, soffitY, cz + 0.07f, T);
                        DecorBuilder lb(dv, cx, cz, 0.0f, 1.0f);
                        lb.box(0.0f, pendY + 0.02f, 0.0f, 1.85f, 0.14f, 1.85f, DECOR_MAT_DARK_METAL);
                        lmPushLightQuad(lamp == 1 ? out.lv : out.lvOff,
                                        Vec3(cx-0.78f, pendY, cz-0.78f), Vec3(cx+0.78f, pendY, cz-0.78f),
                                        Vec3(cx+0.78f, pendY, cz+0.78f), Vec3(cx-0.78f, pendY, cz+0.78f), true);
                    }
                } else if (auxLamp != 0) {
                    float cx = px + CS * 0.5f, cz = pz + CS * 0.5f;
                    lmPushBox(mv, cx - 0.05f, auxY + 0.16f, cz - 0.05f, cx + 0.05f, ceilY, cz + 0.05f, T);
                    DecorBuilder lb(dv, cx, cz, 0.0f, 1.0f);
                    lb.box(0.0f, auxY + 0.03f, 0.0f, 1.48f, 0.14f, 1.48f, DECOR_MAT_DARK_METAL);
                    lmPushLightQuad(auxLamp == 1 ? out.lv : out.lvOff,
                                    Vec3(cx-0.62f, auxY, cz-0.62f), Vec3(cx+0.62f, auxY, cz-0.62f),
                                    Vec3(cx+0.62f, auxY, cz+0.62f), Vec3(cx-0.62f, auxY, cz+0.62f), true);
                }
            }
        }
    }

    auto tPre1 = std::chrono::high_resolution_clock::now();
    size_t dvPreFloats = dv.size();
    if (!unstruct) lmBuildSlabArchDetail(s, dadoV, dv, xDadoV);
    {
        ZfMesh zm[11] = {{&mv, 8, 0, "mega"}, {&mav, 8, 0, "megaAlt"}, {&wv, 8, 0, "slabWall"}, {&vfv, 8, 0, "vitFloor"},
                         {&vcv, 8, 0, "vitCeil"}, {&xwv, 8, 0, "xWall"}, {&xfv, 8, 0, "xFloor"}, {&xcv, 8, 0, "xCeil"},
                         {&dadoV, 8, 1, "dado"}, {&xDadoV, 8, 1, "xDado"}, {&dv, 8, 2, "slabDecor"}};
        lmResolveZfight(zm, 11);
    }
    auto tArch1 = std::chrono::high_resolution_clock::now();

#ifdef LIMEN_SHOTS
    if (gLmBenchHashOn || gLmBenchHashKeep) {
        unsigned long long h = 1469598103934665603ull;
        const std::vector<float>* vs[13] = {&mv, &mav, &wv, &dadoV, &dv, &vfv, &vcv, &xwv, &xDadoV, &xfv, &xcv, &out.lv, &out.lvOff};
        for (int k = 0; k < 13; k++) {
            size_t n = vs[k]->size() * sizeof(float);
            h = lmFnv64(&n, sizeof(n), h);
            h = lmFnv64(vs[k]->data(), n, h);
        }
        if (!out.cols.empty()) h = lmFnv64(out.cols.data(), out.cols.size() * sizeof(out.cols[0]), h);
        out.vertHash = h;
    }
#endif
#ifdef LIMEN_SHOTS
    if (gLmAuditOn) {
        long long akey = (long long)(((unsigned long long)(u32)s.rect.j << 32) | (unsigned long long)(u32)s.rect.i);
        std::vector<LmAuditTri>& at = gLmAuditSlab[akey];
        at.clear();
        lmAuditAppend(at, mv, LA_MEGA, false);       lmAuditAppend(at, mav, LA_MEGA_ALT, false);
        lmAuditAppend(at, wv, LA_SLAB_WALL, false);  lmAuditAppend(at, dadoV, LA_DADO, false);
        lmAuditAppend(at, dv, LA_SLAB_DECOR, true);  lmAuditAppend(at, vfv, LA_VIT_FLOOR, false);
        lmAuditAppend(at, vcv, LA_VIT_CEIL, true);   lmAuditAppend(at, xwv, LA_X_WALL, false);
        lmAuditAppend(at, xDadoV, LA_X_DADO, false); lmAuditAppend(at, xfv, LA_X_FLOOR, false);
        lmAuditAppend(at, xcv, LA_X_CEIL, true);
        lmAuditAppendLights(at, out.lv); lmAuditAppendLights(at, out.lvOff);
        if (unstruct) {
            // the sky curtain, and the sky itself as a lid over the slab (nothing is drawn there:
            // the backdrop box stands behind everything)
            std::vector<float> lid;
            float lx0 = (s.rect.x0 + 1) * CS, lz0 = (s.rect.z0 + 1) * CS, ly = s.ceilTiers * WH + 16.0f;
            lmPushQuad(lid, Vec3(lx0, ly, lz0), Vec3(lx0 + s.w * CS, ly, lz0), Vec3(lx0 + s.w * CS, ly, lz0 + s.h * CS), Vec3(lx0, ly, lz0 + s.h * CS), 1.0f);
            lmAuditAppend(at, skyV, LA_UNSTRUCT_SKY, true);
            lmAuditAppend(at, lid, LA_UNSTRUCT_SKY, true);
        }
    }
#endif
    out.megaVC      = (int)mv.size() / 8;
    out.megaAltVC   = (int)mav.size() / 8;
    out.skyVC       = (int)skyV.size() / 8;
    out.wallVC      = (int)wv.size() / 8;
    out.dadoVC      = (int)dadoV.size() / 8;
    out.decorVC     = (int)dv.size() / 8;
    out.vitFloorVC  = (int)vfv.size() / 8;
    out.vitCeilVC   = (int)vcv.size() / 8;
    out.xenoWallVC  = (int)xwv.size() / 8;
    out.xenoDadoVC  = (int)xDadoV.size() / 8;
    out.xenoFloorVC = (int)xfv.size() / 8;
    out.xenoCeilVC  = (int)xcv.size() / 8;
    if (out.megaVC > 0)      setupVAO(out.megaVAO,      out.megaVBO,      mv,     true);
    if (out.megaAltVC > 0)   setupVAO(out.megaAltVAO,   out.megaAltVBO,   mav,    true);
    if (out.skyVC > 0)       setupVAO(out.skyVAO,       out.skyVBO,       skyV,   true);
    if (out.wallVC > 0)      setupVAO(out.wallVAO,      out.wallVBO,      wv,     true);
    if (out.dadoVC > 0)      setupVAO(out.dadoVAO,      out.dadoVBO,      dadoV,  true);
    if (out.decorVC > 0)     setupVAO(out.decorVAO,     out.decorVBO,     dv,     true);
    if (out.vitFloorVC > 0)  setupVAO(out.vitFloorVAO,  out.vitFloorVBO,  vfv,    true);
    if (out.vitCeilVC > 0)   setupVAO(out.vitCeilVAO,   out.vitCeilVBO,   vcv,    true);
    if (out.xenoWallVC > 0)  setupVAO(out.xenoWallVAO,  out.xenoWallVBO,  xwv,    true);
    if (out.xenoDadoVC > 0)  setupVAO(out.xenoDadoVAO,  out.xenoDadoVBO,  xDadoV, true);
    if (out.xenoFloorVC > 0) setupVAO(out.xenoFloorVAO, out.xenoFloorVBO, xfv,    true);
    if (out.xenoCeilVC > 0)  setupVAO(out.xenoCeilVAO,  out.xenoCeilVBO,  xcv,    true);
    auto tUp1 = std::chrono::high_resolution_clock::now();

    int preCachedVerts = out.megaVC + out.megaAltVC + out.wallVC + (int)(dvPreFloats / 8) +
                         out.vitFloorVC + out.vitCeilVC + out.xenoWallVC + out.xenoFloorVC + out.xenoCeilVC;
    int baseOneStorey  = lmCountSlabOneStoreyBaseVerts(s);
    int archAddedVerts = out.dadoVC + out.xenoDadoVC + (out.decorVC - (int)(dvPreFloats / 8));
    gLmPerf.slabsBuilt++;
    gLmPerf.preTotalVerts  += (long long)(baseOneStorey + preCachedVerts);
    gLmPerf.archAddedVerts += (long long)archAddedVerts;
    gLmPerf.preBuildMs     += std::chrono::duration<double, std::milli>(tPre1 - tPre0).count();
    gLmPerf.archBuildMs    += std::chrono::duration<double, std::milli>(tArch1 - tPre1).count();
    gLmPerf.gpuUploadMs    += std::chrono::duration<double, std::milli>(tUp1 - tArch1).count();
}
