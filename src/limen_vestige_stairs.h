#pragma once
// Part of limen_vestige.h: orphan frames, basin details, severed spans, Escher knots, delirium props.

inline void lmBuildOrphanFrames(const LimenSlab& s, std::vector<float>& dv, std::vector<LimenAabb2D>& outCols) {
    u32 baseH = lmHash((u32)s.rect.j, (u32)s.rect.i, 59u, 0x4652u);
    if ((baseH % 100u) >= 65u) return;
    for (int attempt = 0; attempt < 20; attempt++) {
        u32 h = lmHash(baseH, (u32)attempt, 3u, 0x4653u);
        int lx = 4 + (int)(h % (u32)(s.w > 8 ? s.w - 8 : 1));
        int lz = 4 + (int)((h >> 7) % (u32)(s.h > 8 ? s.h - 8 : 1));
        if (s.tag[lx][lz] != LT_HALL) continue;
        if (lmTagLocal(s, lx - 1, lz) != LT_HALL || lmTagLocal(s, lx + 1, lz) != LT_HALL ||
            lmTagLocal(s, lx, lz - 1) != LT_HALL || lmTagLocal(s, lx, lz + 1) != LT_HALL) continue;
        if (lmLampLocal(s, lx, lz)) continue;
        if (lmStoryNear(s, lx, lz, 1)) continue;

        float cx = (s.rect.x0 + 1 + lx + 0.5f) * CS;
        float cz = (s.rect.z0 + 1 + lz + 0.5f) * CS;
        bool alongZ = ((h >> 14) & 1u) != 0u;
        float yaw = alongZ ? 1.57079633f : 0.0f;
        DecorBuilder b(dv, cx, cz, yaw, 1.0f);
        b.box(-1.12f, 0.0f, 0.0f, 0.96f, 3.25f, 0.26f, DECOR_MAT_PLASTER);
        b.box(1.12f, 0.0f, 0.0f, 0.96f, 3.25f, 0.26f, DECOR_MAT_PLASTER);
        b.box(0.0f, 2.38f, 0.0f, 1.30f, 0.87f, 0.26f, DECOR_MAT_PLASTER);
        b.box(-0.62f, 0.0f, 0.0f, 0.13f, 2.28f, 0.32f, DECOR_MAT_TRIM);
        b.box(0.62f, 0.0f, 0.0f, 0.13f, 2.28f, 0.32f, DECOR_MAT_TRIM);
        b.box(0.0f, 2.28f, 0.0f, 1.37f, 0.13f, 0.32f, DECOR_MAT_TRIM);
        if (!alongZ) {
            outCols.push_back({cx - 1.60f, cz - 0.18f, cx - 0.56f, cz + 0.18f});
            outCols.push_back({cx + 0.56f, cz - 0.18f, cx + 1.60f, cz + 0.18f});
        } else {
            outCols.push_back({cx - 0.18f, cz - 1.60f, cx + 0.18f, cz - 0.56f});
            outCols.push_back({cx - 0.18f, cz + 0.56f, cx + 0.18f, cz + 1.60f});
        }
        return;
    }
}

