#pragma once
// Part of limen_world.h: zones, walkability, ground height, vaults, lamps and pillars.

inline unsigned char lmZoneAt(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return 0;
    return lmSlab(r.j, r.i).zone[wx - r.x0 - 1][wz - r.z0 - 1];
}

// Walkable floor at y=0 (excludes walls, chasm voids, and sunken basins).
inline bool lmWalkable(int wx, int wz) { return lmIsWalkableTag(lmTagAt(wx, wz)); }

// Cell value stored in Chunk::cells[lx][lz]:
//   0 = walkable y=0 floor (AI & objectives may stand/path here)
//   1 = solid wall
//   2 = open non-floor drop (LT_VOID / LT_BASIN: visible across, never walked by AI or snapped by objectives!)
inline int lmCell(int wx, int wz) {
    int t = lmTagAt(wx, wz);
    if (t == LT_WALL) return 1;
    return lmIsWalkableTag(t) ? 0 : 2;
}

// True if the player on foot can step into this cell (includes stair-equipped dry LT_BASIN pools!).
inline bool lmPlayerWalkableAt(int wx, int wz) {
    int t = lmTagAt(wx, wz);
    if (lmIsWalkableTag(t)) return true;
    return t == LT_BASIN && (lmFlagAt(wx, wz) & LF_WALK_BASIN) != 0;
}

// Watertight cell-corner floor wave/step height in drunken 1-storey pockets (-0.28 m .. +0.64 m).
// Returns 0.0f at any seam, vault, xeno, or vitrine boundary so transitions never tear open!
inline float lmDrunkVertexFloorY(int vx, int vz) {
    int d = lmDeliriumCell255(vx, vz);
    if (d < 72) return 0.0f;
    for (int ox = -1; ox <= 0; ox++) {
        for (int oz = -1; oz <= 0; oz++) {
            int cx = vx + ox, cz = vz + oz;
            LimenRect r = lmSlabAt(cx, cz);
            if (cx == r.x0 || cz == r.z0) return 0.0f;
            const LimenSlab& s = lmSlab(r.j, r.i);
            if (lmSlabIsVault(s)) return 0.0f;
            int slx = cx - r.x0 - 1, slz = cz - r.z0 - 1;
            if ((s.zone[slx][slz] & (LZ_XENO | LZ_STD_CEIL | LZ_TALL_WELL | LZ_HALO)) != 0 || s.tag[slx][slz] == LT_VITRINE) return 0.0f;
            if (s.flag[slx][slz] & LF_CRAMPED) return 0.0f;
            if (lmSlabHasSite(s, slx, slz, LS_VITRINE_WIN) || lmSlabHasSite(s, slx, slz, LS_CEIL_STAIR) ||
                lmSlabHasSite(s, slx - 1, slz, LS_CEIL_STAIR) || lmSlabHasSite(s, slx, slz, LS_PHASE_SHIFT)) return 0.0f;
            if (s.zone[slx][slz] & LZ_STORY) return 0.0f;
        }
    }
    float amt = (float)(d - 72) / 183.0f;
    float boost = (d >= 145) ? 1.45f : 1.0f; // pronounced steps/waves in strong (>0.57) pockets!
    u32 h = lmHash((u32)vx, (u32)vz, 17u, 0x4657u);
    float wave = sinf((float)vx * 0.78f + (float)vz * 0.57f) * 0.52f
               + cosf((float)vx * 0.41f - (float)vz * 0.85f) * 0.30f;
    float step = (((int)(h & 3u) == 0) ? 0.36f : (((int)(h & 3u) == 1) ? -0.24f : 0.08f));
    float raw  = (wave * 0.65f + step * 0.65f + 0.16f) * amt * boost;
    if (raw < -0.32f) raw = -0.32f;
    if (raw >  0.84f) raw =  0.84f;
    return raw;
}

const int LM_TERR_SUB = 3; // 3x3 sub-tiles per 5m cell (1.667 m stepped terraces)

