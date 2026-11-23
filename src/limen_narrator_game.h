#pragma once
// The narrator in the game: where it learns the rig's lane, how it watches the player, and how
// a slab it rewrote behind the player's back reaches the meshes, the colliders and the
// furniture. Included after game.cpp and limen_mega.h.
//
//   lmNarRigHook      the moment the game has put the rig down (start cell and heading are the
//                     whole lane): the level is planned along it before anything is placed
//                     on it and before anybody sees it. Runs on every peer with the same result.
//   lmNarFrame        once a frame: the player's eyes, the pacing, and (single player) the
//                     slabs ahead that nobody has seen yet.
//   lmNarApplyDirty   pushes rewritten slabs into the game's chunks, lamps, props and meshes.

// Anything of the game's inside these world cells? Such a slab is left as it is.
inline bool lmNarBusy(int x0, int z0, int x1, int z1) {
    auto in = [&](const Vec3& p) {
        int wx = (int)floorf(p.x / CS), wz = (int)floorf(p.z / CS);
        return wx >= x0 - 1 && wx <= x1 + 1 && wz >= z0 - 1 && wz <= z1 + 1;
    };
    for (const auto& it : worldItems) if (it.active && in(it.pos)) return true;
    for (const auto& e : entityMgr.entities) if (in(e.pos)) return true;
    for (const auto& n : storyMgr.notes) if (n.active && !n.collected && in(n.pos)) return true;
    for (const auto& h : floorHoles) if (h.wx >= x0 - 1 && h.wx <= x1 + 1 && h.wz >= z0 - 1 && h.wz <= z1 + 1) return true;
    if (abyss.active && abyss.centerX + abyss.radius >= x0 - 1 && abyss.centerX - abyss.radius <= x1 + 1 &&
        abyss.centerZ + abyss.radius >= z0 - 1 && abyss.centerZ - abyss.radius <= z1 + 1) return true;
    if (coop.initialized) {
        if (in(heist.drillPos) || in(heist.rigStart)) return true;
        if (heist.hasKiosk && in(heist.kioskPos)) return true;
        if (heist.hasGarage) {
            if (in(heist.portalPos) || in(heist.corePos)) return true;
            for (int k = 0; k < HEIST_TUTORIAL_LOOT; k++) if (in(heist.lessonPos[k])) return true;
        }
        // the stretch of lane the rig will bore within the next few minutes
        for (int step = -2; step <= rigFrontStep(rigTravelled(), CS) + 24; step++) {
            int wx = 0, wz = 0;
            rigLaneCell(step, wx, wz);
            if (wx >= x0 && wx <= x1 && wz >= z0 && wz <= z1) return true;
        }
    }
    for (const auto& kv : gCellOverrides) {
        int wx = (int)(kv.first >> 32), wz = (int)(kv.first & 0xFFFFFFFF);
        if (wx >= x0 && wx <= x1 && wz >= z0 && wz <= z1) return true;
    }
    return false;
}

inline void lmNarRefreshCells(int x0, int z0, int x1, int z1) {
    for (auto& p : chunks) {
        Chunk& c = p.second;
        int bx = c.cx * CHUNK_SIZE, bz = c.cz * CHUNK_SIZE;
        if (bx > x1 || bx + CHUNK_SIZE <= x0 || bz > z1 || bz + CHUNK_SIZE <= z0) continue;
        for (int lx = 0; lx < CHUNK_SIZE; lx++) for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int wx = bx + lx, wz = bz + lz;
            if (wx < x0 || wx > x1 || wz < z0 || wz > z1) continue;
            auto ov = gCellOverrides.find(cellKey(wx, wz));
            c.cells[lx][lz] = (ov != gCellOverrides.end()) ? ov->second : lmCell(wx, wz);
        }
    }
}

inline std::vector<long long> gLmNarRemesh;      // slabs beside a rewritten one: see lmNarRemeshStep
inline bool gLmNarRemeshRegather = false;

// The whole world changed and nobody has seen it yet (level start).
inline void lmNarRebuildWorld() {
    gLmStoryEpoch++;
    gLmStorySlabEpoch.clear();
    lmSlabCacheClear();
    for (auto& p : chunks) {
        Chunk& c = p.second;
        for (int lx = 0; lx < CHUNK_SIZE; lx++) for (int lz = 0; lz < CHUNK_SIZE; lz++) {
            int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
            auto ov = gCellOverrides.find(cellKey(wx, wz));
            c.cells[lx][lz] = (ov != gCellOverrides.end()) ? ov->second : lmCell(wx, wz);
        }
    }
    resetMapContent();
    gNar.dirty.clear();
    gLmNarRemesh.clear(); gLmNarRemeshRegather = false;
}