inline void lmBuildBasinDetails(const LimenSlab& s, int slx, int slz, float px, float pz,
                                float basinBot, float tile, std::vector<float>& mv, std::vector<float>& dv,
                                std::vector<LimenAabb2D>& outCols) {
    unsigned short flg = s.flag[slx][slz];
    if (flg & LF_STAIR_DESC) {
        int dir = lmStairRimDir(flg);
        const int steps = 10;
        for (int k = 0; k < steps; k++) {
            float t0 = (float)k / (float)steps, t1 = (float)(k + 1) / (float)steps;
            float topY = basinBot * ((float)k / (float)steps);
            if (dir == 0)      lmPushBox(mv, px + 0.18f, basinBot, pz + t0 * CS, px + CS - 0.18f, topY, pz + t1 * CS + 0.02f, tile);
            else if (dir == 1) lmPushBox(mv, px + 0.18f, basinBot, pz + (1.0f - t1) * CS - 0.02f, px + CS - 0.18f, topY, pz + (1.0f - t0) * CS, tile);
            else if (dir == 2) lmPushBox(mv, px + t0 * CS, basinBot, pz + 0.18f, px + t1 * CS + 0.02f, topY, pz + CS - 0.18f, tile);
            else               lmPushBox(mv, px + (1.0f - t1) * CS - 0.02f, basinBot, pz + 0.18f, px + (1.0f - t0) * CS, topY, pz + CS - 0.18f, tile);
        }
        float yaw = (dir == 0) ? 0.0f : (dir == 1 ? 3.14159265f : (dir == 2 ? 1.57079633f : -1.57079633f));
        DecorBuilder hb(dv, px + CS * 0.5f, pz + CS * 0.5f, yaw, 1.0f);
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            float rx = 2.15f * (float)sgn;
            decorRod(hb, Vec3(rx, 0.92f, -CS * 0.46f), Vec3(rx, basinBot + 0.92f, CS * 0.44f), 0.036f, DECOR_MAT_METAL, 8);
            decorRod(hb, Vec3(rx, 0.0f, -CS * 0.46f), Vec3(rx, 0.92f, -CS * 0.46f), 0.035f, DECOR_MAT_METAL, 8);
            decorRod(hb, Vec3(rx, basinBot, CS * 0.44f), Vec3(rx, basinBot + 0.92f, CS * 0.44f), 0.035f, DECOR_MAT_METAL, 8);
        }
        return;
    }

    u32 h = lmHash((u32)(s.rect.x0 + slx), (u32)(s.rect.z0 + slz), 19u, 0x4253u);
    bool nearStair = ((lmFlagLocal(s, slx - 1, slz) | lmFlagLocal(s, slx + 1, slz) |
                       lmFlagLocal(s, slx, slz - 1) | lmFlagLocal(s, slx, slz + 1)) & LF_STAIR_DESC) != 0;

    auto emitBasinProp = [&](int ptype, float wxPos, float wzPos, float pyaw) -> bool {
        if (ptype < 0) return false;
        MapProp pr{};
        pr.pos = Vec3(wxPos, 0.0f, wzPos);
        pr.type = ptype;
        pr.scale = 1.0f;
        pr.yaw = pyaw;
        float hx = 0.0f, hz = 0.0f;
        mapPropHalfExtents(pr, hx, hz);
        for (const auto& cb : outCols) {
            if (wxPos + hx + 0.08f > cb.x0 && wxPos - hx - 0.08f < cb.x1 &&
                wzPos + hz + 0.08f > cb.z0 && wzPos - hz - 0.08f < cb.z1) return false;
        }
        size_t v0 = dv.size();
        buildMapPropModel(dv, pr);
        for (size_t vi = v0 + 1; vi < dv.size(); vi += 8) dv[vi] += basinBot;
        if (isPropSolidType(ptype) && hx > 0.01f && hz > 0.01f) {
            outCols.push_back({wxPos - hx, wzPos - hz, wxPos + hx, wzPos + hz});
        }
        return true;
    };

    // Grouped, grid-locked furnishing on walkable dry basin floors (aligned to pendant grid & basin walls;
    // intentionally stark when lmSlabHallIsBare(s) is true — zero stray singletons!).
    bool basinInterior = (lmTagLocal(s, slx - 1, slz) == LT_BASIN && lmTagLocal(s, slx + 1, slz) == LT_BASIN &&
                          lmTagLocal(s, slx, slz - 1) == LT_BASIN && lmTagLocal(s, slx, slz + 1) == LT_BASIN);
    int bPitch = s.lampPitch < 3 ? 3 : (s.lampPitch > 5 ? 5 : s.lampPitch);
    int bOff   = bPitch / 2;
    int drive  = ((int)gLimenDNA.propDensity * 3 + (int)s.genome.g[LG_USE] * 3 + 70) / 6;
    int decayG = (int)s.genome.g[LG_DECAY];
    u32 slabH  = lmHash((u32)s.rect.j, (u32)s.rect.i, 61u, 0x425Au);

    if ((flg & LF_WALK_BASIN) != 0 && !nearStair && !lmSlabHallIsBare(s)) {
        unsigned themes = lmPickSlabDecorTheme(s, 23u);
        if (basinInterior && (slx % bPitch) == bOff && (slz % bPitch) == bOff) {
            int zx = slx / (bPitch * 2), zz = slz / (bPitch * 2);
            u32 zh = lmHash(slabH, (u32)zx, (u32)zz, 0x4251u);
            if ((int)(zh % 250u) < (85 + drive * 3 / 4)) {
                float axisYaw = (s.h >= s.w) ? 0.0f : 1.57079633f;
                Vec3 fwd(sinf(axisYaw), 0.0f, cosf(axisYaw));
                Vec3 right(cosf(axisYaw), 0.0f, -sinf(axisYaw));
                Vec3 center(px + CS * 0.5f, 0.0f, pz + CS * 0.5f);
                int mode = (int)((zh >> 5) % 3u);
                int hostT = lmPickHostWithCompanion(themes, zh >> 9, 1u << PROP_PLACE_FLOOR);
                if (mode == 0 && hostT < 0) mode = 1;

                if (mode == 0 && hostT >= 0) {
                    const PropCompanion* rule = propCompanionFor(hostT);
                    float hhx = 0.0f, hhz = 0.0f, bhx = 0.0f, bhz = 0.0f;
                    mapPropBaseExtents(hostT, hhx, hhz);
                    if (rule) mapPropBaseExtents(rule->buddy, bhx, bhz);
                    int cols = (hhx < 0.62f) ? 3 : 2;
                    float colStep = hhx * 2.0f + 0.24f;
                    float rowOff  = hhz + 0.10f;
                    for (int sd = -1; sd <= 1; sd += 2) {
                        float hyaw = (sd > 0) ? axisYaw : (axisYaw + 3.14159265f);
                        Vec3 hf = fwd * (float)sd;
                        for (int cl = 0; cl < cols; cl++) {
                            u32 sh2 = lmHash(h, (u32)(sd + 2), (u32)cl, 0x4252u);
                            if ((int)(sh2 % 380u) < decayG) continue;
                            float lat = ((float)cl - 0.5f * (float)(cols - 1)) * colStep;
                            Vec3 hp = center + hf * rowOff + right * lat;
                            if (emitBasinProp(hostT, hp.x, hp.z, hyaw) && rule) {
                                Vec3 bp = (rule->mode == PROP_BUDDY_FRONT)
                                    ? (hp + hf * (hhz + bhz + 0.16f))
                                    : (hp + right * (hhx + bhx + 0.14f));
                                float byaw = (rule->mode == PROP_BUDDY_FRONT) ? (hyaw + 3.14159265f) : hyaw;
                                emitBasinProp(rule->buddy, bp.x, bp.z, byaw);
                            }
                        }
                    }
                } else if (mode == 1) {
                    int rtype = lmPickAlignedRowProp(themes, zh >> 11);
                    if (rtype >= 0) {
                        float bx = 0.0f, bz = 0.0f;
                        mapPropBaseExtents(rtype, bx, bz);
                        int cols = (bx < 0.62f) ? 3 : 2;
                        float colStep = bx * 2.0f + 0.22f;
                        float rowStep = bz * 2.0f + 1.15f;
                        for (int rw = 0; rw < 2; rw++) {
                            for (int cl = 0; cl < cols; cl++) {
                                u32 sh2 = lmHash(h, (u32)rw, (u32)cl, 0x4254u);
                                if ((int)(sh2 % 400u) < decayG) continue;
                                float along = ((float)rw - 0.5f) * rowStep;
                                float lat   = ((float)cl - 0.5f * (float)(cols - 1)) * colStep;
                                Vec3 p = center + fwd * along + right * lat;
                                emitBasinProp(rtype, p.x, p.z, axisYaw);
                            }
                        }
                    }
                } else {
                    int itype = lmPickStructuredIslandProp(themes, zh >> 13);
                    int btype = lmPickStructuredIslandProp(themes, (zh >> 19) + 5u);
                    if (itype >= 0) {
                        float bx = 0.0f, bz = 0.0f;
                        mapPropBaseExtents(itype, bx, bz);
                        float sx = bx + 0.16f, sz = bz + 0.16f;
                        for (int ix = -1; ix <= 1; ix += 2) {
                            for (int iz = -1; iz <= 1; iz += 2) {
                                u32 sh2 = lmHash(h, (u32)(ix + 2), (u32)(iz + 2), 0x4255u);
                                if ((int)(sh2 % 420u) < decayG) continue;
                                int ut = (ix == 1 && iz == 1 && btype >= 0) ? btype : itype;
                                Vec3 p = center + right * ((float)ix * sx) + fwd * ((float)iz * sz);
                                emitBasinProp(ut, p.x, p.z, axisYaw + (iz < 0 ? 3.14159265f : 0.0f));
                            }
                        }
                    }
                }
            }
        } else if (!basinInterior && (h % 6u) != 0u) {
            // Wall-hugging row along the sheer 2.85 m basin perimeter wall!
            static const int bdx[4] = {-1, 1, 0, 0}, bdz[4] = {0, 0, -1, 1};
            int wd = -1, wc = 0;
            for (int d = 0; d < 4; d++) {
                if (lmTagLocal(s, slx + bdx[d], slz + bdz[d]) != LT_BASIN) { wc++; wd = d; }
            }
            if (wc == 1 && wd >= 0) {
                int runC = (bdx[wd] != 0) ? slz : slx;
                u32 bh = lmHash(slabH, (u32)((runC / 3) * 13 + wd), 9u, 0x4257u);
                if ((int)(bh % 280u) < drive) {
                    int wtype = pickThemedPropType(themes, bh >> 7, 1u << PROP_PLACE_WALL);
                    if (wtype >= 0) {
                        float bx = 0.0f, bz = 0.0f;
                        mapPropBaseExtents(wtype, bx, bz);
                        int nU = (bx < 0.60f) ? 3 : ((bx < 0.95f) ? 2 : 1);
                        float step = bx * 2.0f + 0.26f;
                        float wyaw = atan2f((float)-bdx[wd], (float)-bdz[wd]);
                        float reach = CS * 0.5f - bz - 0.14f;
                        Vec3 c0(px + CS * 0.5f + bdx[wd] * reach, 0.0f, pz + CS * 0.5f + bdz[wd] * reach);
                        Vec3 along((float)-bdz[wd], 0.0f, (float)bdx[wd]);
                        for (int u = 0; u < nU; u++) {
                            if ((int)(lmHash(bh, (u32)(runC * 5 + u), 3u, 0x4258u) % 420u) < decayG) continue;
                            Vec3 p = c0 + along * (((float)u - 0.5f * (float)(nU - 1)) * step);
                            emitBasinProp(wtype, p.x, p.z, wyaw);
                        }
                    }
                }
            }
        }
    } else if ((h % 7u) == 0u) {
        DecorBuilder db(dv, px + CS * 0.5f, pz + CS * 0.5f, 0.0f, 1.0f);
        db.box(0.0f, basinBot, 0.0f, 0.95f, 0.04f, 0.95f, DECOR_MAT_DARK_METAL);
        db.box(0.0f, basinBot + 0.04f, 0.0f, 0.72f, 0.02f, 0.72f, DECOR_MAT_RUBBER);
    }
    if ((h % 6u) != 0u) return;
    int side = -1;
    if (lmTagLocal(s, slx, slz - 1) == LT_HALL) side = 0;
    else if (lmTagLocal(s, slx + 1, slz) == LT_HALL) side = 1;
    else if (lmTagLocal(s, slx - 1, slz) == LT_HALL) side = 2;
    else if (lmTagLocal(s, slx + 1, slz) == LT_HALL) side = 3;
    if (side < 0) return;

    float lx = px + CS * 0.5f, lz = pz + CS * 0.5f, yaw = 0.0f;
    if (side == 0) { lz = pz + 0.18f; yaw = 0.0f; }
    else if (side == 1) { lz = pz + CS - 0.18f; yaw = 3.14159265f; }
    else if (side == 2) { lx = px + 0.18f; yaw = 1.57079633f; }
    else { lx = px + CS - 0.18f; yaw = -1.57079633f; }

    DecorBuilder lb(dv, lx, lz, yaw, 1.0f);
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        float rx = 0.32f * (float)sgn;
        decorRod(lb, Vec3(rx, basinBot + 0.25f, 0.0f), Vec3(rx, 0.95f, 0.0f), 0.032f, DECOR_MAT_METAL, 8);
        decorRod(lb, Vec3(rx, 0.95f, 0.0f), Vec3(rx, 0.95f, -0.42f), 0.032f, DECOR_MAT_METAL, 8);
        decorRod(lb, Vec3(rx, 0.95f, -0.42f), Vec3(rx, 0.0f, -0.42f), 0.032f, DECOR_MAT_METAL, 8);
    }
    for (float ry = basinBot + 0.55f; ry < -0.20f; ry += 0.45f) {
        lb.box(0.0f, ry, 0.0f, 0.64f, 0.04f, 0.09f, DECOR_MAT_METAL);
    }
}