inline bool lmUnstructHasTerrain(int mf) { return mf == MF_SPIRES || mf == MF_GROTTO; }

// Deterministic quantized terraced terrain height for MF_SPIRES & MF_GROTTO.
// Strictly 0.0f on the outer 1-cell border & LZ_STORY cells so all seam doors stay flush!
inline float lmUnstructSubHeight(const LimenSlab& s, int gx, int gz) {
    if (!lmUnstructHasTerrain(s.megaForm)) return 0.0f;
    int gw = s.w * LM_TERR_SUB, gh = s.h * LM_TERR_SUB;
    if (gx < 0 || gz < 0 || gx >= gw || gz >= gh) return 0.0f;
    int slx = gx / LM_TERR_SUB, slz = gz / LM_TERR_SUB;
    if (!lmIsWalkableTag(s.tag[slx][slz]) || (s.zone[slx][slz] & LZ_STORY) != 0) return 0.0f;
    if (slx == 0 || slz == 0 || slx == s.w - 1 || slz == s.h - 1) return 0.0f;
    int edgeDist = gx < (gw - 1 - gx) ? gx : (gw - 1 - gx);
    int edZ      = gz < (gh - 1 - gz) ? gz : (gh - 1 - gz);
    if (edZ < edgeDist) edgeDist = edZ;
    float edgeT = (float)(edgeDist - LM_TERR_SUB) / 4.5f;
    if (edgeT <= 0.0f) return 0.0f;
    if (edgeT > 1.0f) edgeT = 1.0f;
    edgeT = edgeT * edgeT * (3.0f - 2.0f * edgeT);

    u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, (u32)s.megaSub, 0x544552u);
    float p1 = (float)(h & 255u) * 0.0245437f;
    float p2 = (float)((h >> 8) & 255u) * 0.0245437f;
    float p3 = (float)((h >> 16) & 255u) * 0.0245437f;
    float p4 = (float)((h >> 24) & 255u) * 0.0245437f;

    if (s.megaForm == MF_SPIRES) {
        float u = (float)gx * 0.21f, v = (float)gz * 0.21f;
        float raw = 2.05f
                  + 1.35f * sinf(u * 0.85f + p1) * cosf(v * 0.80f + p2)
                  + 0.82f * sinf((u + v) * 0.52f + p3)
                  + 0.55f * cosf((u * 1.25f - v * 0.95f) + p4);
        if (raw < 0.0f) raw = 0.0f;
        int st = (int)floorf((raw * edgeT) / 0.42f + 0.5f);
        if (st < 0) st = 0; else if (st > 11) st = 11;
        return (float)st * 0.42f;
    }

    // MF_GROTTO: flat central clearing surrounded by dark topographic contour rock terraces
    float cx = (float)gw * 0.5f, cz = (float)gh * 0.5f;
    float rx = ((float)gx + 0.5f - cx) / fmaxf(cx * 0.92f, 1.0f);
    float rz = ((float)gz + 0.5f - cz) / fmaxf(cz * 0.92f, 1.0f);
    float rad = sqrtf(rx * rx + rz * rz);
    float ang = atan2f(rz, rx);
    float clearR = 0.29f + 0.042f * sinf(ang * 3.0f + p1) + 0.028f * cosf(ang * 5.0f + p2);
    if (rad <= clearR) return 0.0f;
    float bowlT = (rad - clearR) / fmaxf(0.92f - clearR, 0.2f);
    if (bowlT > 1.0f) bowlT = 1.0f;
    float ridge = 1.0f + 0.36f * sinf(ang * 4.0f + p2) + 0.24f * cosf((float)gx * 0.24f - (float)gz * 0.21f + p3);
    float entryDip = (fabsf(rx) < 0.17f || fabsf(rz) < 0.17f) ? 0.50f : 1.0f;
    float val = (bowlT * bowlT * 3.10f + bowlT * 0.60f) * ridge * entryDip * edgeT;
    int st = (int)floorf(val / 0.38f + 0.5f);
    if (st < 0) st = 0; else if (st > 9) st = 9;
    return (float)st * 0.38f;
}

