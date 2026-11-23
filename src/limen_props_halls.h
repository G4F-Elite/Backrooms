#pragma once
// Part of limen_props.h: hall zones, bridges, chunk themes, processions, quirk props.

// Structured architectural furnishing for LT_HALL:
//   1) Intentionally stark monumental halls (zero floor clutter when lmSlabHallIsBare)
//   2) Continuous aligned wall-hugging banks + companions along hall perimeters
//   3) Rim/parapet observation & barrier pairs along sunken basins / chasms
//   4) Grid-locked open-floor rows, workstation+companion islands, and squared depots
//      aligned strictly to the slab's column (4x4) and lamp (s.lampPitch) lattice!
inline void lmSpawnHallZones(const Chunk& c) {
    static const int dx[4] = {0, 1, 0, -1}, dz[4] = {1, 0, -1, 0};

    for (int lx = 1; lx < CHUNK_SIZE - 1; lx++) {
        for (int lz = 1; lz < CHUNK_SIZE - 1; lz++) {
            int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
            if (!lmAllowPropAt(wx, wz, true)) continue;
            LimenRect sr = lmSlabAt(wx, wz);
            if (wx == sr.x0 || wz == sr.z0) continue;
            const LimenSlab& s = lmSlab(sr.j, sr.i);
            int slx = wx - sr.x0 - 1, slz = wz - sr.z0 - 1;
            if (s.tag[slx][slz] != LT_HALL || s.dark) continue;

            bool inXeno = s.xeno.active && (s.zone[slx][slz] & LZ_XENO) != 0;
            if (!inXeno && lmSlabHallIsBare(s)) continue;

            int useG   = inXeno ? (int)s.xeno.useGene : (int)s.genome.g[LG_USE];
            int orderG = (int)s.genome.g[LG_ORDER];
            int decayG = (int)s.genome.g[LG_DECAY];
            int lattG  = (int)s.genome.g[LG_LATTICE];
            int drive  = ((int)gLimenDNA.propDensity * 3 + useG * 3 + 80) / 6; // ~30..255
            u32 slabH  = lmHash((u32)sr.j, (u32)sr.i, inXeno ? 91u : 19u, 0x485Au);
            unsigned themes = lmPickCellDecorTheme(s, slx, slz, 11u);

            // A. Perimeter wall-hugging functional banks (2..3 matching units per cell along wall bays)
            int wallDir = -1, wallCount = 0;
            for (int d = 0; d < 4; d++) {
                if (lmTagAt(wx + dx[d], wz + dz[d]) == LT_WALL) {
                    wallCount++;
                    if (wallDir < 0 && !lmSlabHasSite(s, slx + dx[d], slz + dz[d], LS_VITRINE_WIN) &&
                        !lmSlabHasSite(s, slx + dx[d], slz + dz[d], LS_WALL_DOOR)) wallDir = d;
                }
            }
            if (wallCount == 1 && wallDir >= 0) {
                int runCoord = (dx[wallDir] != 0) ? slz : slx;
                int bayIdx   = runCoord / 3;
                u32 bayH     = lmHash(slabH, (u32)(bayIdx * 17 + wallDir), 3u, 0x5742u);
                if ((int)(bayH % 260u) < drive) {
                    int wtype = pickThemedPropType(themes, bayH >> 8, 1u << PROP_PLACE_WALL);
                    if (wtype >= 0) {
                        float bx = 0.0f, bz = 0.0f;
                        mapPropBaseExtents(wtype, bx, bz);
                        int nUnits = (bx < 0.55f) ? 3 : ((bx < 0.95f) ? 2 : 1);
                        float step = bx * 2.0f + 0.28f;
                        std::mt19937 wr(bayH ^ (u32)(wx * 131 + wz));
                        bool faceWallAnomaly = ((int)((bayH >> 18) % 340u) < (int)gLimenDNA.faceWallTendency / 2);
                        if (faceWallAnomaly) {
                            lmApplyRuleFaceWall(wx, wz, themes, wr);
                        } else {
                            for (int u = 0; u < nUnits; u++) {
                                u32 uh = lmHash(bayH, (u32)(runCoord * 7 + u), 5u, 0x5755u);
                                if ((int)(uh % 420u) < decayG) continue; // decayed gap in bank
                                float slide = ((float)u - 0.5f * (float)(nUnits - 1)) * step;
                                if (placePropOnWall(wx, wz, dx[wallDir], dz[wallDir], wtype, 1.0f, slide)) {
                                    lmTryPlaceCompanions(wtype, themes, wr, 1);
                                }
                            }
                        }
                    }
                }
                continue;
            }
            if (wallCount > 0) continue;

            // B. Vault basin/chasm parapet rim lines (aligned pairs parallel to the drop)
            int dropDir = -1;
            for (int d = 0; d < 4; d++) {
                int nt = lmTagLocal(s, slx + dx[d], slz + dz[d]);
                if (!lmIsPitTag(nt)) continue;
                if (nt == LT_BASIN && lmBasinStairOpensToward(s, slx + dx[d], slz + dz[d], -dx[d], -dz[d])) continue;
                dropDir = d; break;
            }
            if (dropDir >= 0) {
                int rimCoord = (dx[dropDir] != 0) ? slz : slx;
                int rimBay   = rimCoord / 3;
                u32 rimH     = lmHash(slabH, (u32)(rimBay * 23 + dropDir), 7u, 0x524Du);
                if ((rimCoord % 3) != 2 && (int)(rimH % 300u) < drive) {
                    int rtype = lmPickAlignedRowProp(themes, rimH >> 7);
                    if (rtype >= 0) {
                        float bx = 0.0f, bz = 0.0f;
                        mapPropBaseExtents(rtype, bx, bz);
                        float yaw = atan2f((float)dx[dropDir], (float)dz[dropDir]);
                        Vec3 fwd((float)dx[dropDir], 0.0f, (float)dz[dropDir]);
                        Vec3 right((float)dz[dropDir], 0.0f, (float)-dx[dropDir]);
                        Vec3 base = cellCenter(wx, wz) - fwd * 0.55f;
                        int cols = (bx < 0.72f) ? 2 : 1;
                        float cStep = bx * 2.0f + 0.28f;
                        for (int cl = 0; cl < cols; cl++) {
                            float off = ((float)cl - 0.5f * (float)(cols - 1)) * cStep;
                            addMapProp(base + right * off, rtype, 1.0f, yaw);
                        }
                    }
                }
                continue;
            }

            // C. Open-floor interior groups locked strictly to the column (4x4) or lamp lattice!
            if (lmTagLocal(s, slx - 1, slz) != LT_HALL || lmTagLocal(s, slx + 1, slz) != LT_HALL ||
                lmTagLocal(s, slx, slz - 1) != LT_HALL || lmTagLocal(s, slx, slz + 1) != LT_HALL) continue;
            if (lmPillarAt(wx, wz)) continue;

            int gPitch = s.pillars ? 4 : (s.lampPitch < 3 ? 3 : (s.lampPitch > 4 ? 4 : s.lampPitch));
            int gOff   = s.pillars ? 3 : (gPitch / 2); // between 4 pillars, or directly in each lamp's pool of light!
            if ((slx % gPitch) != gOff || (slz % gPitch) != gOff) continue;

            // Spatial department clustering: active in coherent multi-anchor zones rather than isolated specks
            int zoneX = slx / (gPitch * 2), zoneZ = slz / (gPitch * 2);
            u32 zoneH = lmHash(slabH, (u32)zoneX, (u32)zoneZ, 0x5A4Eu);
            int zoneThresh = 90 + drive * 3 / 4;
            if ((int)(zoneH % 260u) >= zoneThresh) continue;

            u32 cellH = lmHash(zoneH, (u32)slx, (u32)slz, 0x4953u);
            if ((int)(cellH % 260u) >= (135 + drive / 2)) continue;

            bool majorAlongZ = (s.h >= s.w);
            float axisYaw = majorAlongZ ? 0.0f : 1.57079633f;
            if (((zoneH >> 9) & 3u) == 0u) axisYaw += 3.14159265f;
            Vec3 fwd(sinf(axisYaw), 0.0f, cosf(axisYaw));
            Vec3 right(cosf(axisYaw), 0.0f, -sinf(axisYaw));
            Vec3 center = cellCenter(wx, wz);

            // Slab-consistent primary arrangement mode:
            //   0 = workstation / table / bed + companion island (2x2 or 1x3 pairs)
            //   1 = parallel aligned rows (2 rows x 3 cols = 6 units)
            //   2 = tight orthogonal 2x2 depot / partition lounge cluster
            int prefMode = (lattG > 145 || orderG > 140) ? ((zoneH & 1u) ? 1 : 0) : (int)((zoneH >> 4) % 3u);
            int hostType = lmPickHostWithCompanion(themes, zoneH >> 8, 1u << PROP_PLACE_FLOOR);
            if (prefMode == 0 && hostType < 0) prefMode = 1;

            if (prefMode == 0 && hostType >= 0) {
                const PropCompanion* rule = propCompanionFor(hostType);
                float hhx = 0.0f, hhz = 0.0f;
                mapPropBaseExtents(hostType, hhx, hhz);
                int cols = (hhx < 0.62f) ? 3 : 2;
                float colStep = hhx * 2.0f + 0.22f;
                float rowOff  = hhz + 0.08f;
                std::mt19937 ir(cellH);
                // Back-to-back double island of hosts, each with its companion chair/stool/stand!
                for (int side = -1; side <= 1; side += 2) {
                    float hyaw = (side > 0) ? axisYaw : (axisYaw + 3.14159265f);
                    Vec3 hf = fwd * (float)side;
                    for (int cl = 0; cl < cols; cl++) {
                        u32 slotH = lmHash(cellH, (u32)(side + 2), (u32)cl, 0x534Cu);
                        if ((int)(slotH % 380u) < decayG) continue;
                        float lat = ((float)cl - 0.5f * (float)(cols - 1)) * colStep;
                        float jitterYaw = (orderG < 75) ? (((float)(slotH % 21u) - 10.0f) * 0.012f) : 0.0f;
                        Vec3 hpos = center + hf * rowOff + right * lat;
                        if (addMapProp(hpos, hostType, 1.0f, hyaw + jitterYaw) && rule) {
                            placePropCompanion(mapProps.back(), *rule, ir);
                        }
                    }
                }
            } else if (prefMode == 1) {
                int rtype = lmPickAlignedRowProp(themes, zoneH >> 11);
                if (rtype >= 0) {
                    float bx = 0.0f, bz = 0.0f;
                    mapPropBaseExtents(rtype, bx, bz);
                    int cols = (bx < 0.58f) ? 3 : 2;
                    int rows = (bz < 0.52f) ? 2 : 2;
                    float colStep = bx * 2.0f + 0.20f;
                    float rowStep = bz * 2.0f + 1.18f;
                    for (int rw = 0; rw < rows; rw++) {
                        for (int cl = 0; cl < cols; cl++) {
                            u32 slotH = lmHash(cellH, (u32)rw, (u32)cl, 0x5257u);
                            if ((int)(slotH % 400u) < decayG) continue;
                            float along = ((float)rw - 0.5f * (float)(rows - 1)) * rowStep;
                            float side  = ((float)cl - 0.5f * (float)(cols - 1)) * colStep;
                            float jYaw  = (orderG < 70 && ((slotH >> 9) % 7u) == 0u) ? 0.20f : 0.0f;
                            addMapProp(center + fwd * along + right * side, rtype, 1.0f, axisYaw + jYaw);
                        }
                    }
                }
            } else {
                int itype = lmPickStructuredIslandProp(themes, zoneH >> 13);
                int btype = lmPickStructuredIslandProp(themes, (zoneH >> 19) + 7u);
                if (itype >= 0) {
                    float bx = 0.0f, bz = 0.0f;
                    mapPropBaseExtents(itype, bx, bz);
                    float sx = bx + 0.14f, sz = bz + 0.14f;
                    // Tight squared 2x2 depot / equipment island (4–5 pieces grouped cleanly together)
                    for (int ix = -1; ix <= 1; ix += 2) {
                        for (int iz = -1; iz <= 1; iz += 2) {
                            u32 slotH = lmHash(cellH, (u32)(ix + 2), (u32)(iz + 2), 0x4450u);
                            if ((int)(slotH % 420u) < decayG) continue;
                            int useT = ((ix == 1 && iz == 1) && btype >= 0) ? btype : itype;
                            float pyaw = axisYaw + ((iz < 0) ? 3.14159265f : 0.0f);
                            addMapProp(center + right * ((float)ix * sx) + fwd * ((float)iz * sz), useT, 1.0f, pyaw);
                        }
                    }
                    std::mt19937 dr(cellH ^ 0x99u);
                    if (!mapProps.empty()) lmTryPlaceCompanions(itype, themes, dr, 2);
                }
            }
        }
    }
}

