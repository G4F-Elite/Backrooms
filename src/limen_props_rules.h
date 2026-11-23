#pragma once
// LIMEN prop & Stage-2 liminal furniture operators:
//   - Continuous weighted theme selection from `gLimenDNA.themeWeight` (or `s.xeno.themeWeight`
//     inside leaked foreign-level `LZ_XENO` slices!) steered by `LG_USE` & `LG_GRAFT`.
//   - Stage-2 operators driven by slab genes + level DNA proclivities:
//       a) ruleMissingHost  - companion suite placed around an excised/missing centerpiece
//       b) ruleFaceWall     - orderly furniture turned 180 deg nose-first into walls/corners
//       c) ruleUncannyTwin  - in a clone-room row (`nodeKey`), 1 room mutates a single detail
//       d) ruleMonoFlood    - absurd regular lattice of 1 repeated catalog prop with narrow aisle
//       e) ruleWaitingVoid  - aligned rows of seating/barriers/counters facing blank wall or pit
//       +  solitary spotlight prop beneath the lone burning lamp of `LS_DARK_SLAB`
//       +  3..4x identical corridor slice repetition on `LZ_ECHO_CORR`

static_assert(LM_THEME_COUNT == THEME_COUNT, "LM_THEME_COUNT must match THEME_COUNT");

inline bool lmIsChunkInView(float x, float z, int pcx, int pcz) {
    int cx = (int)floorf(x / (CS * CHUNK_SIZE));
    int cz = (int)floorf(z / (CS * CHUNK_SIZE));
    return abs(cx - pcx) <= LM_VIEW_CHUNKS && abs(cz - pcz) <= LM_VIEW_CHUNKS;
}

inline bool lmIsReservedByGame(int wx, int wz) {
    if (gDoorwayVeto && gDoorwayVeto(wx, wz)) return true;
    return !gCellOverrides.empty() && gCellOverrides.find(cellKey(wx, wz)) != gCellOverrides.end();
}

// True if prop footprint overlaps any 0.26 m LF_THIN_* drywall partition or blocks a thin/thick doorway vestibule.
inline bool lmPropHitsThinOrDoor(const MapProp& pr) {
    float hx = 0.0f, hz = 0.0f;
    mapPropHalfExtents(pr, hx, hz);
    if (hx < 0.02f) hx = 0.16f;
    if (hz < 0.02f) hz = 0.16f;
    float x0 = pr.pos.x - hx, x1 = pr.pos.x + hx, z0 = pr.pos.z - hz, z1 = pr.pos.z + hz;
    int wx0 = (int)floorf(x0 / CS) - 1, wx1 = (int)floorf(x1 / CS) + 1;
    int wz0 = (int)floorf(z0 / CS) - 1, wz1 = (int)floorf(z1 / CS) + 1;
    const float th = LM_THIN_HALF + 0.05f, dh = LM_THIN_DOOR_HALF + 0.18f, vest = 1.12f;
    for (int cx = wx0; cx <= wx1; cx++) {
        for (int cz = wz0; cz <= wz1; cz++) {
            unsigned short f = lmFlagAt(cx, cz);
            float px = cx * CS, pz = cz * CS;
            if (f & LF_THIN_W) {
                if (x1 > px - th && x0 < px + th && z1 > pz && z0 < pz + CS) return true;
                if ((f & LF_THIN_DOOR_W) && x1 > px - vest && x0 < px + vest &&
                    z1 > pz + CS * 0.5f - dh && z0 < pz + CS * 0.5f + dh) return true;
            }
            if (f & LF_THIN_N) {
                if (z1 > pz - th && z0 < pz + th && x1 > px && x0 < px + CS) return true;
                if ((f & LF_THIN_DOOR_N) && z1 > pz - vest && z0 < pz + vest &&
                    x1 > px + CS * 0.5f - dh && x0 < px + CS * 0.5f + dh) return true;
            }
            if (lmDoorwayAt(cx, cz).axis != 0) {
                if (x1 > px - 0.55f && x0 < px + CS + 0.55f && z1 > pz - 0.55f && z0 < pz + CS + 0.55f) return true;
            }
        }
    }
    return false;
}

