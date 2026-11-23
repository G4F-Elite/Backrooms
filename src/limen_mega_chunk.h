#pragma once
// Part of limen_mega.h: slab mesh upkeep, drunk doorways, chunk prop meshes, post-build hook.

inline const LmCachedSlabMesh& lmEnsureSlabMesh(int j, int i) {
    long long key = (long long)(((unsigned long long)(u32)j << 32) | (unsigned long long)(u32)i);
    auto it = gLmSlabMeshes.find(key);
    if (it != gLmSlabMeshes.end() &&
        it->second.builtSeed == worldSeed &&
        it->second.builtLevel == gCurrentLevel) {
        // A lamp dying behind the player's back bumps the epoch for the whole world; only the
        // slab that owns the lamp needs a new mesh.
        bool current = (it->second.builtEpoch == gLmSiteEpoch);
        if (it->second.storyEpoch != lmStoryEpochOf(key)) current = false;   // the game edited this slab
        else
        if (!current && it->second.siteSig == lmSlabSiteSignature(lmSlab(j, i))) {
            it->second.builtEpoch = gLmSiteEpoch;
            current = true;
        }
#ifdef LIMEN_SHOTS
        if (current && gLmBenchHashOn) {
            unsigned long long before = it->second.vertHash;
            lmBuildSlabMesh(lmSlab(j, i), it->second);
            if (it->second.vertHash != before) { gLmBenchSlabStale++; if (gLmBenchStaleFn) gLmBenchStaleFn(j, i); }
        }
#endif
        if (current) {
            it->second.lastUsed = ++gLmMeshUseClock;
            return it->second;
        }
    }
#ifdef LIMEN_SHOTS
    size_t kMaxCachedSlabMeshes = 96;
#else
    size_t kMaxCachedSlabMeshes = 30;
#endif
    // never so small that slabs in view (plus the prewarmed ring) evict one another
    if (kMaxCachedSlabMeshes < gLmSlabMeshesWanted) kMaxCachedSlabMeshes = gLmSlabMeshesWanted;
    if (gLmSlabMeshes.size() >= kMaxCachedSlabMeshes && it == gLmSlabMeshes.end()) {
        long long victim = 0;
        unsigned long long oldest = ~0ull;
        for (const auto& kv : gLmSlabMeshes) {
            if (kv.second.lastUsed < oldest) { oldest = kv.second.lastUsed; victim = kv.first; }
        }
        gLmSlabMeshes[victim].destroyGL();
        gLmSlabMeshes.erase(victim);
    }
    const LimenSlab& s = lmSlab(j, i);
    LmCachedSlabMesh& cm = gLmSlabMeshes[key];
    cm.lastUsed = ++gLmMeshUseClock;
    lmBuildSlabMesh(s, cm);
    return cm;
}

