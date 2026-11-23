#pragma once
// Part of limen_props.h: vignettes, chunk content rebuild, updateMapContent.

// Curated Spatial Vignettes ("сцены-события"): rare, composed, memorable non-random scenes
// so turning a corner can reveal an unmistakable human trace instead of uniform prop noise!
inline void lmSpawnLiminalVignettes(const Chunk& c, std::mt19937& r) {
    static const int dx[4] = {0, 1, 0, -1}, dz[4] = {1, 0, -1, 0};

    // Pass A: Vitrine Audience — whenever a sealed illuminated window (`LS_VITRINE_WIN`) faces a hall/corridor,
    // place a quiet bench or waiting-seat row facing straight into the unreachable lit room!
    for (int lx = 1; lx < CHUNK_SIZE - 1; lx++) {
        for (int lz = 1; lz < CHUNK_SIZE - 1; lz++) {
            int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
            if ((lmZoneAt(wx, wz) & LZ_STORY) != 0) continue;
            LimenRect sr = lmSlabAt(wx, wz);
            if (wx == sr.x0 || wz == sr.z0) continue;
            const LimenSlab& s = lmSlab(sr.j, sr.i);
            int slx = wx - sr.x0 - 1, slz = wz - sr.z0 - 1;
            int tag = s.tag[slx][slz];
            if (tag != LT_HALL && tag != LT_CORR) continue;
            if (s.flag[slx][slz] & (LF_NARROW_X | LF_NARROW_Z | LF_THIN_W | LF_THIN_N)) continue;
            if (lmDoorwayAt(wx, wz).axis != 0) continue;
            for (int d = 0; d < 4; d++) {
                if (!lmSlabHasSite(s, slx + dx[d], slz + dz[d], LS_VITRINE_WIN)) continue;
                u32 vh = lmHash((u32)wx, (u32)wz, 1u, 0x564155u);
                if ((vh % 100u) >= 58u) break;
                float yaw = atan2f((float)dx[d], (float)dz[d]);
                Vec3 back((float)-dx[d], 0.0f, (float)-dz[d]);
                Vec3 seatPos = cellCenter(wx, wz) + back * 0.65f;
                int seatType = ((vh >> 8) & 1u) ? MAP_PROP_WAITING_SEATS : MAP_PROP_BENCH;
                lmAddRoomPropSafe(seatPos, seatType, 1.0f, yaw);
                break;
            }
        }
    }

    // Pass B: rare standalone arrangement (~1 in 3 chunks). Four generic layout grammars, each parameterised by
    // the chunk hash, so a level never repeats one hand-placed scene:
    //   Ring      - N identical seats around a centre prop, under a burning lamp (N 5..8, radius 1.10..1.90 m)
    //   Queue     - a desk at one end of a lane of stanchion pairs, with a machine beside it
    //   Threshold - a desk with one stanchion pair in front of it and a sign to the side
    //   Debris    - a fallen-unit pivot with a scatter of loose pieces (2..5, radius 0.60..2.20 m)
    u32 ch = lmHash((u32)c.cx, (u32)c.cz, (u32)gLimen.level, 0x564947u);
    if ((ch % 100u) >= 36u) return;
    int family = (int)((ch >> 8) % 4u);
    int nRing = 5 + (int)((ch >> 12) % 4u);
    float rRing = 1.10f + 0.80f * (float)((ch >> 16) & 255u) / 255.0f;
    int nQueue = 2 + (int)((ch >> 12) % 3u);
    float pitch = 0.80f + 0.50f * (float)((ch >> 16) & 255u) / 255.0f;
    int nDebris = 2 + (int)((ch >> 12) % 4u);
    float rDebris = 0.60f + 1.60f * (float)((ch >> 16) & 255u) / 255.0f;
    float yaw0 = (float)((ch >> 20) & 3u) * 1.57079633f;
    int centreKind = ((ch >> 24) & 1u) ? MAP_PROP_FLOOR_DRAIN : MAP_PROP_AV_CART;
    int deskKind = ((ch >> 25) & 1u) ? MAP_PROP_RECEPTION_DESK : MAP_PROP_TICKET_BOOTH;
    static const int debrisPool[5] = {MAP_PROP_STEP_LADDER, MAP_PROP_JANITOR_CART, MAP_PROP_WET_FLOOR_SIGN,
                                      MAP_PROP_PUDDLE, MAP_PROP_FALLEN_TILES};

    for (int att = 0; att < 18; att++) {
        int lx = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int lz = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int wx = c.cx * CHUNK_SIZE + lx, wz = c.cz * CHUNK_SIZE + lz;
        if (!lmAllowPropAt(wx, wz, false)) continue;
        int tag = lmTagAt(wx, wz);
        if (tag != LT_HALL && tag != LT_ROOM) continue;
        if (countWallNeighborsWorld(wx, wz) > 1) continue;
        if (family == 0 && lmLampAt(wx, wz) != 1) continue; // a ring is centred under a burning lamp

        Vec3 cc = cellCenter(wx, wz);
        if (family == 0) {
            int placed = 0;
            lmAddRoomPropSafe(cc, centreKind, 1.0f, 0.0f);
            for (int k = 0; k < nRing; k++) {
                float a = (float)k * (6.28318530f / (float)nRing);
                Vec3 p(cc.x + sinf(a) * rRing, 0.0f, cc.z + cosf(a) * rRing);
                if (lmAddRoomPropSafe(p, MAP_PROP_CHAIR, 1.0f, a + 3.14159265f)) placed++;
            }
            if (placed >= nRing - 3) return;
        } else if (family == 1) {
            Vec3 fwd(sinf(yaw0), 0.0f, cosf(yaw0)), right(cosf(yaw0), 0.0f, -sinf(yaw0));
            if (lmAddRoomPropSafe(cc - fwd * 0.55f, deskKind, 1.0f, yaw0)) {
                for (int k = 0; k < nQueue; k++) {
                    float fd = 1.15f + pitch * (float)k;
                    lmAddRoomPropSafe(cc + fwd * fd - right * 0.85f, MAP_PROP_STANCHIONS, 1.0f, yaw0);
                    lmAddRoomPropSafe(cc + fwd * fd + right * 0.85f, MAP_PROP_STANCHIONS, 1.0f, yaw0);
                }
                lmAddRoomPropSafe(cc - fwd * 0.55f + right * 1.65f, MAP_PROP_VENDING_MACHINE, 1.0f, yaw0);
                return;
            }
        } else if (family == 2) {
            Vec3 fwd(sinf(yaw0), 0.0f, cosf(yaw0)), right(cosf(yaw0), 0.0f, -sinf(yaw0));
            if (lmAddRoomPropSafe(cc - fwd * 0.55f, deskKind, 1.0f, yaw0)) {
                lmAddRoomPropSafe(cc + fwd * 1.15f - right * 0.85f, MAP_PROP_STANCHIONS, 1.0f, yaw0);
                lmAddRoomPropSafe(cc + fwd * 1.15f + right * 0.85f, MAP_PROP_STANCHIONS, 1.0f, yaw0);
                lmAddRoomPropSafe(cc - fwd * 0.55f - right * 1.10f, MAP_PROP_WET_FLOOR_SIGN, 1.0f, yaw0 + 0.6f);
                return;
            }
        } else {
            lmAddRoomPropSafe(cc, MAP_PROP_FALLEN_TILES, 1.0f, 0.3f);
            int placed = 0;
            for (int k = 0; k < nDebris; k++) {
                u32 dh = lmHash(ch, (u32)k, 5u, 0x44425Au);
                float a = (float)(dh & 255u) * (6.28318530f / 256.0f);
                float rr = rDebris * (0.45f + 0.55f * (float)((dh >> 8) & 255u) / 255.0f);
                Vec3 p(cc.x + sinf(a) * rr, 0.0f, cc.z + cosf(a) * rr);
                int kind = debrisPool[(dh >> 16) % 5u];
                if (lmAddRoomPropSafe(p, kind, 1.0f, a + (float)((dh >> 24) & 3u) * 1.5708f)) placed++;
            }
            if (placed >= 1) return;
        }
    }
}

