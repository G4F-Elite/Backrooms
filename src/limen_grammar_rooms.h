#pragma once
// Part of limen_grammar.h: shells, cells and the room-scale figures.

// ---------------------------------------------------------------- shell
inline void lgShell(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int W = n.x1 - n.x0 + 1, H = n.z1 - n.z0 + 1, A = W < H ? W : H;
    int m = lgR(n, 80u, b.vault ? 3 : 2);
    while (m > 0 && A - 2 * m < 4) m--;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, lgFloorTag(n));
    int x0 = n.x0 + m, z0 = n.z0 + m, x1 = n.x1 - m, z1 = n.z1 - m;
    lgFill(b, x0, z0, x1, z1, LT_WALL);
    // In a vault a shell can be 4.5 m low, an intermediate 9.0 m two-storey mezzanine pavilion,
    // or full-height to the vault roof.
    bool tall = b.vault && lgR(n, 81u, 256) < g[LG_MASS] / 4;
    bool makeMezz = b.vault && !tall && !n.low && b.s->ceilTiers >= 4 && lgR(n, 83u, 100) < 38;
    int doorTag = b.vault ? LT_LOW : LT_CORR;
    int doors = 1 + lgR(n, 82u, 3);
    for (int d = 0; d < doors; d++) {
        u32 dh = lmHash(n.h, (u32)d, 8u, 0x444Fu);
        for (int t = 0; t < 4; t++) {
            int side = (int)((dh + (u32)t) % 4u);
            if (m == 0 && (n.walled & (1u << side))) continue;
            int x = x0 + 1 + (int)((dh >> 4) % (u32)(x1 - x0 - 1)), z = z0 + 1 + (int)((dh >> 12) % (u32)(z1 - z0 - 1));
            if (side == 0) x = x0; else if (side == 1) x = x1; else if (side == 2) z = z0; else z = z1;
            lgSet(b, x, z, doorTag);
            break;
        }
    }
    LgNode c = lgChild(n, x0 + 1, z0 + 1, x1 - 1, z1 - 1, 4u, 15u, 4);
    if (b.vault) { c.low = true; c.mezz = makeMezz; }
    lgGrow(b, c);
    if (!b.vault || tall) return;
    unsigned short shFlag = LF_LOW_WALL | (makeMezz ? LF_MEZZ : 0u);
    for (int x = x0; x <= x1; x++) {
        for (int z = z0; z <= z1; z++) {
            if (b.s->tag[x][z] == LT_WALL) b.s->flag[x][z] |= shFlag;
            else if (makeMezz && lmIsLowShellTag(b.s->tag[x][z])) b.s->flag[x][z] |= LF_MEZZ;
        }
    }
}

