#pragma once
// Part of limen_grammar.h: growing a slab and the built megastructures.

// ---------------------------------------------------------------- choice
enum {
    LO_SPINE = 0, LO_SPLIT, LO_RING, LO_SINK, LO_SHELL, LO_CELLS, LO_MASS, LO_FIELD,
    LO_PARTITIONS, LO_ENFILADE, LO_CLOISTER, LO_CONCOURSE, LO_COUNT
};

inline void lgGrow(LgBuild& b, const LgNode& n) {
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1;
    if (W < 1 || H < 1) return;
    b.nodes++;
    const unsigned char* g = n.g.g;
    int A = W < H ? W : H, B = W < H ? H : W, grain = lgGrain(n);
    bool rooms = !b.vault || n.low;

    // In delirious pockets, regions spontaneously curl into 3..4x nested Matryoshka shells or spirals!
    if (n.depth <= 2 && A >= 11 && (!b.vault || n.low)) {
        int mwx = b.s->rect.x0 + 1 + (n.x0 + n.x1) / 2;
        int mwz = b.s->rect.z0 + 1 + (n.z0 + n.z1) / 2;
        int del = lmDeliriumCell255(mwx, mwz);
        if (del >= 88 && lgR(n, 2u, 256) < del * 3 / 4 + 24) {
            if (A >= 12 && (((b.s->rect.j + b.s->rect.i) & 1) == 0)) { lgNestedMatryoshka(b, n); return; }
            lgSpiralCorridor(b, n);
            return;
        }
    }
    int w[LO_COUNT] = {};
    if (n.depth < 8 && b.nodes < 550) {
        if (A >= 9 && B >= 12 && n.depth <= 2)
            w[LO_SPINE] = 16 + g[LG_SPAN] / 3 + g[LG_ORDER] / 5 + (!b.vault ? 28 : 0);
        if (A >= 3 && B >= 7)
            w[LO_SPLIT] = B > 2 * grain ? 240 : (B > grain ? 95 : 18);
        if (A >= 9)
            w[LO_RING] = 14 + g[LG_ORDER] / 5 + (rooms ? lgBias(g[LG_CELLS]) / 4 + 14 : 0);
        if (b.vault && !n.low && A >= 5)
            w[LO_SINK] = g[LG_SINK] / 2 + lgBias(g[LG_SINK]);
        if (!n.low && A >= 6 && (n.depth > 0 || b.vault))
            w[LO_SHELL] = b.vault ? (g[LG_SHELL] / 2 + lgBias(g[LG_SHELL])) : (14 + g[LG_SHELL] / 4 + lgBias(g[LG_MASS]) / 5);
        if (rooms && A >= 3 && B >= 5)
            w[LO_CELLS] = 24 + lgBias(g[LG_CELLS]) + (!b.vault ? g[LG_CELLS] / 2 : 0);
        if (n.depth > 0 && n.walled != 15u && A >= 2 && B <= 8)
            w[LO_MASS] = lgBias(g[LG_MASS]) / 3;

        // Continuous operator-family affinities steer the topology (no per-district kind is drawn).
        if (!b.vault && A >= 6 && B >= 7) {
            int aP = lgFamAff(b, n, LM_WING_PARTITIONS), aC = lgFamAff(b, n, LM_WING_CORRIDORS);
            int aE = lgFamAff(b, n, LM_WING_ENFILADE),   aK = lgFamAff(b, n, LM_WING_CONCOURSE);
            int aS = lgFamAff(b, n, LM_WING_SERVICE);
            w[LO_PARTITIONS] += lgFamW(aP, n.depth <= 1 ? 260 : 190) + lgFamW(aS, 45);
            w[LO_CELLS]      += lgFamW(aC, 95) + lgFamW(aS, 70);
            w[LO_SPINE]      += lgFamW(aC, 55) + lgFamW(aK, 60) + lgFamW(aS, 80);
            if (B >= 8) w[LO_ENFILADE] += lgFamW(aE, 210);
            if (A >= 9 && B >= 9) {
                w[LO_CLOISTER]  += lgFamW(aC, 85) + lgFamW(aE, 95) + lgFamW(aK, 55);
                w[LO_CONCOURSE] += lgFamW(aK, 220);
            }
            w[LO_SPLIT] = w[LO_SPLIT] * (512 - aP - aE) / 512;
        }
    }
    // Keep bare open fields modest unless lattice/openness explicitly ask for an expanse.
    w[LO_FIELD] = 8 + lgBias(g[LG_LATTICE]) / 6 + lgBias(255 - g[LG_MASS]) / 6;
    if (!b.vault) w[LO_FIELD] = (w[LO_FIELD] * 2) / 5 + 4;
    if (n.depth == 0) w[LO_FIELD] /= 4;

    int total = 0;
    for (int k = 0; k < LO_COUNT; k++) total += w[k];
    int roll = lgR(n, 1u, total), op = 0;
    while (op < LO_COUNT - 1 && roll >= w[op]) roll -= w[op++];
    if (op == LO_SPINE)           lgSpine(b, n);
    else if (op == LO_SPLIT)      lgSplit(b, n);
    else if (op == LO_RING)       lgRing(b, n);
    else if (op == LO_SINK)       lgSink(b, n);
    else if (op == LO_SHELL)      lgShell(b, n);
    else if (op == LO_CELLS)      lgCells(b, n);
    else if (op == LO_MASS)       lgMass(b, n);
    else if (op == LO_PARTITIONS) lgPartitionLabyrinth(b, n);
    else if (op == LO_ENFILADE)   lgEnfilade(b, n);
    else if (op == LO_CLOISTER)   lgCloisterCore(b, n);
    else if (op == LO_CONCOURSE)  lgSkylightConcourse(b, n);
    else                          lgField(b, n);
}