inline void lmRebuildChunkContent(const Chunk& c) {
    lmReset(worldSeed, gCurrentLevel);
    LmLocalContentScope localScope(c);
    u32 cSeed = lmHash((u32)c.cx, (u32)c.cz, (u32)gLimen.level, 0xA18F331u);
    std::mt19937 r(cSeed);
    { LM_PROF("content:lmSpawnLiminalVignettes"); lmSpawnLiminalVignettes(c, r); }
    { LM_PROF("content:lmSpawnRoomSuites"); lmSpawnRoomSuites(c); }
    { LM_PROF("content:lmSpawnHallZones"); lmSpawnHallZones(c); }
    { LM_PROF("content:lmSpawnWaitingVoidRows"); lmSpawnWaitingVoidRows(c, r); }
    { LM_PROF("content:lmSpawnDeliriumProcession"); lmSpawnDeliriumProcession(c, r); }
    { LM_PROF("content:lmSpawnThemedPoi"); lmSpawnThemedPoi(c, r); }
    { LM_PROF("content:lmSpawnChunkThemed"); lmSpawnChunkThemed(c, r); }
    { LM_PROF("content:lmSpawnBridgeProps"); lmSpawnBridgeProps(c, r); }
    { LM_PROF("content:lmApplyQuirkProps"); lmApplyQuirkProps(c, localScope.nLocal, r); }
}

