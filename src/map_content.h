#pragma once

#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>

#include "world.h"
#include "progression.h"
#include "prop_catalog.h"

// pos = centre of the footprint on the floor, yaw = facing (local +Z is the front).
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

// pos = centre of the POI cell. The built set stands against that cell's wall.
struct MapPoi {
    Vec3 pos;
    int type;
    float radius;
    int id;
};

extern std::vector<MapProp> mapProps;
extern std::vector<MapPoi> mapPois;

// Chunks whose props/POIs were already placed (keeps pushed props from respawning).
inline std::unordered_set<long long> mapContentChunks;

inline void resetMapContent() {
    mapProps.clear();
    mapPois.clear();
    mapContentChunks.clear();
}

inline bool isMapPropPushableType(int type) {
    return type == MAP_PROP_CRATE_STACK ||
           type == MAP_PROP_DEBRIS ||
           type == MAP_PROP_BOX_PALLET ||
           (isLibraryPropType(type) && propHasFlag(type, PROP_FLAG_PUSHABLE));
}

inline bool isMapPropWallType(int type) {
    return type == MAP_PROP_DESK ||
           type == MAP_PROP_CABINET ||
           type == MAP_PROP_LOCKER_BANK ||
           (isLibraryPropType(type) && isPropAgainstWallType(type));
}

// Unscaled, unrotated half extents of the model footprint (see prop_models.h).
inline void mapPropBaseExtents(int type, float& hx, float& hz) {
    if (type == MAP_PROP_CRATE_STACK) { hx = 0.50f; hz = 0.50f; return; }
    if (type == MAP_PROP_CONE_CLUSTER) { hx = 0.50f; hz = 0.50f; return; }
    if (type == MAP_PROP_BARRIER) { hx = 0.80f; hz = 0.24f; return; }
    if (type == MAP_PROP_CABLE_REEL) { hx = 0.50f; hz = 0.50f; return; }
    if (type == MAP_PROP_PUDDLE) { hx = 0.0f; hz = 0.0f; return; }
    if (type == MAP_PROP_DEBRIS) { hx = 0.60f; hz = 0.50f; return; }
    if (type == MAP_PROP_DESK) { hx = 0.70f; hz = 0.35f; return; }
    if (type == MAP_PROP_CHAIR) { hx = 0.25f; hz = 0.25f; return; }
    if (type == MAP_PROP_CABINET) { hx = 0.25f; hz = 0.30f; return; }
    if (type == MAP_PROP_PARTITION) { hx = 0.75f; hz = 0.14f; return; }
    if (type == MAP_PROP_BOX_PALLET) { hx = 0.60f; hz = 0.50f; return; }
    if (type == MAP_PROP_DRUM_STACK) { hx = 0.60f; hz = 0.56f; return; }
    if (type == MAP_PROP_LOCKER_BANK) { hx = 0.57f; hz = 0.23f; return; }
    if (isLibraryPropType(type)) { hx = PROP_CATALOG[type].hx; hz = PROP_CATALOG[type].hz; return; }
    hx = 0.5f; hz = 0.5f;
}

// World-space AABB half extents of the rotated, scaled footprint.
inline void mapPropHalfExtents(const MapProp& p, float& hx, float& hz) {
    float bx = 0.0f, bz = 0.0f;
    mapPropBaseExtents(p.type, bx, bz);
    float c = fabsf(cosf(p.yaw));
    float s = fabsf(sinf(p.yaw));
    hx = (bx * c + bz * s) * p.scale;
    hz = (bx * s + bz * c) * p.scale;
}

inline float mapPropCollisionRadius(const MapProp& p) {
    float hx = 0.0f, hz = 0.0f;
    mapPropHalfExtents(p, hx, hz);
    return hx > hz ? hx : hz;
}

// ---------------------------------------------------------------------------
// POI sets
// ---------------------------------------------------------------------------