// ---------------------------------------------------------------- megastructure root operators
// MF_ZIGGURAT: (0) slab-wide ceremonial staircase rising into a sheer blank wall,
//              (1) towering stepped ziggurat pyramid in a moated court, or
//              (2/3) sunken multi-tier terraced amphitheatre cascade.
inline void lgMegaZiggurat(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1, A = W < H ? W : H;
    b.nodes++;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_HALL);
    if (A < 9) return;

    int subMode = b.s->megaSub & 3;
    if (subMode == 0) {
        // Grand ceremonial staircase spanning full width & climbing steeply right into the vault roof!
        int axis = (b.s->megaSub >> 2) & 1;
        bool posEnd = ((b.s->megaSub >> 3) & 1) != 0;
        int L = axis == 0 ? H : W, C = axis == 0 ? W : H;
        int a0 = axis == 0 ? n.z0 : n.x0, a1 = axis == 0 ? n.z1 : n.x1;
        int c0 = axis == 0 ? n.x0 : n.z0, c1 = axis == 0 ? n.x1 : n.z1;
        int marginC = C >= 16 ? 2 : 1;
        int stairLen = lmClampInt(L * 38 / 100, 6, 10); // steep ~40 deg flight leaving wide forecourt plaza!
        if (stairLen > L - 6) stairLen = L - 6;
        if (stairLen < 4) stairLen = 4;
        if (!posEnd) {
            int wallA = a0 + 1, sA0 = wallA + 1, sA1 = wallA + stairLen;
            if (axis == 0) {
                lgFill(b, c0 + marginC, wallA, c1 - marginC, wallA, LT_WALL);
                lgFill(b, c0 + marginC, sA0, c1 - marginC, sA1, LT_BASIN);
            } else {
                lgFill(b, wallA, c0 + marginC, wallA, c1 - marginC, LT_WALL);
                lgFill(b, sA0, c0 + marginC, sA1, c1 - marginC, LT_BASIN);
            }
        } else {
            int wallA = a1 - 1, sA1 = wallA - 1, sA0 = wallA - stairLen;
            if (axis == 0) {
                lgFill(b, c0 + marginC, wallA, c1 - marginC, wallA, LT_WALL);
                lgFill(b, c0 + marginC, sA0, c1 - marginC, sA1, LT_BASIN);
            } else {
                lgFill(b, wallA, c0 + marginC, wallA, c1 - marginC, LT_WALL);
                lgFill(b, sA0, c0 + marginC, sA1, c1 - marginC, LT_BASIN);
            }
        }
        return;
    }

    if (subMode == 1) {
        // Rising stepped ziggurat: compact 9..13-cell steep pyramid footprint surrounded by wide viewing plaza!
        int maxP = lmClampInt(9 + b.s->ceilTiers / 3, 9, 13);
        int pW = (W - 6 < maxP) ? (W - 6) : maxP;
        int pH = (H - 6 < maxP) ? (H - 6) : maxP;
        if (pW < 5 || pH < 5) return;
        int px0 = n.x0 + (W - pW) / 2 + b.s->megaOffX, pz0 = n.z0 + (H - pH) / 2 + b.s->megaOffZ;
        px0 = lmClampInt(px0, n.x0 + 1, n.x1 - pW);
        pz0 = lmClampInt(pz0, n.z0 + 1, n.z1 - pH);
        int px1 = px0 + pW - 1, pz1 = pz0 + pH - 1;
        lgFill(b, px0, pz0, px1, pz1, LT_BASIN);
        return;
    }

    // Sunken terraced amphitheatre (subMode >= 2): steep concentric pit + 2-cell cantilevered overlook!
    int maxB = lmClampInt(11 + (int)g[LG_GRAIN] / 64, 11, 16);
    int bW = (W - 6 < maxB) ? (W - 6) : maxB;
    int bH = (H - 6 < maxB) ? (H - 6) : maxB;
    if (bW < 6 || bH < 6) return;
    int px0 = n.x0 + (W - bW) / 2, px1 = px0 + bW - 1;
    int pz0 = n.z0 + (H - bH) / 2, pz1 = pz0 + bH - 1;
    lgFill(b, px0, pz0, px1, pz1, LT_BASIN);
    int mx = (px0 + px1) / 2, mz = (pz0 + pz1) / 2;
    if ((b.s->megaSub >> 2) & 1) lgDeckA(b, 0, px0, px0 + 1, mz);
    else                         lgDeckA(b, 1, pz0, pz0 + 1, mx);
}