inline void updateMapContent(int pcx, int pcz) {
    lmReset(worldSeed, gCurrentLevel);
    if (gLmStorySync) gLmStorySync();
    if (mapProps.empty() && mapPois.empty()) mapContentChunks.clear();

    mapProps.erase(
        std::remove_if(mapProps.begin(), mapProps.end(), [&](const MapProp& p) {
            int wx = (int)floorf(p.pos.x / CS), wz = (int)floorf(p.pos.z / CS);
            // (furniture placed before the game stamped a set or bored the lane there goes too)
            return !lmIsChunkInView(p.pos.x, p.pos.z, pcx, pcz) || !lmWalkable(wx, wz) ||
                   (lmZoneAt(wx, wz) & LZ_STORY) != 0;
        }),
        mapProps.end()
    );
    mapPois.erase(
        std::remove_if(mapPois.begin(), mapPois.end(), [&](const MapPoi& p) {
            int wx = (int)floorf(p.pos.x / CS), wz = (int)floorf(p.pos.z / CS);
            return !lmIsChunkInView(p.pos.x, p.pos.z, pcx, pcz) || !lmWalkable(wx, wz);
        }),
        mapPois.end()
    );
    for (auto it = mapContentChunks.begin(); it != mapContentChunks.end();) {
        int cx = (int)(*it >> 32);
        int cz = (int)(*it & 0xFFFFFFFF);
        if (abs(cx - pcx) > LM_VIEW_CHUNKS || abs(cz - pcz) > LM_VIEW_CHUNKS) it = mapContentChunks.erase(it);
        else ++it;
    }

    // First fill of a level (or a teleport) furnishes everything now; while walking, the outer
    // ring that just came into view is left to `lmPumpDeferredContent`.
    bool walking = gLmStreaming && !mapContentChunks.empty();
    for (int ring = 0; ring <= LM_VIEW_CHUNKS; ring++) {
        for (int dcx = -ring; dcx <= ring; dcx++) {
            for (int dcz = -ring; dcz <= ring; dcz++) {
                if (abs(dcx) != ring && abs(dcz) != ring) continue;
                long long key = chunkKey(pcx + dcx, pcz + dcz);
                auto it = chunks.find(key);
                if (it == chunks.end()) continue;
                if (mapContentChunks.find(key) != mapContentChunks.end()) continue;
                if (walking && ring >= LM_VIEW_CHUNKS) { gLmContentPending = true; continue; }
                mapContentChunks.insert(key);
                lmRebuildChunkContent(it->second);
            }
        }
    }
}

// Furnishes one waiting chunk (nearest ring first, same order `updateMapContent` would have used).
// Returns true if it did any work.
inline bool lmPumpDeferredContent() {
    if (!gLmContentPending) return false;
    lmReset(worldSeed, gCurrentLevel);
    int pcx = playerChunkX, pcz = playerChunkZ;
    for (int ring = 0; ring <= LM_VIEW_CHUNKS; ring++) {
        for (int dcx = -ring; dcx <= ring; dcx++) {
            for (int dcz = -ring; dcz <= ring; dcz++) {
                if (abs(dcx) != ring && abs(dcz) != ring) continue;
                long long key = chunkKey(pcx + dcx, pcz + dcz);
                auto it = chunks.find(key);
                if (it == chunks.end()) continue;
                if (!mapContentChunks.insert(key).second) continue;
                lmRebuildChunkContent(it->second);
                gLmChunkPropPending = true;
                return true;
            }
        }
    }
    gLmContentPending = false;
    return false;
}
