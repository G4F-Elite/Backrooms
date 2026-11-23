#pragma once
// Part of limen_grammar.h: unstructured megastructures, beat stamps, xenografts.

// ---------------------------------------------------------------- mega-unstructures ("мегабезструктуры")
// Non-manmade monumental phenomena: zero rooms, zero corridors-with-doors, zero human architecture.

// MF_MONOLITH: colossal raw overhanging grey monolith slabs separated by narrow vertical mist canyons.
inline void lgMegaMonolith(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    b.nodes++;
    lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_HALL);
    if (s.w < 10 || s.h < 10) return;

    int axis = (s.megaSub & 1);
    int L = (axis == 0) ? s.w : s.h;
    int C = (axis == 0) ? s.h : s.w;
    auto setA = [&](int a, int c, int tag) {
        int x = (axis == 0) ? a : c, z = (axis == 0) ? c : a;
        if (x >= 1 && z >= 1 && x < s.w - 1 && z < s.h - 1) {
            s.tag[x][z] = (unsigned char)tag;
            s.flag[x][z] = 0u;
        }
    };

    int shift = ((s.megaSub >> 2) % 3) - 1;
    int slitW = (C >= 15) ? 2 : 1;
    int cMid = lmClampInt(C / 2 + shift, 4, C - 5);
    int cLo = cMid - slitW / 2, cHi = cLo + slitW - 1;
    const bool onRoad = s.roadThrough && s.roadAxis == axis;   // the street between the monoliths is the road
    if (onRoad) { cLo = lmClampInt((int)s.roadC - (slitW - 1), 4, C - 5); cHi = cLo + slitW - 1; if (s.roadC < cLo) { cLo = s.roadC; cHi = cLo + slitW - 1; } }

    // Pack asymmetric colossal monolith masses on both flanks of the central vertical canyon slit
    for (int side = 0; side < 2; side++) {
        int f0 = (side == 0) ? 1 : (cHi + 1);
        int f1 = (side == 0) ? (cLo - 1) : (C - 2);
        if (f1 - f0 + 1 < 2) continue;
        int a = 1;
        int idx = 0;
        while (a <= L - 3) {
            u32 hm = lmHash(n.h, (u32)(side * 41 + idx), 11u, 0x4D4Fu);
            int mLen = 2 + (int)(hm % 4u); // 2..5 cells long
            int gap  = 1 + (int)((hm >> 4) % 2u); // 1..2 cell vertical cross-fissure
            if (a + mLen > L - 2) mLen = L - 1 - a;
            if (mLen < 2) break;

            int fSpan = f1 - f0 + 1;
            if (fSpan >= 6 && ((hm >> 7) & 1u)) {
                // Split deep flank into two stepped monolithic slabs separated by a 1-cell chasm
                int splitAt = f0 + 2 + (int)((hm >> 9) % (u32)(fSpan - 4));
                for (int aa = a; aa < a + mLen; aa++) {
                    for (int cc = f0; cc <= f1; cc++) {
                        if (cc == splitAt) continue;
                        setA(aa, cc, LT_WALL);
                    }
                }
            } else {
                int inset0 = (side == 1 && ((hm >> 12) % 3u) == 0u) ? 1 : 0;
                int inset1 = (side == 0 && ((hm >> 12) % 3u) == 0u) ? 1 : 0;
                int cc0 = f0 + inset0, cc1 = f1 - inset1;
                if (cc1 >= cc0 + 1) {
                    for (int aa = a; aa < a + mLen; aa++)
                        for (int cc = cc0; cc <= cc1; cc++) setA(aa, cc, LT_WALL);
                }
            }
            a += mLen + gap;
            idx++;
        }
    }

    // Stagger 1..2 asymmetrical buttresses into a 2-cell central canyon so vistas frame layered monoliths
    if (slitW >= 2 && L >= 14) {
        int a1 = L / 3, a2 = (L * 2) / 3;
        int side1 = (s.megaSub >> 4) & 1;
        int c1 = side1 ? cHi : cLo, c2 = side1 ? cLo : cHi;
        if (onRoad) { c1 = c2 = (s.roadC == cLo) ? cHi : cLo; }   // buttresses never stand on the road
        setA(a1, c1, LT_WALL);
        setA(a1 + 1, c1, LT_WALL);
        setA(a2, c2, LT_WALL);
        setA(a2 + 1, c2, LT_WALL);
    }
}