// MF_CANYON: narrow deep vertical gorge (3..6 cells wide) flanked by cliffside galleries,
// rooms carved behind sheer cliff walls, and multi-level catwalk bridges.
inline void lgMegaCanyon(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1;
    b.nodes++;
    int axis = (b.s->megaSub & 1);
    if ((axis == 0 ? H : W) < 9) axis ^= 1;
    int L = axis == 0 ? W : H, C = axis == 0 ? H : W;
    int a0 = axis == 0 ? n.x0 : n.z0, a1 = axis == 0 ? n.x1 : n.z1;
    int c0 = axis == 0 ? n.z0 : n.x0, c1 = axis == 0 ? n.z1 : n.x1;
    if (C < 7 || L < 7) { lgSink(b, n); return; }

    int slotW = (g[LG_GRAIN] > 145 && C >= 11) ? 3 : 2; // tight 2..3 cell chasm -> 20..25m cliff-to-cliff gorge!
    if (slotW > C - 4) slotW = C - 4;
    if (slotW < 2) slotW = 2;
    int sLo = c0 + (C - slotW) / 2, sHi = sLo + slotW - 1;

    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_HALL);
    // Flanking cliff blocks behind 1-cell rim galleries (sLo - 1 and sHi + 1)
    int wLo = sLo - 2, wHi = sHi + 2;
    if (wLo >= c0 + 2) {
        if (axis == 0) {
            LgNode fl = lgChild(n, a0 + 1, c0 + 1, a1 - 1, wLo - 1, 21u, 15u, 1);
            fl.low = true; fl.mezz = (g[LG_HEIGHT] > 175);
            lgFill(b, a0 + 1, wLo, a1 - 1, wLo, LT_WALL);
            lgCells(b, fl);
            for (int x = a0 + 1; x <= a1 - 1; x++) for (int z = c0 + 1; z <= wLo; z++)
                if (b.s->tag[x][z] == LT_WALL) b.s->flag[x][z] |= LF_LOW_WALL;
        } else {
            LgNode fl = lgChild(n, c0 + 1, a0 + 1, wLo - 1, a1 - 1, 21u, 15u, 1);
            fl.low = true; fl.mezz = (g[LG_HEIGHT] > 175);
            lgFill(b, wLo, a0 + 1, wLo, a1 - 1, LT_WALL);
            lgCells(b, fl);
            for (int x = c0 + 1; x <= wLo; x++) for (int z = a0 + 1; z <= a1 - 1; z++)
                if (b.s->tag[x][z] == LT_WALL) b.s->flag[x][z] |= LF_LOW_WALL;
        }
        int pStep = 4 + lgR(n, 201u, 3);
        for (int a = a0 + 2; a <= a1 - 2; a += pStep) lgFillA(b, axis, a, wLo, a, wLo, LT_LOW);
    } else if (wLo >= c0 + 1) {
        lgFillA(b, axis, a0 + 1, wLo, a1 - 1, wLo, LT_WALL);
        for (int a = a0 + 3; a <= a1 - 3; a += 5) lgFillA(b, axis, a, wLo, a, wLo, LT_CORR);
    }
    if (wHi <= c1 - 2) {
        if (axis == 0) {
            LgNode fr = lgChild(n, a0 + 1, wHi + 1, a1 - 1, c1 - 1, 22u, 15u, 1);
            fr.low = true; fr.mezz = (g[LG_HEIGHT] > 175);
            lgFill(b, a0 + 1, wHi, a1 - 1, wHi, LT_WALL);
            lgCells(b, fr);
            for (int x = a0 + 1; x <= a1 - 1; x++) for (int z = wHi; z <= c1 - 1; z++)
                if (b.s->tag[x][z] == LT_WALL) b.s->flag[x][z] |= LF_LOW_WALL;
        } else {
            LgNode fr = lgChild(n, wHi + 1, a0 + 1, c1 - 1, a1 - 1, 22u, 15u, 1);
            fr.low = true; fr.mezz = (g[LG_HEIGHT] > 175);
            lgFill(b, wHi, a0 + 1, wHi, a1 - 1, LT_WALL);
            lgCells(b, fr);
            for (int x = wHi; x <= c1 - 1; x++) for (int z = a0 + 1; z <= a1 - 1; z++)
                if (b.s->tag[x][z] == LT_WALL) b.s->flag[x][z] |= LF_LOW_WALL;
        }
        int pStep = 4 + lgR(n, 202u, 3);
        for (int a = a0 + 2; a <= a1 - 2; a += pStep) lgFillA(b, axis, a, wHi, a, wHi, LT_LOW);
    } else if (wHi <= c1 - 1) {
        lgFillA(b, axis, a0 + 1, wHi, a1 - 1, wHi, LT_WALL);
        for (int a = a0 + 3; a <= a1 - 3; a += 5) lgFillA(b, axis, a, wHi, a, wHi, LT_CORR);
    }

    // Note: strip LF_LOW_WALL right on wLo / wHi so the immediate canyon cliff rises full height to ceilY!
    if (wLo >= c0 + 1) {
        for (int a = a0 + 1; a <= a1 - 1; a++) {
            int wx = axis == 0 ? a : wLo, wz = axis == 0 ? wLo : a;
            if (b.s->tag[wx][wz] == LT_WALL) b.s->flag[wx][wz] &= ~(unsigned short)LF_LOW_WALL;
        }
    }
    if (wHi <= c1 - 1) {
        for (int a = a0 + 1; a <= a1 - 1; a++) {
            int wx = axis == 0 ? a : wHi, wz = axis == 0 ? wHi : a;
            if (b.s->tag[wx][wz] == LT_WALL) b.s->flag[wx][wz] &= ~(unsigned short)LF_LOW_WALL;
        }
    }

    // Carve the narrow canyon chasm & y=0 cliffside galleries
    lgFillA(b, axis, a0 + 1, sLo - 1, a1 - 1, sLo - 1, LT_HALL);
    lgFillA(b, axis, a0 + 1, sHi + 1, a1 - 1, sHi + 1, LT_HALL);
    lgFillA(b, axis, a0 + 1, sLo, a1 - 1, sHi, LT_VOID);

    // 2..5 narrow catwalk bridges across the gorge
    int nBridges = 2 + (int)g[LG_SPAN] * 3 / 256;
    for (int q = 0; q < nBridges; q++) {
        int ba = a0 + 2 + (L - 5) * (q + 1) / (nBridges + 1);
        u32 bh = lmHash(n.h, (u32)q, 9u, 0x434Eu);
        if (q > 0 && (int)(bh % 256u) < (int)g[LG_DECAY] * 2 / 3 && slotW >= 3) {
            int half = slotW / 2;
            if (bh & 1u) lgDeckA(b, axis ^ 1, sLo, sLo + half - 1, ba);
            else         lgDeckA(b, axis ^ 1, sHi - half + 1, sHi, ba);
        } else {
            lgDeckA(b, axis ^ 1, sLo, sHi, ba);
        }
    }
}