inline void lmSpawnBridgeProps(const Chunk& c, std::mt19937& r) {
    int want = 1 + (int)gLimenDNA.propDensity * 3 / 255;
    int placed = 0;
    static const int dx[4] = {0, 1, 0, -1}, dz[4] = {1, 0, -1, 0};
    for (int attempt = 0; attempt < want * 12 && placed < want; attempt++) {
        int lx = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int lz = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
        if (!lmHighAt(wx, wz) || !lmAllowPropAt(wx, wz, true) || lmLampAt(wx, wz) != 0) continue;
        LimenRect sr = lmSlabAt(wx, wz);
        if (wx == sr.x0 || wz == sr.z0) continue;
        const LimenSlab& s = lmSlab(sr.j, sr.i);
        int slx = wx - sr.x0 - 1, slz = wz - sr.z0 - 1;
        if (s.tag[slx][slz] != LT_BRIDGE) continue;
        unsigned themes = lmPickCellDecorTheme(s, slx, slz, (u32)(attempt + 17));
        for (int d = 0; d < 4; d++) {
            if (!lmIsPitTag(lmTagLocal(s, slx + dx[d], slz + dz[d]))) continue;
            int ptype = lmPickAlignedRowProp(themes, r());
            if (ptype < 0) break;
            float yaw = atan2f((float)dx[d], (float)dz[d]);
            Vec3 pos((wx + 0.5f - dx[d] * 0.28f) * CS, 0.0f, (wz + 0.5f - dz[d] * 0.28f) * CS);
            if (addMapProp(pos, ptype, 1.0f, yaw)) placed++;
            break;
        }
    }
}