// ---------------------------------------------------------------- cells
inline void lgCells(LgBuild& b, const LgNode& n) {
    const unsigned char* g = n.g.g;
    int path = lgPathTag(n);
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);
    int x0 = n.x0 + ((n.walled & LG_W) ? 0 : 1), x1 = n.x1 - ((n.walled & LG_E) ? 0 : 1);
    int z0 = n.z0 + ((n.walled & LG_N) ? 0 : 1), z1 = n.z1 - ((n.walled & LG_S) ? 0 : 1);
    int W = x1 - x0 + 1, H = z1 - z0 + 1;
    if (W < 1 || H < 1) return;
    int axis = W >= H ? 0 : 1;
    if (lgR(n, 60u, 256) < (255 - g[LG_ORDER]) / 6) axis ^= 1;
    int L = axis == 0 ? W : H, D = axis == 0 ? H : W;
    int a0 = axis == 0 ? x0 : z0, a1 = a0 + L - 1, c0 = axis == 0 ? z0 : x0, c1 = c0 + D - 1;
    bool openA0 = !(n.walled & (axis == 0 ? LG_W : LG_N)), openA1 = !(n.walled & (axis == 0 ? LG_E : LG_S));
    bool openC0 = !(n.walled & (axis == 0 ? LG_N : LG_W)), openC1 = !(n.walled & (axis == 0 ? LG_S : LG_E));
    // Human-proportioned room footprints (5x5m, 5x10m, 10x10m) instead of 20x25m hangar barns!
    bool compactWing = !b.vault && (lgFamAff(b, n, LM_WING_PARTITIONS) + lgFamAff(b, n, LM_WING_CORRIDORS) +
                                    lgFamAff(b, n, LM_WING_SERVICE) >=
                                    lgFamAff(b, n, LM_WING_ENFILADE) + lgFamAff(b, n, LM_WING_CONCOURSE));
    int roomW = compactWing ? (1 + lgR(n, 61u, 2)) : (2 + (g[LG_GRAIN] > 180 ? 1 : 0));
    int roomD = compactWing ? (1 + (g[LG_GRAIN] > 140 ? 1 : 0)) : (2 + (g[LG_GRAIN] > 195 ? 1 : 0));

    auto corridor = [&](int c) {
        lgFillA(b, axis, openA0 ? a0 - 1 : a0, c, openA1 ? a1 + 1 : a1, c, path);
    };
    auto cross = [&](int a, int from, int to, bool mouths) {
        lgFillA(b, axis, a, (mouths && openC0) ? c0 - 1 : from, a, (mouths && openC1) ? c1 + 1 : to, path);
    };
    if (L < 5 || D < roomD + 2) {
        lgFillA(b, axis, a0, c0, a1, c1, LT_ROOM);
        lgStampRoomIdentity(b, axis, a0, c0, a1, c1, lmHash(n.h, 1u, 0u, 0x524Du));
        int index = b.doorCounter++;
        if (index == b.gapDoor) { b.gapX = axis == 0 ? a0 - 1 : c0 + D / 2; b.gapZ = axis == 0 ? c0 + D / 2 : a0 - 1; return; }
        if (openA0) lgFillA(b, axis, a0 - 1, c0 + D / 2, a0 - 1, c0 + D / 2, path);
        else if (openA1) lgFillA(b, axis, a1 + 1, c0 + D / 2, a1 + 1, c0 + D / 2, path);
        else if (openC0) lgFillA(b, axis, a0 + L / 2, c0 - 1, a0 + L / 2, c0 - 1, path);
        else if (openC1) lgFillA(b, axis, a0 + L / 2, c1 + 1, a0 + L / 2, c1 + 1, path);
        return;
    }
    bool ends = L >= 9;
    int unit = 2 * roomD + 3, bands = (D + 1) / (unit + 1);
    if (bands == 0) {
        bool flip = lgR(n, 63u, 2) != 0;
        int corr = flip ? c0 : c1;
        corridor(corr);
        if (ends) { cross(a0, c0, c1, true); cross(a1, c0, c1, true); }
        lgRoomRow(b, n, axis, ends ? a0 + 2 : a0, ends ? a1 - 2 : a1, flip ? c0 + 2 : c0, flip ? c1 : c1 - 2,
                  flip ? c0 + 1 : c1 - 1, roomW, path);
        return;
    }
    int rest = D - (bands * (unit + 1) - 1), top = rest / 2;
    int sections = (L - 1) / 14;
    if (sections < 1) sections = 1;
    int c = c0, firstCorr = -1, lastCorr = -1;
    for (int k = 0; k < bands; k++) {
        int nD = roomD + (k == 0 ? top : 0), sD = roomD + (k == bands - 1 ? rest - top : 0);
        int corr = c + nD + 1;
        corridor(corr);
        if (firstCorr < 0) firstCorr = corr;
        lastCorr = corr;
        for (int q = 0; q < sections; q++) {
            int q0 = a0 + (L - 1) * q / sections, q1 = a0 + (L - 1) * (q + 1) / sections;
            int r0 = (q > 0 || ends) ? q0 + 2 : q0, r1 = (q < sections - 1 || ends) ? q1 - 2 : q1;
            lgRoomRow(b, n, axis, r0, r1, c, c + nD - 1, corr - 1, roomW, path);
            lgRoomRow(b, n, axis, r0, r1, corr + 2, corr + 1 + sD, corr + 1, roomW, path);
        }
        c = corr + sD + 3;
    }
    if (ends) { cross(a0, c0, c1, true); cross(a1, c0, c1, true); }
    for (int q = 1; q < sections; q++) cross(a0 + (L - 1) * q / sections, firstCorr, lastCorr, false);
}