// Direction from the POI cell towards the wall its set stands against.
inline void mapPoiWallDir(const MapPoi& poi, int& dx, int& dz) {
    int wx = (int)floorf(poi.pos.x / CS);
    int wz = (int)floorf(poi.pos.z / CS);
    dx = 0;
    dz = -1;
    if (getCellWorld(wx, wz - 1) == 1) return;
    if (getCellWorld(wx, wz + 1) == 1) { dz = 1; return; }
    if (getCellWorld(wx - 1, wz) == 1) { dx = -1; dz = 0; return; }
    if (getCellWorld(wx + 1, wz) == 1) { dx = 1; dz = 0; return; }
}

// Width along the wall and depth out of the wall of each built set (see poi_models.h).
inline void mapPoiSetSize(int type, float& width, float& depth) {
    if (type == MAP_POI_OFFICE) { width = 2.60f; depth = 0.80f; return; }
    if (type == MAP_POI_SERVER) { width = 2.10f; depth = 0.95f; return; }
    if (type == MAP_POI_STORAGE) { width = 2.40f; depth = 0.60f; return; }
    width = 2.20f;
    depth = 0.60f;
}

// Yaw that makes the set's local +Z point away from the wall.
inline float mapPoiYaw(const MapPoi& poi) {
    int dx = 0, dz = 0;
    mapPoiWallDir(poi, dx, dz);
    return atan2f((float)-dx, (float)-dz);
}

// Local origin of the set: on the wall face, centred on the cell.
inline Vec3 mapPoiAnchor(const MapPoi& poi) {
    int dx = 0, dz = 0;
    mapPoiWallDir(poi, dx, dz);
    return Vec3(poi.pos.x + (float)dx * CS * 0.5f, 0.0f, poi.pos.z + (float)dz * CS * 0.5f);
}

inline void mapPoiFootprint(const MapPoi& poi, float& minX, float& maxX, float& minZ, float& maxZ) {
    int dx = 0, dz = 0;
    mapPoiWallDir(poi, dx, dz);
    float width = 0.0f, depth = 0.0f;
    mapPoiSetSize(poi.type, width, depth);
    float cx = poi.pos.x + (float)dx * (CS * 0.5f - depth * 0.5f);
    float cz = poi.pos.z + (float)dz * (CS * 0.5f - depth * 0.5f);
    float hx = (dx != 0) ? depth * 0.5f : width * 0.5f;
    float hz = (dx != 0) ? width * 0.5f : depth * 0.5f;
    minX = cx - hx;
    maxX = cx + hx;
    minZ = cz - hz;
    maxZ = cz + hz;
}