inline bool lmAddRoomPropSafe(const Vec3& pos, int type, float scale, float yaw) {
    MapProp pr{};
    pr.pos = Vec3(pos.x, 0.0f, pos.z);
    pr.type = type;
    pr.scale = scale;
    pr.yaw = yaw;
    if (lmPropHitsThinOrDoor(pr)) return false;
    return addMapProp(pos, type, scale, yaw);
}

inline bool lmAllowPropAt(int wx, int wz, bool allowVaultFloor = true, bool allowThinRoom = false) {
    if (lmIsReservedByGame(wx, wz)) return false;
    if (lmZoneAt(wx, wz) & LZ_STORY) return false;
    int tag = lmTagAt(wx, wz);
    if (!lmIsWalkableTag(tag)) return false;
    bool high = lmHighAt(wx, wz);
    if (high && !allowVaultFloor) return false;
    unsigned short flg = lmFlagAt(wx, wz);
    if (flg & (LF_NARROW_X | LF_NARROW_Z)) return false;
    if (!allowThinRoom && (flg & (LF_THIN_W | LF_THIN_N))) return false;
    if (lmZoneAt(wx, wz) & LZ_PLUG_CORR) return false;
    if (lmDoorwayAt(wx, wz).axis != 0) return false;
    LimenRect sr = lmSlabAt(wx, wz);
    if (wx != sr.x0 && wz != sr.z0) {
        const LimenSlab& s = lmSlab(sr.j, sr.i);
        if (lmIsMegaUnstruct(s.megaForm)) return false;
        int slx = wx - sr.x0 - 1, slz = wz - sr.z0 - 1;
        if (lmSlabHasSite(s, slx, slz, LS_CEIL_STAIR) || lmSlabHasSite(s, slx - 1, slz, LS_CEIL_STAIR) ||
            lmSlabHasSite(s, slx, slz, LS_PHASE_SHIFT)) return false;
        if (lmSlabHasSite(s, slx - 1, slz, LS_VITRINE_WIN) || lmSlabHasSite(s, slx + 1, slz, LS_VITRINE_WIN) ||
            lmSlabHasSite(s, slx, slz - 1, LS_VITRINE_WIN) || lmSlabHasSite(s, slx, slz + 1, slz >= 0 ? LS_VITRINE_WIN : LS_VITRINE_WIN)) return false;
        if (lmLampLocal(s, slx, slz) && high && lmLampIsBollard(s, slx, slz)) return false;
        if (tag == LT_BRIDGE && lmPitDegreeLocal(s, slx, slz) >= 2) return false;
    }
    return countWallNeighborsWorld(wx, wz) <= 3;
}

inline bool lmCanHangCeilingPropAt(int wx, int wz) {
    if (lmHighAt(wx, wz) || lmLampAt(wx, wz) != 0) return false;
    unsigned short flg = lmFlagAt(wx, wz);
    unsigned char zn = lmZoneAt(wx, wz);
    return (flg & (LF_CRAMPED | LF_MEZZ)) == 0 && (zn & (LZ_STD_CEIL | LZ_TALL_WELL)) == 0;
}

inline bool lmCanHangHighWallPropAt(int wx, int wz) {
    if (lmHighAt(wx, wz)) return false;
    unsigned short flg = lmFlagAt(wx, wz);
    unsigned char zn = lmZoneAt(wx, wz);
    return (flg & LF_CRAMPED) == 0 && (zn & (LZ_STD_CEIL | LZ_TALL_WELL)) == 0;
}