// ---------------------------------------------------------------- delirium structures
// Matryoshka: 3..4 concentric nested rooms-within-rooms with telescoping sightline doorways!
inline void lgNestedMatryoshka(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    int cx0 = n.x0 + 1, cz0 = n.z0 + 1, cx1 = n.x1 - 1, cz1 = n.z1 - 1;
    int path = lgPathTag(n);
    lgFill(b, n.x0, n.z0, n.x1, n.z1, path);
    int sightSide = (int)(n.h & 3u);
    int origX0 = cx0, origZ0 = cz0, origX1 = cx1, origZ1 = cz1;
    int mx = (cx0 + cx1) / 2, mz = (cz0 + cz1) / 2;
    int rings = 0;
    while (cx1 - cx0 >= 4 && cz1 - cz0 >= 4 && rings < 4) {
        lgFill(b, cx0, cz0, cx1, cz1, LT_WALL);
        lgFill(b, cx0 + 1, cz0 + 1, cx1 - 1, cz1 - 1, (rings & 1) ? LT_ROOM : path);
        lgStampRoomIdentity(b, 0, cx0 + 1, cz0 + 1, cx1 - 1, cz1 - 1, lmHash(n.h, (u32)rings, 3u, 0x4D54u));
        if (b.vault) {
            unsigned short wf = LF_LOW_WALL | (((rings & 1) == 0 && s.ceilTiers >= 3) ? LF_MEZZ : 0u);
            for (int x = cx0; x <= cx1; x++) for (int z = cz0; z <= cz1; z++)
                if (s.tag[x][z] == LT_WALL) s.flag[x][z] |= wf;
        }
        // Telescoping axial doorway + glowing inner lamp cell so all 3..4 nested shells are visible straight through!
        if (sightSide == 0)      lgSet(b, cx0, mz, path);
        else if (sightSide == 1) lgSet(b, cx1, mz, path);
        else if (sightSide == 2) lgSet(b, mx, cz0, path);
        else                     lgSet(b, mx, cz1, path);
        int alt = (sightSide + 1 + rings) & 3;
        if (alt == 0)      lgSet(b, cx0, mz, path);
        else if (alt == 1) lgSet(b, cx1, mz, path);
        else if (alt == 2) lgSet(b, mx, cz0, path);
        else               lgSet(b, mx, cz1, path);
        cx0 += 2; cz0 += 2; cx1 -= 2; cz1 -= 2;
        rings++;
    }
    if (rings >= 2) {
        s.deliriumStruct = (s.deliriumStruct & ~15) | (rings > 15 ? 15 : rings);
        s.nestedRings = (short)rings;
        s.nestedDir   = (short)sightSide; // 0=from -X(+X dir), 1=from +X(-X dir), 2=from -Z(+Z dir), 3=from +Z(-Z dir)
        s.nestedX     = (short)(sightSide == 0 ? (origX0 - 1) : (sightSide == 1 ? (origX1 + 1) : mx));
        s.nestedZ     = (short)(sightSide == 2 ? (origZ0 - 1) : (sightSide == 3 ? (origZ1 + 1) : mz));
    }
}

// Spiral corridor: open chamber housing a continuous polygonal/curved Nautilus spiral corridor
// whose width & ceiling height compress inward and whose lamp/door cadence accelerates to the core!
inline void lgSpiralCorridor(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    int path = lgPathTag(n);
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);
    if (b.vault) {
        for (int x = n.x0; x <= n.x1; x++) for (int z = n.z0; z <= n.z1; z++) s.flag[x][z] |= LF_LOW_WALL;
    }
    int x0 = n.x0 + 1, z0 = n.z0 + 1, x1 = n.x1 - 1, z1 = n.z1 - 1;
    int sw = x1 - x0 + 1, sh = z1 - z0 + 1;
    if (sw > 11) { x0 += (sw - 11) / 2; x1 = x0 + 10; sw = 11; }
    if (sh > 11) { z0 += (sh - 11) / 2; z1 = z0 + 10; sh = 11; }
    if (sw < 7 || sh < 7) { lgFill(b, n.x0, n.z0, n.x1, n.z1, path); return; }

    lgFill(b, x0, z0, x1, z1, LT_HALL);
    int mx = (x0 + x1) / 2, mz = (z0 + z1) / 2;
    lgFill(b, n.x0, mz, x0, mz, path);
    lgFill(b, x1, mz, n.x1, mz, path);
    lgFill(b, mx, n.z0, mx, z0, path);
    lgFill(b, mx, z1, mx, n.z1, path);

    int turns = 3;
    s.deliriumStruct = (s.deliriumStruct & 15) | (turns << 4);
    s.spiralX = (short)x0; s.spiralZ = (short)z0;
    s.spiralW = (short)sw; s.spiralH = (short)sh;
}

