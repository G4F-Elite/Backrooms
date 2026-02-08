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
    MAP_PROP_DEBRIS = 5
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

inline float mapPropCollisionRadius(const MapProp& p) {
    if (p.type == MAP_PROP_PUDDLE) return 0.0f;
    if (p.type == MAP_PROP_CONE_CLUSTER) return 0.34f * CS;
    if (p.type == MAP_PROP_BARRIER) return 0.48f * CS;
    if (p.type == MAP_PROP_CABLE_REEL) return 0.30f * CS;
    if (p.type == MAP_PROP_DEBRIS) return 0.27f * CS;
    return 0.36f * CS;
}

inline bool collideMapProps(float x, float z, float pr) {
    for (const auto& p : mapProps) {
        float r = mapPropCollisionRadius(p);
        if (r <= 0.001f) continue;
        if (fabsf(x - p.pos.x) < (r + pr) && fabsf(z - p.pos.z) < (r + pr)) return true;
    }
    return false;
}

inline int countOpenNeighbors(const Chunk& c, int lx, int lz) {
    int open = 0;
    if (lx > 0 && c.cells[lx - 1][lz] == 0) open++;
    if (lx < CHUNK_SIZE - 1 && c.cells[lx + 1][lz] == 0) open++;
    if (lz > 0 && c.cells[lx][lz - 1] == 0) open++;
    if (lz < CHUNK_SIZE - 1 && c.cells[lx][lz + 1] == 0) open++;
    return open;
}

inline bool isMapPropCellValid(const Chunk& c, int lx, int lz) {
    if (lx < 2 || lx > CHUNK_SIZE - 3 || lz < 2 || lz > CHUNK_SIZE - 3) return false;
    if (c.cells[lx][lz] != 0) return false;
    return countOpenNeighbors(c, lx, lz) >= 2;
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
    int baseCount = 4 + (int)(cr() % 7);
    int maxAttempts = 60;

    for (int i = 0; i < baseCount && maxAttempts > 0; i++) {
        int lx = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        int lz = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        if (!isMapPropCellValid(c, lx, lz)) {
            maxAttempts--;
            continue;
        }
        int weighted = (int)(cr() % 100);
        int type = MAP_PROP_DEBRIS;
        if (weighted < 26) type = MAP_PROP_CRATE_STACK;
        else if (weighted < 40) type = MAP_PROP_CABLE_REEL;
        else if (weighted < 54) type = MAP_PROP_CONE_CLUSTER;
        else if (weighted < 68) type = MAP_PROP_BARRIER;
        else if (weighted < 84) type = MAP_PROP_DEBRIS;
        else type = MAP_PROP_PUDDLE;
        float scale = 0.78f + ((float)(cr() % 45) / 100.0f);
        float yaw = ((float)(cr() % 628) / 100.0f);
        pushMapPropUnique(c.cx, c.cz, lx, lz, type, scale, yaw);
    }
}

inline void spawnChunkPoiClusters(const Chunk& c) {
    std::mt19937 cr(chunkMapContentSeed(c.cx, c.cz, 0x39BC21u));
    int clusterCount = 1 + (int)(cr() % 2);

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
            if (theme == 0) type = (j % 2 == 0) ? MAP_PROP_CRATE_STACK : MAP_PROP_BARRIER;
            if (theme == 1) type = (j % 2 == 0) ? MAP_PROP_CABLE_REEL : MAP_PROP_CONE_CLUSTER;
            if (theme == 2) type = (j % 2 == 0) ? MAP_PROP_PUDDLE : MAP_PROP_DEBRIS;
            float scale = 0.88f + ((float)(cr() % 35) / 100.0f);
            float yaw = ((float)(cr() % 628) / 100.0f);
            pushMapPropUnique(c.cx, c.cz, lx, lz, type, scale, yaw);
        }
    }
}

inline void rebuildChunkMapContent(const Chunk& c) {
    spawnChunkProps(c);
    spawnChunkPoiClusters(c);
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