inline unsigned lmSampleThemeArray(const unsigned char weights[LM_THEME_COUNT], u32 posPermille) {
    int total = 0;
    for (int t = 0; t < LM_THEME_COUNT; t++) total += (int)weights[t];
    if (total <= 0) return 1u;
    int target = (int)((posPermille % 1000u) * (u32)total / 1000u);
    for (int t = 0; t < LM_THEME_COUNT; t++) {
        target -= (int)weights[t];
        if (target < 0) return 1u << t;
    }
    return 1u << (LM_THEME_COUNT - 1);
}

// Strict functional theme per wing district: prevents mixing playground slides, hospital gurneys,
// and warehouse pallets inside the same room/hall!
inline unsigned lmPickSlabDecorTheme(const LimenSlab& s, u32 extraSalt = 0u) {
    (void)extraSalt;
    int wt = lmClampInt(s.wingTheme, 0, LM_THEME_COUNT - 1);
    return 1u << wt;
}

// Cell-aware theme picker: switches cleanly to one coherent foreign theme inside an `LZ_XENO` cut!
inline unsigned lmPickCellDecorTheme(const LimenSlab& s, int slx, int slz, u32 extraSalt = 0u) {
    if (slx >= 0 && slz >= 0 && slx < s.w && slz < s.h && (s.zone[slx][slz] & LZ_XENO) != 0 && s.xeno.active) {
        u32 h = lmHash((u32)s.xeno.foreignLevel, 0u, (u32)s.xeno.useGene, 0x5854u);
        u32 pos0 = (u32)(((int)s.xeno.useGene * 1000 / 256 + (int)(h % 120u)) % 1000);
        return lmSampleThemeArray(s.xeno.themeWeight, pos0);
    }
    return lmPickSlabDecorTheme(s, extraSalt);
}

inline void lmTryPlaceCompanions(int hostType, unsigned themes, std::mt19937& r, int maxChain = 2) {
    const PropCompanion* rule = propCompanionFor(hostType);
    if (!rule) return;
    MapProp curHost = mapProps.back();
    for (int step = 0; step < maxChain; step++) {
        if ((int)(r() % 100u) >= rule->chance) break;
        if (step > 0 && (PROP_CATALOG[rule->buddy].themes & themes) == 0) break;
        if (!placePropCompanion(curHost, *rule, r)) break;
        curHost = mapProps.back();
        if (rule->buddy != hostType) break;
    }
}

inline bool lmPickRoomWall(int wx, int wz, u32 roll, int& dx, int& dz) {
    static const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    int found[4], n = 0;
    for (int i = 0; i < 4; i++) {
        if (getCellWorld(wx + dirs[i][0], wz + dirs[i][1]) == 1) found[n++] = i;
    }
    if (n == 0) return false;
    int pick = found[roll % (u32)n];
    dx = dirs[pick][0];
    dz = dirs[pick][1];
    return true;
}

// Catalog query: picks a host prop belonging to `themes` & `placementMask` that has a companion rule.
inline int lmPickHostWithCompanion(unsigned themes, u32 roll, unsigned placementMask) {
    int cands[32], n = 0;
    for (const PropCompanion& c : PROP_COMPANIONS) {
        if (c.buddy == c.host) continue; // prefer asymmetric host+buddy pairs (table+chair, bed+nightstand, board+desk)
        const PropInfo* info = propInfo(c.host);
        if (!info || (info->themes & themes) == 0) continue;
        if (((1u << info->placement) & placementMask) == 0) continue;
        if (n < 32) cands[n++] = c.host;
    }
    if (n == 0) return -1;
    return cands[roll % (u32)n];
}

