#pragma once

#include <vector>
#include <random>
#include <algorithm>

#include "world.h"

enum MapPropType {
    MAP_PROP_CRATE_STACK = 0,
    MAP_PROP_CONE_CLUSTER = 1,
    MAP_PROP_BARRIER = 2,
    MAP_PROP_CABLE_REEL = 3,
    MAP_PROP_PUDDLE = 4,
    MAP_PROP_DEBRIS = 5,
    MAP_PROP_DESK = 6,
    MAP_PROP_CHAIR = 7,
    MAP_PROP_CABINET = 8,
    MAP_PROP_PARTITION = 9,
    MAP_PROP_BOX_PALLET = 10,
    MAP_PROP_DRUM_STACK = 11,
    MAP_PROP_LOCKER_BANK = 12
};

struct MapProp {
    Vec3 pos;
    int type;
    float scale;
    float yaw;
};

enum MapPoiType {
    MAP_POI_OFFICE = 0,
    MAP_POI_SERVER = 1,
    MAP_POI_STORAGE = 2,
    MAP_POI_RESTROOM = 3
};

struct MapPoi {
    Vec3 pos;
    int type;
    float radius;
    int id;
};

extern std::vector<MapProp> mapProps;
extern std::vector<MapPoi> mapPois;

inline unsigned int chunkMapContentSeed(int cx, int cz, unsigned int salt) {
    unsigned int s = worldSeed ^ (unsigned int)(cx * 92837111) ^ (unsigned int)(cz * 689287499);
    return s ^ salt;
}

inline bool hasMapPropNear(const Vec3& p, float radius) {
    float r2 = radius * radius;
    for (const auto& it : mapProps) {
        float dx = it.pos.x - p.x;
        float dz = it.pos.z - p.z;
        if (dx * dx + dz * dz < r2) return true;
    }
    return false;
}

inline bool hasMapPoiNear(const Vec3& p, float radius) {
    float r2 = radius * radius;
    for (const auto& it : mapPois) {
        float dx = it.pos.x - p.x;
        float dz = it.pos.z - p.z;
        if (dx * dx + dz * dz < r2) return true;
    }
    return false;
}

inline bool isMapPropPushableType(int type) {
    return type == MAP_PROP_CRATE_STACK ||
           type == MAP_PROP_DEBRIS ||
           type == MAP_PROP_BOX_PALLET;
}

inline void mapPropHalfExtents(const MapProp& p, float& hx, float& hz) {
    float s = p.scale;
    if (p.type == MAP_PROP_CONE_CLUSTER) { hx = 0.62f * s; hz = 0.62f * s; return; }
    if (p.type == MAP_PROP_BARRIER) { hx = 0.82f * s; hz = 0.28f * s; return; }
    if (p.type == MAP_PROP_CABLE_REEL) { hx = 0.54f * s; hz = 0.54f * s; return; }
    if (p.type == MAP_PROP_PUDDLE) { hx = 0.0f; hz = 0.0f; return; }
    if (p.type == MAP_PROP_DEBRIS) { hx = 0.76f * s; hz = 0.72f * s; return; }
    if (p.type == MAP_PROP_DESK) { hx = 0.62f * s; hz = 0.46f * s; return; }
    if (p.type == MAP_PROP_CHAIR) { hx = 0.32f * s; hz = 0.32f * s; return; }
    if (p.type == MAP_PROP_CABINET) { hx = 0.40f * s; hz = 0.34f * s; return; }
    if (p.type == MAP_PROP_PARTITION) { hx = 0.72f * s; hz = 0.18f * s; return; }
    if (p.type == MAP_PROP_BOX_PALLET) { hx = 0.68f * s; hz = 0.60f * s; return; }
    if (p.type == MAP_PROP_DRUM_STACK) { hx = 0.52f * s; hz = 0.52f * s; return; }
    if (p.type == MAP_PROP_LOCKER_BANK) { hx = 0.66f * s; hz = 0.30f * s; return; }
    hx = 0.80f * s; hz = 0.74f * s;
}

inline float mapPropCollisionRadius(const MapProp& p) {
    float hx = 0.0f, hz = 0.0f;
    mapPropHalfExtents(p, hx, hz);
    return hx > hz ? hx : hz;
}