// MF_HYPOSTYLE: vast unbroken pillared hall (custom full/broken/stalactite shafts built in 3D).
inline void lgMegaHypostyle(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1, A = W < H ? W : H;
    b.nodes++;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_HALL);
    if (A >= 12 && g[LG_ORDER] >= 90) {
        lgChamferCorners(b, n.x0 + 1, n.z0 + 1, n.x1 - 1, n.z1 - 1, A >= 18 ? 3 : 2);
    }
    // Optional shallow ritual impluvium in the middle of the column forest
    if (A >= 15 && g[LG_SINK] >= 165) {
        int mx = (n.x0 + n.x1) / 2, mz = (n.z0 + n.z1) / 2;
        lgFill(b, mx - 2, mz - 2, mx + 2, mz + 2, LT_BASIN);
    }
}

// MF_SILO: enclosed vertical octagonal chimney/tube (compact 11..15 cell drum diameter so height > width!).
inline void lgMegaSilo(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    LimenSlab& s = *b.s;
    b.nodes++;
    lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_WALL);
    if (s.w < 11 || s.h < 11) { lgSink(b, n); return; }

    // Outer 1-cell perimeter ring for seam ports + 4 cardinal radial access tunnels!
    lgFill(b, 0, 0, s.w - 1, 0, LT_CORR);
    lgFill(b, 0, s.h - 1, s.w - 1, s.h - 1, LT_CORR);
    lgFill(b, 0, 0, 0, s.h - 1, LT_CORR);
    lgFill(b, s.w - 1, 0, s.w - 1, s.h - 1, LT_CORR);

    int maxD = lmClampInt(11 + (int)g[LG_GRAIN] / 64, 11, 15);
    int dX = (s.w - 4 < maxD) ? (s.w - 4) : maxD;
    int dZ = (s.h - 4 < maxD) ? (s.h - 4) : maxD;
    int D  = dX < dZ ? dX : dZ;
    int sx0 = (s.w - D) / 2 + s.megaOffX, sz0 = (s.h - D) / 2 + s.megaOffZ, sx1 = sx0 + D - 1, sz1 = sz0 + D - 1;
    int mx = (sx0 + sx1) / 2, mz = (sz0 + sz1) / 2;

    // Carve octagonal interior of the vertical tube
    int cut = D / 3;
    for (int x = sx0; x <= sx1; x++) {
        for (int z = sz0; z <= sz1; z++) {
            int dx = (x - sx0 < sx1 - x) ? (x - sx0) : (sx1 - x);
            int dz = (z - sz0 < sz1 - z) ? (z - sz0) : (sz1 - z);
            if (dx + dz >= cut) lgSet(b, x, z, LT_HALL);
        }
    }
    // 4 cardinal tunnel passages from perimeter ring into the silo's y=0 ring catwalk
    lgFill(b, 0, mz, sx0, mz, LT_CORR);
    lgFill(b, sx1, mz, s.w - 1, mz, LT_CORR);
    lgFill(b, mx, 0, mx, sz0, LT_CORR);
    lgFill(b, mx, sz1, mx, s.h - 1, LT_CORR);

    // Deep octagonal central shaft (leaving a 2-cell ring catwalk around the tube wall!)
    int cx0 = sx0 + 2, cz0 = sz0 + 2, cx1 = sx1 - 2, cz1 = sz1 - 2;
    if (cx1 - cx0 >= 3 && cz1 - cz0 >= 3) {
        int cCut = (cx1 - cx0 + 1) / 3;
        for (int x = cx0; x <= cx1; x++) {
            for (int z = cz0; z <= cz1; z++) {
                int dx = (x - cx0 < cx1 - x) ? (x - cx0) : (cx1 - x);
                int dz = (z - cz0 < cz1 - z) ? (z - cz0) : (cz1 - z);
                if (dx + dz >= cCut) lgSet(b, x, z, LT_VOID);
            }
        }
        if (g[LG_SPAN] >= 120) {
            if (s.megaSub & 1) lgDeckA(b, 0, cx0, cx1, mz);
            else               lgDeckA(b, 1, cz0, cz1, mx);
        } else {
            lgDeckA(b, 0, cx0, mx, mz);
        }
    }
}