// Emits a 0.26 m thin Backrooms drywall partition (solid or with a framed 1.36 m doorway).
inline void lmEmitThinPartition(std::vector<float>& wv, std::vector<float>& dv,
                                float x0, float z0, bool alongZ, bool hasDoor, float cH, float yBot) {
    const float ht = LM_THIN_HALF, dh = LM_THIN_DOOR_HALF, doorH = 2.26f;
    if (alongZ) {
        // Partition on West edge x = x0, spanning z in [z0, z0 + CS]
        if (!hasDoor) {
            lmPushWallSpan(wv, x0 + ht, yBot, z0, 0.0f, CS, cH);
            lmPushWallSpan(wv, x0 - ht, yBot, z0 + CS, 0.0f, -CS, cH);
        } else {
            float seg = CS * 0.5f - dh;
            lmPushWallSpan(wv, x0 + ht, yBot, z0, 0.0f, seg, cH);
            lmPushWallSpan(wv, x0 - ht, yBot, z0 + seg, 0.0f, -seg, cH);
            lmPushWallSpan(wv, x0 + ht, yBot, z0 + CS - seg, 0.0f, seg, cH);
            lmPushWallSpan(wv, x0 - ht, yBot, z0 + CS, 0.0f, -seg, cH);
            lmPushWallSpan(wv, x0 + ht, doorH, z0 + seg, 0.0f, dh * 2.0f, cH);
            lmPushWallSpan(wv, x0 - ht, doorH, z0 + CS - seg, 0.0f, -dh * 2.0f, cH);
            DecorBuilder b(dv, x0, z0 + CS * 0.5f, 1.57079633f, 1.0f);
            b.box(-dh, 0.0f, 0.0f, 0.12f, doorH, ht * 2.3f, DECOR_MAT_TRIM);
            b.box( dh, 0.0f, 0.0f, 0.12f, doorH, ht * 2.3f, DECOR_MAT_TRIM);
            b.box(0.0f, doorH, 0.0f, dh * 2.0f + 0.12f, 0.12f, ht * 2.3f, DECOR_MAT_TRIM);
        }
    } else {
        // Partition on North edge z = z0, spanning x in [x0, x0 + CS]
        if (!hasDoor) {
            lmPushWallSpan(wv, x0 + CS, yBot, z0 + ht, -CS, 0.0f, cH);
            lmPushWallSpan(wv, x0, yBot, z0 - ht, CS, 0.0f, cH);
        } else {
            float seg = CS * 0.5f - dh;
            lmPushWallSpan(wv, x0 + seg, yBot, z0 + ht, -seg, 0.0f, cH);
            lmPushWallSpan(wv, x0, yBot, z0 - ht, seg, 0.0f, cH);
            lmPushWallSpan(wv, x0 + CS, yBot, z0 + ht, -seg, 0.0f, cH);
            lmPushWallSpan(wv, x0 + CS - seg, yBot, z0 - ht, seg, 0.0f, cH);
            lmPushWallSpan(wv, x0 + CS - seg, doorH, z0 + ht, -dh * 2.0f, 0.0f, cH);
            lmPushWallSpan(wv, x0 + seg, doorH, z0 - ht, dh * 2.0f, 0.0f, cH);
            DecorBuilder b(dv, x0 + CS * 0.5f, z0, 0.0f, 1.0f);
            b.box(-dh, 0.0f, 0.0f, 0.12f, doorH, ht * 2.3f, DECOR_MAT_TRIM);
            b.box( dh, 0.0f, 0.0f, 0.12f, doorH, ht * 2.3f, DECOR_MAT_TRIM);
            b.box(0.0f, doorH, 0.0f, dh * 2.0f + 0.12f, 0.12f, ht * 2.3f, DECOR_MAT_TRIM);
        }
    }
}

// Emits 2.50 m human-scale narrowed corridor solid wall wings (watertight from every angle).
inline void lmEmitNarrowWallBox(std::vector<float>& wv, float x0, float y0, float z0, float x1, float y1, float z1) {
    lmPushWallSpan(wv, x1, y0, z1, x0 - x1, 0.0f, y1);
    lmPushWallSpan(wv, x0, y0, z0, x1 - x0, 0.0f, y1);
    lmPushWallSpan(wv, x1, y0, z0, 0.0f, z1 - z0, y1);
    lmPushWallSpan(wv, x0, y0, z1, 0.0f, z0 - z1, y1);
}

inline void lmEmitNarrowCorridor(std::vector<float>& wv, int wx, int wz, float px, float pz,
                                 unsigned short flg, float cH, float yBot) {
    const float in = LM_NARROW_INSET;
    if (flg & LF_NARROW_X) {
        lmEmitNarrowWallBox(wv, px, yBot, pz, px + CS, cH, pz + in);
        lmEmitNarrowWallBox(wv, px, yBot, pz + CS - in, px + CS, cH, pz + CS);
    }
    if (flg & LF_NARROW_Z) {
        lmEmitNarrowWallBox(wv, px, yBot, pz, px + in, cH, pz + CS);
        lmEmitNarrowWallBox(wv, px + CS - in, yBot, pz, px + CS, cH, pz + CS);
    }
}

// Watertight cell-corner ceiling pitch/sag offset in drunken 1-storey pockets (-1.25 m .. +0.75 m).
inline float lmDrunkVertexCeilOffset(int vx, int vz) {
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
            if (s.zone[slx][slz] & LZ_STORY) return 0.0f;
        }
    }
    float amt = (float)(d - 72) / 183.0f;
    float tilt = sinf((float)vx * 0.61f - (float)vz * 0.79f) * 0.95f - 0.25f;
    float res = tilt * amt;
    return res < -1.25f ? -1.25f : (res > 0.75f ? 0.75f : res);
}