inline float lmUnstructTerrainY(const LimenSlab& s, float x, float z, float cs) {
    if (!lmUnstructHasTerrain(s.megaForm)) return 0.0f;
    float sub = cs / (float)LM_TERR_SUB;
    int gx = (int)floorf((x - (float)(s.rect.x0 + 1) * cs) / sub);
    int gz = (int)floorf((z - (float)(s.rect.z0 + 1) * cs) / sub);
    return lmUnstructSubHeight(s, gx, gz);
}

// Smooth ground elevation under the player's feet (0.0 m on ordinary decks/halls, ramps smoothly
// down LF_STAIR_DESC staircases to -2.85 m on walkable LT_BASIN floors, steps across natural
// mega-unstructure terraces, and rolls smoothly over warped drunken 1-storey floors!).
inline float lmGroundHeightAt(float x, float z, float cs) {
    int wx = (int)floorf(x / cs);
    int wz = (int)floorf(z / cs);
    float fx = (x - wx * cs) / cs, fz = (z - wz * cs) / cs;
    if (fx < 0.0f) fx = 0.0f; else if (fx > 1.0f) fx = 1.0f;
    if (fz < 0.0f) fz = 0.0f; else if (fz > 1.0f) fz = 1.0f;
    int tag = lmTagAt(wx, wz);
    if (tag == LT_BASIN) {
        unsigned short f = lmFlagAt(wx, wz);
        if ((f & LF_WALK_BASIN) == 0) return 0.0f;
        const float basinBot = -2.85f;
        if ((f & LF_STAIR_DESC) == 0) return basinBot;
        int dir = lmStairRimDir(f); // 0=-Z, 1=+Z, 2=-X, 3=+X
        float t = (dir == 0) ? fz : (dir == 1 ? (1.0f - fz) : (dir == 2 ? fx : (1.0f - fx)));
        if (t < 0.08f) t = 0.0f;
        else if (t > 0.90f) t = 1.0f;
        else t = (t - 0.08f) / 0.82f;
        return basinBot * t;
    }
    LimenRect r = lmSlabAt(wx, wz);
    if (wx != r.x0 && wz != r.z0) {
        const LimenSlab& s = lmSlab(r.j, r.i);
        if (lmUnstructHasTerrain(s.megaForm)) return lmUnstructTerrainY(s, x, z, cs);
    }
    // Exactly the two triangles the floor mesh is built from ((00,01,11) and (00,11,10)); a
    // bilinear blend drifts up to ~0.5 m off the rendered surface on twisted cells.
    float y00 = lmDrunkVertexFloorY(wx, wz),     y10 = lmDrunkVertexFloorY(wx + 1, wz);
    float y01 = lmDrunkVertexFloorY(wx, wz + 1), y11 = lmDrunkVertexFloorY(wx + 1, wz + 1);
    if (fz >= fx) return y00 + (y11 - y01) * fx + (y01 - y00) * fz;
    return y00 + (y10 - y00) * fx + (y11 - y10) * fz;
}

// True where the cell stands under a vault ceiling (seam wall lines never do).
inline bool lmVaultAt(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return false;
    return lmSlabIsVault(lmSlab(r.j, r.i));
}

// High cell of a vault: rendered by the megastructure builder rather than as a 1-storey cell.
inline bool lmHighAt(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return false;
    const LimenSlab& s = lmSlab(r.j, r.i);
    return lmSlabIsVault(s) && !lmIsLowShellTag(s.tag[wx - r.x0 - 1][wz - r.z0 - 1]);
}

inline bool lmLampIsBollard(const LimenSlab& s, int lx, int lz) {
    if (lmIsMegaUnstruct(s.megaForm)) return false;
    if (lmTagLocal(s, lx, lz) == LT_BRIDGE) return true;
    if (s.megaForm == MF_PLATFORM || s.megaForm == MF_ZIGGURAT) return false;
    return lmPitDegreeLocal(s, lx, lz) > 0;
}