// MF_HANGING: wide open hall floor beneath crushing 2.65 m inverted pyramid & floating blocks.
inline void lgMegaHanging(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1, A = W < H ? W : H;
    b.nodes++;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_HALL);
    if (A >= 15 && g[LG_CELLS] > 165) {
        // Low perimeter room cloister framing the central open hall
        int roomW = 2 + (g[LG_GRAIN] > 150 ? 1 : 0);
        lgFill(b, n.x0 + 1, n.z0 + 1, n.x1 - 1, n.z0 + 2, LT_WALL);
        lgRoomRow(b, n, 0, n.x0 + 2, n.x1 - 2, n.z0 + 1, n.z0 + 1, n.z0 + 2, roomW, LT_LOW);
        for (int x = n.x0 + 1; x <= n.x1 - 1; x++)
            for (int z = n.z0 + 1; z <= n.z0 + 2; z++)
                if (b.s->tag[x][z] == LT_WALL) b.s->flag[x][z] |= LF_LOW_WALL;
    }
}

// MF_CELLBLOCK: compact enclosed courtyard (11..17 cells across) surrounded by 7..12 storey apartment walls!
inline void lgMegaCellblock(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    LimenSlab& s = *b.s;
    b.nodes++;
    lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_CORR);
    if (s.w < 13 || s.h < 13) { lgFill(b, 0, 0, s.w - 1, s.h - 1, LT_HALL); return; }

    int maxC = lmClampInt(12 + (int)g[LG_GRAIN] / 48, 12, 18);
    int cW = (s.w - 4 < maxC) ? (s.w - 4) : maxC;
    int cH = (s.h - 4 < maxC) ? (s.h - 4) : maxC;
    int wx0 = (s.w - cW) / 2 - 1 + s.megaOffX, wx1 = wx0 + cW + 1;
    int wz0 = (s.h - cH) / 2 - 1 + s.megaOffZ, wz1 = wz0 + cH + 1;

    // Solid perimeter block from 1..s.w-2, hollowed out inside [wx0+1..wx1-1] x [wz0+1..wz1-1]
    lgFill(b, 1, 1, s.w - 2, s.h - 2, LT_WALL);
    lgFill(b, wx0 + 1, wz0 + 1, wx1 - 1, wz1 - 1, LT_HALL);

    // 4 cardinal entrance passages from outer perimeter into the apartment courtyard
    int mx = (wx0 + wx1) / 2, mz = (wz0 + wz1) / 2;
    lgFill(b, 0, mz, wx0, mz, LT_CORR);
    lgFill(b, wx1, mz, s.w - 1, mz, LT_CORR);
    lgFill(b, mx, 0, mx, wz0, LT_CORR);
    lgFill(b, mx, wz1, mx, s.h - 1, LT_CORR);

    int cx0 = wx0 + 4, cz0 = wz0 + 4, cx1 = wx1 - 4, cz1 = wz1 - 4;
    if (cx1 - cx0 >= 4 && cz1 - cz0 >= 4 && g[LG_SINK] >= 135) {
        int pitTag = (g[LG_ABYSS] > 165) ? LT_VOID : LT_BASIN;
        lgFill(b, cx0, cz0, cx1, cz1, pitTag);
        lgDeckA(b, 0, cx0, cx1, mz);
    }
}