// Height-fitted framed doorway (handles 2.72 m / 3.25 m / 9.00 m ceilings as well as warped pockets).
inline void lmBuildDrunkDoorway(std::vector<float>& v, int wx, int wz, const LmDoorway& dw, float yTop) {
    int axis = dw.axis;
    float openHalf = dw.openHalf, openH = dw.openH, yBot = dw.yBot;
    float postW    = dw.postW, fillT = dw.drunk ? 0.34f : doorwayDims().fillT, frameT = dw.frameT;
    float yaw = (axis == 1) ? 0.0f : 1.5707963f;
    DecorBuilder b(v, ((float)wx + 0.5f) * CS, ((float)wz + 0.5f) * CS, yaw, 1.0f);
    float half = CS * 0.5f, outer = openHalf + postW * 0.5f;
    float sideW = half - outer, sideC = (half + outer) * 0.5f;
    float lintelTop = yBot + openH + postW;
    float baseBot = dw.drunk ? (yBot - 0.35f) : 0.0f;
    float trimBot = dw.drunk ? (yBot - 0.10f) : 0.0f;
    b.box(-sideC, baseBot, 0.0f, sideW, yTop - baseBot, fillT, DECOR_MAT_PLASTER);
    b.box( sideC, baseBot, 0.0f, sideW, yTop - baseBot, fillT, DECOR_MAT_PLASTER);
    if (yTop > lintelTop) b.box(0.0f, lintelTop, 0.0f, outer * 2.0f, yTop - lintelTop, fillT, DECOR_MAT_PLASTER);
    b.box(-openHalf, trimBot, 0.0f, postW, (yBot + openH) - trimBot, frameT, DECOR_MAT_TRIM);
    b.box( openHalf, trimBot, 0.0f, postW, (yBot + openH) - trimBot, frameT, DECOR_MAT_TRIM);
    b.box(0.0f, yBot + openH, 0.0f, outer * 2.0f, postW, frameT, DECOR_MAT_TRIM);
}

inline void lmPushLeaningPillar(std::vector<float>& pv, float cx, float cz, float leanX, float leanZ, float sz, float y0, float y1) {
    float hs = sz * 0.5f;
    Vec3 b00(cx-hs, y0, cz-hs), b10(cx+hs, y0, cz-hs), b11(cx+hs, y0, cz+hs), b01(cx-hs, y0, cz+hs);
    Vec3 t00(cx+leanX-hs, y1, cz+leanZ-hs), t10(cx+leanX+hs, y1, cz+leanZ-hs);
    Vec3 t11(cx+leanX+hs, y1, cz+leanZ+hs), t01(cx+leanX-hs, y1, cz+leanZ+hs);
    lmPushQuad(pv, b01, b11, t11, t01, CS);
    lmPushQuad(pv, b10, b00, t00, t10, CS);
    lmPushQuad(pv, b11, b10, t10, t11, CS);
    lmPushQuad(pv, b00, b01, t01, t00, CS);
}

// ============================================================================
// PROP MESH CACHE
// Props used to be re-modelled into one ~350 MB decor buffer and re-uploaded on every
// `buildGeom` (each chunk crossing, each lamp event). Each 8x8-cell tile (a quarter chunk) now
// keeps its own VAO, rebuilt only when the props standing in it change, and drawn with the
// same culling as slab meshes.
// ============================================================================
const int LM_PROP_TILE = 8;   // cells; CHUNK_SIZE is a multiple of it
struct LmChunkPropMesh {
    int tx = 0, tz = 0;   // tile coordinates
    unsigned long long hash = 0;       // of the props this mesh was modelled from
    unsigned long long vertHash = 0;   // bench self-check only
    unsigned int seenSerial = 0;
    bool built = false;
    GLuint vao = 0, vbo = 0;
    int vc = 0;
};
inline std::unordered_map<long long, LmChunkPropMesh> gLmChunkPropMeshes;
inline unsigned int gLmChunkPropSerial = 0;

enum { LM_PROPSYNC_ALL = 0, LM_PROPSYNC_NEAR = 1, LM_PROPSYNC_SLICE = 2 };
const double LM_PROPSYNC_SLICE_MS = 2.0;   // per-frame budget while catching up
inline void lmAppendPropModel(std::vector<float>& dv, const MapProp& pr) {
    size_t v0 = dv.size();
    buildMapPropModel(dv, pr);
    int pwx = (int)floorf(pr.pos.x / CS), pwz = (int)floorf(pr.pos.z / CS);
    int pDel = lmDeliriumCell255(pwx, pwz);
    if (pDel >= 55) {
        float yOff = lmGroundHeightAt(pr.pos.x, pr.pos.z, CS);
        if (fabsf(yOff) > 0.002f) {
            for (size_t vi = v0 + 1; vi < dv.size(); vi += 8) dv[vi] += yOff;
        }
    }
    // Giant furniture is pressed under the ceiling it does not fit beneath.
    if (pr.scale > 1.3f && lmHasQuirk(gLimenDNA, LQ_GIANT) && !lmHighAt(pwx, pwz)) {
        float top = 0.0f;
        for (size_t vi = v0 + 1; vi < dv.size(); vi += 8) if (dv[vi] > top) top = dv[vi];
        float room = lmShellCeilHeight(lmFlagAt(pwx, pwz), 1, WH, lmZoneAt(pwx, pwz)) - 0.04f;
        if (lmHasQuirk(gLimenDNA, LQ_SKY) && (lmFlagAt(pwx, pwz) & LF_CRAMPED) == 0) room = 1e9f;
        if (top > room) {
            float k = room / top;
            for (size_t vi = v0 + 1; vi < dv.size(); vi += 8) dv[vi] *= k;
        }
    }
}

