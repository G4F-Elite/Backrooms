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

extern std::vector<MapProp> mapProps;

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

inline bool isMapPropPushableType(int type) {
    return type == MAP_PROP_CRATE_STACK ||
           type == MAP_PROP_DEBRIS ||
           type == MAP_PROP_BOX_PALLET;
}

inline float mapPropCollisionRadius(const MapProp& p) {
    if (p.type == MAP_PROP_PUDDLE) return 0.0f;
    if (p.type == MAP_PROP_CONE_CLUSTER) return 0.72f;
    if (p.type == MAP_PROP_BARRIER) return 0.96f;
    if (p.type == MAP_PROP_CABLE_REEL) return 0.66f;
    if (p.type == MAP_PROP_DEBRIS) return 0.58f;
    if (p.type == MAP_PROP_CHAIR) return 0.46f;
    if (p.type == MAP_PROP_DESK) return 0.92f;
    if (p.type == MAP_PROP_CABINET) return 0.80f;
    if (p.type == MAP_PROP_PARTITION) return 0.95f;
    if (p.type == MAP_PROP_BOX_PALLET) return 0.86f;
    if (p.type == MAP_PROP_DRUM_STACK) return 0.68f;
    if (p.type == MAP_PROP_LOCKER_BANK) return 0.84f;
    return 0.78f;
}

inline bool collideMapPropsEx(float x, float z, float pr, int ignoreIndex) {
    for (const auto& p : mapProps) {
        int idx = (int)(&p - &mapProps[0]);
        if (idx == ignoreIndex) continue;
        float r = mapPropCollisionRadius(p);
        if (r <= 0.001f) continue;
        if (fabsf(x - p.pos.x) < (r + pr) && fabsf(z - p.pos.z) < (r + pr)) return true;
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
        float r = mapPropCollisionRadius(p);
        if (r <= 0.001f) continue;
        if (fabsf(playerX - p.pos.x) >= (r + pr)) continue;
        if (fabsf(playerZ - p.pos.z) >= (r + pr)) continue;
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
    float propRadius = mapPropCollisionRadius(p) * 0.82f;
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
    Vec3 wp(
        ((float)(cx * CHUNK_SIZE + lx) + 0.5f) * CS,
        0.0f,
        ((float)(cz * CHUNK_SIZE + lz) + 0.5f) * CS
    );
    if (hasMapPropNear(wp, CS * 0.55f)) return;
    MapProp pr{};
    pr.pos = wp;
    pr.type = type;
    pr.scale = scale;
    pr.yaw = yaw;
    mapProps.push_back(pr);
}

inline void spawnChunkProps(const Chunk& c) {
    std::mt19937 cr(chunkMapContentSeed(c.cx, c.cz, 0xA18F331u));
    int baseCount = 2 + (int)(cr() % 3);
    int maxAttempts = 52;

    for (int i = 0; i < baseCount && maxAttempts > 0; i++) {
        int lx = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        int lz = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        if (!isMapPropCellValid(c, lx, lz)) {
            maxAttempts--;
            continue;
        }
        int weighted = (int)(cr() % 100);
        int type = MAP_PROP_CRATE_STACK;
        if (weighted < 34) type = MAP_PROP_CRATE_STACK;
        else if (weighted < 56) type = MAP_PROP_BOX_PALLET;
        else if (weighted < 68) type = MAP_PROP_BARRIER;
        else if (weighted < 77) type = MAP_PROP_CABLE_REEL;
        else if (weighted < 84) type = MAP_PROP_DRUM_STACK;
        else if (weighted < 90) type = MAP_PROP_DEBRIS;
        else if (weighted < 94) type = MAP_PROP_LOCKER_BANK;
        else if (weighted < 97) type = MAP_PROP_CABINET;
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
    int rows = 1 + (int)(cr() % 2);
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
    }
}

inline void rebuildChunkMapContent(const Chunk& c) {
    spawnChunkProps(c);
    spawnChunkPoiClusters(c);
    spawnOfficeFurniture(c);
}

inline bool isMapPropTooFar(const MapProp& p, float cx, float cz, float md) {
    return fabsf(p.pos.x - cx) > md || fabsf(p.pos.z - cz) > md;
}

inline bool isMapPropOnWall(const MapProp& p) {
    int wx = (int)floorf(p.pos.x / CS);
    int wz = (int)floorf(p.pos.z / CS);
    return getCellWorld(wx, wz) == 1;
}

inline void updateMapContent(int pcx, int pcz) {
    float cx = (pcx + 0.5f) * CHUNK_SIZE * CS;
    float cz = (pcz + 0.5f) * CHUNK_SIZE * CS;
    float md = (VIEW_CHUNKS + 1) * CHUNK_SIZE * CS;

    mapProps.erase(
        std::remove_if(
            mapProps.begin(),
            mapProps.end(),
            [&](const MapProp& p) { return isMapPropTooFar(p, cx, cz, md) || isMapPropOnWall(p); }
        ),
        mapProps.end()
    );

    for (int dcx = -VIEW_CHUNKS; dcx <= VIEW_CHUNKS; dcx++) {
        for (int dcz = -VIEW_CHUNKS; dcz <= VIEW_CHUNKS; dcz++) {
            auto it = chunks.find(chunkKey(pcx + dcx, pcz + dcz));
            if (it == chunks.end()) continue;
            rebuildChunkMapContent(it->second);
        }
    }
}