// MF_SPIRES: unbroken undulating ash/snow terrain pierced by colossal dark forked basalt spires.
inline void lgMegaSpires(LgBuild& b, const LgNode& n) {
    (void)n;
    LimenSlab& s = *b.s;
    b.nodes++;
    lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_HALL);
}

// MF_GROTTO: dark organic subterranean cavern dome with rugged rock corners & wide cardinal mouths.
inline void lgMegaGrotto(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    b.nodes++;
    lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_HALL);
    if (s.w < 12 || s.h < 12) return;

    float cx = (float)(s.w - 1) * 0.5f, cz = (float)(s.h - 1) * 0.5f;
    float rx = (float)(s.w - 2) * 0.5f, rz = (float)(s.h - 2) * 0.5f;
    float p1 = (float)(n.h & 255u) * 0.0245f;
    float p2 = (float)((n.h >> 8) & 255u) * 0.0245f;
    int mx = s.w / 2, mz = s.h / 2;

    for (int x = 1; x < s.w - 1; x++) {
        for (int z = 1; z < s.h - 1; z++) {
            // Keep 3-cell wide organic radial entry mouths open on all 4 sides
            if (abs(x - mx) <= 1 || abs(z - mz) <= 1) continue;
            float nx = ((float)x - cx) / rx, nz = ((float)z - cz) / rz;
            float rad = sqrtf(nx * nx + nz * nz);
            float ang = atan2f(nz, nx);
            float rim = 0.86f + 0.11f * sinf(ang * 3.0f + p1) + 0.07f * cosf(ang * 5.0f + p2);
            if (rad > rim) {
                s.tag[x][z] = LT_WALL;
                s.flag[x][z] = 0u;
            }
        }
    }
}

// MF_TENDRILS: crooked branching floating earth ribbons (`LT_BRIDGE` over `LT_VOID`) in open sky-void.
inline void lgMegaTendrils(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    b.nodes++;
    lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_VOID);
    if (s.w < 7 || s.h < 7) { lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_HALL); return; }

    auto mark = [&](int x, int z) {
        if (x < 0 || z < 0 || x >= s.w || z >= s.h) return;
        s.tag[x][z] = LT_BRIDGE;
        s.flag[x][z] = LF_OVER_VOID;
    };

    auto walkCrooked = [&](int x, int z, int tx, int tz, u32 seed) {
        mark(x, z);
        int guard = 0;
        while ((x != tx || z != tz) && guard++ < (s.w + s.h) * 3) {
            u32 h = lmHash(seed, (u32)guard, (u32)(x * 37 + z), 0x544Eu);
            int dx = tx - x, dz = tz - z;
            bool stepX = (abs(dx) > 0 && abs(dz) > 0) ? ((h & 3u) < (abs(dx) >= abs(dz) ? 3u : 1u)) : (dx != 0);
            int seg = 2 + (int)((h >> 3) % 3u);
            for (int k = 0; k < seg; k++) {
                if (stepX) {
                    if (x == tx) break;
                    x += (dx > 0 ? 1 : -1);
                } else {
                    if (z == tz) break;
                    z += (dz > 0 ? 1 : -1);
                }
                mark(x, z);
            }
            // Occasional organic lateral dogleg jog
            if (((h >> 7) % 5u) == 0u) {
                int jx = stepX ? 0 : (((h >> 10) & 1u) ? 1 : -1);
                int jz = stepX ? (((h >> 10) & 1u) ? 1 : -1) : 0;
                int nx = lmClampInt(x + jx, 1, s.w - 2);
                int nz = lmClampInt(z + jz, 1, s.h - 2);
                x = nx; z = nz;
                mark(x, z);
            }
        }
    };

    int mx = s.w / 2, mz = s.h / 2;
    int zW = lmClampInt(mz + (int)((n.h % 7u) - 3), 2, s.h - 3);
    int zE = lmClampInt(mz + (int)(((n.h >> 4) % 7u) - 3), 2, s.h - 3);
    int xN = lmClampInt(mx + (int)(((n.h >> 8) % 7u) - 3), 2, s.w - 3);
    int xS = lmClampInt(mx + (int)(((n.h >> 12) % 7u) - 3), 2, s.w - 3);

    // Two primary crooked crossing aerial root pathways
    walkCrooked(1, zW, s.w - 2, zE, n.h ^ 0x1111u);
    walkCrooked(xN, 1, xS, s.h - 2, n.h ^ 0x2222u);

    // Two secondary meandering loops framing open sky-void windows
    int qx0 = lmClampInt(s.w / 4, 2, s.w - 3), qx1 = lmClampInt((s.w * 3) / 4, 2, s.w - 3);
    int qz0 = lmClampInt(s.h / 4, 2, s.h - 3), qz1 = lmClampInt((s.h * 3) / 4, 2, s.h - 3);
    walkCrooked(qx0, qz0, qx1, qz0, n.h ^ 0x3333u);
    walkCrooked(qx1, qz0, qx1, qz1, n.h ^ 0x4444u);
    walkCrooked(qx0, qz1, qx1, qz1, n.h ^ 0x5555u);
    walkCrooked(qx0, qz0, qx0, qz1, n.h ^ 0x6666u);

    // 3 short cantilevered root spurs jutting straight out into the sky void
    static const int sdx[4] = {1, -1, 0, 0}, sdz[4] = {0, 0, 1, -1};
    int spurs = 0;
    for (int att = 0; att < 32 && spurs < 3; att++) {
        u32 hs = lmHash(n.h, (u32)att, 19u, 0x5350u);
        int sx = 2 + (int)(hs % (u32)(s.w - 4));
        int sz = 2 + (int)((hs >> 7) % (u32)(s.h - 4));
        if (s.tag[sx][sz] != LT_BRIDGE) continue;
        int d = (int)((hs >> 14) & 3u);
        int len = 2 + (int)((hs >> 16) % 3u);
        bool clear = true;
        for (int k = 1; k <= len; k++) {
            int cx = sx + sdx[d] * k, cz = sz + sdz[d] * k;
            if (cx < 2 || cz < 2 || cx >= s.w - 2 || cz >= s.h - 2 || s.tag[cx][cz] != LT_VOID) { clear = false; break; }
        }
        if (!clear) continue;
        for (int k = 1; k <= len; k++) mark(sx + sdx[d] * k, sz + sdz[d] * k);
        spurs++;
    }
}