// Catalog query: picks an aligned elongated floor prop (seating rows, barriers, stanchions, counters, turnstiles)
// purely from `PROP_CATALOG` placement/flags/footprint aspect ratio — zero hardcoded prop IDs!
inline int lmPickAlignedRowProp(unsigned themes, u32 roll) {
    int cands[48], weights[48], n = 0, wSum = 0;
    for (int i = 0; i < MAP_PROP_TYPE_COUNT && n < 48; i++) {
        const PropInfo& p = PROP_CATALOG[i];
        if (p.placement != PROP_PLACE_FLOOR || (p.themes & themes) == 0) continue;
        if ((p.flags & PROP_FLAG_ALIGNED) == 0) continue;
        if (p.hx < 0.36f || p.hx < p.hz * 1.15f) continue;
        cands[n] = p.type;
        weights[n] = p.weight;
        wSum += p.weight;
        n++;
    }
    if (n == 0 || wSum <= 0) return pickThemedPropType(themes, roll, 1u << PROP_PLACE_FLOOR);
    int at = (int)(roll % (u32)wSum);
    for (int k = 0; k < n; k++) {
        at -= weights[k];
        if (at < 0) return cands[k];
    }
    return cands[0];
}

// Catalog query: picks a compact floor prop suitable for absurd mono-flood grids.
inline int lmPickMonoFloodProp(unsigned themes, u32 roll) {
    int cands[64], n = 0;
    for (int i = 0; i < MAP_PROP_TYPE_COUNT && n < 64; i++) {
        const PropInfo& p = PROP_CATALOG[i];
        if (p.placement != PROP_PLACE_FLOOR || (p.themes & themes) == 0) continue;
        if (p.hx > 0.62f || p.hz > 0.62f || p.hx < 0.15f) continue;
        cands[n++] = p.type;
    }
    if (n == 0) return pickThemedPropType(themes, roll, 1u << PROP_PLACE_FLOOR);
    return cands[roll % (u32)n];
}

// A furniture rule's chance in this slab: as rolled, or (when the narrator asked for the rule)
// near certain in the rooms beside the road and merely likely further off.
inline int lmBeatChance(const LimenSlab& s, int slx, int slz, unsigned bit, int base, int nearCh, int farCh) {
    if ((s.beatForce & bit) == 0u) return base;
    int f = (nearCh + farCh) / 2;
    if (s.focusAxis >= 0) { int d = abs((s.focusAxis == 0 ? slz : slx) - (int)s.focusC); f = d <= 5 ? nearCh : farCh; }
    return base > f ? base : f;
}

// (a) ruleMissingHost: places 2..4 companions around the ghost footprint of an excised centerpiece!
inline bool lmApplyRuleMissingHost(int wx, int wz, unsigned themes, std::mt19937& rr) {
    int wdx = 0, wdz = 0;
    bool hasWall = lmPickRoomWall(wx, wz, rr(), wdx, wdz);
    unsigned mask = (1u << PROP_PLACE_FLOOR) | (hasWall ? (1u << PROP_PLACE_WALL) : 0u);
    int hostType = lmPickHostWithCompanion(themes, rr(), mask);
    if (hostType < 0) hostType = lmPickHostWithCompanion(THEME_ALL, rr(), mask);
    if (hostType < 0) return false;
    const PropCompanion* rule = propCompanionFor(hostType);
    if (!rule) return false;

    float hhx = 0.0f, hhz = 0.0f, bhx = 0.0f, bhz = 0.0f;
    mapPropBaseExtents(hostType, hhx, hhz);
    mapPropBaseExtents(rule->buddy, bhx, bhz);

    MapProp ghost{};
    ghost.type = hostType;
    ghost.scale = 1.0f;
    if (isMapPropWallType(hostType) && hasWall) {
        float reach = CS * 0.5f - hhz - 0.04f;
        Vec3 c = cellCenter(wx, wz);
        ghost.pos = Vec3(c.x + (float)wdx * reach, 0.0f, c.z + (float)wdz * reach);
        ghost.yaw = atan2f((float)-wdx, (float)-wdz);
    } else {
        ghost.pos = cellCenter(wx, wz);
        ghost.yaw = (rr() & 1u) ? 0.0f : 1.57079633f;
    }

    Vec3 right(cosf(ghost.yaw), 0.0f, -sinf(ghost.yaw));
    Vec3 front(sinf(ghost.yaw), 0.0f, cosf(ghost.yaw));
    int placed = 0;
    if (rule->mode == PROP_BUDDY_FRONT) {
        // Surround the empty table/board spot with inward-facing companions
        float reach = hhz + bhz + 0.18f;
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            Vec3 p1 = ghost.pos + front * reach + right * ((float)sgn * (bhx + 0.14f));
            if (addMapProp(p1, rule->buddy, 1.0f, ghost.yaw + 3.14159265f)) placed++;
            if (!isMapPropWallType(hostType)) {
                Vec3 p2 = ghost.pos - front * reach + right * ((float)sgn * (bhx + 0.14f));
                if (addMapProp(p2, rule->buddy, 1.0f, ghost.yaw)) placed++;
            }
        }
    } else {
        // Two flanking companions (e.g. two nightstands on either side of a missing bed!)
        float back = (isMapPropWallType(hostType) && isMapPropWallType(rule->buddy)) ? (bhz - hhz) : 0.0f;
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            Vec3 p = ghost.pos + right * ((float)sgn * (hhx + bhx + 0.12f)) + front * back;
            if (addMapProp(p, rule->buddy, 1.0f, ghost.yaw)) placed++;
        }
    }
    // Optional flat floor trace (fallen tiles / floor mat / puddle) marking where the host stood
    int flatTrace = pickThemedPropType(themes | THEME_ALL, rr(), 1u << PROP_PLACE_FLAT);
    if (flatTrace >= 0) addMapProp(ghost.pos, flatTrace, 1.0f, ghost.yaw);
    return placed > 0;
}