// ---------------------------------------------------------------- liminal human-scale operators
// 1. HobbyTown Segmented Partition Labyrinth: low 2.72 m ceiling, staggered 0.26 m drywall
//    partitions, partial baffle screens you walk around, open pass-throughs, and eerily bare
//    carpet/wallpaper bays (`LZ_SPARSE`) with long diagonal sightlines across 3..4 empty rooms.
inline void lgPartitionLabyrinth(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    int path = lgPathTag(n);
    int x0 = n.x0 + ((n.walled & LG_W) ? 0 : 1), x1 = n.x1 - ((n.walled & LG_E) ? 0 : 1);
    int z0 = n.z0 + ((n.walled & LG_N) ? 0 : 1), z1 = n.z1 - ((n.walled & LG_S) ? 0 : 1);
    int W = x1 - x0 + 1, H = z1 - z0 + 1;
    if (W < 5 || H < 5) { lgCells(b, n); return; }

    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);
    lgFill(b, x0, z0, x1, z1, LT_ROOM);
    if (!(n.walled & LG_W)) lgSet(b, n.x0, z0 + H / 2, path);
    if (!(n.walled & LG_E)) lgSet(b, n.x1, z0 + H / 2, path);
    if (!(n.walled & LG_N)) lgSet(b, x0 + W / 2, n.z0, path);
    if (!(n.walled & LG_S)) lgSet(b, x0 + W / 2, n.z1, path);

    for (int x = x0; x <= x1; x++) {
        for (int z = z0; z <= z1; z++) {
            s.flag[x][z] |= LF_CRAMPED;
            s.zone[x][z] |= LZ_SPARSE;
            s.nodeKey[x][z] = (unsigned short)(1u + (lmHash(n.h, (u32)(x / 2), (u32)(z / 2), 0x5042u) % 65534u));
            s.roomRel[x][z] = (unsigned char)(((x & 1) << 4) | (z & 1));
        }
    }

    // Irregular vertical 0.26 m drywall lines with open gaps (baffle ends) & framed pass-throughs
    int px = x0 + 1 + lgR(n, 301u, 2);
    int lineIdx = 0;
    while (px <= x1) {
        u32 lh = lmHash(n.h, (u32)lineIdx, 1u, 0x44574Cu);
        int gapMode = (int)(lh % 3u); // 0 = open walk-around gap at north end, 1 = south end, 2 = middle gap
        int gapZ = (gapMode == 0) ? z0 : ((gapMode == 1) ? z1 : (z0 + 1 + (int)((lh >> 4) % (u32)(H - 2))));
        int doorZ = z0 + (int)((lh >> 10) % (u32)H);
        for (int z = z0; z <= z1; z++) {
            if (z == gapZ) continue; // partial HobbyTown screen wall ending in open carpet!
            unsigned short bits = LF_THIN_W;
            if (z == doorZ || ((z - z0) % 4 == 2 && ((lh >> 16) & 1u))) bits |= LF_THIN_DOOR_W;
            s.flag[px][z] |= bits;
        }
        lineIdx++;
        px += 1 + (int)((lh >> 20) % 3u); // 1, 2, or 3 cells wide
    }

    // Irregular horizontal 0.26 m drywall cross-partitions (sparser so bays are oblong, not checkerboards)
    int pz = z0 + 2 + lgR(n, 302u, 2);
    int hIdx = 0;
    while (pz <= z1) {
        u32 lh = lmHash(n.h, (u32)hIdx, 2u, 0x445748u);
        int runStart = x0 + (int)(lh % 2u);
        int runEnd   = x1 - (int)((lh >> 3) % 2u);
        int doorX1   = x0 + (int)((lh >> 6) % (u32)W);
        int doorX2   = x0 + (int)((lh >> 12) % (u32)W);
        for (int x = runStart; x <= runEnd; x++) {
            if (((lh >> 18) & 1u) && x == (x0 + x1) / 2) continue; // open walk-through gap
            unsigned short bits = LF_THIN_N;
            if (x == doorX1 || x == doorX2) bits |= LF_THIN_DOOR_N;
            s.flag[x][pz] |= bits;
        }
        hIdx++;
        pz += 2 + (int)((lh >> 22) % 2u);
    }

    // Occasional solid 1x1 or 1x2 structural column/wall block anchoring the partition field
    if (W >= 8 && H >= 8) {
        int bx = x0 + 2 + lgR(n, 303u, W - 4);
        int bz = z0 + 2 + lgR(n, 304u, H - 4);
        lgSet(b, bx, bz, LT_WALL);
        if (lgR(n, 305u, 2) == 0 && bx + 1 < x1 - 1) lgSet(b, bx + 1, bz, LT_WALL);
    }
}

