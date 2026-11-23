#pragma once
// Part of limen_props.h: room suites, themed POIs, structured islands.

// Furnishes `LT_ROOM` interiors and applies Stage-2 room operators (`ruleMonoFlood`,
// `ruleMissingHost`, `ruleFaceWall`, `ruleUncannyTwin`), plus `LZ_ECHO_CORR` corridor loops
// and solitary spotlight props beneath `LS_DARK_SLAB`'s sole lamp!
inline void lmSpawnRoomSuites(const Chunk& c) {
    int furnish = ((int)gLimenDNA.roomFurnish * 2 + (int)gLimenDNA.propDensity) / 3;
    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
            LimenRect sr = lmSlabAt(wx, wz);
            if (wx == sr.x0 || wz == sr.z0) continue;
            const LimenSlab& s = lmSlab(sr.j, sr.i);
            int slx = wx - sr.x0 - 1, slz = wz - sr.z0 - 1;
            unsigned char zn = s.zone[slx][slz];

            // Solitary spotlight relic directly under the single live lamp of a dark slab!
            if (s.dark && slx == s.specialLampX && slz == s.specialLampZ && lmAllowPropAt(wx, wz, true)) {
                u32 sh = lmHash((u32)sr.j, (u32)sr.i, 99u, 0x444Bu);
                unsigned darkTheme = lmSampleThemeArray(gLimenDNA.graftWeight, sh % 1000u) |
                                     lmPickCellDecorTheme(s, slx, slz, 3u);
                int stype = pickThemedPropType(darkTheme, sh >> 5, 1u << PROP_PLACE_FLOOR);
                if (stype >= 0) addMapProp(cellCenter(wx, wz), stype, 1.0f, (float)(sh & 3u) * 1.57079633f);
                continue;
            }

            // Deja-vu 3..4x corridor loop (`LZ_ECHO_CORR`): identical wall/floor prop on each 2-cell beat!
            if ((zn & LZ_ECHO_CORR) != 0 && s.nodeKey[slx][slz] != 0 && lmAllowPropAt(wx, wz, true)) {
                unsigned short ek = s.nodeKey[slx][slz];
                unsigned char erel = s.roomRel[slx][slz];
                if (erel == 1) {
                    std::mt19937 er(lmHash((u32)ek, (u32)erel, 7u, 0x4543u));
                    unsigned ethemes = lmPickCellDecorTheme(s, slx, slz, ek & 7u);
                    int wdx = 0, wdz = 0;
                    if (lmPickRoomWall(wx, wz, 0u, wdx, wdz)) {
                        int etype = pickThemedPropType(ethemes, er(), (1u << PROP_PLACE_WALL) | (1u << PROP_PLACE_WALL_HIGH));
                        if (etype >= 0) placePropOnWall(wx, wz, wdx, wdz, etype, 1.0f, 0.0f);
                    }
                }
                continue;
            }

            if (s.tag[slx][slz] != LT_ROOM) continue;
            if (!lmAllowPropAt(wx, wz, true, true)) continue; // furnish thin-partitioned rooms too!

            unsigned short nk  = s.nodeKey[slx][slz];
            unsigned char  rel = s.roomRel[slx][slz];
            bool cloneLocked   = (nk != 0 && s.genome.g[LG_ORDER] >= 115);
            int useG           = ((zn & LZ_XENO) != 0 && s.xeno.active) ? (int)s.xeno.useGene : (int)s.genome.g[LG_USE];
            int roomBudget     = lmClampInt((furnish * 2 + useG * 2 + 80) / 4, 48, 252);

            int approxRoomOrigin = (wx - (int)(rel >> 4)) * 131 + (wz - (int)(rel & 15)) * 17;
            u32 roomInstH = lmHash((u32)sr.j, (u32)sr.i, (u32)(nk ^ (unsigned short)approxRoomOrigin), 0x5457u);
            u32 roomTypeH = cloneLocked ? lmHash((u32)nk, (u32)useG, 0u, 0x524Fu) : roomInstH;

            // Strict single cohesive wing theme per room (no multi-theme prop soup!)
            unsigned themes = lmPickCellDecorTheme(s, slx, slz, 0u);

            // Foreshadowing Halo border: strip rooms down to stark transitional emptiness!
            if ((zn & LZ_HALO) != 0) {
                if ((roomInstH % 7u) == 0u) lmAddRoomPropSafe(cellCenter(wx, wz), MAP_PROP_WET_FLOOR_SIGN, 1.0f, 0.4f);
                continue;
            }

            // HobbyTown Segmented Partition Labyrinth (`LZ_SPARSE`): 88% eerily bare empty rooms
            // with unobstructed sightlines; rare solitary chair, stain, or forgotten fixture!
            if ((zn & LZ_SPARSE) != 0) {
                u32 sh = lmHash((u32)wx, (u32)wz, 13u, 0x535052u);
                int roll = (int)(sh % 100u);
                if (roll >= 12) continue;
                Vec3 cc = cellCenter(wx, wz);
                if (roll < 4) {
                    lmAddRoomPropSafe(Vec3(cc.x + 0.85f, 0.0f, cc.z + 0.85f), MAP_PROP_CHAIR, 1.0f, 0.785f + (float)(sh & 3u) * 1.57f);
                } else if (roll < 8) {
                    int flatT = ((sh >> 8) & 1u) ? MAP_PROP_FALLEN_TILES : MAP_PROP_PUDDLE;
                    lmAddRoomPropSafe(cc, flatT, 1.0f, (float)((sh >> 9) & 3u) * 1.57f);
                } else {
                    int wdx = 0, wdz = 0;
                    if (lmPickRoomWall(wx, wz, sh >> 10, wdx, wdz)) {
                        placePropOnWall(wx, wz, wdx, wdz, MAP_PROP_WATER_COOLER, 1.0f, 0.0f);
                    }
                }
                continue;
            }

            // Delirium room contrast: in drunken pockets, adjacent rooms alternate sharply
            // between surgically bare emptiness and crammed wall-to-wall hoarding!
            int cellDel = lmDeliriumCell255(wx, wz);
            if (cellDel >= 95) {
                if ((roomInstH & 1u) == 0u) continue; // starkly empty twin right beside crammed room!
                roomBudget = 252;
                int cramT = lmPickMonoFloodProp(themes, roomInstH >> 5);
                if (cramT >= 0) {
                    Vec3 cc = cellCenter(wx, wz);
                    for (int qx = -1; qx <= 1; qx += 2) {
                        for (int qz = -1; qz <= 1; qz += 2) {
                            u32 qh = lmHash(roomInstH, (u32)(qx + 2), (u32)(qz + 2), 0x4352u);
                            lmAddRoomPropSafe(Vec3(cc.x + qx * 1.15f, 0.0f, cc.z + qz * 1.15f),
                                              cramT, 1.0f, (float)(qh % 16u) * 0.3927f);
                        }
                    }
                }
            }

            // Deliberately stripped / vacant rooms (~8..30% inverse to roomBudget, consistent across clone rows)
            int bareRoomChance = lmClampInt(40 - roomBudget / 7, 8, 30);
            if (cellDel < 95 && (int)((roomTypeH >> 3) % 100u) < bareRoomChance) {
                if (((roomTypeH >> 11) & 3u) == 0u) {
                    int traceT = pickThemedPropType(themes | THEME_ALL, roomTypeH >> 14, 1u << PROP_PLACE_FLAT);
                    if (traceT >= 0) lmAddRoomPropSafe(cellCenter(wx, wz), traceT, 1.0f, 0.0f);
                }
                continue;
            }

            // Classify all 4 cell edges (including 0.26 m LF_THIN_* drywall partitions & thin doors!)
            struct RoomWallSlot { int dx, dz; float extraInset; bool isSolidWall; bool hasDoor; };
            unsigned short fSelf  = lmFlagAt(wx, wz);
            unsigned short fEast  = lmFlagAt(wx + 1, wz);
            unsigned short fSouth = lmFlagAt(wx, wz + 1);
            RoomWallSlot edges[4] = {
                {-1, 0, (fSelf  & LF_THIN_W) ? (LM_THIN_HALF + 0.04f) : 0.0f,
                 (getCellWorld(wx - 1, wz) == 1) || ((fSelf  & (LF_THIN_W | LF_THIN_DOOR_W)) == LF_THIN_W),
                 ((fSelf  & LF_THIN_DOOR_W) != 0) || (getCellWorld(wx - 1, wz) == 0 && lmTagAt(wx - 1, wz) != LT_ROOM)},
                { 1, 0, (fEast  & LF_THIN_W) ? (LM_THIN_HALF + 0.04f) : 0.0f,
                 (getCellWorld(wx + 1, wz) == 1) || ((fEast  & (LF_THIN_W | LF_THIN_DOOR_W)) == LF_THIN_W),
                 ((fEast  & LF_THIN_DOOR_W) != 0) || (getCellWorld(wx + 1, wz) == 0 && lmTagAt(wx + 1, wz) != LT_ROOM)},
                { 0,-1, (fSelf  & LF_THIN_N) ? (LM_THIN_HALF + 0.04f) : 0.0f,
                 (getCellWorld(wx, wz - 1) == 1) || ((fSelf  & (LF_THIN_N | LF_THIN_DOOR_N)) == LF_THIN_N),
                 ((fSelf  & LF_THIN_DOOR_N) != 0) || (getCellWorld(wx, wz - 1) == 0 && lmTagAt(wx, wz - 1) != LT_ROOM)},
                { 0, 1, (fSouth & LF_THIN_N) ? (LM_THIN_HALF + 0.04f) : 0.0f,
                 (getCellWorld(wx, wz + 1) == 1) || ((fSouth & (LF_THIN_N | LF_THIN_DOOR_N)) == LF_THIN_N),
                 ((fSouth & LF_THIN_DOOR_N) != 0) || (getCellWorld(wx, wz + 1) == 0 && lmTagAt(wx, wz + 1) != LT_ROOM)}
            };
            int solidIdx[4], nSolid = 0;
            for (int e = 0; e < 4; e++) {
                int nslx = slx + edges[e].dx, nslz = slz + edges[e].dz;
                bool winOrDoorSite = (nslx >= 0 && nslz >= 0 && nslx < s.w && nslz < s.h) &&
                    (lmSlabHasSite(s, nslx, nslz, LS_VITRINE_WIN) || lmSlabHasSite(s, nslx, nslz, LS_WALL_DOOR));
                if (edges[e].isSolidWall && !edges[e].hasDoor && !winOrDoorSite) solidIdx[nSolid++] = e;
            }

            int monoChance = ((int)gLimenDNA.monoFloodTendency * 2 + (int)s.genome.g[LG_LATTICE]) / 10;
            int lackChance = ((int)gLimenDNA.lackTendency * 2 + (int)s.genome.g[LG_DECAY]) / 7;
            int faceChance = ((int)gLimenDNA.faceWallTendency * 2 + (int)s.genome.g[LG_DECAY]) / 7;
            int twinChance = ((int)gLimenDNA.twinMutTendency * 2 + (int)s.genome.g[LG_DECAY]) / 6;
            monoChance = lmBeatChance(s, slx, slz, LBF_MONO, monoChance, 190, 40);
            lackChance = lmBeatChance(s, slx, slz, LBF_LACK, lackChance, 190, 60);
            faceChance = lmBeatChance(s, slx, slz, LBF_FACEWALL, faceChance, 190, 60);
            twinChance = lmBeatChance(s, slx, slz, LBF_TWIN, twinChance, 220, 220);

            if (!cloneLocked && (int)(roomTypeH % 260u) < monoChance) {
                int mtype = lmPickMonoFloodProp(themes, roomTypeH >> 8);
                if (mtype >= 0) {
                    float myaw = ((roomTypeH >> 16) & 3u) * 1.57079633f;
                    float bx = 0.25f, bz = 0.25f;
                    mapPropBaseExtents(mtype, bx, bz);
                    float stepX = fmaxf(bx * 2.0f + 0.30f, 0.84f), stepZ = fmaxf(bz * 2.0f + 0.30f, 0.84f);
                    Vec3 c = cellCenter(wx, wz);
                    for (float ox = -1.75f; ox <= 1.76f; ox += stepX) {
                        for (float oz = -1.75f; oz <= 1.76f; oz += stepZ) {
                            if (fabsf(ox) < 0.56f || fabsf(oz) < 0.56f) continue;
                            lmAddRoomPropSafe(Vec3(c.x + ox, 0.0f, c.z + oz), mtype, 1.0f, myaw);
                        }
                    }
                    lmNarNote(LN_MONO, wx, wz);
                    continue;
                }
            }

            u32 cellSeed = cloneLocked
                ? lmHash((u32)nk, (u32)rel, (u32)useG, 0x434Cu)
                : lmHash((u32)wx, (u32)wz, (u32)nk, 0x524Du);
            std::mt19937 rr(cellSeed);

            bool doMissingHost = !cloneLocked && ((int)((roomTypeH >> 8) % 240u) < lackChance);
            bool doFaceWall    = !cloneLocked && !doMissingHost && ((int)((roomTypeH >> 16) % 240u) < faceChance);
            bool isUncannyTwin = cloneLocked && ((roomInstH % 5u) == 1u) &&
                                 ((int)((roomInstH >> 8) % 220u) < twinChance) && (rel == 0x11 || rel == 0x01 || rel == 0x00);
            int twinMode = isUncannyTwin ? (int)((roomInstH >> 16) % 3u) : -1;
            if (twinMode == 0) doMissingHost = true;
            if (twinMode == 2) doFaceWall = true;
            if (isUncannyTwin) lmNarNote(LN_TWIN, wx, wz);
            else if (doMissingHost) lmNarNote(LN_LACK, wx, wz);
            else if (doFaceWall) lmNarNote(LN_FACEWALL, wx, wz);

            auto placeRoomWallUnit = [&](const RoomWallSlot& ws, int ptype, float slide, bool flipToWall) -> bool {
                if (ptype < 0) return false;
                float bx = 0.0f, bz = 0.0f;
                mapPropBaseExtents(ptype, bx, bz);
                float maxS = CS * 0.5f - bx - 0.24f;
                if (maxS < 0.0f) maxS = 0.0f;
                if (slide > maxS) slide = maxS;
                if (slide < -maxS) slide = -maxS;
                float reach = CS * 0.5f - bz - 0.04f - ws.extraInset;
                Vec3 c = cellCenter(wx, wz);
                Vec3 pos(c.x + (float)ws.dx * reach - (float)ws.dz * slide, 0.0f,
                         c.z + (float)ws.dz * reach + (float)ws.dx * slide);
                float yaw = flipToWall ? atan2f((float)ws.dx, (float)ws.dz) : atan2f((float)-ws.dx, (float)-ws.dz);
                if (!lmAddRoomPropSafe(pos, ptype, 1.0f, yaw)) return false;
                const PropCompanion* rule = propCompanionFor(ptype);
                if (rule && !flipToWall) {
                    MapProp host = mapProps.back();
                    float bhx = 0.0f, bhz = 0.0f;
                    mapPropBaseExtents(rule->buddy, bhx, bhz);
                    Vec3 front(sinf(yaw), 0.0f, cosf(yaw)), right(cosf(yaw), 0.0f, -sinf(yaw));
                    Vec3 bpos = (rule->mode == PROP_BUDDY_FRONT)
                        ? (host.pos + front * (bz + bhz + 0.16f))
                        : (host.pos + right * (bx + bhx + 0.14f));
                    float byaw = (rule->mode == PROP_BUDDY_FRONT) ? (yaw + 3.14159265f) : yaw;
                    lmAddRoomPropSafe(bpos, rule->buddy, 1.0f, byaw);
                }
                return true;
            };

            // 1. Primary & secondary wall suites (2..3 walls dressed with coordinated workstations/storage + companions)
            int startWall = (nSolid > 0) ? (int)(rr() % (u32)nSolid) : 0;
            for (int wi = 0; wi < nSolid && wi < 3; wi++) {
                if (wi == 1 && roomBudget < 80) break;
                if (wi == 2 && roomBudget < 145) break;
                const RoomWallSlot& ws = edges[solidIdx[(startWall + wi) % nSolid]];
                if (wi == 0 && doMissingHost) {
                    int sideT = pickThemedPropType(themes, rr(), 1u << PROP_PLACE_WALL);
                    placeRoomWallUnit(ws, sideT, 1.35f, false);
                    continue;
                }
                int wtypeA = (wi == 0) ? lmPickHostWithCompanion(themes, rr(), 1u << PROP_PLACE_WALL) : -1;
                if (wtypeA < 0) wtypeA = pickThemedPropType(themes, rr(), 1u << PROP_PLACE_WALL);
                int wtypeB = pickThemedPropType(themes, rr(), 1u << PROP_PLACE_WALL);
                if (wi == 0) {
                    placeRoomWallUnit(ws, wtypeA, -0.78f, doFaceWall);
                    if (roomBudget >= 105) placeRoomWallUnit(ws, wtypeB, 1.05f, doFaceWall);
                } else if (wi == 1) {
                    placeRoomWallUnit(ws, wtypeB, -0.65f, doFaceWall);
                    if (roomBudget >= 155) placeRoomWallUnit(ws, wtypeB, 0.85f, doFaceWall);
                } else {
                    placeRoomWallUnit(ws, wtypeB, 0.0f, doFaceWall);
                }
            }

            // 2. Center / floor furniture group (or excised-host companion ring / alien twin swap)
            Vec3 rc = cellCenter(wx, wz);
            for (int e = 0; e < 4; e++) {
                if (edges[e].hasDoor) { rc.x -= edges[e].dx * 0.42f; rc.z -= edges[e].dz * 0.42f; }
            }
            if (twinMode == 1) {
                unsigned alienTheme = lmSampleThemeArray(gLimenDNA.graftWeight, (roomInstH >> 18) % 1000u);
                int atype = pickThemedPropType(alienTheme, roomInstH >> 4, 1u << PROP_PLACE_FLOOR);
                lmAddRoomPropSafe(rc, atype, 1.0f, 3.14159265f);
            } else if (doMissingHost) {
                int hostT = lmPickHostWithCompanion(themes, rr(), 1u << PROP_PLACE_FLOOR);
                if (hostT < 0) hostT = lmPickHostWithCompanion(THEME_ALL, rr(), 1u << PROP_PLACE_FLOOR);
                const PropCompanion* rule = (hostT >= 0) ? propCompanionFor(hostT) : nullptr;
                if (rule) {
                    float hhx = 0.45f, hhz = 0.45f, bhx = 0.25f, bhz = 0.25f;
                    mapPropBaseExtents(hostT, hhx, hhz);
                    mapPropBaseExtents(rule->buddy, bhx, bhz);
                    float gyaw = (rr() & 1u) ? 0.0f : 1.57079633f;
                    Vec3 front(sinf(gyaw), 0.0f, cosf(gyaw)), right(cosf(gyaw), 0.0f, -sinf(gyaw));
                    float reach = hhz + bhz + 0.18f;
                    for (int sgn = -1; sgn <= 1; sgn += 2) {
                        lmAddRoomPropSafe(rc + front * reach + right * ((float)sgn * (bhx + 0.14f)), rule->buddy, 1.0f, gyaw + 3.14159265f);
                        lmAddRoomPropSafe(rc - front * reach + right * ((float)sgn * (bhx + 0.14f)), rule->buddy, 1.0f, gyaw);
                    }
                }
                int flatT = pickThemedPropType(themes | THEME_ALL, rr(), 1u << PROP_PLACE_FLAT);
                if (flatT >= 0) lmAddRoomPropSafe(rc, flatT, 1.0f, 0.0f);
            } else if (roomBudget >= 88) {
                int ftype = lmPickHostWithCompanion(themes, rr(), 1u << PROP_PLACE_FLOOR);
                if (ftype < 0 || (rr() % 3u) == 0u) ftype = pickThemedPropType(themes, rr(), 1u << PROP_PLACE_FLOOR);
                if (ftype >= 0) {
                    float fyaw = ((rr() & 1u) ? 0.0f : 1.57079633f) + (doFaceWall ? 3.14159265f : 0.0f);
                    if (lmAddRoomPropSafe(rc, ftype, 1.0f, fyaw)) {
                        const PropCompanion* rule = propCompanionFor(ftype);
                        if (rule && !doFaceWall) {
                            float fhx = 0.0f, fhz = 0.0f, bhx = 0.0f, bhz = 0.0f;
                            mapPropBaseExtents(ftype, fhx, fhz);
                            mapPropBaseExtents(rule->buddy, bhx, bhz);
                            Vec3 front(sinf(fyaw), 0.0f, cosf(fyaw));
                            lmAddRoomPropSafe(rc + front * (fhz + bhz + 0.16f), rule->buddy, 1.0f, fyaw + 3.14159265f);
                            if (rule->mode == PROP_BUDDY_FRONT && fhx >= 0.65f) {
                                lmAddRoomPropSafe(rc - front * (fhz + bhz + 0.16f), rule->buddy, 1.0f, fyaw);
                            }
                        }
                    }
                }
                if ((int)(rr() % 256u) < roomBudget) {
                    int flatT = pickThemedPropType(themes, rr(), 1u << PROP_PLACE_FLAT);
                    if (flatT >= 0) lmAddRoomPropSafe(cellCenter(wx, wz), flatT, 1.0f, (rr() & 1u) ? 0.0f : 1.57079633f);
                }
            }

            // 3. High-wall & ceiling room fixtures (clocks, wall TVs, PA speakers, pipes, fans)
            if (nSolid > 0 && lmCanHangHighWallPropAt(wx, wz) && (int)(rr() % 256u) < roomBudget) {
                const RoomWallSlot& hws = edges[solidIdx[(startWall + 1) % nSolid]];
                int hwType = pickThemedPropType(themes, rr(), 1u << PROP_PLACE_WALL_HIGH);
                if (hwType >= 0) {
                    float bx = 0.0f, bz = 0.0f;
                    mapPropBaseExtents(hwType, bx, bz);
                    float reach = CS * 0.5f - bz - 0.03f - hws.extraInset;
                    Vec3 c = cellCenter(wx, wz);
                    Vec3 pos(c.x + (float)hws.dx * reach, 0.0f, c.z + (float)hws.dz * reach);
                    addMapProp(pos, hwType, 1.0f, atan2f((float)-hws.dx, (float)-hws.dz));
                }
            }
            if (lmCanHangCeilingPropAt(wx, wz) && (int)(rr() % 256u) < roomBudget * 3 / 5) {
                int cType = pickThemedPropType(themes, rr(), 1u << PROP_PLACE_CEILING);
                if (cType >= 0) placeProp(wx, wz, cType, rr);
            }
        }
    }
}

