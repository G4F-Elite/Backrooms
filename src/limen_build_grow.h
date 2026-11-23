#pragma once
// Part of limen_build.h: growing a slab, liminal excisions, measurement.

inline void lmGrowSlab(LimenSlab& s, LgBuild& b, const int* perSide) {
    memset(s.tag, LT_WALL, sizeof(s.tag));
    memset(s.flag, 0, sizeof(s.flag));
    memset(s.zone, 0, sizeof(s.zone));
    memset(s.nodeKey, 0, sizeof(s.nodeKey));
    memset(s.roomRel, 0, sizeof(s.roomRel));
    s.xeno = {};
    LgNode root;
    root.x0 = 0; root.z0 = 0; root.x1 = s.w - 1; root.z1 = s.h - 1;
    root.g = s.genome;
    root.h = lmHash((u32)s.rect.j, (u32)s.rect.i, 0u, 0x494Eu);
    root.depth = 0; root.walled = 15u; root.low = false; root.mezz = false;
    if (perSide[0] * 3 >= s.h) { lgFill(b, 0, 0, 0, s.h - 1, LT_HALL); root.x0++; root.walled &= ~(unsigned)LG_W; }
    if (perSide[1] * 3 >= s.h) { lgFill(b, s.w - 1, 0, s.w - 1, s.h - 1, LT_HALL); root.x1--; root.walled &= ~(unsigned)LG_E; }
    if (perSide[2] * 3 >= s.w) { lgFill(b, 0, 0, s.w - 1, 0, LT_HALL); root.z0++; root.walled &= ~(unsigned)LG_N; }
    if (perSide[3] * 3 >= s.w) { lgFill(b, 0, s.h - 1, s.w - 1, s.h - 1, LT_HALL); root.z1--; root.walled &= ~(unsigned)LG_S; }
    if (b.vault) lgGrowMegaRoot(b, root);
    else lgGrow(b, root);
    lgOpXenograft(b);
    lgOpBeatStamp(b);
}