// 2. Room-to-Room Enfilade: chain of pass-through rooms WITHOUT a side corridor — either an
//    aligned axial telescope of 4..7 doorways in perspective, or a diagonal chicane sequence!
inline void lgEnfilade(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    int path = lgPathTag(n);
    int x0 = n.x0 + ((n.walled & LG_W) ? 0 : 1), x1 = n.x1 - ((n.walled & LG_E) ? 0 : 1);
    int z0 = n.z0 + ((n.walled & LG_N) ? 0 : 1), z1 = n.z1 - ((n.walled & LG_S) ? 0 : 1);
    int W = x1 - x0 + 1, H = z1 - z0 + 1;
    int axis = W >= H ? 0 : 1;
    int L = axis == 0 ? W : H, D = axis == 0 ? H : W;
    if (L < 7 || D < 3) { lgCells(b, n); return; }

    int a0 = axis == 0 ? x0 : z0, a1 = a0 + L - 1;
    int c0 = axis == 0 ? z0 : x0, c1 = c0 + D - 1;
    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);

    bool twinParallel = (D >= 7);
    int midWall = twinParallel ? (c0 + D / 2) : -1;
    bool axialTelescope = (lgR(n, 310u, 100) < 68); // 68% hypnotic aligned portal vista!
    int bayLen = 2 + lgR(n, 311u, 2); // 2..3 cells long per room
    u32 cloneKey = lmHash(n.h, 7u, 0u, 0x454E46u);
    bool cloneChain = (n.g.g[LG_ORDER] >= 115);

    auto buildTrack = [&](int tc0, int tc1, u32 salt) {
        int tD = tc1 - tc0 + 1;
        if (tD < 2) return;
        int doorC_A = tc0 + tD / 2;
        int doorC_B = (tD >= 3) ? (tc1 - (tD > 3 ? 1 : 0)) : tc0;
        int curA = a0, bayIdx = 0;
        while (curA <= a1) {
            int nextWall = curA + bayLen;
            if (nextWall >= a1 - 1) nextWall = a1 + 1;
            int rA1 = nextWall - 1;
            lgFillA(b, axis, curA, tc0, rA1, tc1, LT_ROOM);
            u32 rk = cloneChain ? lmHash(cloneKey, salt, 0u, 0x4552u) : lmHash(cloneKey, salt, (u32)bayIdx, 0x4552u);
            lgStampRoomIdentity(b, axis, curA, tc0, rA1, tc1, rk);
            for (int a = curA; a <= rA1; a++) {
                for (int c = tc0; c <= tc1; c++) {
                    int wx = axis == 0 ? a : c, wz = axis == 0 ? c : a;
                    if (!b.vault) s.zone[wx][wz] |= LZ_STD_CEIL; // 3.25 m gallery/vestibule ceiling
                }
            }
            if (nextWall <= a1) {
                int dc = axialTelescope ? doorC_A : ((bayIdx & 1) ? doorC_B : tc0);
                lgFillA(b, axis, nextWall, tc0, nextWall, tc1, LT_WALL);
                lgFillA(b, axis, nextWall, dc, nextWall, dc, path);
                if (!b.vault) {
                    int dwx = axis == 0 ? nextWall : dc, dwz = axis == 0 ? dc : nextWall;
                    s.zone[dwx][dwz] |= LZ_STD_CEIL;
                }
            }
            curA = nextWall + 1;
            bayIdx++;
        }
        if (!(n.walled & (axis == 0 ? LG_W : LG_N))) lgFillA(b, axis, n.x0 + (axis == 0 ? 0 : (doorC_A - n.x0)), axis == 0 ? doorC_A : n.z0, n.x0 + (axis == 0 ? 0 : (doorC_A - n.x0)), axis == 0 ? doorC_A : n.z0, path);
        if (!(n.walled & (axis == 0 ? LG_E : LG_S))) lgFillA(b, axis, axis == 0 ? n.x1 : doorC_A, axis == 0 ? doorC_A : n.z1, axis == 0 ? n.x1 : doorC_A, axis == 0 ? doorC_A : n.z1, path);
    };

    if (twinParallel) {
        buildTrack(c0, midWall - 1, 1u);
        buildTrack(midWall + 1, c1, 2u);
        // Connect the two parallel enfilades at ends so they form a mysterious loop around a solid core wall
        int openA_lo = a0 + 1, openA_hi = a1 - 1;
        if (s.tag[axis == 0 ? openA_lo : (midWall - 1)][axis == 0 ? (midWall - 1) : openA_lo] != LT_WALL &&
            s.tag[axis == 0 ? openA_lo : (midWall + 1)][axis == 0 ? (midWall + 1) : openA_lo] != LT_WALL) {
            lgFillA(b, axis, openA_lo, midWall, openA_lo, midWall, path);
        }
        if (openA_hi > openA_lo + 2 &&
            s.tag[axis == 0 ? openA_hi : (midWall - 1)][axis == 0 ? (midWall - 1) : openA_hi] != LT_WALL &&
            s.tag[axis == 0 ? openA_hi : (midWall + 1)][axis == 0 ? (midWall + 1) : openA_hi] != LT_WALL) {
            lgFillA(b, axis, openA_hi, midWall, openA_hi, midWall, path);
        }
    } else {
        buildTrack(c0, c1, 1u);
    }
    if (!(n.walled & LG_N)) lgSet(b, (x0 + x1) / 2, n.z0, path);
    if (!(n.walled & LG_S)) lgSet(b, (x0 + x1) / 2, n.z1, path);
    if (!(n.walled & LG_W)) lgSet(b, n.x0, (z0 + z1) / 2, path);
    if (!(n.walled & LG_E)) lgSet(b, n.x1, (z0 + z1) / 2, path);
}