// Pure O(1) order-independent Escher knot selector (~3.5..5.5% of mega-slabs, strictly non-adjacent!).
inline bool lmSlabEscherRawCand(int j, int i) {
    int tiers = lmSlabTiers(j, i);
    if (tiers < 4) return false;
    int mf = lmSlabMegaForm(j, i);
    if (mf == MF_SILO || mf == MF_HANGING || mf == MF_ZIGGURAT || lmIsMegaUnstruct(mf)) return false;
    LimenRect r = lmSlabRect(j, i);
    int w = r.x1 - r.x0 - 1, h = r.z1 - r.z0 - 1;
    int peak = 0;
    for (int sx = 2; sx < w; sx += 5) {
        for (int sz = 2; sz < h; sz += 5) {
            int d = lmDeliriumCell255(r.x0 + 1 + sx, r.z0 + 1 + sz);
            if (d > peak) peak = d;
        }
    }
    if (lmBeatForces(j, i, LBF_ESCHER)) return true;   // the narrator asked for the knot
    if (peak < 158) return false; // only inside strong (>0.62) delirious pockets!
    u32 hr = lmHash((u32)j, (u32)i, 93u, 0x4553u);
    return (hr % 100u) < 15u;
}

inline bool lmSlabHasEscherKnotAt(int j, int i) {
    if (!lmSlabEscherRawCand(j, i)) return false;
    u32 myTie = lmHash((u32)j, (u32)i, 94u, 0x4553u);
    for (int dj = -1; dj <= 1; dj++) {
        for (int di = -1; di <= 1; di++) {
            if (dj == 0 && di == 0) continue;
            if (!lmSlabEscherRawCand(j + dj, i + di)) continue;
            u32 nbTie = lmHash((u32)(j + dj), (u32)(i + di), 94u, 0x4553u);
            if (nbTie > myTie || (nbTie == myTie && (dj < 0 || (dj == 0 && di < 0)))) return false;
        }
    }
    return true;
}