inline bool collideMapPropsEx(float x, float z, float pr, int ignoreIndex) {
    for (const auto& p : mapProps) {
        int idx = (int)(&p - &mapProps[0]);
        if (idx == ignoreIndex) continue;
        float hx = 0.0f, hz = 0.0f;
        mapPropHalfExtents(p, hx, hz);
        if (hx <= 0.001f || hz <= 0.001f) continue;
        if (fabsf(x - p.pos.x) < (hx + pr) && fabsf(z - p.pos.z) < (hz + pr)) return true;
    }
    return false;
}

inline bool collideMapProps(float x, float z, float pr) {
    return collideMapPropsEx(x, z, pr, -1);
}

inline bool tryPushMapProps(float playerX, float playerZ, float pr, float moveX, float moveZ) {
    float dirLenSq = moveX * moveX + moveZ * moveZ;
    if (dirLenSq < 0.000001f) return false;
    float invLen = 1.0f / sqrtf(dirLenSq);
    float dirX = moveX * invLen;
    float dirZ = moveZ * invLen;

    int targetIndex = -1;
    float bestDistSq = 1e30f;
    for (int i = 0; i < (int)mapProps.size(); i++) {
        const MapProp& p = mapProps[i];
        if (!isMapPropPushableType(p.type)) continue;
        float hx = 0.0f, hz = 0.0f;
        mapPropHalfExtents(p, hx, hz);
        if (hx <= 0.001f || hz <= 0.001f) continue;
        if (fabsf(playerX - p.pos.x) >= (hx + pr)) continue;
        if (fabsf(playerZ - p.pos.z) >= (hz + pr)) continue;
        float dx = p.pos.x - playerX;
        float dz = p.pos.z - playerZ;
        float ds = dx * dx + dz * dz;
        if (ds < bestDistSq) {
            bestDistSq = ds;
            targetIndex = i;
        }
    }
    if (targetIndex < 0) return false;

    MapProp& p = mapProps[targetIndex];
    float pushStrength = 0.32f;
    if (p.type == MAP_PROP_DEBRIS) pushStrength = 0.42f;
    else if (p.type == MAP_PROP_CRATE_STACK) pushStrength = 0.28f;
    else if (p.type == MAP_PROP_BOX_PALLET) pushStrength = 0.24f;

    float tryX = p.pos.x + dirX * pushStrength;
    float tryZ = p.pos.z + dirZ * pushStrength;
    float hx = 0.0f, hz = 0.0f;
    mapPropHalfExtents(p, hx, hz);
    float propRadius = (hx > hz ? hx : hz) * 0.82f;
    if (collideWorld(tryX, tryZ, propRadius)) return false;
    if (collideMapPropsEx(tryX, tryZ, propRadius, targetIndex)) return false;

    p.pos.x = tryX;
    p.pos.z = tryZ;
    return true;
}

inline int countOpenNeighbors(const Chunk& c, int lx, int lz) {
    int open = 0;
    if (lx > 0 && c.cells[lx - 1][lz] == 0) open++;
    if (lx < CHUNK_SIZE - 1 && c.cells[lx + 1][lz] == 0) open++;
    if (lz > 0 && c.cells[lx][lz - 1] == 0) open++;
    if (lz < CHUNK_SIZE - 1 && c.cells[lx][lz + 1] == 0) open++;
    return open;
}

inline int countWallNeighbors(const Chunk& c, int lx, int lz) {
    int walls = 0;
    if (lx > 0 && c.cells[lx - 1][lz] == 1) walls++;
    if (lx < CHUNK_SIZE - 1 && c.cells[lx + 1][lz] == 1) walls++;
    if (lz > 0 && c.cells[lx][lz - 1] == 1) walls++;
    if (lz < CHUNK_SIZE - 1 && c.cells[lx][lz + 1] == 1) walls++;
    return walls;
}

inline bool isMapPropCellValid(const Chunk& c, int lx, int lz) {
    if (lx < 2 || lx > CHUNK_SIZE - 3 || lz < 2 || lz > CHUNK_SIZE - 3) return false;
    if (c.cells[lx][lz] != 0) return false;
    int open = countOpenNeighbors(c, lx, lz);
    if (open < 2) return false;
    int walls = countWallNeighbors(c, lx, lz);
    return walls >= 1;
}