// 3. Cloister Core: low 2.72 m perimeter corridor wrapping a solid central core (`5x5..7x7`)
//    that Stage-2 (`lmApplyLiminalExcisions`) hollows into a multi-windowed sealed inner courtyard
//    (`LT_VITRINE`) so you peer across a bright unreachable room to the opposite corridor!
inline void lgCloisterCore(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    int path = lgPathTag(n);
    int x0 = n.x0 + ((n.walled & LG_W) ? 0 : 1), x1 = n.x1 - ((n.walled & LG_E) ? 0 : 1);
    int z0 = n.z0 + ((n.walled & LG_N) ? 0 : 1), z1 = n.z1 - ((n.walled & LG_S) ? 0 : 1);
    int W = x1 - x0 + 1, H = z1 - z0 + 1;
    if (W < 9 || H < 9) { lgCells(b, n); return; }

    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);
    if (!(n.walled & LG_W)) lgSet(b, n.x0, z0 + H / 2, path);
    if (!(n.walled & LG_E)) lgSet(b, n.x1, z0 + H / 2, path);
    if (!(n.walled & LG_N)) lgSet(b, x0 + W / 2, n.z0, path);
    if (!(n.walled & LG_S)) lgSet(b, x0 + W / 2, n.z1, path);

    // Optional outer suite ring when wide enough
    int rx0 = x0, rx1 = x1, rz0 = z0, rz1 = z1;
    if (W >= 13) {
        lgRoomRow(b, n, 1, z0 + 1, z1 - 1, x0, x0 + 1, x0 + 2, 2, path);
        lgRoomRow(b, n, 1, z0 + 1, z1 - 1, x1 - 1, x1, x1 - 2, 2, path);
        rx0 = x0 + 2; rx1 = x1 - 2;
    }
    if (H >= 13) {
        lgRoomRow(b, n, 0, rx0 + 1, rx1 - 1, z0, z0 + 1, z0 + 2, 2, path);
        lgRoomRow(b, n, 0, rx0 + 1, rx1 - 1, z1 - 1, z1, z1 - 2, 2, path);
        rz0 = z0 + 2; rz1 = z1 - 2;
    }
    // Cloister loop corridor around central solid core [rx0+1..rx1-1] x [rz0+1..rz1-1]
    lgFill(b, rx0, rz0, rx1, rz0, path);
    lgFill(b, rx0, rz1, rx1, rz1, path);
    lgFill(b, rx0, rz0, rx0, rz1, path);
    lgFill(b, rx1, rz0, rx1, rz1, path);
    lgFill(b, rx0 + 1, rz0 + 1, rx1 - 1, rz1 - 1, LT_WALL);
    if (!b.vault) {
        for (int x = x0; x <= x1; x++) for (int z = z0; z <= z1; z++)
            if (lmIsWalkableTag(s.tag[x][z])) s.flag[x][z] |= LF_CRAMPED;
    }
}