inline bool collideMapPois(float x, float z, float pr) {
    for (const auto& poi : mapPois) {
        if (fabsf(poi.pos.x - x) > CS || fabsf(poi.pos.z - z) > CS) continue;
        float minX, maxX, minZ, maxZ;
        mapPoiFootprint(poi, minX, maxX, minZ, maxZ);
        if (x > minX - pr && x < maxX + pr && z > minZ - pr && z < maxZ + pr) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Collision / pushing
// ---------------------------------------------------------------------------

inline bool collideMapPropsEx(float x, float z, float pr, int ignoreIndex) {
    for (int idx = 0; idx < (int)mapProps.size(); idx++) {
        if (idx == ignoreIndex) continue;
        const MapProp& p = mapProps[idx];
        float hx = 0.0f, hz = 0.0f;
        mapPropHalfExtents(p, hx, hz);
        if (hx <= 0.001f || hz <= 0.001f || !isPropSolidType(p.type)) continue;
        if (fabsf(x - p.pos.x) < (hx + pr) && fabsf(z - p.pos.z) < (hz + pr)) return true;
    }
    return collideMapPois(x, z, pr);
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

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------

inline int countWallNeighborsWorld(int wx, int wz) {
    int walls = 0;
    if (getCellWorld(wx - 1, wz) == 1) walls++;
    if (getCellWorld(wx + 1, wz) == 1) walls++;
    if (getCellWorld(wx, wz - 1) == 1) walls++;
    if (getCellWorld(wx, wz + 1) == 1) walls++;
    return walls;
}

// Cells next to chunk-view edges are skipped (lx/lz 1..14) so a missing neighbour chunk
// is never mistaken for a wall.
inline bool isMapPropCellValid(const Chunk& c, int lx, int lz) {
    if (lx < 1 || lx > CHUNK_SIZE - 2 || lz < 1 || lz > CHUNK_SIZE - 2) return false;
    if (c.cells[lx][lz] != 0) return false;
    int wx = c.cx * CHUNK_SIZE + lx;
    int wz = c.cz * CHUNK_SIZE + lz;
    if (doorwayAxisAt(wx, wz) != 0) return false;
    return countWallNeighborsWorld(wx, wz) <= 2;
}

// True when the footprint touches a wall cell, a doorway, a pillar, a prop or a POI set.
inline bool isMapPropSpotBlocked(const MapProp& pr) {
    float hx = 0.0f, hz = 0.0f;
    mapPropHalfExtents(pr, hx, hz);
    int x0 = (int)floorf((pr.pos.x - hx) / CS), x1 = (int)floorf((pr.pos.x + hx) / CS);
    int z0 = (int)floorf((pr.pos.z - hz) / CS), z1 = (int)floorf((pr.pos.z + hz) / CS);
    for (int wx = x0; wx <= x1; wx++) {
        for (int wz = z0; wz <= z1; wz++) {
            if (getCellWorld(wx, wz) != 0) return true;
            if (doorwayAxisAt(wx, wz) != 0) return true;
        }
    }
    const float gap = 0.06f;
    for (const auto& p : pillars) {
        float half = PILLAR_SIZE * 0.5f + gap;
        if (fabsf(p.x - pr.pos.x) < hx + half && fabsf(p.z - pr.pos.z) < hz + half) return true;
    }
    for (const auto& other : mapProps) {
        float ox = 0.0f, oz = 0.0f;
        mapPropHalfExtents(other, ox, oz);
        // overhead props (pipes, ducts) share a spot with whatever stands below them
        if (isPropOverheadType(other.type) != isPropOverheadType(pr.type)) continue;
        if (other.type == MAP_PROP_PUDDLE || pr.type == MAP_PROP_PUDDLE) {
            if (other.type != pr.type) continue;
            ox = oz = hx = hz = 1.0f;
        }
        if (fabsf(other.pos.x - pr.pos.x) < hx + ox + gap && fabsf(other.pos.z - pr.pos.z) < hz + oz + gap) return true;
    }
    for (const auto& poi : mapPois) {
        float minX, maxX, minZ, maxZ;
        mapPoiFootprint(poi, minX, maxX, minZ, maxZ);
        if (pr.pos.x + hx + gap > minX && pr.pos.x - hx - gap < maxX &&
            pr.pos.z + hz + gap > minZ && pr.pos.z - hz - gap < maxZ) return true;
    }
    return false;
}

inline bool addMapProp(const Vec3& pos, int type, float scale, float yaw) {
    MapProp pr{};
    pr.pos = Vec3(pos.x, 0.0f, pos.z);
    pr.type = type;
    pr.scale = scale;
    pr.yaw = yaw;
    if (isMapPropSpotBlocked(pr)) return false;
    mapProps.push_back(pr);
    return true;
}

inline Vec3 cellCenter(int wx, int wz) {
    return Vec3(((float)wx + 0.5f) * CS, 0.0f, ((float)wz + 0.5f) * CS);
}

inline float rand01(std::mt19937& r) {
    return (float)(r() % 10000u) / 10000.0f;
}

// Picks one of the walls around the cell. Returns false when the cell has none.
inline bool pickWallSide(int wx, int wz, std::mt19937& r, int& dx, int& dz) {
    const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    int found[4];
    int n = 0;
    for (int i = 0; i < 4; i++) {
        if (getCellWorld(wx + dirs[i][0], wz + dirs[i][1]) == 1) found[n++] = i;
    }
    if (n == 0) return false;
    int pick = found[r() % (unsigned)n];
    dx = dirs[pick][0];
    dz = dirs[pick][1];
    return true;
}

// Puts the prop flush against the wall at (dx, dz), facing the room. `slide` moves it
// along the wall (metres from the cell centre).
inline bool placePropOnWall(int wx, int wz, int dx, int dz, int type, float scale, float slide, float out = 0.0f) {
    float bx = 0.0f, bz = 0.0f;
    mapPropBaseExtents(type, bx, bz);
    float maxSlide = CS * 0.5f - bx * scale - 0.12f;
    if (slide > maxSlide) slide = maxSlide;
    if (slide < -maxSlide) slide = -maxSlide;
    float reach = CS * 0.5f - bz * scale - 0.02f - out;
    Vec3 c = cellCenter(wx, wz);
    Vec3 pos(c.x + (float)dx * reach - (float)dz * slide, 0.0f, c.z + (float)dz * reach + (float)dx * slide);
    return addMapProp(pos, type, scale, atan2f((float)-dx, (float)-dz));
}

inline bool placePropLoose(int wx, int wz, int type, float scale, std::mt19937& r) {
    Vec3 c = cellCenter(wx, wz);
    for (int attempt = 0; attempt < 3; attempt++) {
        float reach = CS * (0.30f - 0.08f * (float)attempt);
        Vec3 pos(c.x + (rand01(r) * 2.0f - 1.0f) * reach, 0.0f, c.z + (rand01(r) * 2.0f - 1.0f) * reach);
        float yaw = rand01(r) * 6.2831853f;
        if (type == MAP_PROP_PARTITION || type == MAP_PROP_BARRIER || propHasFlag(type, PROP_FLAG_ALIGNED)) yaw = (r() & 1u) ? 0.0f : 1.5707963f;
        if (addMapProp(pos, type, scale, yaw)) return true;
    }
    return false;
}

inline bool placeProp(int wx, int wz, int type, std::mt19937& r) {
    if (isMapPropWallType(type)) {
        int dx = 0, dz = 0;
        if (!pickWallSide(wx, wz, r, dx, dz)) return false;
        return placePropOnWall(wx, wz, dx, dz, type, 1.0f, (rand01(r) - 0.5f) * 3.0f);
    }
    float scale = 1.0f;
    if (type == MAP_PROP_CRATE_STACK || type == MAP_PROP_DEBRIS || type == MAP_PROP_BOX_PALLET || type == MAP_PROP_PUDDLE) {
        scale = 0.88f + rand01(r) * 0.24f;
    }
    return placePropLoose(wx, wz, type, scale, r);
}

inline int chooseChunkPropType(int weighted) {
    if (isParkingLevel(gCurrentLevel)) {
        if (weighted < 24) return MAP_PROP_BARRIER;
        if (weighted < 43) return MAP_PROP_CONE_CLUSTER;
        if (weighted < 62) return MAP_PROP_DRUM_STACK;
        if (weighted < 76) return MAP_PROP_CABLE_REEL;
        if (weighted < 88) return MAP_PROP_LOCKER_BANK;
        if (weighted < 96) return MAP_PROP_DEBRIS;
        return MAP_PROP_PUDDLE;
    }
    if (weighted < 26) return MAP_PROP_CRATE_STACK;
    if (weighted < 46) return MAP_PROP_BOX_PALLET;
    if (weighted < 56) return MAP_PROP_BARRIER;
    if (weighted < 66) return MAP_PROP_CABLE_REEL;
    if (weighted < 74) return MAP_PROP_DRUM_STACK;
    if (weighted < 82) return MAP_PROP_DEBRIS;
    if (weighted < 90) return MAP_PROP_LOCKER_BANK;
    if (weighted < 96) return MAP_PROP_CABINET;
    return MAP_PROP_PUDDLE;
}

inline void spawnChunkProps(const Chunk& c, std::mt19937& r) {
    int want = isParkingLevel(gCurrentLevel) ? (3 + (int)(r() % 3u)) : (3 + (int)(r() % 2u));
    int placed = 0;
    for (int attempt = 0; attempt < 40 && placed < want; attempt++) {
        int lx = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int lz = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int type = chooseChunkPropType((int)(r() % 100u));
        if (!isMapPropCellValid(c, lx, lz)) continue;
        if (placeProp(c.cx * CHUNK_SIZE + lx, c.cz * CHUNK_SIZE + lz, type, r)) placed++;
    }
}

// A themed pile: several related props sharing one corner of a room.
inline void spawnChunkCluster(const Chunk& c, std::mt19937& r) {
    if ((r() % 100u) >= 55u) return;
    const int themes[3][2] = {
        {MAP_PROP_CRATE_STACK, MAP_PROP_BOX_PALLET},
        {MAP_PROP_CABLE_REEL, MAP_PROP_DRUM_STACK},
        {MAP_PROP_BOX_PALLET, MAP_PROP_DEBRIS}
    };
    int theme = (int)(r() % 3u);
    for (int attempt = 0; attempt < 12; attempt++) {
        int lx = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int lz = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        if (!isMapPropCellValid(c, lx, lz)) continue;
        int wx = c.cx * CHUNK_SIZE + lx;
        int wz = c.cz * CHUNK_SIZE + lz;
        if (countWallNeighborsWorld(wx, wz) == 0) continue;
        int count = 3 + (int)(r() % 2u);
        for (int j = 0; j < count; j++) placeProp(wx, wz, themes[theme][j % 2], r);
        return;
    }
}

// Work spots: desk against a wall, chair pulled up to it, cabinet beside it.
inline void spawnOfficeFurniture(const Chunk& c, std::mt19937& r) {
    if (chunkStyleAt(c.cx, c.cz) != CHUNK_STYLE_OFFICE) return;
    int want = 5 + (int)(r() % 4u);
    int placed = 0;
    for (int attempt = 0; attempt < 60 && placed < want; attempt++) {
        int lx = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        int lz = 1 + (int)(r() % (unsigned)(CHUNK_SIZE - 2));
        if (!isMapPropCellValid(c, lx, lz)) continue;
        int wx = c.cx * CHUNK_SIZE + lx;
        int wz = c.cz * CHUNK_SIZE + lz;
        int dx = 0, dz = 0;
        if (!pickWallSide(wx, wz, r, dx, dz)) continue;

        float slide = (rand01(r) - 0.5f) * 1.2f;
        if (!placePropOnWall(wx, wz, dx, dz, MAP_PROP_DESK, 1.0f, slide)) continue;
        placed++;
        const MapProp desk = mapProps.back();
        Vec3 front(sinf(desk.yaw), 0.0f, cosf(desk.yaw));
        Vec3 chairPos = desk.pos + front * (0.35f + 0.38f + rand01(r) * 0.25f);
        addMapProp(chairPos, MAP_PROP_CHAIR, 1.0f, desk.yaw + 3.1415927f + (rand01(r) - 0.5f) * 0.9f);
        float side = (r() & 1u) ? 1.0f : -1.0f;
        if ((r() % 100u) < 60u) placePropOnWall(wx, wz, dx, dz, MAP_PROP_CABINET, 1.0f, slide + side * 1.08f);
        if ((r() % 100u) < 35u) placePropOnWall(wx, wz, dx, dz, MAP_PROP_LOCKER_BANK, 1.0f, slide - side * 1.45f);
        if ((r() % 100u) < 30u) placePropLoose(wx, wz, MAP_PROP_PARTITION, 1.0f, r);
    }
}

inline void spawnChunkPoi(const Chunk& c, std::mt19937& r) {
    if ((int)(r() % 100u) > 45) return;
    for (int tries = 0; tries < 14; tries++) {
        int lx = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int lz = 2 + (int)(r() % (unsigned)(CHUNK_SIZE - 4));
        int typeRoll = (int)(r() % 100u);
        if (!isMapPropCellValid(c, lx, lz)) continue;
        int wx = c.cx * CHUNK_SIZE + lx;
        int wz = c.cz * CHUNK_SIZE + lz;
        // Exactly one wall behind the set, open floor on the other three sides.
        if (countWallNeighborsWorld(wx, wz) != 1) continue;
        int type = MAP_POI_RESTROOM;
        if (typeRoll < 46) type = MAP_POI_OFFICE;
        else if (typeRoll < 66) type = MAP_POI_SERVER;
        else if (typeRoll < 86) type = MAP_POI_STORAGE;
        MapPoi poi{};
        poi.pos = cellCenter(wx, wz);
        poi.type = type;
        poi.radius = CS * 1.8f;
        poi.id = (c.cx * 73856093) ^ (c.cz * 19349663) ^ (lx * 83492791) ^ (lz * 297121507);
        mapPois.push_back(poi);
        return;
    }
}

inline void spawnThemedChunkProps(const Chunk& c, std::mt19937& r);

inline void rebuildChunkMapContent(const Chunk& c) {
    std::mt19937 r(hashCell(c.cx, c.cz, 0xA18F331u));
    spawnChunkPoi(c, r);
    spawnOfficeFurniture(c, r);
    spawnChunkCluster(c, r);
    spawnChunkProps(c, r);
    spawnThemedChunkProps(c, r);
}

inline bool isMapPropOnWall(const MapProp& p) {
    int wx = (int)floorf(p.pos.x / CS);
    int wz = (int)floorf(p.pos.z / CS);
    return getCellWorld(wx, wz) == 1;
}

inline bool isChunkInView(float x, float z, int pcx, int pcz) {
    int cx = (int)floorf(x / (CS * CHUNK_SIZE));
    int cz = (int)floorf(z / (CS * CHUNK_SIZE));
    return abs(cx - pcx) <= VIEW_CHUNKS && abs(cz - pcz) <= VIEW_CHUNKS;
}

inline void updateMapContent(int pcx, int pcz) {
    if (mapProps.empty() && mapPois.empty()) mapContentChunks.clear();

    mapProps.erase(
        std::remove_if(
            mapProps.begin(),
            mapProps.end(),
            [&](const MapProp& p) { return !isChunkInView(p.pos.x, p.pos.z, pcx, pcz) || isMapPropOnWall(p); }
        ),
        mapProps.end()
    );
    mapPois.erase(
        std::remove_if(
            mapPois.begin(),
            mapPois.end(),
            [&](const MapPoi& p) {
                if (!isChunkInView(p.pos.x, p.pos.z, pcx, pcz)) return true;
                return getCellWorld((int)floorf(p.pos.x / CS), (int)floorf(p.pos.z / CS)) == 1;
            }
        ),
        mapPois.end()
    );
    for (auto it = mapContentChunks.begin(); it != mapContentChunks.end();) {
        int cx = (int)(*it >> 32);
        int cz = (int)(*it & 0xFFFFFFFF);
        if (abs(cx - pcx) > VIEW_CHUNKS || abs(cz - pcz) > VIEW_CHUNKS) it = mapContentChunks.erase(it);
        else ++it;
    }

    // Inner chunks first: their neighbours exist, so wall lookups are exact.
    for (int ring = 0; ring <= VIEW_CHUNKS; ring++) {
        for (int dcx = -ring; dcx <= ring; dcx++) {
            for (int dcz = -ring; dcz <= ring; dcz++) {
                if (abs(dcx) != ring && abs(dcz) != ring) continue;
                long long key = chunkKey(pcx + dcx, pcz + dcz);
                auto it = chunks.find(key);
                if (it == chunks.end()) continue;
                if (!mapContentChunks.insert(key).second) continue;
                rebuildChunkMapContent(it->second);
            }
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

#include "map_theme_spawn.h"