// (b) ruleFaceWall: places furniture strictly on grid, facing nose-first into the wall!
inline bool lmApplyRuleFaceWall(int wx, int wz, unsigned themes, std::mt19937& rr) {
    int wdx = 0, wdz = 0;
    if (!lmPickRoomWall(wx, wz, rr(), wdx, wdz)) return false;
    int ptype = pickThemedPropType(themes, rr(), (1u << PROP_PLACE_FLOOR) | (1u << PROP_PLACE_WALL));
    if (ptype < 0) return false;
    float bx = 0.0f, bz = 0.0f;
    mapPropBaseExtents(ptype, bx, bz);
    float faceIntoWallYaw = atan2f((float)wdx, (float)wdz); // local +Z (front) points INTO the wall!
    Vec3 c = cellCenter(wx, wz);
    Vec3 along((float)-wdz, 0.0f, (float)wdx);
    float reach = CS * 0.5f - bz - 0.14f;
    int count = (bx < 0.45f) ? 3 : ((bx < 0.85f) ? 2 : 1);
    float step = bx * 2.0f + 0.24f;
    int placed = 0;
    for (int k = 0; k < count; k++) {
        float slide = ((float)k - 0.5f * (float)(count - 1)) * step;
        Vec3 pos = c + Vec3((float)wdx * reach, 0.0f, (float)wdz * reach) + along * slide;
        if (addMapProp(pos, ptype, 1.0f, faceIntoWallYaw)) placed++;
    }
    return placed > 0;
}

// (d) ruleMonoFlood: floods a room cell with an absurd regular lattice of 1 single prop type,
// keeping a narrow 1.05 m walkway aisle along the center so navigation never deadlocks.
inline int lmApplyRuleMonoFlood(int wx, int wz, int ptype, float yaw) {
    float bx = 0.0f, bz = 0.0f;
    mapPropBaseExtents(ptype, bx, bz);
    if (bx < 0.12f) bx = 0.25f;
    if (bz < 0.12f) bz = 0.25f;
    float stepX = bx * 2.0f + 0.30f;
    float stepZ = bz * 2.0f + 0.30f;
    if (stepX < 0.82f) stepX = 0.82f;
    if (stepZ < 0.82f) stepZ = 0.82f;
    Vec3 c = cellCenter(wx, wz);
    int placed = 0;
    for (float ox = -1.85f; ox <= 1.86f; ox += stepX) {
        for (float oz = -1.85f; oz <= 1.86f; oz += stepZ) {
            if (fabsf(ox) < 0.54f && fabsf(oz) < 1.95f) continue; // narrow central walkway aisle
            if (addMapProp(Vec3(c.x + ox, 0.0f, c.z + oz), ptype, 1.0f, yaw)) placed++;
        }
    }
    return placed;
}

