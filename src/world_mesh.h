#pragma once
#include <vector>
#include "geometry.h"
#include "map_content.h"
#include "structure_models.h"
#include "zfight_guard.h"

// ============================================================================
// WORLD MESH
// Vertex data of everything that is baked into the level: shell (floor, ceiling,
// walls, shafts), pillars, decor (door frames, props, POI sets, bay lines) and
// lamp panels. No OpenGL here, so the build check (tools/zfight_check.cpp) meshes
// worlds exactly the way the game does.
// ============================================================================

struct WorldMesh {
    std::vector<float> walls, floors, ceilings, pillars, decor;  // pos uv normal
    std::vector<float> lampsOn, lampsOff;                        // pos uv
};

inline void worldMeshFloorQuad(std::vector<float>& v, float px, float pz, float y, float ny) {
    float q[] = {
        px, y, pz, 0, 0, 0, ny, 0,
        px, y, pz + CS, 0, 1, 0, ny, 0,
        px + CS, y, pz + CS, 1, 1, 0, ny, 0,
        px, y, pz, 0, 0, 0, ny, 0,
        px + CS, y, pz + CS, 1, 1, 0, ny, 0,
        px + CS, y, pz, 1, 0, 0, ny, 0
    };
    v.insert(v.end(), q, q + 48);
}

// holeAt(wx, wz): the floor is missing there (trap holes, the abyss).
template <class HoleFn>
inline void worldMeshCell(WorldMesh& m, int wx, int wz, HoleFn holeAt) {
    float px = wx * CS, pz = wz * CS;
    if (!holeAt(wx, wz)) {
        worldMeshFloorQuad(m.floors, px, pz, 0.0f, 1.0f);
    } else {
        const float shaftDepth = 30.0f;
        auto solid = [&](int x, int z) {
            int cell = getCellWorld(x, z);
            return cell == 1 || (cell == 0 && !holeAt(x, z));
        };
        if (solid(wx - 1, wz)) mkShaftWall(m.walls, px, pz, 0, CS, 0, shaftDepth, CS);
        if (solid(wx + 1, wz)) mkShaftWall(m.walls, px + CS, pz + CS, 0, -CS, 0, shaftDepth, CS);
        if (solid(wx, wz - 1)) mkShaftWall(m.walls, px + CS, pz, -CS, 0, 0, shaftDepth, CS);
        if (solid(wx, wz + 1)) mkShaftWall(m.walls, px, pz + CS, CS, 0, 0, shaftDepth, CS);
    }
    worldMeshFloorQuad(m.ceilings, px, pz, WH, -1.0f);
    if (getCellWorld(wx - 1, wz) == 1) mkWall(m.walls, px, pz, 0, CS, WH, CS, WH);
    if (getCellWorld(wx + 1, wz) == 1) mkWall(m.walls, px + CS, pz + CS, 0, -CS, WH, CS, WH);
    if (getCellWorld(wx, wz - 1) == 1) mkWall(m.walls, px + CS, pz, -CS, 0, WH, CS, WH);
    if (getCellWorld(wx, wz + 1) == 1) mkWall(m.walls, px, pz + CS, CS, 0, WH, CS, WH);

    int doorAxis = doorwayAxisAt(wx, wz);
    if (doorAxis != 0) buildDoorwayModel(m.decor, wx, wz, doorAxis);
}

// Bay lines are laid per open cell of parking halls, so they stop at walls.
template <class HoleFn>
inline void worldMeshParkingLines(WorldMesh& m, int cx, int cz, HoleFn holeAt) {
    if (chunkStyleAt(cx, cz) != CHUNK_STYLE_PARKING) return;
    for (int lx = 0; lx < CHUNK_SIZE; lx++) {
        for (int lz = 0; lz < CHUNK_SIZE; lz += 3) {
            int wx = cx * CHUNK_SIZE + lx, wz = cz * CHUNK_SIZE + lz;
            if (getCellWorld(wx, wz) != 0 || holeAt(wx, wz)) continue;
            DecorBuilder line(m.decor, ((float)wx + 0.5f) * CS, ((float)wz + 0.5f) * CS, 0.0f, 1.0f);
            line.decal(0.0f, 0.012f, 0.0f, CS, 0.16f, DECOR_MAT_WHITE);
        }
    }
}

// Everything around chunk (pcx, pcz) that does not depend on the run in progress.
template <class HoleFn>
inline void buildWorldBaseMesh(WorldMesh& m, int pcx, int pcz, HoleFn holeAt) {
    bool parkingLevel = isParkingLevel(gCurrentLevel);
    for (int dcx = -VIEW_CHUNKS; dcx <= VIEW_CHUNKS; dcx++) {
        for (int dcz = -VIEW_CHUNKS; dcz <= VIEW_CHUNKS; dcz++) {
            auto it = chunks.find(chunkKey(pcx + dcx, pcz + dcz));
            if (it == chunks.end()) continue;
            for (int lx = 0; lx < CHUNK_SIZE; lx++) {
                for (int lz = 0; lz < CHUNK_SIZE; lz++) {
                    if (it->second.cells[lx][lz] != 0) continue;
                    worldMeshCell(m, (pcx + dcx) * CHUNK_SIZE + lx, (pcz + dcz) * CHUNK_SIZE + lz, holeAt);
                }
            }
        }
    }
    for (auto& p : pillars) {
        mkPillar(m.pillars, p.x, p.z, PILLAR_SIZE, WH);
        buildPillarTrimModel(m.decor, p, parkingLevel);
    }
    for (const auto& pr : mapProps) buildMapPropModel(m.decor, pr);
    for (const auto& poi : mapPois) buildMapPoiModel(m.decor, poi);
    if (parkingLevel) {
        for (int dcx = -VIEW_CHUNKS; dcx <= VIEW_CHUNKS; dcx++) {
            for (int dcz = -VIEW_CHUNKS; dcz <= VIEW_CHUNKS; dcz++) {
                if (chunks.find(chunkKey(pcx + dcx, pcz + dcz)) == chunks.end()) continue;
                worldMeshParkingLines(m, pcx + dcx, pcz + dcz, holeAt);
            }
        }
    }
}

inline void buildWorldLampMesh(WorldMesh& m) {
    for (auto& l : lights) mkLight(l.on ? m.lampsOn : m.lampsOff, l.pos, l.sizeX, l.sizeZ);
}

// The layers are drawn as separate buffers but share one depth buffer, so they are
// checked together. Rank: shell < pillars < decor < lamps.
inline int worldMeshLayers(WorldMesh& m, ZfMesh* out) {
    out[0] = {&m.walls, 8, 0, "walls"};
    out[1] = {&m.floors, 8, 0, "floor"};
    out[2] = {&m.ceilings, 8, 0, "ceiling"};
    out[3] = {&m.pillars, 8, 1, "pillars"};
    out[4] = {&m.decor, 8, 2, "decor"};
    out[5] = {&m.lampsOn, 5, 3, "lamps"};
    out[6] = {&m.lampsOff, 5, 3, "dark lamps"};
    return 7;
}

inline ZfReport scanWorldMesh(WorldMesh& m) {
    ZfMesh layers[7];
    int n = worldMeshLayers(m, layers);
    return zfightScan(layers, n);
}

// What the game does with every world mesh before it is uploaded.
inline ZfReport resolveWorldMesh(WorldMesh& m) {
    ZfMesh layers[7];
    int n = worldMeshLayers(m, layers);
    return zfightRepair(layers, n, "world mesh");
}