inline bool gLmNarBooted = false;
inline void lmNarBoot() {
    if (gLmNarBooted) return;
    gLmNarBooted = true;
    gNar.busy = lmNarBusy;
#ifdef LIMEN_SHOTS
    // the frame grabber and the audits want the bare generator unless told otherwise
    gNar.enabled = getenv("LM_NARRATOR") != nullptr && atoi(getenv("LM_NARRATOR")) != 0;
    gNar.adaptive = gNar.enabled && atoi(getenv("LM_NARRATOR")) != 2;      // 2: script mode, as in co-op
    gNar.savePath.clear();
    if (getenv("LM_NAR_WISH")) gNar.wish = atoi(getenv("LM_NAR_WISH"));     // LmFeat index to show first
    if (getenv("LM_NAR_ARC")) gNar.arcPin = atoi(getenv("LM_NAR_ARC"));
#else
    gNar.savePath = "limen_narrator.sav";
    lmNarLoad();
#endif
}

// Called from inside the game's rigPlace() (see the rigFaultDelay shim in game.cpp).
void lmNarRigHook() {
    if (!coop.initialized) return;
    lmNarBoot();
    lmReset(worldSeed, gCurrentLevel);
    int sx = (int)floorf(heist.rigStart.x / CS), sz = (int)floorf(heist.rigStart.z / CS);
    int dx = rigDirX(heist.rigDir), dz = rigDirZ(heist.rigDir);
#ifndef LIMEN_SHOTS
    gNar.adaptive = (multiState != MULTI_IN_GAME);
#endif
    bool fresh = lmNarSync();
    if (!fresh && lmNarRoadIs(sx, sz, dx, dz)) return;     // a repair asking for the next fault, not a new lane
    if (!fresh && gNar.adaptive) { lmNarSetRoad(sx, sz, dx, dz); return; }   // mid-level: what was seen stays
    if (!fresh) {
        // script mode has to match the host's lane exactly: lay the level out again
        gLmBeats.clear(); gLmPockets.clear();
        gNar.slabs.clear(); gNar.placed.clear();
        memset(gNar.levelPlaced, 0, sizeof(gNar.levelPlaced));
        gNar.levelWitnessed = 0;     // (what this peer happened to see must not colour a plan all peers share)
    }
    lmNarSetRoad(sx, sz, dx, dz);
    if (!gNar.enabled) return;
    lmNarPlanRoad();
#ifdef LIMEN_SHOTS
    printf("NARRATOR L%d seed %u (%s): road from cell (%d,%d) heading (%d,%d)\n", gCurrentLevel, worldSeed, gNar.adaptive ? "adaptive" : "script", sx, sz, dx, dz);
    for (const LmNarPlaced& p : gNar.placed) {
        LimenRect r = lmSlabRect((int)(p.slab >> 32), (int)(p.slab & 0xFFFFFFFFll));
        printf("   %-16s slab (%d,%d) cells x %d..%d z %d..%d\n", lmFeatInfo(p.feat).name, r.j, r.i, r.x0, r.x1, r.z0, r.z1);
    }
#endif
    lmNarRebuildWorld();
}

// A slab's cached mesh is not made from that slab alone: its outermost cells are modelled
// against what stands across the seam (wall faces, door frames, ceiling steps, the skirt of a
// vault). So when a slab is rewritten, the meshes of the slabs around it are stale along that
// seam even though nothing inside them changed: pieces of the old neighbour stay standing, cut
// off from the new one. They are modelled again, one a frame so that a rewrite does not hitch.
inline bool lmNarRemeshStep() {
    while (!gLmNarRemesh.empty()) {
        long long key = gLmNarRemesh.back();
        gLmNarRemesh.pop_back();
        auto it = gLmSlabMeshes.find(key);
        if (it == gLmSlabMeshes.end() || it->second.builtSeed != worldSeed || it->second.builtLevel != gCurrentLevel) continue;   // never meshed
        if (it->second.storyEpoch != lmStoryEpochOf(key)) continue;       // rewritten itself: meshed anew when next in view
        LmCachedSlabMesh& cm = it->second;
        std::vector<float> lv0 = cm.lv, off0 = cm.lvOff;
        size_t cols0 = cm.cols.size();
        lmBuildSlabMesh(lmSlab((int)(key >> 32), (int)(key & 0xFFFFFFFFll)), cm);
        // lamp quads and colliders of every slab in view live in shared buffers
        if (cm.lv != lv0 || cm.lvOff != off0 || cm.cols.size() != cols0) gLmNarRemeshRegather = true;
        return true;
    }
    if (gLmNarRemeshRegather) { gLmNarRemeshRegather = false; buildGeom(); return true; }
    return false;
}