inline void pushMapPropUnique(int cx, int cz, int lx, int lz, int type, float scale, float yaw) {
    Vec3 base(
        ((float)(cx * CHUNK_SIZE + lx) + 0.5f) * CS,
        0.0f,
        ((float)(cz * CHUNK_SIZE + lz) + 0.5f) * CS
    );
    Vec3 wp = base;
    bool foundValid = false;
    MapProp probe{};
    probe.type = type;
    probe.scale = scale;
    probe.yaw = yaw;
    float propRadius = mapPropCollisionRadius(probe) * 0.86f;
    const float jitter = CS * 0.22f;
    for (int attempt = 0; attempt < 3; attempt++) {
        float mul = (attempt == 0) ? 1.0f : (attempt == 1 ? 0.6f : 0.3f);
        float ox = sinf(yaw * 1.37f + (float)attempt * 0.91f) * jitter * mul;
        float oz = cosf(yaw * 1.61f + (float)attempt * 1.23f) * jitter * mul;
        Vec3 cand(base.x + ox, 0.0f, base.z + oz);
        int wx = (int)floorf(cand.x / CS);
        int wz = (int)floorf(cand.z / CS);
        if (getCellWorld(wx, wz) != 0) continue;
        if (propRadius > 0.001f && collideWorld(cand.x, cand.z, propRadius)) continue;
        wp = cand;
        foundValid = true;
        break;
    }
    if (!foundValid) return;
    if (hasMapPropNear(wp, CS * (0.40f + scale * 0.22f))) return;
    MapProp pr{};
    pr.pos = wp;
    pr.type = type;
    pr.scale = scale;
    pr.yaw = yaw;
    mapProps.push_back(pr);
}

inline void spawnChunkProps(const Chunk& c) {
    std::mt19937 cr(chunkMapContentSeed(c.cx, c.cz, 0xA18F331u));
    int baseCount = 1 + (int)(cr() % 2);
    int maxAttempts = 44;

    for (int i = 0; i < baseCount && maxAttempts > 0; i++) {
        int lx = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        int lz = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        if (!isMapPropCellValid(c, lx, lz)) {
            maxAttempts--;
            continue;
        }
        int wallsNear = countWallNeighbors(c, lx, lz);
        if (wallsNear < 2 && (cr() % 100) < 90) {
            maxAttempts--;
            continue;
        }
        int weighted = (int)(cr() % 100);
        int type = MAP_PROP_CRATE_STACK;
        if (weighted < 36) type = MAP_PROP_CRATE_STACK;
        else if (weighted < 60) type = MAP_PROP_BOX_PALLET;
        else if (weighted < 70) type = MAP_PROP_BARRIER;
        else if (weighted < 80) type = MAP_PROP_CABLE_REEL;
        else if (weighted < 87) type = MAP_PROP_DRUM_STACK;
        else if (weighted < 93) type = MAP_PROP_DEBRIS;
        else if (weighted < 97) type = MAP_PROP_LOCKER_BANK;
        else if (weighted < 99) type = MAP_PROP_CABINET;
        else type = MAP_PROP_PUDDLE;
        float scale = 0.78f + ((float)(cr() % 45) / 100.0f);
        float yaw = ((float)(cr() % 628) / 100.0f);
        pushMapPropUnique(c.cx, c.cz, lx, lz, type, scale, yaw);
    }
}