// A POI set is placed against whichever wall `mapPoiAnchor` finds, so the anchor and yaw are
// part of what its mesh was modelled from.
inline void lmAppendPoiModel(std::vector<float>& dv, const MapPoi& poi) {
    size_t v0 = dv.size();
    buildMapPoiModel(dv, poi);
    float yOff = lmGroundHeightAt(poi.pos.x, poi.pos.z, CS);
    if (fabsf(yOff) > 0.002f) {
        for (size_t vi = v0 + 1; vi < dv.size(); vi += 8) dv[vi] += yOff;
    }
}

inline void lmDropChunkPropMeshes() {
    for (auto& kv : gLmChunkPropMeshes) {
        if (kv.second.vao) glDeleteVertexArrays(1, &kv.second.vao);
        if (kv.second.vbo) glDeleteBuffers(1, &kv.second.vbo);
    }
    gLmChunkPropMeshes.clear();
}

// Brings the prop meshes in line with `mapProps` / `mapPois`; only tiles whose props changed
// are modelled and uploaded again.
//   LM_PROPSYNC_ALL    everything now (level load, bench verification)
//   LM_PROPSYNC_NEAR   the 3x3 chunks around the player now, the rest left pending
//   LM_PROPSYNC_SLICE  nearest pending tiles until the per-frame budget is spent
// New chunks enter the view 160+ m out, so the few frames their furniture takes to appear
// are hidden by fog.
inline void lmSyncChunkPropMeshes(int mode) {
#ifdef LIMEN_SHOTS
    if (gLmBenchHashOn) mode = LM_PROPSYNC_ALL;
#endif
    static unsigned int syncedSeed = 0;
    static int syncedLevel = -999999999;
    if (syncedSeed != worldSeed || syncedLevel != gCurrentLevel) {
        lmDropChunkPropMeshes();   // another level: nothing of the old one may linger while streaming
        syncedSeed = worldSeed; syncedLevel = gCurrentLevel;
    }
    if (gLmChunkPropMeshes.empty() || !gLmStreaming) mode = LM_PROPSYNC_ALL;
    auto tStart = std::chrono::high_resolution_clock::now();
    unsigned int serial = ++gLmChunkPropSerial;
    // per chunk: indices into mapProps, and (negative, -1 - index) into mapPois
    static std::unordered_map<long long, std::vector<int>> buckets;
    for (auto& kv : buckets) kv.second.clear();
    const float tileM = CS * LM_PROP_TILE;
    const int tilesPerChunk = CHUNK_SIZE / LM_PROP_TILE;
    for (int k = 0; k < (int)mapProps.size(); k++) {
        int tx = (int)floorf(mapProps[k].pos.x / tileM), tz = (int)floorf(mapProps[k].pos.z / tileM);
        buckets[chunkKey(tx, tz)].push_back(k);
    }
    for (int k = 0; k < (int)mapPois.size(); k++) {
        int tx = (int)floorf(mapPois[k].pos.x / tileM), tz = (int)floorf(mapPois[k].pos.z / tileM);
        buckets[chunkKey(tx, tz)].push_back(-1 - k);
    }
    static std::vector<float> verts;
    struct Todo { int dist; long long key; };
    static std::vector<Todo> order;
    order.clear();
    for (auto it = buckets.begin(); it != buckets.end();) {
        if (it->second.empty()) { it = buckets.erase(it); continue; }
        int tx = (int)(it->first >> 32), tz = (int)(it->first & 0xFFFFFFFF);
        int cx = (int)floorf((float)tx / (float)tilesPerChunk), cz = (int)floorf((float)tz / (float)tilesPerChunk);
        int ax = abs(cx - playerChunkX), az = abs(cz - playerChunkZ);
        order.push_back({ax > az ? ax : az, it->first});
        ++it;
    }
    std::sort(order.begin(), order.end(), [](const Todo& a, const Todo& b) {
        return a.dist != b.dist ? a.dist < b.dist : a.key < b.key;
    });
    gLmChunkPropPending = false;
    bool budgetSpent = false;
    for (const Todo& todo : order) {
        auto it = buckets.find(todo.key);
        const std::vector<int>& idx = it->second;
        unsigned long long h = lmFnv64(&worldSeed, sizeof(worldSeed));
        h = lmFnv64(&gCurrentLevel, sizeof(gCurrentLevel), h);
        for (int k : idx) {
            if (k < 0) {
                const MapPoi& poi = mapPois[(size_t)(-1 - k)];
                Vec3 anchor = mapPoiAnchor(poi);
                float yaw = mapPoiYaw(poi);
                h = lmFnv64(&poi.pos, sizeof(poi.pos), h);
                h = lmFnv64(&poi.type, sizeof(poi.type), h);
                h = lmFnv64(&anchor, sizeof(anchor), h);
                h = lmFnv64(&yaw, sizeof(yaw), h);
                continue;
            }
            const MapProp& pr = mapProps[(size_t)k];
            h = lmFnv64(&pr.pos, sizeof(pr.pos), h);
            h = lmFnv64(&pr.type, sizeof(pr.type), h);
            h = lmFnv64(&pr.scale, sizeof(pr.scale), h);
            h = lmFnv64(&pr.yaw, sizeof(pr.yaw), h);
        }
        LmChunkPropMesh& m = gLmChunkPropMeshes[it->first];
        bool fresh = (!m.built || m.hash != h);
        m.seenSerial = serial;
        if (fresh && ((mode == LM_PROPSYNC_NEAR && todo.dist > 1) || (mode == LM_PROPSYNC_SLICE && budgetSpent))) {
            gLmChunkPropPending = true;   // keeps whatever it showed until its turn comes
            continue;
        }
#ifdef LIMEN_SHOTS
        bool check = gLmBenchHashOn && !fresh;
#else
        const bool check = false;
#endif
        if (fresh || check) {
            verts.clear();
            for (int k : idx) {
                if (k < 0) lmAppendPoiModel(verts, mapPois[(size_t)(-1 - k)]);
                else lmAppendPropModel(verts, mapProps[(size_t)k]);
            }
            { ZfMesh zm[1] = {{&verts, 8, 0, "props"}}; lmResolveZfight(zm, 1); }
#ifdef LIMEN_SHOTS
            unsigned long long vh = (gLmBenchHashOn || gLmBenchHashKeep) ? lmFnv64(verts.data(), verts.size() * sizeof(float)) : 0ull;
            if (check && vh != m.vertHash) gLmBenchPropStale++;
            m.vertHash = vh;
            if (fresh) gLmBenchPropChunksBuilt++;
#endif
        }
        if (fresh) {
            m.tx = (int)(todo.key >> 32); m.tz = (int)(todo.key & 0xFFFFFFFF);
            m.hash = h;
            m.built = true;
            m.vc = (int)verts.size() / 8;
            if (m.vc > 0) setupVAO(m.vao, m.vbo, verts, true);
            if (mode == LM_PROPSYNC_SLICE &&
                std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tStart).count() >= LM_PROPSYNC_SLICE_MS) {
                budgetSpent = true;
            }
        }
    }
    for (auto it = gLmChunkPropMeshes.begin(); it != gLmChunkPropMeshes.end();) {
        if (it->second.seenSerial == serial) { ++it; continue; }
        if (it->second.vao) glDeleteVertexArrays(1, &it->second.vao);
        if (it->second.vbo) glDeleteBuffers(1, &it->second.vbo);
        it = gLmChunkPropMeshes.erase(it);
    }
    if (verts.capacity() > (size_t)(8u << 20)) std::vector<float>().swap(verts);
}