// Rewritten slabs: cells, lamps, furniture and meshes follow. Only what lies inside a
// rewritten slab is replaced; everything else in the chunks it shares stays exactly as it was.
inline void lmNarApplyDirty() {
    if (gNar.dirty.empty()) return;
    std::vector<LimenRect> rects;
    for (long long key : gNar.dirty) rects.push_back(lmSlabRect((int)(key >> 32), (int)(key & 0xFFFFFFFFll)));
    for (long long key : gNar.dirty) {
        long long nb[16];
        int n = lmNarNeighbours((int)(key >> 32), (int)(key & 0xFFFFFFFFll), nb, 16);
        for (int k = 0; k < n; k++) {
            bool skip = false;
            for (long long d : gNar.dirty) if (d == nb[k]) skip = true;        // rebuilt anyway
            for (long long q : gLmNarRemesh) if (q == nb[k]) skip = true;
            if (!skip) gLmNarRemesh.push_back(nb[k]);
        }
    }
    gNar.dirty.clear();
    auto inDirty = [&](float x, float z) {
        int wx = (int)floorf(x / CS), wz = (int)floorf(z / CS);
        for (const LimenRect& r : rects) if (wx >= r.x0 && wx < r.x1 && wz >= r.z0 && wz < r.z1) return true;
        return false;
    };
    for (const LimenRect& r : rects) lmNarRefreshCells(r.x0, r.z0, r.x1, r.z1);
    // A furniture mesh is cached by the furniture it was modelled from, but a model also reads the
    // cell it stands in (a tall thing is cut to the ceiling). The same chair under a ceiling that a
    // rewrite moved hashes the same and would keep its old shape: tiles over rewritten slabs are
    // modelled again whatever their furniture.
    for (auto& kv : gLmChunkPropMeshes) {
        LmChunkPropMesh& m = kv.second;
        int x0 = m.tx * LM_PROP_TILE, z0 = m.tz * LM_PROP_TILE;
        for (const LimenRect& r : rects)
            if (x0 <= r.x1 + 1 && x0 + LM_PROP_TILE >= r.x0 - 1 && z0 <= r.z1 + 1 && z0 + LM_PROP_TILE >= r.z0 - 1) { m.built = false; break; }
    }
    mapProps.erase(std::remove_if(mapProps.begin(), mapProps.end(), [&](const MapProp& p) { return inDirty(p.pos.x, p.pos.z); }), mapProps.end());
    mapPois.erase(std::remove_if(mapPois.begin(), mapPois.end(), [&](const MapPoi& p) { return inDirty(p.pos.x, p.pos.z); }), mapPois.end());
    for (long long ck : std::vector<long long>(mapContentChunks.begin(), mapContentChunks.end())) {
        auto it = chunks.find(ck);
        if (it == chunks.end()) continue;
        const Chunk& c = it->second;
        int bx = c.cx * CHUNK_SIZE, bz = c.cz * CHUNK_SIZE;
        bool hit = false;
        for (const LimenRect& r : rects) if (!(bx >= r.x1 || bx + CHUNK_SIZE <= r.x0 || bz >= r.z1 || bz + CHUNK_SIZE <= r.z0)) hit = true;
        if (!hit) continue;
        size_t p0 = mapProps.size(), q0 = mapPois.size();
        lmRebuildChunkContent(c);
        // the furnisher did the whole chunk again: keep only what landed in the rewritten slabs
        size_t w = p0;
        for (size_t k = p0; k < mapProps.size(); k++) if (inDirty(mapProps[k].pos.x, mapProps[k].pos.z)) mapProps[w++] = mapProps[k];
        mapProps.resize(w);
        w = q0;
        for (size_t k = q0; k < mapPois.size(); k++) if (inDirty(mapPois[k].pos.x, mapPois[k].pos.z)) mapPois[w++] = mapPois[k];
        mapPois.resize(w);
    }
    updateLightsAndPillars(playerChunkX, playerChunkZ);
    buildGeom();
}

inline void lmNarFrame() {
    lmNarBoot();
    if (gameState != STATE_GAME || isPlayerDead || playerEscaped) return;
    lmReset(worldSeed, gCurrentLevel);
#ifndef LIMEN_SHOTS
    gNar.adaptive = (multiState != MULTI_IN_GAME);
#endif
    if (lmNarTick(dTime, cam.pos.x, cam.pos.z, cam.yaw, CS, 1.10f) > 0) lmNarApplyDirty();
    else lmNarRemeshStep();
}