// MF_PLATFORM: aimless transit concourse — wide platforms flanking 1 or 2 straight sunken
// trackway/canal trenches that dead-end abruptly into solid bulkhead walls at both ends.
inline void lgMegaPlatform(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1;
    b.nodes++;
    int axis = (b.s->megaSub & 1);
    if ((axis == 0 ? H : W) < 9) axis ^= 1;
    int L = axis == 0 ? W : H, C = axis == 0 ? H : W;
    int a0 = axis == 0 ? n.x0 : n.z0, a1 = axis == 0 ? n.x1 : n.z1;
    int c0 = axis == 0 ? n.z0 : n.x0;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_HALL);
    if (L < 8 || C < 8) return;

    bool twin = ((b.s->megaSub >> 1) & 1) != 0 && C >= 15;
    int bridgeA1 = a0 + L / 2;
    int bridgeA2 = (L >= 18 && g[LG_SPAN] > 140) ? (a0 + L / 3) : -1;

    auto cutTrackTrench = [&](int tLo, int tHi) {
        // Solid tunnel bulkhead walls at a0+1 and a1-1 ONLY across the track width;
        // platforms on either side stay open and loop around a0 / a1!
        lgFillA(b, axis, a0 + 1, tLo, a0 + 1, tHi, LT_WALL);
        lgFillA(b, axis, a1 - 1, tLo, a1 - 1, tHi, LT_WALL);
        lgFillA(b, axis, a0 + 2, tLo, a1 - 2, tHi, LT_BASIN);
        lgDeckA(b, axis ^ 1, tLo, tHi, bridgeA1);
        if (bridgeA2 > a0 + 2) lgDeckA(b, axis ^ 1, tLo, tHi, bridgeA2);
    };

    if (twin) {
        int islandW = lmClampInt(3 + (int)g[LG_MASS] / 90, 3, 5);
        int mid = c0 + C / 2;
        int iLo = mid - islandW / 2, iHi = iLo + islandW - 1;
        if (iLo - 2 >= c0 + 2 && iHi + 2 <= c0 + C - 3) {
            cutTrackTrench(iLo - 2, iLo - 1);
            cutTrackTrench(iHi + 1, iHi + 2);
            return;
        }
    }
    int trenchW = (C >= 12 && g[LG_ABYSS] > 110) ? 3 : 2;
    int tLo = c0 + (C - trenchW) / 2;
    cutTrackTrench(tLo, tLo + trenchW - 1);
}