inline void lmBuildSeveredSpanTip(float px, float pz, int dx, int dz, float deckBot, bool dark,
                                  std::vector<float>& dv, std::vector<float>& lv) {
    float cx = px + CS * 0.5f + dx * (CS * 0.5f);
    float cz = pz + CS * 0.5f + dz * (CS * 0.5f);
    float yaw = atan2f((float)dx, (float)dz);
    DecorBuilder b(dv, cx, cz, yaw, 1.0f);
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        float ox = 1.35f * (float)sgn;
        b.box(ox, deckBot + 0.35f, 1.35f, 0.26f, 0.48f, 2.90f, DECOR_MAT_DARK_METAL);
        b.box(ox, deckBot + 0.80f, 1.35f, 0.44f, 0.07f, 2.90f, DECOR_MAT_DARK_METAL);
        b.box(ox, deckBot + 0.30f, 1.35f, 0.44f, 0.07f, 2.90f, DECOR_MAT_DARK_METAL);
    }
    b.box(0.0f, 0.0f, -0.22f, CS - 0.90f, 0.34f, 0.28f, DECOR_MAT_CAUTION);
    if (!dark) {
        b.box(-1.65f, 0.0f, -0.22f, 0.08f, 2.75f, 0.08f, DECOR_MAT_DARK_METAL);
        b.box( 1.65f, 0.0f, -0.22f, 0.08f, 2.75f, 0.08f, DECOR_MAT_DARK_METAL);
        b.box( 0.0f,  2.75f, -0.22f, 3.38f, 0.08f, 0.08f, DECOR_MAT_DARK_METAL);
        lmBuildExitLightbox(cx - dx * 0.22f, 2.36f, cz - dz * 0.22f, yaw + 3.14159265f, dv, lv);
    }
}