inline void spawnChunkPoiClusters(const Chunk& c) {
    std::mt19937 cr(chunkMapContentSeed(c.cx, c.cz, 0x39BC21u));
    int clusterCount = (int)(cr() % 2);

    for (int cl = 0; cl < clusterCount; cl++) {
        int centerX = 3 + (int)(cr() % (CHUNK_SIZE - 6));
        int centerZ = 3 + (int)(cr() % (CHUNK_SIZE - 6));
        if (!isMapPropCellValid(c, centerX, centerZ)) continue;

        int theme = (int)(cr() % 3);
        int inCluster = 2 + (int)(cr() % 2);
        for (int j = 0; j < inCluster; j++) {
            int ox = (int)(cr() % 5) - 2;
            int oz = (int)(cr() % 5) - 2;
            int lx = centerX + ox;
            int lz = centerZ + oz;
            if (!isMapPropCellValid(c, lx, lz)) continue;

            int type = MAP_PROP_DEBRIS;
            if (theme == 0) type = (j % 2 == 0) ? MAP_PROP_CRATE_STACK : MAP_PROP_BOX_PALLET;
            if (theme == 1) type = (j % 2 == 0) ? MAP_PROP_CABLE_REEL : MAP_PROP_DRUM_STACK;
            if (theme == 2) type = (j % 2 == 0) ? MAP_PROP_CABINET : MAP_PROP_LOCKER_BANK;
            float scale = 0.88f + ((float)(cr() % 35) / 100.0f);
            float yaw = ((float)(cr() % 628) / 100.0f);
            pushMapPropUnique(c.cx, c.cz, lx, lz, type, scale, yaw);
        }
    }
}

inline bool isLikelyOfficeChunk(const Chunk& c) {
    int stripWalls = 0;
    for (int x = 2; x < CHUNK_SIZE - 2; x++) {
        int w = 0;
        for (int z = 2; z < CHUNK_SIZE - 2; z++) {
            if (c.cells[x][z] == 1) w++;
        }
        if (w >= CHUNK_SIZE - 6) stripWalls++;
    }
    return stripWalls >= 2;
}

inline void spawnOfficeFurniture(const Chunk& c) {
    if (!isLikelyOfficeChunk(c)) return;
    std::mt19937 cr(chunkMapContentSeed(c.cx, c.cz, 0xB44231u));
    int rows = 2 + (int)(cr() % 3);
    for (int i = 0; i < rows; i++) {
        int lx = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        int lz = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        if (!isMapPropCellValid(c, lx, lz)) continue;

        pushMapPropUnique(c.cx, c.cz, lx, lz, MAP_PROP_DESK, 0.95f, 0.0f);
        if (isMapPropCellValid(c, lx + 1, lz)) {
            pushMapPropUnique(c.cx, c.cz, lx + 1, lz, MAP_PROP_CHAIR, 0.9f, 0.0f);
        }
        if (isMapPropCellValid(c, lx, lz + 1) && (cr() % 100) < 55) {
            pushMapPropUnique(c.cx, c.cz, lx, lz + 1, MAP_PROP_CABINET, 0.95f, 0.0f);
        }
        if (isMapPropCellValid(c, lx - 1, lz) && (cr() % 100) < 45) {
            pushMapPropUnique(c.cx, c.cz, lx - 1, lz, MAP_PROP_PARTITION, 1.0f, 0.0f);
        }
        if (isMapPropCellValid(c, lx + 1, lz + 1) && (cr() % 100) < 62) {
            pushMapPropUnique(c.cx, c.cz, lx + 1, lz + 1, MAP_PROP_CHAIR, 0.92f, 0.0f);
        }
        if (isMapPropCellValid(c, lx - 1, lz - 1) && (cr() % 100) < 56) {
            pushMapPropUnique(c.cx, c.cz, lx - 1, lz - 1, MAP_PROP_LOCKER_BANK, 0.90f, 0.0f);
        }
        if (isMapPropCellValid(c, lx, lz - 1) && (cr() % 100) < 52) {
            pushMapPropUnique(c.cx, c.cz, lx, lz - 1, MAP_PROP_PARTITION, 0.92f, 0.0f);
        }
    }
}

inline void pushMapPoiUnique(int cx, int cz, int lx, int lz, int type, float radius) {
    Vec3 p(
        ((float)(cx * CHUNK_SIZE + lx) + 0.5f) * CS,
        0.0f,
        ((float)(cz * CHUNK_SIZE + lz) + 0.5f) * CS
    );
    int wx = (int)floorf(p.x / CS);
    int wz = (int)floorf(p.z / CS);
    if (getCellWorld(wx, wz) != 0) return;
    if (hasMapPoiNear(p, CS * 4.0f)) return;
    MapPoi poi{};
    poi.pos = p;
    poi.type = type;
    poi.radius = radius;
    poi.id = (cx * 73856093) ^ (cz * 19349663) ^ (lx * 83492791) ^ (lz * 297121507);
    mapPois.push_back(poi);
}