inline void lgGrowMegaRoot(LgBuild& b, const LgNode& root) {
    int mf = b.s->megaForm;
    if (mf == MF_ZIGGURAT)       lgMegaZiggurat(b, root);
    else if (mf == MF_CANYON)    lgMegaCanyon(b, root);
    else if (mf == MF_HYPOSTYLE) lgMegaHypostyle(b, root);
    else if (mf == MF_SILO)      lgMegaSilo(b, root);
    else if (mf == MF_HANGING)   lgMegaHanging(b, root);
    else if (mf == MF_CELLBLOCK) lgMegaCellblock(b, root);
    else if (mf == MF_PLATFORM)  lgMegaPlatform(b, root);
    else if (mf == MF_MONOLITH)  lgMegaMonolith(b, root);
    else if (mf == MF_SPIRES)    lgMegaSpires(b, root);
    else if (mf == MF_GROTTO)    lgMegaGrotto(b, root);
    else if (mf == MF_TENDRILS)  lgMegaTendrils(b, root);
    else                         lgGrow(b, root);
}

// ---------------------------------------------------------------- narrator stamps
// A beat that asks for nested rooms or a spiral corridor cannot wait for the delirium field to
// roll one: the structure is stamped over the grown layout (like a xenograft, the repair pass
// reconnects whatever it cut off), beside the road and never on it.
inline void lgOpBeatStamp(LgBuild& b) {
    LimenSlab& s = *b.s;
    bool wantNest = (s.beatForce & LBF_NESTED) != 0u, wantSpiral = (s.beatForce & LBF_SPIRAL) != 0u;
    if ((!wantNest && !wantSpiral) || b.vault) return;
    if (wantNest && s.nestedRings >= 2) wantNest = false;        // the field already grew one
    if (wantSpiral && s.spiralW > 0) wantSpiral = false;
    u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, 5u, 0x5354u);
    for (int kind = 0; kind < 2; kind++) {
        bool nest = (kind == 0);
        if (nest ? !wantNest : !wantSpiral) continue;
        int size = nest ? 17 : 13;
        int availX = s.w - 4, availZ = s.h - 4;
        int sx = size, sz = size;
        // room across the road: the wider side of it
        int lo = 2, hi = -1;                 // across-range the stamp may use
        int C = (s.focusAxis == 0) ? s.h : s.w;
        if (s.focusAxis >= 0) {
            int below = (int)s.focusC - 3, above = C - 4 - (int)s.focusC;   // cells on either side, one clear of the road
            bool up = above > below || (above == below && (h & 1u));
            if (kind == 1 && wantNest) up = !up;                 // two stamps take opposite sides
            if (up) { lo = (int)s.focusC + 2; hi = C - 3; } else { lo = 2; hi = (int)s.focusC - 2; }
        } else { lo = 2; hi = C - 3; }
        int across = hi - lo + 1;
        int& sAcross = (s.focusAxis == 0) ? sz : sx;
        int& sAlong  = (s.focusAxis == 0) ? sx : sz;
        int availAlong = (s.focusAxis == 0) ? availX : availZ;
        if (s.focusAxis < 0) { if (sx > availX) sx = availX; if (sz > availZ) sz = availZ; }
        else { if (sAcross > across) sAcross = across; if (sAlong > availAlong) sAlong = availAlong; }
        if (sx < (nest ? 11 : 9) || sz < (nest ? 11 : 9)) continue;
        LgNode n;
        int L = (s.focusAxis == 0) ? s.w : s.h;
        int a0 = 2 + (L - 4 - sAlong) / 2 + (int)((h >> (4 + kind * 3)) % 5u) - 2;
        a0 = lmClampInt(a0, 2, L - 2 - sAlong);
        int c0 = 0;
        if (s.focusAxis < 0) { n.x0 = 2 + (availX - sx) / 2; n.z0 = 2 + (availZ - sz) / 2; }
        else {
            c0 = (lo > (int)s.focusC) ? lo : (hi - sAcross + 1);  // hard by the road
            if (s.focusAxis == 0) { n.x0 = a0; n.z0 = c0; } else { n.x0 = c0; n.z0 = a0; }
        }
        n.x1 = n.x0 + sx - 1; n.z1 = n.z0 + sz - 1;
        n.g = s.genome;
        n.h = lmHash(h, (u32)kind, 7u, 0x5355u);
        n.depth = 1; n.walled = 0u; n.low = false; n.mezz = false;
        for (int x = n.x0; x <= n.x1; x++) for (int z = n.z0; z <= n.z1; z++) s.zone[x][z] &= (unsigned char)~(LZ_XENO | LZ_STD_CEIL | LZ_TALL_WELL | LZ_SPARSE);
        if (nest) lgNestedMatryoshka(b, n); else lgSpiralCorridor(b, n);
        s.stampX0 = (short)n.x0; s.stampZ0 = (short)n.z0; s.stampX1 = (short)n.x1; s.stampZ1 = (short)n.z1;
    }
}