inline void lmSpawnChunkThemed(const Chunk& c, std::mt19937& r) {
    int wantLow  = 1 + (int)gLimenDNA.propDensity * 5 / 255 + (int)(r() % 2u);
    int wantHigh = 1 + (int)gLimenDNA.propDensity * 4 / 255;

    for (int pass = 0; pass < 2; pass++) {
        int want = (pass == 0) ? wantLow : wantHigh;
        int placed = 0;
        for (int attempt = 0; attempt < want * 14 && placed < want; attempt++) {
            int lx = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
            int lz = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
            int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
            if (!lmAllowPropAt(wx, wz, pass == 0)) continue;

            LimenRect cr = lmSlabAt(wx, wz);
            int slx = -1, slz = -1;
            if (wx != cr.x0 && wz != cr.z0) {
                const LimenSlab& cs = lmSlab(cr.j, cr.i);
                slx = wx - cr.x0 - 1; slz = wz - cr.z0 - 1;
                int ctag = cs.tag[slx][slz];
                if (ctag == LT_ROOM || (cs.zone[slx][slz] & LZ_ECHO_CORR) != 0) continue;
                // Never scatter loose singletons into LT_HALL or LT_BRIDGE (handled by lmSpawnHallZones)!
                if (pass == 0 && (ctag == LT_HALL || ctag == LT_BRIDGE)) continue;
            } else if (pass == 0) {
                continue;
            }

            const LimenSlab& s = lmSlab(cr.j, cr.i);
            unsigned themes = lmPickCellDecorTheme(s, slx, slz, (u32)(pass * 31 + attempt));
            unsigned mask = 0u;
            if (pass == 0) {
                mask = (1u << PROP_PLACE_WALL) | (1u << PROP_PLACE_FLAT);
            } else {
                if (lmCanHangHighWallPropAt(wx, wz)) mask |= (1u << PROP_PLACE_WALL_HIGH);
                if (lmCanHangCeilingPropAt(wx, wz))  mask |= (1u << PROP_PLACE_CEILING);
            }
            if (mask == 0u) continue;

            int type = pickThemedPropType(themes, r(), mask);
            if (type < 0) continue;
            if (!placeProp(wx, wz, type, r)) continue;
            placed++;
            lmTryPlaceCompanions(type, themes, r, 2);
        }
    }
}