// (e) ruleWaitingVoid: rows of aligned seating/stanchions/counters facing a blank wall or abyss!
inline void lmSpawnWaitingVoidRows(const Chunk& c, std::mt19937& r) {
    LimenRect sr = lmSlabAt(c.cx * CHUNK_SIZE + CHUNK_SIZE / 2, c.cz * CHUNK_SIZE + CHUNK_SIZE / 2);
    const LimenSlab& s = lmSlab(sr.j, sr.i);
    int chance = ((int)gLimenDNA.waitVoidTendency * 2 + (int)s.genome.g[LG_SPAN] + (int)s.genome.g[LG_ORDER]) / 6;
    if (s.beatForce & LBF_WAITVOID) chance = 200;
    if ((int)(r() % 200u) >= chance) return;

    static const int dx[4] = {0, 1, 0, -1}, dz[4] = {1, 0, -1, 0};
    for (int att = 0; att < 22; att++) {
        int lx = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int lz = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
        if (!lmAllowPropAt(wx, wz, true)) continue;
        LimenRect cr = lmSlabAt(wx, wz);
        if (wx == cr.x0 || wz == cr.z0) continue;
        const LimenSlab& cs = lmSlab(cr.j, cr.i);
        int slx = wx - cr.x0 - 1, slz = wz - cr.z0 - 1;
        int tag = cs.tag[slx][slz];
        if (tag != LT_HALL && tag != LT_BRIDGE) continue;

        int targetDir = -1;
        for (int d = 0; d < 4; d++) {
            int nt = lmTagLocal(cs, slx + dx[d], slz + dz[d]);
            if (lmIsPitTag(nt) && !(nt == LT_BASIN && lmBasinStairOpensToward(cs, slx + dx[d], slz + dz[d], -dx[d], -dz[d]))) {
                targetDir = d; break;
            }
            if (nt == LT_WALL && doorwayAxisAt(wx + dx[d], wz + dz[d]) == 0 &&
                !lmSlabHasSite(cs, slx + dx[d], slz + dz[d], LS_VITRINE_WIN)) {
                targetDir = d; break;
            }
        }
        if (targetDir < 0) continue;

        unsigned themes = lmPickCellDecorTheme(cs, slx, slz, 91u + (u32)att);
        int ptype = lmPickAlignedRowProp(themes, r());
        if (ptype < 0) continue;
        float bx = 0.0f, bz = 0.0f;
        mapPropBaseExtents(ptype, bx, bz);
        float yaw = atan2f((float)dx[targetDir], (float)dz[targetDir]);
        Vec3 fwd((float)dx[targetDir], 0.0f, (float)dz[targetDir]);
        Vec3 right((float)dz[targetDir], 0.0f, (float)-dx[targetDir]);
        Vec3 base = cellCenter(wx, wz);
        int rows = 2, cols = (bx < 0.72f) ? 3 : 2;
        float rowStep = bz * 2.0f + 0.92f;
        float colStep = bx * 2.0f + 0.24f;
        int placed = 0;
        for (int rw = 0; rw < rows; rw++) {
            for (int cl = 0; cl < cols; cl++) {
                float alongOff = 1.25f - (float)rw * rowStep;
                float sideOff  = ((float)cl - 0.5f * (float)(cols - 1)) * colStep;
                if (addMapProp(base + fwd * alongOff + right * sideOff, ptype, 1.0f, yaw)) placed++;
            }
        }
        if (placed >= 2) { lmNarNote(LN_WAITVOID, wx, wz); return; }
    }
}