inline bool lmSlabHasEscherKnot(const LimenSlab& s) {
    return lmSlabIsVault(s) && !s.dark && lmSlabHasEscherKnotAt(s.rect.j, s.rect.i);
}

// Delirium Structure: rare (~4% of mega-slabs, non-adjacent!) Escherian knot of intersecting &
// upside-down staircases built in the slab's own wall & dado materials (`wv`/`dadoV`)!
inline void lmBuildEscherStairKnot(const LimenSlab& s, float tile, std::vector<float>& wv,
                                   std::vector<float>& dadoV, std::vector<float>& dv, std::vector<float>& lv) {
    if (!lmSlabHasEscherKnot(s)) return;
    float ceilY = s.ceilTiers * WH;
    float x0 = (s.rect.x0 + 1) * CS, x1 = s.rect.x1 * CS;
    float z0 = (s.rect.z0 + 1) * CS, z1 = s.rect.z1 * CS;
    float midX = 0.5f * (x0 + x1), midZ = 0.5f * (z0 + z1);
    u32 baseH = lmHash((u32)s.rect.j, (u32)s.rect.i, 91u, 0x4553u);
    int nFlights = 9 + (s.deliriumPeak - 140) * 4 / 115;
    if (nFlights < 9) nFlights = 9;

    for (int f = 0; f < nFlights; f++) {
        u32 hf = lmHash(baseH, (u32)f, 3u, 0x4554u);
        int dir = f & 3;
        bool inverted = (f % 3) == 1;
        float ox = midX + (((float)(hf % 100u) / 99.0f) - 0.5f) * 15.5f;
        float oz = midZ + (((float)((hf >> 7) % 100u) / 99.0f) - 0.5f) * 15.5f;
        float spanY = fminf(ceilY - 8.5f, 13.5f);
        float baseY = 4.5f + ((float)(f % 5) / 4.0f) * fmaxf(spanY, 2.5f);
        const int steps = 11;
        const float run = 0.52f, rise = inverted ? -0.34f : 0.34f, wHalf = 0.90f;
        // A descending flight must end above head height: the knot hangs over the floor, nobody
        // can be standing inside its lowest landing.
        if (inverted && baseY + steps * rise - 0.32f < 2.9f) baseY = 2.9f + 0.32f - steps * rise;
        float dx = (dir == 0) ? 1.0f : (dir == 1 ? -1.0f : 0.0f);
        float dz = (dir == 2) ? 1.0f : (dir == 3 ? -1.0f : 0.0f);

        std::vector<float>& landingDst = (f & 1) ? dadoV : wv;
        lmPushBox(landingDst, ox - 1.25f, baseY - 0.32f, oz - 1.25f, ox + 1.25f, baseY, oz + 1.25f, CS);
        float ex = ox + dx * (steps * run + 0.95f), ez = oz + dz * (steps * run + 0.95f);
        float endY = baseY + steps * rise;
        lmPushBox(landingDst, ex - 1.25f, endY - 0.32f, ez - 1.25f, ex + 1.25f, endY, ez + 1.25f, CS);

        for (int k = 0; k < steps; k++) {
            float sx = ox + dx * ((k + 0.5f) * run), sz = oz + dz * ((k + 0.5f) * run);
            float sy0 = baseY + k * rise - 0.28f, sy1 = baseY + (k + 1) * rise;
            if (sy0 > sy1) { float tmp = sy0; sy0 = sy1 - 0.26f; sy1 = tmp + 0.26f; }
            float hx = (dx != 0.0f) ? (run * 0.56f) : wHalf;
            float hz = (dz != 0.0f) ? (run * 0.56f) : wHalf;
            lmPushBox((k & 1) ? dadoV : wv, sx - hx, sy0, sz - hz, sx + hx, sy1, sz + hz, CS);
        }
        // Subtle handrail + localized landing lamps (instead of blinding white stringers)
        if ((f % 2) == 0) {
            lmPushLightBox(lv, ox, baseY + (inverted ? -0.62f : 0.05f), oz, 0.35f, 0.18f, 0.35f);
        }
        if ((f % 3) == 0) {
            float dYaw = atan2f(dx, dz);
            lmBuildClosedWallDoor(ex, inverted ? (endY - 2.50f) : endY, ez, dYaw, false, dv);
            lmBuildExitLightbox(ex, endY + (inverted ? -2.85f : 2.50f), ez, dYaw, dv, lv);
        }
    }
    (void)tile;
}