// Delirium Furniture: single-file queue / procession of identical props marching across a hall/corridor!
inline void lmSpawnDeliriumProcession(const Chunk& c, std::mt19937& r) {
    int midWX = c.cx * CHUNK_SIZE + CHUNK_SIZE / 2, midWZ = c.cz * CHUNK_SIZE + CHUNK_SIZE / 2;
    auto asked = [](int wx, int wz) {            // the narrator wants a procession in this slab
        LimenRect q = lmSlabAt(wx, wz);
        return lmBeatForces(q.j, q.i, LBF_PROCESSION);
    };
    bool chunkAsked = asked(midWX, midWZ) || asked(c.cx * CHUNK_SIZE + 2, c.cz * CHUNK_SIZE + 2) ||
                      asked(c.cx * CHUNK_SIZE + CHUNK_SIZE - 3, c.cz * CHUNK_SIZE + CHUNK_SIZE - 3);
    if (!chunkAsked && lmDeliriumCell255(midWX, midWZ) < 88) return;
    for (int att = 0; att < (chunkAsked ? 40 : 12); att++) {
        int lx = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int lz = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
        if (lmDeliriumCell255(wx, wz) < 92 && !asked(wx, wz)) continue;
        int axis = (att & 1);
        int run = 0;
        while (run < 6) {
            int tx = wx + (axis == 0 ? run : 0), tz = wz + (axis == 0 ? 0 : run);
            int tt = lmTagAt(tx, tz);
            if ((tt != LT_HALL && tt != LT_CORR) || !lmAllowPropAt(tx, tz, true)) break;
            run++;
        }
        if (run < 4) continue;
        LimenRect sr = lmSlabAt(wx, wz);
        const LimenSlab& s = lmSlab(sr.j, sr.i);
        unsigned themes = lmPickSlabDecorTheme(s, 71u);
        int ptype = lmPickMonoFloodProp(themes, r());
        if (ptype < 0) continue;
        float bx = 0.28f, bz = 0.28f;
        mapPropBaseExtents(ptype, bx, bz);
        float stepM = fmaxf((axis == 0 ? bx : bz) * 2.0f + 0.28f, 1.05f);
        float totalLen = (float)run * CS - 1.2f;
        float yaw = (axis == 0) ? 1.57079633f : 0.0f;
        float sx = (wx + 0.5f) * CS, sz = (wz + 0.5f) * CS;
        for (float d = 0.0f; d <= totalLen; d += stepM) {
            float sway = sinf(d * 0.42f) * 0.35f + 0.85f; // offset aside so aisle stays passable!
            Vec3 p = (axis == 0) ? Vec3(sx + d, 0.0f, sz + sway) : Vec3(sx + sway, 0.0f, sz + d);
            lmAddRoomPropSafe(p, ptype, 1.0f, yaw + cosf(d * 0.42f) * 0.14f);
        }
        lmNarNote(LN_PROCESSION, wx, wz);
        return;
    }
}