// ---------------------------------------------------------------- opXenograft
// Splices a sharp rectangular fragment of a completely different level's DNA directly into
// the slab: foreign genome, foreign wall/floor surfaces, foreign prop themes, foreign lamp
// cadence, and shifted ceiling height, sheared straight across the host layout without frames.
inline void lgOpXenograft(LgBuild& b) {
    LimenSlab& s = *b.s;
    if (s.w < 13 || s.h < 13) return;
    if (lmStrangeness(s.rect.j, s.rect.i) == 0) return;
    if (b.vault && s.megaForm != MF_CLASSIC && s.megaForm != MF_HYPOSTYLE) return;

    u32 xh = lmHash((u32)s.rect.j, (u32)s.rect.i, 77u, 0x5845u);
    int chance = ((int)gLimenDNA.xenograftTendency * 3 + (int)s.genome.g[LG_GRAFT] * 2) / 10;
    const bool forced = (s.beatForce & LBF_XENO) != 0u;
    if (!forced && (int)(xh % 180u) >= chance) return;

    int fLevel = (int)((lmHash(xh, 1u, 0u, 0x5846u) % 65521u) + 7u);
    if (fLevel == gLimen.level) fLevel += 19;
    LimenLevelDNA fdna = lmMakeLevelDNA(gLimen.seed, fLevel);
    LimenGenome fg = lmForeignGenome(fdna, s.rect.j, s.rect.i, 0x5800u);

    int maxW = lmClampInt(s.w / 2 + 1, 6, 15);
    int maxH = lmClampInt(s.h / 2 + 1, 6, 15);
    int xw = 6 + (int)((xh >> 8) % (u32)(maxW - 5));
    int zh = 6 + (int)((xh >> 14) % (u32)(maxH - 5));
    if (xw > s.w - 4) xw = s.w - 4;
    if (zh > s.h - 4) zh = s.h - 4;
    int x0 = 2 + (int)((xh >> 20) % (u32)(s.w - 3 - xw));
    int z0 = 2 + (int)((xh >> 25) % (u32)(s.h - 3 - zh));
    if (forced && s.focusAxis >= 0) {
        // the cut lies across the road, so whoever travels it walks straight through another level
        int off = 1 + (int)((xh >> 3) % 3u);
        if (s.focusAxis == 0) z0 = lmClampInt((int)s.focusC - off, 2, s.h - 2 - zh);
        else                  x0 = lmClampInt((int)s.focusC - off, 2, s.w - 2 - xw);
    }
    int x1 = x0 + xw - 1, z1 = z0 + zh - 1;

    s.xeno.active = true;
    s.xeno.foreignLevel = fLevel;
    s.xeno.x0 = x0; s.xeno.z0 = z0; s.xeno.x1 = x1; s.xeno.z1 = z1;
    // Abrupt ceiling jump: cramped 2.65 m, standard 4.5 m, or 9.0 m tall shaft cut
    if (fdna.geneBias[LG_HEIGHT] < -20 || fg.g[LG_HEIGHT] < 78) s.xeno.ceilMode = 0;
    else if (fdna.vaultThreshold < 126 || fg.g[LG_HEIGHT] > 142 || ((xh >> 5) & 1u)) s.xeno.ceilMode = 2;
    else s.xeno.ceilMode = 1;
    int fEffL = (fg.g[LG_LIGHT] * 2 + (int)fdna.lightDensity * 3) / 5;
    s.xeno.lampPitch = fEffL > 165 ? 2 : (fEffL > 110 ? 3 : (fEffL > 60 ? 4 : 6));
    s.xeno.hue = (unsigned char)((s.genome.g[LG_HUE] + 108 + (xh & 63u)) & 255u);
    s.xeno.useGene = fg.g[LG_USE];
    s.xeno.wallSurf = fdna.wallSurf;
    s.xeno.floorSurf = fdna.floorSurf;
    memcpy(s.xeno.themeWeight, fdna.themeWeight, LM_THEME_COUNT);

    // Shear the host layout and grow the foreign node directly inside [x0..x1] x [z0..z1]
    lgFill(b, x0, z0, x1, z1, LT_HALL);
    LgNode fn;
    fn.x0 = x0; fn.z0 = z0; fn.x1 = x1; fn.z1 = z1;
    fn.g = fg;
    fn.h = lmHash(xh, (u32)fLevel, 3u, 0x5847u);
    fn.depth = 1;
    fn.walled = 0u;
    fn.low = b.vault && (s.xeno.ceilMode < 2);
    fn.mezz = (s.xeno.ceilMode == 2);
    lgGrow(b, fn);

    for (int x = x0; x <= x1; x++) {
        for (int z = z0; z <= z1; z++) {
            s.zone[x][z] |= LZ_XENO;
            if (b.vault && fn.low && s.tag[x][z] == LT_WALL) {
                s.flag[x][z] |= LF_LOW_WALL | (fn.mezz ? LF_MEZZ : 0u);
            }
        }
    }

    // Raw frameless shear cuts (2 cells wide so doorwayAxisAt never puts a portal frame!)
    int cutTag = (b.vault && fn.low) ? LT_LOW : LT_HALL;
    int mx = (x0 + x1) / 2, mz = (z0 + z1) / 2;
    if (x0 >= 2)         { lgSet(b, x0, mz, cutTag); if (mz + 1 <= z1) lgSet(b, x0, mz + 1, cutTag); lgSet(b, x0 - 1, mz, LT_CORR); if (mz + 1 <= z1) lgSet(b, x0 - 1, mz + 1, LT_CORR); }
    if (x1 <= s.w - 3)   { lgSet(b, x1, mz, cutTag); if (mz + 1 <= z1) lgSet(b, x1, mz + 1, cutTag); lgSet(b, x1 + 1, mz, LT_CORR); if (mz + 1 <= z1) lgSet(b, x1 + 1, mz + 1, LT_CORR); }
    if (z0 >= 2)         { lgSet(b, mx, z0, cutTag); if (mx + 1 <= x1) lgSet(b, mx + 1, z0, cutTag); lgSet(b, mx, z0 - 1, LT_CORR); if (mx + 1 <= x1) lgSet(b, mx + 1, z0 - 1, LT_CORR); }
    if (z1 <= s.h - 3)   { lgSet(b, mx, z1, cutTag); if (mx + 1 <= x1) lgSet(b, mx + 1, z1, cutTag); lgSet(b, mx, z1 + 1, LT_CORR); if (mx + 1 <= x1) lgSet(b, mx + 1, z1 + 1, LT_CORR); }
    for (int x = x0; x <= x1; x++) for (int z = z0; z <= z1; z++) s.zone[x][z] |= LZ_XENO;
}