// 4. Skylight Concourse: low 2.72 m / 3.25 m perimeter arcades stepping through a colonnade
//    into a 9.00 m double-height skylit central hall (`LZ_TALL_WELL`) inside a 1-storey slab!
inline void lgSkylightConcourse(LgBuild& b, const LgNode& n) {
    LimenSlab& s = *b.s;
    int path = lgPathTag(n);
    int x0 = n.x0 + ((n.walled & LG_W) ? 0 : 1), x1 = n.x1 - ((n.walled & LG_E) ? 0 : 1);
    int z0 = n.z0 + ((n.walled & LG_N) ? 0 : 1), z1 = n.z1 - ((n.walled & LG_S) ? 0 : 1);
    int W = x1 - x0 + 1, H = z1 - z0 + 1;
    if (b.vault || W < 8 || H < 8) { lgField(b, n); return; }

    lgFill(b, n.x0, n.z0, n.x1, n.z1, LT_WALL);
    lgFill(b, x0, z0, x1, z1, LT_CORR);
    if (!(n.walled & LG_W)) lgSet(b, n.x0, z0 + H / 2, path);
    if (!(n.walled & LG_E)) lgSet(b, n.x1, z0 + H / 2, path);
    if (!(n.walled & LG_N)) lgSet(b, x0 + W / 2, n.z0, path);
    if (!(n.walled & LG_S)) lgSet(b, x0 + W / 2, n.z1, path);

    // Low 2.72 m outer compression arcade
    for (int x = x0; x <= x1; x++) for (int z = z0; z <= z1; z++) s.flag[x][z] |= LF_CRAMPED;

    // Colonnade pier ring at offset 1, framing the 9.0 m double-height central well at offset 2+
    int wx0 = x0 + 2, wx1 = x1 - 2, wz0 = z0 + 2, wz1 = z1 - 2;
    if (wx1 - wx0 < 3 || wz1 - wz0 < 3) return;
    for (int x = wx0 - 1; x <= wx1 + 1; x++) {
        if ((x - x0) % 3 == 0) { lgSet(b, x, wz0 - 1, LT_WALL); lgSet(b, x, wz1 + 1, LT_WALL); }
    }
    for (int z = wz0; z <= wz1; z++) {
        if ((z - z0) % 3 == 0) { lgSet(b, wx0 - 1, z, LT_WALL); lgSet(b, wx1 + 1, z, LT_WALL); }
    }
    lgFill(b, wx0, wz0, wx1, wz1, LT_HALL);
    bool tallSky = !lmHasQuirk(gLimenDNA, LQ_SKY);
    for (int x = wx0; x <= wx1; x++) {
        for (int z = wz0; z <= wz1; z++) {
            s.flag[x][z] &= ~(unsigned short)LF_CRAMPED;
            if (tallSky) s.zone[x][z] |= LZ_TALL_WELL;
        }
    }
}