// Streaming: when the player crosses into a new chunk, work for the chunks that just entered
// the view (160+ m out, in fog) is spread over the following frames instead of stalling one.
// The frame grabber needs everything in place at once, so it runs with streaming off.
#ifdef LIMEN_SHOTS
inline bool gLmStreaming = false;
#else
inline bool gLmStreaming = true;
#endif
inline bool gLmContentPending = false;     // a chunk in view is still waiting for its furniture
inline bool gLmChunkPropPending = false;   // some chunk still shows an out-of-date (or no) prop mesh

// The game's placement test (`isMapPropSpotBlocked`) walks every prop and pillar in view for
// each candidate. With ~13k props in a 7x7 chunk view that made furnishing one new chunk cost
// ~40 ms. While a chunk is being furnished `mapProps` / `pillars` hold only its neighbourhood;
// the full lists are put back (same order, new props appended) when it is done.
struct LmLocalContentScope {
    std::vector<MapProp> allProps;
    std::vector<Vec3> allPillars;
    std::vector<int> localIdx;
    size_t nLocal = 0;
    explicit LmLocalContentScope(const Chunk& c) {
        const float margin = 3.0f * CS;   // far beyond any prop footprint or companion offset
        float x0 = c.cx * CHUNK_SIZE * CS - margin, x1 = (c.cx + 1) * CHUNK_SIZE * CS + margin;
        float z0 = c.cz * CHUNK_SIZE * CS - margin, z1 = (c.cz + 1) * CHUNK_SIZE * CS + margin;
        allProps.swap(mapProps);
        for (int k = 0; k < (int)allProps.size(); k++) {
            const Vec3& p = allProps[k].pos;
            if (p.x < x0 || p.x > x1 || p.z < z0 || p.z > z1) continue;
            mapProps.push_back(allProps[k]);
            localIdx.push_back(k);
        }
        nLocal = mapProps.size();
        allPillars.swap(pillars);
        for (const Vec3& p : allPillars) {
            if (p.x < x0 || p.x > x1 || p.z < z0 || p.z > z1) continue;
            pillars.push_back(p);
        }
    }
    ~LmLocalContentScope() {
        size_t keep = mapProps.size() < nLocal ? mapProps.size() : nLocal;
        for (size_t k = 0; k < keep; k++) allProps[(size_t)localIdx[k]] = mapProps[k];
        for (size_t k = nLocal; k < mapProps.size(); k++) allProps.push_back(mapProps[k]);
        mapProps.swap(allProps);
        pillars.swap(allPillars);
    }
};