inline void lmSpawnThemedPoi(const Chunk& c, std::mt19937& r) {
    if ((int)(r() % 100u) >= (int)gLimenDNA.poiChance) return;
    const auto& tw = gLimenDNA.themeWeight;
    int wOffice   = 4 + tw[0] * 2 + tw[4] + tw[7];
    int wServer   = 4 + tw[11] * 2 + tw[2] + tw[10] + tw[0];
    int wStorage  = 4 + tw[3] * 2 + tw[1] + tw[2] + tw[8];
    int wRestroom = 4 + tw[5] * 2 + tw[6] * 2 + tw[12] + tw[9] + tw[4];
    int wTot = wOffice + wServer + wStorage + wRestroom;

    for (int tries = 0; tries < 16; tries++) {
        int lx = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int lz = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
        if (!lmAllowPropAt(wx, wz, true)) continue;
        if (countWallNeighborsWorld(wx, wz) != 1) continue;
        if (lmFlagAt(wx, wz) & LF_CRAMPED) continue;

        int roll = (int)(r() % (u32)wTot);
        int type = MAP_POI_RESTROOM;
        if (roll < wOffice) type = MAP_POI_OFFICE;
        else if (roll < wOffice + wServer) type = MAP_POI_SERVER;
        else if (roll < wOffice + wServer + wStorage) type = MAP_POI_STORAGE;

        MapPoi poi{};
        poi.pos = cellCenter(wx, wz);
        poi.type = type;
        poi.radius = CS * 1.8f;
        poi.id = (c.cx * 73856093) ^ (c.cz * 19349663) ^ (lx * 83492791) ^ (lz * 297121507);
        mapPois.push_back(poi);
        return;
    }
}

// True when a slab's open hall is intentionally left stark/monumental (never "3 stray crates on 2000 m2").
inline bool lmSlabHallIsBare(const LimenSlab& s) {
    int useG = (int)s.genome.g[LG_USE];
    int furnishDrive = ((int)gLimenDNA.propDensity * 2 + useG * 2) / 4;
    int barePct = lmClampInt(54 - furnishDrive / 4, 12, 60);
    u32 h = lmHash((u32)s.rect.j, (u32)s.rect.i, 37u, 0x4842u);
    return (int)(h % 100u) < barePct;
}

// Catalog query: picks a substantial floor prop (hx >= 0.34 m) suitable for structured depot/island quads.
inline int lmPickStructuredIslandProp(unsigned themes, u32 roll) {
    int cands[48], weights[48], n = 0, wSum = 0;
    for (int i = 0; i < MAP_PROP_TYPE_COUNT && n < 48; i++) {
        const PropInfo& p = PROP_CATALOG[i];
        if (p.placement != PROP_PLACE_FLOOR || (p.themes & themes) == 0) continue;
        if (p.hx < 0.34f || p.hz < 0.24f || p.hx > 1.15f || p.hz > 1.15f) continue;
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