inline void spawnChunkPoiRooms(const Chunk& c) {
    std::mt19937 cr(chunkMapContentSeed(c.cx, c.cz, 0x71D9A3u));
    int spawnRoll = (int)(cr() % 100);
    if (spawnRoll > 27) return;
    int tries = 10;
    while (tries-- > 0) {
        int lx = 3 + (int)(cr() % (CHUNK_SIZE - 6));
        int lz = 3 + (int)(cr() % (CHUNK_SIZE - 6));
        if (!isMapPropCellValid(c, lx, lz)) continue;
        int open = countOpenNeighbors(c, lx, lz);
        if (open < 3) continue;
        int typeRoll = (int)(cr() % 100);
        int type = MAP_POI_OFFICE;
        if (typeRoll < 30) type = MAP_POI_OFFICE;
        else if (typeRoll < 55) type = MAP_POI_SERVER;
        else if (typeRoll < 80) type = MAP_POI_STORAGE;
        else type = MAP_POI_RESTROOM;
        pushMapPoiUnique(c.cx, c.cz, lx, lz, type, CS * 1.8f);
        break;
    }
}

inline void rebuildChunkMapContent(const Chunk& c) {
    spawnChunkProps(c);
    spawnChunkPoiClusters(c);
    spawnOfficeFurniture(c);
    spawnChunkPoiRooms(c);
}

inline bool isMapPropTooFar(const MapProp& p, float cx, float cz, float md) {
    return fabsf(p.pos.x - cx) > md || fabsf(p.pos.z - cz) > md;
}

inline bool isMapPropOnWall(const MapProp& p) {
    int wx = (int)floorf(p.pos.x / CS);
    int wz = (int)floorf(p.pos.z / CS);
    return getCellWorld(wx, wz) == 1;
}

inline bool isMapPropEmbeddedInWall(const MapProp& p) {
    float r = mapPropCollisionRadius(p) * 0.86f;
    if (r <= 0.001f) return false;
    return collideWorld(p.pos.x, p.pos.z, r);
}

inline void updateMapContent(int pcx, int pcz) {
    float cx = (pcx + 0.5f) * CHUNK_SIZE * CS;
    float cz = (pcz + 0.5f) * CHUNK_SIZE * CS;
    float md = (VIEW_CHUNKS + 1) * CHUNK_SIZE * CS;

    mapProps.erase(
        std::remove_if(
            mapProps.begin(),
            mapProps.end(),
            [&](const MapProp& p) { return isMapPropTooFar(p, cx, cz, md) || isMapPropOnWall(p) || isMapPropEmbeddedInWall(p); }
        ),
        mapProps.end()
    );
    mapPois.erase(
        std::remove_if(
            mapPois.begin(),
            mapPois.end(),
            [&](const MapPoi& p) {
                if (fabsf(p.pos.x - cx) > md || fabsf(p.pos.z - cz) > md) return true;
                int wx = (int)floorf(p.pos.x / CS);
                int wz = (int)floorf(p.pos.z / CS);
                return getCellWorld(wx, wz) == 1;
            }
        ),
        mapPois.end()
    );

    for (int dcx = -VIEW_CHUNKS; dcx <= VIEW_CHUNKS; dcx++) {
        for (int dcz = -VIEW_CHUNKS; dcz <= VIEW_CHUNKS; dcz++) {
            auto it = chunks.find(chunkKey(pcx + dcx, pcz + dcz));
            if (it == chunks.end()) continue;
            rebuildChunkMapContent(it->second);
        }
    }
}

inline int nearestMapPoiIndex(const Vec3& pos, float maxDist) {
    float best = maxDist * maxDist;
    int idx = -1;
    for (int i = 0; i < (int)mapPois.size(); i++) {
        float dx = mapPois[i].pos.x - pos.x;
        float dz = mapPois[i].pos.z - pos.z;
        float d2 = dx * dx + dz * dz;
        if (d2 < best) {
            best = d2;
            idx = i;
        }
    }
    return idx;
}