inline void lmPostBuildGeomImpl();
#ifdef LIMEN_SHOTS
extern double gLmBenchPostMs;
#endif
inline void lmPostBuildGeom() {
#ifdef LIMEN_SHOTS
    auto t0 = std::chrono::high_resolution_clock::now();
    lmPostBuildGeomImpl();
    gLmBenchPostMs += std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t0).count();
#else
    lmPostBuildGeomImpl();
#endif
}
// Folds the game's layout edits into LIMEN's world (see LmStory). Cheap when nothing changed.
inline void lmSyncStoryFromGame() {
    LmStory next;
    for (const auto& kv : gCellOverrides) next.edits[kv.first] = kv.second != 0 ? 1 : 0;
    int rsx = (int)floorf(heist.rigStart.x / CS), rsz = (int)floorf(heist.rigStart.z / CS);
    auto opened = [&](int wx, int wz) { auto it = gCellOverrides.find(cellKey(wx, wz)); return it != gCellOverrides.end() && it->second == 0; };
    if (coop.initialized && opened(rsx, rsz)) {
        next.lane = true; next.laneX = rsx; next.laneZ = rsz;
        next.laneDX = rigDirX(heist.rigDir); next.laneDZ = rigDirZ(heist.rigDir);
    }
    if (coop.initialized && heist.hasGarage && opened(heist.baseX, heist.baseZ + GARAGE_NEAR_Z)) {
        int bx = heist.baseX, bz = heist.baseZ;
        int rc[3][4] = {{bx - GARAGE_HALF_W - 1, bz + GARAGE_FAR_Z - 1, bx + GARAGE_HALF_W + 1, bz + GARAGE_NEAR_Z + 1},
                        {bx, bz + PORTAL_DZ, bx, bz}, {bx - 3, bz - 2, bx - 1, bz}};
        for (int k = 0; k < 3; k++) { for (int q = 0; q < 4; q++) next.clear[k][q] = rc[k][q]; }
        next.nClear = 3;
        next.nSet = 1;
    }
    if (coop.initialized && next.nClear > 0 && next.nClear < 12) {
        // the way dug out of the arrival area when the stamps walled it in
        int bx = heist.baseX, bz = heist.baseZ, dug = 0;
        for (int step = 1; step <= 14; step++) if (opened(bx, bz + step)) dug = step;
        if (dug > 0) { int* c = next.clear[next.nClear++]; c[0] = c[2] = bx; c[1] = bz + 1; c[3] = bz + dug; }
    }
    if (coop.initialized && next.lane) {
        // single cells a set piece stands on (kiosk, tutorial loot, the core)
        auto spot = [&](const Vec3& p) {
            if (next.nClear >= 12) return;
            int wx = (int)floorf(p.x / CS), wz = (int)floorf(p.z / CS);
            int* c = next.clear[next.nClear++];
            c[0] = c[2] = wx; c[1] = c[3] = wz;
        };
        if (heist.hasKiosk) spot(heist.kioskPos);
        if (heist.hasGarage) {
            for (int i = 0; i < HEIST_TUTORIAL_LOOT; i++) spot(heist.lessonPos[i]);
            spot(heist.corePos);
        }
    }
    LmStory& cur = gLmStory;
    bool shape = next.lane != cur.lane || next.nClear != cur.nClear || next.nSet != cur.nSet ||
                 (next.lane && (next.laneX != cur.laneX || next.laneZ != cur.laneZ || next.laneDX != cur.laneDX || next.laneDZ != cur.laneDZ));
    for (int k = 0; k < next.nClear && !shape; k++) for (int q = 0; q < 4; q++) if (next.clear[k][q] != cur.clear[k][q]) shape = true;
    std::vector<long long> changed;
    if (!shape) {
        for (const auto& kv : next.edits) { auto it = cur.edits.find(kv.first); if (it == cur.edits.end() || it->second != kv.second) changed.push_back(kv.first); }
        for (const auto& kv : cur.edits) if (next.edits.find(kv.first) == next.edits.end()) changed.push_back(kv.first);
        if (changed.empty()) return;
    }
    cur = next;
    if (shape) {
        // a new lane or set: every slab may differ, and so may what the loaded chunks hold
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
        return;
    }
    for (long long ck : changed) {
        int wx = (int)(ck >> 32), wz = (int)(ck & 0xFFFFFFFF);
        for (int dx = -1; dx <= 1; dx++) for (int dz = -1; dz <= 1; dz++) {
            LimenRect r = lmSlabAt(wx + dx, wz + dz);
            long long skey = (long long)(((unsigned long long)(u32)r.j << 32) | (unsigned long long)(u32)r.i);
            gLmStorySlabEpoch[skey]++;
            if (gLimenSlabs.erase(skey)) { gLimenSlabUse.erase(skey); gLimenLastSlab = nullptr; }
        }
    }
}
inline bool gLmStoryHooked = ((gLmStorySync = lmSyncStoryFromGame), true);