// ---------------------------------------------------------------- level quirks and furniture
// MONO: the one free-standing object of the catalogue this level cannot stop repeating.
inline int lmMonoObsessionType() {
    int cand[MAP_PROP_TYPE_COUNT], n = 0;
    for (int t = 0; t < MAP_PROP_TYPE_COUNT; t++) {
        if (propPlacement(t) != PROP_PLACE_FLOOR || isPropOverheadType(t)) continue;
        float bx = 0.0f, bz = 0.0f;
        mapPropBaseExtents(t, bx, bz);
        if (bx > 0.80f || bz > 0.80f || bx < 0.12f || bz < 0.12f) continue;   // a room must hold many of them
        cand[n++] = t;
    }
    if (n == 0) return MAP_PROP_CHAIR;
    return cand[lmLvlHash(gLimen.seed, gLimen.level, 940u) % (u32)n];
}

// Quirks rework the furniture a chunk has just been given (`mapProps[first..]`). Everything is a
// function of the prop's own position, so the result does not depend on streaming order.
inline void lmApplyQuirkProps(const Chunk& c, size_t first, std::mt19937& r) {
    const LimenLevelDNA& D = gLimenDNA;
    bool sky = lmHasQuirk(D, LQ_SKY), mono = lmHasQuirk(D, LQ_MONO);
    bool giant = lmHasQuirk(D, LQ_GIANT), party = lmHasQuirk(D, LQ_PARTY);
    if (!sky && !mono && !giant && !party) return;
    if (first > mapProps.size()) first = mapProps.size();
    if (sky || mono || giant) {
        std::vector<MapProp> fresh(mapProps.begin() + (long long)first, mapProps.end());
        mapProps.resize(first);
        int monoType = mono ? lmMonoObsessionType() : -1;
        int gk = lmGiantKind(D);
        auto tryAdd = [&](const MapProp& q) {
            if (lmPropHitsThinOrDoor(q) || isMapPropSpotBlocked(q)) return false;
            mapProps.push_back(q);
            return true;
        };
        for (const MapProp& orig : fresh) {
            MapProp pr = orig;
            int place = propPlacement(pr.type);
            bool overhead = isPropOverheadType(pr.type);
            int wx = (int)floorf(pr.pos.x / CS), wz = (int)floorf(pr.pos.z / CS);
            u32 h = lmHash((u32)(int)floorf(pr.pos.x * 4.0f), (u32)(int)floorf(pr.pos.z * 4.0f), (u32)pr.type, 0x5150u);
            // nothing hangs from a ceiling that is not there
            if (sky && (overhead || place == PROP_PLACE_CEILING) && !lmHighAt(wx, wz) && (lmFlagAt(wx, wz) & LF_CRAMPED) == 0) continue;
            bool changed = false;
            if (mono && place == PROP_PLACE_FLOOR && !overhead && (h % 100u) < 74u) {
                pr.type = monoType; pr.scale = 1.0f; changed = true;
            }
            if (giant && !overhead && (place == PROP_PLACE_FLOOR || place == PROP_PLACE_WALL)) {
                float t = (float)((h >> 8) & 255u) / 255.0f;
                bool big = (gk == LM_GIANT_BIG) || (gk == LM_GIANT_ALICE && ((h >> 20) & 1u) != 0u);
                if (big) {
                    if (place == PROP_PLACE_FLOOR) {
                        if (((h >> 16) % 100u) < 40u) continue;          // giants need elbow room
                        pr.scale *= 1.9f + 0.9f * t; changed = true;
                    }
                } else {
                    pr.scale *= 0.34f + 0.20f * t; changed = true;
                }
            }
            if (!changed) { mapProps.push_back(orig); continue; }
            if (tryAdd(pr)) continue;
            if (giant && pr.scale > orig.scale * 1.5f) {            // too big for the spot: try merely oversized
                pr.scale = orig.scale * 1.5f;
                if (tryAdd(pr)) continue;
            }
            mapProps.push_back(orig);
        }
    }
    if (party) {
        // Balloons, presents, laid tables and bunting in every bay that will take them.
        for (int lx = 1; lx < CHUNK_SIZE - 1; lx++) {
            for (int lz = 1; lz < CHUNK_SIZE - 1; lz++) {
                if (!isMapPropCellValid(c, lx, lz)) continue;
                int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
                if (!lmAllowPropAt(wx, wz)) continue;
                u32 h = lmHash((u32)wx, (u32)wz, 77u, 0x5054u);
                int roll = (int)(h % 100u);
                if (roll >= 30) continue;
                int type = roll < 13 ? MAP_PROP_BALLOON_BUNCH : (roll < 18 ? MAP_PROP_GIFT_PILE : (roll < 22 ? MAP_PROP_PARTY_TABLE : MAP_PROP_BUNTING));
                if (type == MAP_PROP_BUNTING && !lmCanHangCeilingPropAt(wx, wz)) type = MAP_PROP_BALLOON_BUNCH;
                placeProp(wx, wz, type, r);
                if (type == MAP_PROP_BALLOON_BUNCH && ((h >> 9) & 3u) == 0u) placeProp(wx, wz, MAP_PROP_BALLOON_BUNCH, r);
            }
        }
    }
}