// Delirium Furniture & Structures:
//   (1) Shrinking canted portal frames along 3..4x Matryoshka rooms-within-rooms,
//   (1b) Continuous curving/polygonal Nautilus spiral corridor (narrowing & dropping inward!),
//   (2) Mismatched door trios (0.7m + floating + 3.75m) in strong pockets (>0.55),
//   (3) Upside-down furniture glued to ceilings + floor-to-ceiling furniture towers!
inline void lmBuildSlabDeliriumProps(const LimenSlab& s, std::vector<float>& wv, std::vector<float>& dadoV,
                                     std::vector<float>& dv, std::vector<float>& lv,
                                     std::vector<LimenAabb2D>& outCols) {
    if (s.dark) return;
    static const int ddx[4] = {1, -1, 0, 0}, ddz[4] = {0, 0, 1, -1};
    // 1. Telescoping shrinking canted portals along 3..4x Matryoshka nested shells!
    if (s.nestedRings >= 2 && s.nestedX >= 0) {
        static const int sdx[4] = {1, -1, 0, 0}, sdz[4] = {0, 0, 1, -1};
        int dir = s.nestedDir & 3;
        float yaw = atan2f((float)sdx[dir], (float)sdz[dir]);
        // Bright entrance light right in front of the outer Matryoshka frame so it's never dark!
        float m0x = (s.rect.x0 + 1 + s.nestedX + 0.5f) * CS, m0z = (s.rect.z0 + 1 + s.nestedZ + 0.5f) * CS;
        lmPushLightQuad(lv, Vec3(m0x - 0.65f, 3.85f, m0z - 0.65f), Vec3(m0x + 0.65f, 3.85f, m0z - 0.65f),
                            Vec3(m0x + 0.65f, 3.85f, m0z + 0.65f), Vec3(m0x - 0.65f, 3.85f, m0z + 0.65f), true);
        for (int r = 0; r < s.nestedRings; r++) {
            int doorLX = s.nestedX + sdx[dir] * (1 + r * 2);
            int doorLZ = s.nestedZ + sdz[dir] * (1 + r * 2);
            float cx = (s.rect.x0 + 1 + doorLX + 0.5f) * CS;
            float cz = (s.rect.z0 + 1 + doorLZ + 0.5f) * CS;
            float wOpen = 2.10f - (float)r * 0.36f;
            float hOpen = 3.55f - (float)r * 0.52f;
            int mat = (r & 1) ? DECOR_MAT_CAUTION : DECOR_MAT_TRIM;
            DecorBuilder pb(dv, cx, cz, yaw + ((r & 1) ? 0.08f : -0.08f), 1.0f);
            pb.box(-wOpen * 0.5f - 0.16f, -0.2f, 0.0f, 0.32f, hOpen + 0.4f, 0.44f, mat);
            pb.box( wOpen * 0.5f + 0.16f, -0.2f, 0.0f, 0.32f, hOpen + 0.4f, 0.44f, mat);
            pb.box(0.0f, hOpen, 0.0f, wOpen + 0.64f, 0.34f, 0.46f, mat);
            float rx = cx + sdx[dir] * CS, rz = cz + sdz[dir] * CS;
            lmPushLightQuad(lv, Vec3(rx - 0.65f, hOpen + 0.12f, rz - 0.65f), Vec3(rx + 0.65f, hOpen + 0.12f, rz - 0.65f),
                                Vec3(rx + 0.65f, hOpen + 0.12f, rz + 0.65f), Vec3(rx - 0.65f, hOpen + 0.12f, rz + 0.65f), true);
        }
    }

    // 1b. Polygonal Nautilus Spiral Corridor: curves continuously inward for 2.35 turns, narrowing &
    // dropping its wall/ceiling height per turn with accelerating lamps & shrinking doors!
    if (s.spiralW >= 7 && s.spiralH >= 7 && s.spiralX >= 0) {
        float scx = (s.rect.x0 + 1 + s.spiralX + s.spiralW * 0.5f) * CS;
        float scz = (s.rect.z0 + 1 + s.spiralZ + s.spiralH * 0.5f) * CS;
        float R0 = fminf((float)s.spiralW, (float)s.spiralH) * 0.45f * CS;
        float R1 = 1.65f;
        const int segs = 26;
        const float aStart = 0.42f, aTotal = 2.35f * 6.2831853f;
        // Wall nodes along the spiral and a mitred half-thickness offset at each one, so the two
        // skins of neighbouring segments meet edge to edge (no wedge gaps on the outer bend,
        // no skins poking through each other on the inner one).
        float nodeX[segs + 1], nodeZ[segs + 1], nodeH[segs + 1], offX[segs + 1], offZ[segs + 1];
        for (int k = 0; k <= segs; k++) {
            float u = (float)k / (float)segs, a = aStart + u * aTotal;
            // Squared ease keeps outer turns wide & compresses inner turns rapidly!
            float r = R0 + (R1 - R0) * powf(u, 0.86f);
            nodeX[k] = scx + cosf(a) * r; nodeZ[k] = scz + sinf(a) * r;
            nodeH[k] = 4.30f - 3.10f * u;   // drops 4.30m -> 1.20m so inner turns reveal 2..3 coils!
        }
        for (int k = 0; k <= segs; k++) {
            float ax = 0.0f, az = 0.0f, bx = 0.0f, bz = 0.0f;
            if (k > 0) { ax = nodeX[k] - nodeX[k - 1]; az = nodeZ[k] - nodeZ[k - 1]; }
            if (k < segs) { bx = nodeX[k + 1] - nodeX[k]; bz = nodeZ[k + 1] - nodeZ[k]; }
            float al = sqrtf(ax * ax + az * az), bl = sqrtf(bx * bx + bz * bz);
            float mx = 0.0f, mz = 0.0f, cosHalf = 1.0f;
            if (al > 1e-4f) { mx += -az / al; mz += ax / al; }
            if (bl > 1e-4f) { mx += -bz / bl; mz += bx / bl; }
            float ml = sqrtf(mx * mx + mz * mz);
            if (ml < 1e-4f) { mx = 1.0f; mz = 0.0f; ml = 1.0f; }
            mx /= ml; mz /= ml;
            if (al > 1e-4f && bl > 1e-4f) cosHalf = fmaxf(0.5f, mx * (-az / al) + mz * (ax / al));
            offX[k] = mx * 0.14f / cosHalf; offZ[k] = mz * 0.14f / cosHalf;
        }
        for (int e = 0; e <= segs; e += segs) {   // end caps: the mouth and the core
            float sgn = (e == 0) ? 1.0f : -1.0f;
            Vec3 pa(nodeX[e] + offX[e] * sgn, -0.3f, nodeZ[e] + offZ[e] * sgn), pb(nodeX[e] - offX[e] * sgn, -0.3f, nodeZ[e] - offZ[e] * sgn);
            lmPushQuad(wv, pa, Vec3(pa.x, nodeH[e], pa.z), Vec3(pb.x, nodeH[e], pb.z), pb, CS);
        }
        for (int k = 0; k < segs; k++) {
            float u0 = (float)k / (float)segs, u1 = (float)(k + 1) / (float)segs;
            float a0 = aStart + u0 * aTotal, a1 = aStart + u1 * aTotal;
            float h0 = nodeH[k], h1 = nodeH[k + 1];
            float xA = nodeX[k], zA = nodeZ[k], xB = nodeX[k + 1], zB = nodeZ[k + 1];
            float dx = xB - xA, dz = zB - zA;
            float len = sqrtf(dx * dx + dz * dz);
            float nx = -dz / (len + 1e-6f) * 0.14f, nz = dx / (len + 1e-6f) * 0.14f;
            float oAx = offX[k], oAz = offZ[k], oBx = offX[k + 1], oBz = offZ[k + 1];
            float dadoTop = fminf(1.05f - 0.45f * u0, fminf(h0, h1) * 0.4f);
            // Solid curved spiral wall (both skins face outward) + lower dado band + glowing crest strip!
            lmPushQuad(dadoV, Vec3(xB+oBx, -0.3f, zB+oBz), Vec3(xB+oBx, dadoTop, zB+oBz), Vec3(xA+oAx, dadoTop, zA+oAz), Vec3(xA+oAx, -0.3f, zA+oAz), CS);
            lmPushQuad(dadoV, Vec3(xA-oAx, -0.3f, zA-oAz), Vec3(xA-oAx, dadoTop, zA-oAz), Vec3(xB-oBx, dadoTop, zB-oBz), Vec3(xB-oBx, -0.3f, zB-oBz), CS);
            lmPushQuad(wv, Vec3(xB+oBx, dadoTop, zB+oBz), Vec3(xB+oBx, h1, zB+oBz), Vec3(xA+oAx, h0, zA+oAz), Vec3(xA+oAx, dadoTop, zA+oAz), CS);
            lmPushQuad(wv, Vec3(xA-oAx, dadoTop, zA-oAz), Vec3(xA-oAx, h0, zA-oAz), Vec3(xB-oBx, h1, zB-oBz), Vec3(xB-oBx, dadoTop, zB-oBz), CS);
            lmPushLightQuad(lv, Vec3(xA-oAx, h0, zA-oAz), Vec3(xA+oAx, h0, zA+oAz),
                                Vec3(xB+oBx, h1, zB+oBz), Vec3(xB-oBx, h1, zB-oBz), true);

            float outSpan = fmaxf(1.85f - 1.05f * u0, 0.65f);
            int beat = (u0 < 0.40f) ? 3 : ((u0 < 0.72f) ? 2 : 1);
            if ((k % beat) == 0) {
                float mx = 0.5f * (xA + xB) + cosf(0.5f * (a0 + a1)) * (outSpan * 0.55f);
                float mz = 0.5f * (zA + zB) + sinf(0.5f * (a0 + a1)) * (outSpan * 0.55f);
                float ly = 0.5f * (h0 + h1) - 0.04f;
                float ls = fmaxf(0.46f - 0.22f * u0, 0.20f);
                lmPushLightQuad(lv, Vec3(mx-ls, ly, mz-ls), Vec3(mx+ls, ly, mz-ls), Vec3(mx+ls, ly, mz+ls), Vec3(mx-ls, ly, mz+ls), true);
            }
            if ((k % (u0 < 0.5f ? 4 : 2)) == 1) {
                float dScale = fmaxf(1.0f - 0.52f * u0, 0.46f);
                float fYaw = atan2f(-nx, -nz);
                DecorBuilder db(dv, 0.5f * (xA + xB) - nx * 1.15f, 0.5f * (zA + zB) - nz * 1.15f, fYaw, dScale);
                db.panel(0.0f, 0.0f, 0.02f, 1.05f, 2.15f, 0.06f, DECOR_MAT_DOOR);
                db.box(0.0f, 2.15f, 0.02f, 1.22f, 0.12f, 0.12f, DECOR_MAT_TRIM);
            }
            // The wall is solid along its whole length (a box every 0.3 m, not two per segment)
            int nCol = (int)ceilf(len / 0.30f);
            for (int c = 0; c < nCol; c++) {
                float f = ((float)c + 0.5f) / (float)nCol;
                float xm = xA + dx * f, zm = zA + dz * f;
                outCols.push_back({xm - 0.19f, zm - 0.19f, xm + 0.19f, zm + 0.19f});
            }
        }
        // Tiny glowing plug/door chamber right at the spiral core!
        lmPushLightBox(lv, scx, 0.10f, scz, 0.55f, 1.65f, 0.55f);
    }

    if (s.deliriumPeak < 95) return;
    bool vault = lmSlabIsVault(s);
    int towersBuilt = 0, doorRowsBuilt = 0, invBuilt = 0;
    for (int lx = 2; lx < s.w - 2; lx++) {
        for (int lz = 2; lz < s.h - 2; lz++) {
            int tag = s.tag[lx][lz];
            if (!lmIsWalkableTag(tag)) continue;
            int wx = s.rect.x0 + 1 + lx, wz = s.rect.z0 + 1 + lz;
            int del = lmDeliriumCell255(wx, wz);
            if (del < 98) continue;
            if (s.zone[lx][lz] & LZ_STORY) continue;   // nothing odd grows on the rig's lane or in a stamped set
            u32 hc = lmHash((u32)wx, (u32)wz, 37u, 0x4450u);
            unsigned themes = lmPickCellDecorTheme(s, lx, lz, 29u);
            float cx = (wx + 0.5f) * CS, cz = (wz + 0.5f) * CS;
            bool high = vault && !lmIsLowShellTag(tag);
            float cH = high ? (s.ceilTiers * WH) : lmShellCeilHeight(s.flag[lx][lz], s.ceilTiers, WH, s.zone[lx][lz]);

            // Grounded uncanny door rhythm (narrow 0.68 m service doors standing cleanly on the floor,
            // replacing floating doors and upside-down ceiling props!)
            (void)invBuilt;
            if (!high && del >= 140 && doorRowsBuilt < 4 && !(s.flag[lx][lz] & (LF_NARROW_X | LF_NARROW_Z)) && (hc % 4u) == 0u) {
                for (int d = 0; d < 4; d++) {
                    if (lmTagLocal(s, lx + ddx[d], lz + ddz[d]) != LT_WALL) continue;
                    float wcx = cx + ddx[d] * (CS * 0.5f - 0.04f), wcz = cz + ddz[d] * (CS * 0.5f - 0.04f);
                    float wYaw = atan2f((float)-ddx[d], (float)-ddz[d]);
                    float gBase = lmGroundHeightAt(cx, cz, CS);
                    DecorBuilder db(dv, wcx, wcz, wYaw, 1.0f);
                    for (int di = -1; di <= 1; di++) {
                        float ox = (float)di * 1.42f;
                        db.panel(ox, gBase, 0.04f, 0.68f, 1.98f, 0.07f, DECOR_MAT_DOOR);
                        db.box(ox, gBase + 1.98f, 0.04f, 0.80f, 0.09f, 0.11f, DECOR_MAT_TRIM);
                    }
                    doorRowsBuilt++;
                    break;
                }
            }

            // Stacked archive/chair column in strong pockets (capped at 2 per slab)
            if (towersBuilt < 2 && (tag == LT_HALL || tag == LT_CORR) && del >= 145 &&
                !(s.flag[lx][lz] & (LF_NARROW_X | LF_NARROW_Z)) && !lmPillarAt(wx, wz) &&
                ((hc >> 18) % (high ? 16u : 9u)) == 0u) {
                int ptype = lmPickStructuredIslandProp(themes, hc >> 5);
                if (ptype >= 0) {
                    float offSide = (tag == LT_CORR) ? 1.25f : 0.0f;
                    float tx = cx + offSide, tz = cz;
                    int nStack = high ? (8 + (int)((hc >> 9) % 6u)) : 5;
                    float stepH = 0.86f;
                    float baseG = lmGroundHeightAt(tx, tz, CS);
                    for (int st = 0; st < nStack && baseG + (st + 1) * stepH < cH - 0.25f; st++) {
                        MapProp pr{};
                        pr.pos = Vec3(tx + sinf((float)st * 1.4f) * (0.08f * (float)st), 0.0f,
                                      tz + cosf((float)st * 1.1f) * (0.08f * (float)st));
                        pr.type = ptype; pr.scale = 1.0f - (float)st * 0.02f;
                        pr.yaw = (float)st * 0.55f + (float)(hc & 3u);
                        size_t v0 = dv.size();
                        buildMapPropModel(dv, pr);
                        float yLift = baseG + (float)st * stepH;
                        for (size_t vi = v0 + 1; vi < dv.size(); vi += 8) dv[vi] += yLift;
                    }
                    outCols.push_back({tx - 0.58f, tz - 0.58f, tx + 0.58f, tz + 0.58f});
                    towersBuilt++;
                }
            }
        }
    }
}