// Stamps Stage-2 dead thresholds, intentional sealed vitrine rooms (`LT_VITRINE`),
// plugged numbered corridors (`LZ_PLUG_CORR`), ceiling stairs (`LS_CEIL_STAIR`),
// phase-shifted relics (`LS_PHASE_SHIFT`), and deja-vu corridor loops (`LZ_ECHO_CORR`).
inline void lmApplyLiminalExcisions(LimenSlab& s, const LimenPort* ports, int portCount, u32 h) {
    if (lmIsMegaUnstruct(s.megaForm)) return;
    if (s.xeno.active) {
        lmAddSite(s, LS_XENOGRAFT, (s.xeno.x0 + s.xeno.x1) / 2, (s.xeno.z0 + s.xeno.z1) / 2);
    }
    if (lmStrangeness(s.rect.j, s.rect.i) == 0) return;

    static const int dx[4] = {1, -1, 0, 0}, dz[4] = {0, 0, 1, -1};
    int dtScore = (int)gLimenDNA.deadThreshTendency + (int)s.genome.g[LG_DECAY] / 2;
    // A beat guarantees what is otherwise rolled, and looks for a spot beside the road first.
    const LmBeat* bt = lmBeatAt(s.rect.j, s.rect.i);
    const unsigned force = s.beatForce;

    // 0. Cloister Inner Courtyard Vitrine: if `lgCloisterCore` left a centered solid wall island
    //    framed by corridors, hollow it into a multi-windowed sealed observation tank (`LT_VITRINE`)
    //    so you can look straight across the unreachable lit room to the opposite corridor!
    if (!lmSlabIsVault(s) && s.w >= 11 && s.h >= 11) {
        int mx = s.w / 2, mz = s.h / 2;
        if (s.tag[mx][mz] == LT_WALL && (s.zone[mx][mz] & LZ_XENO) == 0) {
            int x0 = mx, x1 = mx, z0 = mz, z1 = mz;
            while (x0 > 2 && s.tag[x0 - 1][mz] == LT_WALL) x0--;
            while (x1 < s.w - 3 && s.tag[x1 + 1][mz] == LT_WALL) x1++;
            while (z0 > 2 && s.tag[mx][z0 - 1] == LT_WALL) z0--;
            while (z1 < s.h - 3 && s.tag[mx][z1 + 1] == LT_WALL) z1++;
            int bw = x1 - x0 + 1, bh = z1 - z0 + 1;
            if (bw >= 4 && bw <= 10 && bh >= 4 && bh <= 10) {
                bool pureBlock = true;
                for (int tx = x0; tx <= x1 && pureBlock; tx++)
                    for (int tz = z0; tz <= z1 && pureBlock; tz++)
                        if (s.tag[tx][tz] != LT_WALL || (s.flag[tx][tz] & LF_CHAMFER) != 0) pureBlock = false;
                if (pureBlock) {
                    unsigned short vKey = (unsigned short)(1u + (lmHash(h, 49u, 0u, 0x4356u) % 65534u));
                    for (int tx = x0 + 1; tx <= x1 - 1; tx++) {
                        for (int tz = z0 + 1; tz <= z1 - 1; tz++) {
                            s.tag[tx][tz] = LT_VITRINE;
                            s.flag[tx][tz] = 0u;
                            s.nodeKey[tx][tz] = vKey;
                            s.roomRel[tx][tz] = (unsigned char)(((tx - x0 - 1) << 4) | (tz - z0 - 1));
                        }
                    }
                    int winCand[4][4] = {
                        {x0, (z0 + z1) / 2, x0 - 1, (z0 + z1) / 2},
                        {x1, (z0 + z1) / 2, x1 + 1, (z0 + z1) / 2},
                        {(x0 + x1) / 2, z0, (x0 + x1) / 2, z0 - 1},
                        {(x0 + x1) / 2, z1, (x0 + x1) / 2, z1 + 1}
                    };
                    int winsAdded = 0;
                    for (int side = 0; side < 4; side++) {
                        int wx = winCand[side][0], wz = winCand[side][1];
                        int fx = winCand[side][2], fz = winCand[side][3];
                        int ft = lmTagLocal(s, fx, fz);
                        if ((ft != LT_CORR && ft != LT_HALL) || (s.flag[fx][fz] & (LF_THIN_W | LF_THIN_N)) != 0) continue;
                        if (s.flag[fx][fz] & (LF_NARROW_X | LF_NARROW_Z)) {
                            s.flag[fx][fz] &= ~(unsigned short)(LF_NARROW_X | LF_NARROW_Z);
                            if (s.narrowCount > 0) s.narrowCount--;
                        }
                        if (lmAddSite(s, LS_VITRINE_WIN, wx, wz)) { winsAdded++; s.deadThresholds++; }
                    }
                    if (winsAdded > 0) s.vitrineRooms++;
                    else {
                        for (int tx = x0 + 1; tx <= x1 - 1; tx++)
                            for (int tz = z0 + 1; tz <= z1 - 1; tz++) s.tag[tx][tz] = LT_WALL;
                    }
                }
            }
        }
    }

    // 1. Sealed illuminated vitrine room (`LT_VITRINE` behind `LS_VITRINE_WIN` window wall)
    u32 hv = lmHash(h, 51u, 0u, 0x5654u);
    int wantVit = ((force & LBF_VITRINE) != 0u || (int)(hv % 220u) < dtScore + (int)s.genome.g[LG_CELLS] / 3) ? 1 : 0;
    const int vitTries = (force & LBF_VITRINE) ? 160 : 36, vitNear = (force & LBF_VITRINE) ? 110 : 0;
    for (int attempt = 0; attempt < vitTries && s.vitrineRooms < wantVit; attempt++) {
        u32 ha = lmHash(hv, (u32)attempt, 3u, 0x5655u);
        int wx, wz;
        lmBeatPickCell(bt, s.rect, s.w, s.h, ha, attempt, vitNear, 2, 2, wx, wz);
        if (s.tag[wx][wz] != LT_WALL || (s.flag[wx][wz] & LF_CHAMFER) != 0) continue;
        int d = (int)((ha >> 14) & 3u);
        int fx = wx - dx[d], fz = wz - dz[d]; // corridor/hall cell in front of the window
        if (fx < 1 || fz < 1 || fx >= s.w - 1 || fz >= s.h - 1) continue;
        int ft = s.tag[fx][fz];
        if ((ft != LT_CORR && ft != LT_HALL && ft != LT_LOW) ||
            (s.flag[fx][fz] & (LF_NARROW_X | LF_NARROW_Z | LF_THIN_W | LF_THIN_N)) != 0) continue;

        // Carve a 2x2 (or 2x1) sealed LT_VITRINE pocket strictly inside solid LT_WALL behind (wx, wz)
        int px = dx[d], pz = dz[d];
        int sx = -pz, sz = px;
        int vx0 = wx + px, vz0 = wz + pz;
        int vx1 = vx0 + px + sx, vz1 = vz0 + pz + sz;
        int lx0 = vx0 < vx1 ? vx0 : vx1, lx1 = vx0 > vx1 ? vx0 : vx1;
        int lz0 = vz0 < vz1 ? vz0 : vz1, lz1 = vz0 > vz1 ? vz0 : vz1;
        if (lx0 < 2 || lz0 < 2 || lx1 >= s.w - 2 || lz1 >= s.h - 2) continue;
        bool allSolid = true;
        for (int tx = lx0 - 1; tx <= lx1 + 1 && allSolid; tx++) {
            for (int tz = lz0 - 1; tz <= lz1 + 1 && allSolid; tz++) {
                if (s.tag[tx][tz] != LT_WALL) allSolid = false;
            }
        }
        if (!allSolid) continue;
        unsigned short vKey = (unsigned short)(1u + (ha % 65534u));
        unsigned short lowWallBits = (lmSlabIsVault(s) && (ft == LT_LOW || (s.flag[wx][wz] & LF_LOW_WALL))) ? LF_LOW_WALL : 0u;
        for (int tx = lx0 - 1; tx <= lx1 + 1; tx++) {
            for (int tz = lz0 - 1; tz <= lz1 + 1; tz++) {
                if (tx >= lx0 && tx <= lx1 && tz >= lz0 && tz <= lz1) {
                    s.tag[tx][tz] = LT_VITRINE;
                    s.flag[tx][tz] = 0u;
                    s.nodeKey[tx][tz] = vKey;
                    s.roomRel[tx][tz] = (unsigned char)(((tx - lx0) << 4) | (tz - lz0));
                } else if (lowWallBits != 0u && s.tag[tx][tz] == LT_WALL) {
                    s.flag[tx][tz] |= lowWallBits;
                }
            }
        }
        if (lmAddSite(s, LS_VITRINE_WIN, wx, wz)) {
            s.vitrineRooms++;
            s.deadThresholds++;
        }
    }
    // Fallback vitrine on thin-walled room slabs: seal one leaf LT_ROOM behind a window if connectivity stays 100% intact
    if (s.vitrineRooms < wantVit && portCount > 0) {
        static unsigned char rseen[LM_MAX][LM_MAX], checkMark[LM_MAX][LM_MAX];
        memset(rseen, 0, sizeof(rseen));
        int totalWalk = 0;
        for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) if (lmIsWalkableTag(s.tag[x][z])) totalWalk++;
        for (int sx = 2; sx < s.w - 2 && s.vitrineRooms < wantVit; sx++) {
            for (int sz = 2; sz < s.h - 2 && s.vitrineRooms < wantVit; sz++) {
                if (s.tag[sx][sz] != LT_ROOM || rseen[sx][sz]) continue;
                short rcells[16][2];
                int rcnt = 0, head = 0, exits = 0, exX = -1, exZ = -1;
                bool okRoom = true;
                rseen[sx][sz] = 1;
                rcells[rcnt][0] = (short)sx; rcells[rcnt][1] = (short)sz; rcnt++;
                while (head < rcnt) {
                    int cx = rcells[head][0], cz = rcells[head][1]; head++;
                    if (cx < 2 || cz < 2 || cx >= s.w - 2 || cz >= s.h - 2 || (s.flag[cx][cz] & (LF_THIN_W | LF_THIN_N))) okRoom = false;
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + dx[d], nz = cz + dz[d];
                        int nt = lmTagLocal(s, nx, nz);
                        if (nt == LT_ROOM) {
                            if (!rseen[nx][nz]) {
                                rseen[nx][nz] = 1;
                                if (rcnt < 12) { rcells[rcnt][0] = (short)nx; rcells[rcnt][1] = (short)nz; rcnt++; }
                                else okRoom = false;
                            }
                        } else if (lmIsWalkableTag(nt)) {
                            exits++; exX = nx; exZ = nz;
                        }
                    }
                }
                if (!okRoom || rcnt < 2 || rcnt > 8 || exits != 1) continue;
                // Verify exX, exZ is a 1-cell doorway in a wall so sealing it makes a clean window
                bool doorInWall = (lmTagLocal(s, exX - 1, exZ) == LT_WALL && lmTagLocal(s, exX + 1, exZ) == LT_WALL) ||
                                  (lmTagLocal(s, exX, exZ - 1) == LT_WALL && lmTagLocal(s, exX, exZ + 1) == LT_WALL);
                if (!doorInWall) continue;
                bool touchesPort = false;
                for (int k = 0; k < portCount; k++) if (ports[k].lx == exX && ports[k].lz == exZ) touchesPort = true;
                if (touchesPort) continue;

                unsigned char savedDoorT = s.tag[exX][exZ];
                unsigned short savedDoorF = s.flag[exX][exZ];
                s.tag[exX][exZ] = LT_WALL;
                s.flag[exX][exZ] = lmSlabIsVault(s) ? LF_LOW_WALL : 0u;
                for (int k = 0; k < rcnt; k++) s.tag[rcells[k][0]][rcells[k][1]] = LT_VITRINE;

                memset(checkMark, 0, sizeof(checkMark));
                int reached = lmFlood(s, ports[0].lx, ports[0].lz, checkMark, 1);
                bool allPortsOk = (reached == totalWalk - rcnt - 1);
                for (int k = 0; k < portCount && allPortsOk; k++) if (checkMark[ports[k].lx][ports[k].lz] != 1) allPortsOk = false;

                if (!allPortsOk) {
                    s.tag[exX][exZ] = savedDoorT; s.flag[exX][exZ] = savedDoorF;
                    for (int k = 0; k < rcnt; k++) s.tag[rcells[k][0]][rcells[k][1]] = LT_ROOM;
                    continue;
                }
                for (int k = 0; k < rcnt; k++) {
                    int rx = rcells[k][0], rz = rcells[k][1];
                    s.flag[rx][rz] = 0u;
                    if (rx + 1 < s.w) s.flag[rx + 1][rz] &= ~(unsigned short)(LF_THIN_W | LF_THIN_DOOR_W);
                    if (rz + 1 < s.h) s.flag[rx][rz + 1] &= ~(unsigned short)(LF_THIN_N | LF_THIN_DOOR_N);
                }
                if (lmAddSite(s, LS_VITRINE_WIN, exX, exZ)) {
                    s.vitrineRooms++;
                    s.deadThresholds++;
                }
            }
        }
    }

    // 2. Plugged numbered corridor (`LZ_PLUG_CORR` + `LS_PLUG_CORR`): corridor of doors ending at a plug
    u32 hp = lmHash(h, 53u, 0u, 0x504Cu);
    if ((force & LBF_PLUG) != 0u || (int)(hp % 230u) < dtScore + (int)s.genome.g[LG_ORDER] / 3) {
        bool placedPlug = false;
        // First look for an existing straight dead-end corridor spur of length 3..7
        for (int x = 2; x < s.w - 2 && !placedPlug; x++) {
            for (int z = 2; z < s.h - 2 && !placedPlug; z++) {
                int t = s.tag[x][z];
                if (t != LT_CORR && t != LT_LOW) continue;
                int wallNbs = 0, openDir = -1;
                for (int d = 0; d < 4; d++) {
                    int nt = lmTagLocal(s, x + dx[d], z + dz[d]);
                    if (nt == LT_WALL) wallNbs++;
                    else if (lmIsWalkableTag(nt)) openDir = d;
                }
                if (wallNbs != 3 || openDir < 0) continue;
                int run = 1;
                while (run < 6) {
                    int nx = x + dx[openDir] * run, nz = z + dz[openDir] * run;
                    if (nx < 1 || nz < 1 || nx >= s.w - 1 || nz >= s.h - 1) break;
                    int nt = s.tag[nx][nz];
                    if (nt != LT_CORR && nt != LT_LOW) break;
                    int sx = -dz[openDir], sz = dx[openDir];
                    if (lmTagLocal(s, nx + sx, nz + sz) != LT_WALL && lmTagLocal(s, nx - sx, nz - sz) != LT_WALL) break;
                    run++;
                }
                if (run < 3) continue;
                for (int r = 0; r < run; r++) s.zone[x + dx[openDir] * r][z + dz[openDir] * r] |= LZ_PLUG_CORR;
                if (lmAddSite(s, LS_PLUG_CORR, x, z)) { s.deadThresholds++; placedPlug = true; }
            }
        }
        // Or carve a 4-cell plugged hotel corridor spur into a solid wall block off a hall/corridor
        const int plugTries = (force & LBF_PLUG) ? 140 : 24;
        for (int att = 0; att < plugTries && !placedPlug && !lmSlabIsVault(s); att++) {
            u32 ha = lmHash(hp, (u32)att, 5u, 0x504Du);
            int mx, mz;
            lmBeatPickCell(bt, s.rect, s.w, s.h, ha, att, (force & LBF_PLUG) ? 90 : 0, 2, 2, mx, mz);
            if (s.tag[mx][mz] != LT_CORR && s.tag[mx][mz] != LT_HALL) continue;
            int d = (int)((ha >> 14) & 3u);
            int sx = -dz[d], sz = dx[d];
            bool clearBlock = true;
            for (int step = 1; step <= 5 && clearBlock; step++) {
                int cx = mx + dx[d] * step, cz = mz + dz[d] * step;
                if (cx < 2 || cz < 2 || cx >= s.w - 2 || cz >= s.h - 2) { clearBlock = false; break; }
                for (int side = -1; side <= 1; side++) {
                    if (s.tag[cx + sx * side][cz + sz * side] != LT_WALL) clearBlock = false;
                }
            }
            if (!clearBlock) continue;
            for (int step = 1; step <= 4; step++) {
                int cx = mx + dx[d] * step, cz = mz + dz[d] * step;
                s.tag[cx][cz] = LT_CORR;
                s.flag[cx][cz] = 0u;
                s.zone[cx][cz] |= LZ_PLUG_CORR;
            }
            int tipX = mx + dx[d] * 4, tipZ = mz + dz[d] * 4;
            if (lmAddSite(s, LS_PLUG_CORR, tipX, tipZ)) { s.deadThresholds++; placedPlug = true; }
        }
    }

    // 3. Closed doors mounted flush on blank solid walls (`LS_WALL_DOOR`)
    u32 hd = lmHash(h, 55u, 0u, 0x5744u);
    int wantDoors = ((int)(hd % 200u) < dtScore) ? (1 + (int)((hd >> 8) % 3u)) : 0;
    if ((force & LBF_WALL_DOOR) != 0u && wantDoors < 2) wantDoors = 2 + (int)((hd >> 8) & 1u);
    int placedDoors = 0;
    const int doorTries = (force & LBF_WALL_DOOR) ? 120 : 28;
    for (int att = 0; att < doorTries && placedDoors < wantDoors; att++) {
        u32 ha = lmHash(hd, (u32)att, 7u, 0x5745u);
        int lx, lz;
        lmBeatPickCell(bt, s.rect, s.w, s.h, ha, att, (force & LBF_WALL_DOOR) ? 80 : 0, 2, 2, lx, lz);
        int t = s.tag[lx][lz];
        if (t != LT_CORR && t != LT_HALL && t != LT_LOW) continue;
        if (s.flag[lx][lz] & (LF_NARROW_X | LF_NARROW_Z | LF_THIN_W | LF_THIN_N)) continue;
        bool hasSolidWall = false;
        for (int d = 0; d < 4; d++) {
            int wx = lx + dx[d], wz = lz + dz[d];
            if (lmTagLocal(s, wx, wz) == LT_WALL && !lmSlabHasSite(s, wx, wz, LS_VITRINE_WIN) && !lmSlabHasSite(s, wx, wz, LS_DOOR_GAP))
                hasSolidWall = true;
        }
        if (!hasSolidWall) continue;
        if (lmAddSite(s, LS_WALL_DOOR, lx, lz)) { placedDoors++; s.deadThresholds++; }
    }

    // 4. Stairs climbing straight into the solid ceiling (`LS_CEIL_STAIR`)
    u32 hs = lmHash(h, 57u, 0u, 0x4353u);
    int wantStairs = ((int)(hs % 210u) < dtScore + (int)s.genome.g[LG_SPAN] / 3) ? (1 + (int)((hs >> 9) & 1u)) : 0;
    if ((force & LBF_CEIL_STAIR) != 0u && wantStairs < 1) wantStairs = 1;
    int placedStairs = 0;
    const int stairTries = (force & LBF_CEIL_STAIR) ? 200 : 32;
    for (int att = 0; att < stairTries && placedStairs < wantStairs; att++) {
        u32 ha = lmHash(hs, (u32)att, 9u, 0x4354u);
        int lx, lz;
        lmBeatPickCell(bt, s.rect, s.w, s.h, ha, att, (force & LBF_CEIL_STAIR) ? 130 : 0, 2, 3, lx, lz);
        if (s.tag[lx][lz] != LT_HALL || s.tag[lx + 1][lz] != LT_HALL) continue;
        bool clearN = (lmTagLocal(s, lx, lz - 1) == LT_WALL && lmTagLocal(s, lx + 1, lz - 1) == LT_WALL &&
                       lmTagLocal(s, lx, lz + 1) == LT_HALL && lmTagLocal(s, lx + 1, lz + 1) == LT_HALL);
        bool clearS = (lmTagLocal(s, lx, lz + 1) == LT_WALL && lmTagLocal(s, lx + 1, lz + 1) == LT_WALL &&
                       lmTagLocal(s, lx, lz - 1) == LT_HALL && lmTagLocal(s, lx + 1, lz - 1) == LT_HALL);
        if (!clearN && !clearS) continue;
        if (lmAddSite(s, LS_CEIL_STAIR, lx, lz)) { placedStairs++; s.deadThresholds++; }
    }

    // 5. Phase-shifted doors/objects (`LS_PHASE_SHIFT`) + Deja-vu 3..4x corridor echo (`LZ_ECHO_CORR`)
    u32 hph = lmHash(h, 61u, 0u, 0x5048u);
    int phScore = (int)gLimenDNA.phaseShiftTendency + (int)s.genome.g[LG_GRAFT] / 2;
    if ((force & LBF_PHASE) != 0u || (int)(hph % 220u) < phScore) {
        int wantPh = 1 + (int)((hph >> 8) & 1u);
        const int phTries = (force & LBF_PHASE) ? 100 : 24;
        for (int att = 0, pl = 0; att < phTries && pl < wantPh; att++) {
            u32 ha = lmHash(hph, (u32)att, 11u, 0x5049u);
            int lx, lz;
            lmBeatPickCell(bt, s.rect, s.w, s.h, ha, att, (force & LBF_PHASE) ? 70 : 0, 2, 2, lx, lz);
            if (s.tag[lx][lz] != LT_HALL && s.tag[lx][lz] != LT_CORR) continue;
            if (s.flag[lx][lz] & (LF_NARROW_X | LF_NARROW_Z)) continue;
            bool adjWall = false;
            for (int d = 0; d < 4; d++) if (lmTagLocal(s, lx + dx[d], lz + dz[d]) == LT_WALL) adjWall = true;
            if (adjWall && lmAddSite(s, LS_PHASE_SHIFT, lx, lz)) pl++;
        }
    }

    u32 hec = lmHash(h, 63u, 0u, 0x4543u);
    if ((force & LBF_ECHO) != 0u || (int)(hec % 230u) < (int)gLimenDNA.phaseShiftTendency + (int)s.genome.g[LG_ORDER] / 2) {
        unsigned short echoKey = (unsigned short)(50000u + (hec % 15000u));
        bool stampedEcho = false;
        for (int z = 2; z < s.h - 2 && !stampedEcho; z++) {
            int run = 0;
            for (int x = 1; x < s.w - 1; x++) {
                bool isCorr = (s.tag[x][z] == LT_CORR && (lmTagLocal(s, x, z - 1) == LT_WALL || lmTagLocal(s, x, z + 1) == LT_WALL));
                run = isCorr ? (run + 1) : 0;
                if (run >= 8) {
                    for (int k = 0; k < 8; k++) {
                        int cx = x - 7 + k;
                        s.zone[cx][z] |= LZ_ECHO_CORR;
                        s.nodeKey[cx][z] = echoKey;
                        s.roomRel[cx][z] = (unsigned char)(k & 1);
                    }
                    stampedEcho = true; break;
                }
            }
        }
        for (int x = 2; x < s.w - 2 && !stampedEcho; x++) {
            int run = 0;
            for (int z = 1; z < s.h - 1; z++) {
                bool isCorr = (s.tag[x][z] == LT_CORR && (lmTagLocal(s, x - 1, z) == LT_WALL || lmTagLocal(s, x + 1, z) == LT_WALL));
                run = isCorr ? (run + 1) : 0;
                if (run >= 8) {
                    for (int k = 0; k < 8; k++) {
                        int cz = z - 7 + k;
                        s.zone[x][cz] |= LZ_ECHO_CORR;
                        s.nodeKey[x][cz] = echoKey;
                        s.roomRel[x][cz] = (unsigned char)(k & 1);
                    }
                    stampedEcho = true; break;
                }
            }
        }
    }
}

inline void lmMeasure(LimenSlab& s) {
    int count[LT_TAG_COUNT] = {0};
    for (int x = 0; x < s.w; x++) for (int z = 0; z < s.h; z++) count[s.tag[x][z]]++;
    for (int t = 0; t < LT_TAG_COUNT; t++) s.share[t] = count[t] * 100 / (s.w * s.h);
}