inline bool lmLampLocal(const LimenSlab& s, int lx, int lz) {
    if (lmIsMegaUnstruct(s.megaForm)) return false;
    int t = lmTagLocal(s, lx, lz);
    if (t == LT_VITRINE) {
        return lmTagLocal(s, lx - 1, lz) != LT_VITRINE && lmTagLocal(s, lx, lz - 1) != LT_VITRINE;
    }
    if (!lmIsWalkableTag(t)) return false;
    int effPitch = ((s.zone[lx][lz] & LZ_XENO) != 0 && s.xeno.active) ? s.xeno.lampPitch : s.lampPitch;
    if ((s.zone[lx][lz] & LZ_HALO) != 0) effPitch += 2; // dim foreshadowing throat before megastructures!
    if ((s.zone[lx][lz] & LZ_ECHO_CORR) != 0) return s.roomRel[lx][lz] == 0;
    if (t == LT_BRIDGE) {
        int deg = lmPitDegreeLocal(s, lx, lz);
        if (deg >= 3) return true;
        return deg > 0 && ((lx + lz) % 3) == 0;
    }
    if (lmSlabIsVault(s) && t == LT_CORR) return ((lx + lz) % (effPitch < 3 ? 3 : effPitch)) == 0;
    if (t == LT_HALL) {
        // Keep vertical atrium mega-forms free of the 5.6 m office-pendant false ceiling;
        // keep perimeter wall/rim lamps so lighting invariants & dark-slab sites stay intact!
        if (lmSlabIsVault(s) && (s.megaForm == MF_SILO || s.megaForm == MF_CELLBLOCK ||
                                 s.megaForm == MF_CANYON || s.megaForm == MF_ZIGGURAT)) {
            bool nearEdge = (lmTagLocal(s, lx - 1, lz) == LT_WALL || lmTagLocal(s, lx + 1, lz) == LT_WALL ||
                             lmTagLocal(s, lx, lz - 1) == LT_WALL || lmTagLocal(s, lx, lz + 1) == LT_WALL ||
                             lmPitDegreeLocal(s, lx, lz) > 0);
            if (!nearEdge) return false;
            return ((lx + lz) % 4) == 0;
        }
        int p = effPitch, o = p > 2 ? p / 2 : 0;
        return (lx % p) == o && (lz % p) == o;
    }
    int cp = effPitch <= 2 ? 2 : (effPitch <= 3 ? 3 : (effPitch <= 4 ? 4 : 5));
    if (t == LT_CORR || t == LT_LOW) {
        bool alongX = lmIsLowShellTag(lmTagLocal(s, lx - 1, lz)) || lmTagLocal(s, lx - 1, lz) == LT_CORR ||
                      lmIsLowShellTag(lmTagLocal(s, lx + 1, lz)) || lmTagLocal(s, lx + 1, lz) == LT_CORR;
        bool alongZ = lmIsLowShellTag(lmTagLocal(s, lx, lz - 1)) || lmTagLocal(s, lx, lz - 1) == LT_CORR ||
                      lmIsLowShellTag(lmTagLocal(s, lx, lz + 1)) || lmTagLocal(s, lx, lz + 1) == LT_CORR;
        if (alongX && !alongZ) return (lx % cp) == 0;
        if (alongZ && !alongX) return (lz % cp) == 0;
        return (lx % cp) == 0 && (lz % cp) == 0;
    }
    int rp = effPitch <= 3 ? 2 : 3;
    int dx = 0, dz = 0;
    while (dx < 12 && lmTagLocal(s, lx - dx - 1, lz) == LT_ROOM) dx++;
    while (dz < 12 && lmTagLocal(s, lx, lz - dz - 1) == LT_ROOM) dz++;
    return (dx % rp) == 0 && (dz % rp) == 0;
}

// 0 = no lamp here, 1 = lamp on, 2 = lamp present but dark.
inline int lmLampAt(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return 0;
    const LimenSlab& s = lmSlab(r.j, r.i);
    int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
    if (!lmLampLocal(s, lx, lz)) return 0;
    if (s.tag[lx][lz] == LT_VITRINE) return 1; // sealed vitrines always glow eerily!
    bool special = (lx == s.specialLampX && lz == s.specialLampZ);
    if (s.dark) return special ? 1 : 2;
    if (special) return 2;
    if (!gLmActiveSites.empty()) {
        for (int k = 0; k < s.siteCount; k++) {
            if (s.sites[k].type == LS_DYN_LAMP && s.sites[k].lx == lx && s.sites[k].lz == lz &&
                lmIsSiteTriggered(s.sites[k].id)) return 2;
        }
    }
    return 1;
}

inline bool lmPillarAt(int wx, int wz) {
    LimenRect r = lmSlabAt(wx, wz);
    if (wx == r.x0 || wz == r.z0) return false;
    const LimenSlab& s = lmSlab(r.j, r.i);
    if (!s.pillars) return false;
    int lx = wx - r.x0 - 1, lz = wz - r.z0 - 1;
    return s.tag[lx][lz] == LT_HALL && (lx % 4) == 1 && (lz % 4) == 1 && (s.zone[lx][lz] & LZ_STORY) == 0;
}

inline u32 lmWindowChecksum(int x0, int z0, int w, int h) {
    u32 crc = 0x811C9DC5u;
    for (int z = z0; z < z0 + h; z++) {
        for (int x = x0; x < x0 + w; x++) {
            u32 v = (u32)lmTagAt(x, z) | ((u32)lmLampAt(x, z) << 4) |
                    ((lmPillarAt(x, z) ? 1u : 0u) << 6) | ((u32)lmFlagAt(x, z) << 8) |
                    ((u32)lmZoneAt(x, z) << 24);
            crc = (crc ^ v) * 16777619u;
        }
    }
    return crc;
}
