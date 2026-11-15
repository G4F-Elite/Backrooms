#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_map>
#include "math.h"
#include "progression.h"

extern const float CS, WH;
extern std::mt19937 rng;
extern unsigned int worldSeed;

const int CHUNK_SIZE = 16;
const int VIEW_CHUNKS = 2;
const float PILLAR_SIZE = 0.6f;

struct Chunk { int cx, cz, cells[CHUNK_SIZE][CHUNK_SIZE]; bool gen; };
struct Light { Vec3 pos; float sizeX, sizeZ, intensity; bool on; };

extern std::unordered_map<long long, Chunk> chunks;
extern std::vector<Light> lights;
extern std::vector<Vec3> pillars;
extern int playerChunkX, playerChunkZ;

inline long long chunkKey(int cx, int cz) { return ((long long)cx << 32) | (cz & 0xFFFFFFFF); }

inline bool isInnerCell(int x, int z) {
    return x > 0 && x < CHUNK_SIZE - 1 && z > 0 && z < CHUNK_SIZE - 1;
}

inline void carveRect(Chunk& c, int x0, int z0, int w, int h) {
    for (int x = x0; x < x0 + w; x++) {
        for (int z = z0; z < z0 + h; z++) {
            if (isInnerCell(x, z)) c.cells[x][z] = 0;
        }
    }
}

inline void fillRect(Chunk& c, int x0, int z0, int w, int h) {
    for (int x = x0; x < x0 + w; x++) {
        for (int z = z0; z < z0 + h; z++) {
            if (isInnerCell(x, z)) c.cells[x][z] = 1;
        }
    }
}

inline int countOpenCells(const Chunk& c) {
    int open = 0;
    for (int x = 1; x < CHUNK_SIZE - 1; x++) {
        for (int z = 1; z < CHUNK_SIZE - 1; z++) {
            if (c.cells[x][z] == 0) open++;
        }
    }
    return open;
}

inline int countWallsAround(const Chunk& c, int x, int z) {
    int walls = 0;
    if (x <= 0 || c.cells[x-1][z] == 1) walls++;
    if (x >= CHUNK_SIZE-1 || c.cells[x+1][z] == 1) walls++;
    if (z <= 0 || c.cells[x][z-1] == 1) walls++;
    if (z >= CHUNK_SIZE-1 || c.cells[x][z+1] == 1) walls++;
    return walls;
}

#include "world_layout.h"

inline int getCellWorld(int wx, int wz) {
    int cx = wx >= 0 ? wx / CHUNK_SIZE : (wx - CHUNK_SIZE + 1) / CHUNK_SIZE;
    int cz = wz >= 0 ? wz / CHUNK_SIZE : (wz - CHUNK_SIZE + 1) / CHUNK_SIZE;
    int lx = wx - cx * CHUNK_SIZE, lz = wz - cz * CHUNK_SIZE;
    auto it = chunks.find(chunkKey(cx, cz));
    return (it == chunks.end()) ? 1 : it->second.cells[lx][lz];
}

inline void setCellWorld(int wx, int wz, int val) {
    int cx = wx >= 0 ? wx / CHUNK_SIZE : (wx - CHUNK_SIZE + 1) / CHUNK_SIZE;
    int cz = wz >= 0 ? wz / CHUNK_SIZE : (wz - CHUNK_SIZE + 1) / CHUNK_SIZE;
    int lx = wx - cx * CHUNK_SIZE, lz = wz - cz * CHUNK_SIZE;
    auto it = chunks.find(chunkKey(cx, cz));
    if (it != chunks.end()) it->second.cells[lx][lz] = val;
}

inline void generateChunk(int cx, int cz) {
    long long key = chunkKey(cx, cz);
    if (chunks.find(key) != chunks.end()) return;
    Chunk c{};
    c.cx = cx;
    c.cz = cz;
    c.gen = true;
    buildChunkLayout(c);
    chunks[key] = c;
}

inline void updateVisibleChunks(float px, float pz) {
    int pcx = (int)floorf(px / (CS * CHUNK_SIZE)), pcz = (int)floorf(pz / (CS * CHUNK_SIZE));
    for (int dx = -VIEW_CHUNKS; dx <= VIEW_CHUNKS; dx++) for (int dz = -VIEW_CHUNKS; dz <= VIEW_CHUNKS; dz++) generateChunk(pcx+dx, pcz+dz);
    std::vector<long long> rm; for (auto& p : chunks) { int d = abs(p.second.cx-pcx) > abs(p.second.cz-pcz) ? abs(p.second.cx-pcx) : abs(p.second.cz-pcz); if (d > VIEW_CHUNKS+1) rm.push_back(p.first); }
    for (auto k : rm) chunks.erase(k);
    playerChunkX = pcx; playerChunkZ = pcz;
}

inline bool circleOverlapsAabb2D(float x, float z, float r, float minX, float maxX, float minZ, float maxZ) {
    float clx = x < minX ? minX : (x > maxX ? maxX : x);
    float clz = z < minZ ? minZ : (z > maxZ ? maxZ : z);
    float dx = x - clx;
    float dz = z - clz;
    return dx * dx + dz * dz < r * r;
}

// Exit portal cells: always framed, passage along Z.
inline std::vector<std::pair<int, int>> forcedDoorways;

inline bool isForcedDoorway(int wx, int wz) {
    for (const auto& d : forcedDoorways) {
        if (d.first == wx && d.second == wz) return true;
    }
    return false;
}

// 0 = no doorway, 1 = passage runs along Z (frame spans X), 2 = passage runs along X.
// Only single-cell holes in a wall line get a frame; stretches of corridor do not.
inline int doorwayAxisAt(int wx, int wz) {
    if (isForcedDoorway(wx, wz)) return 1;
    if (getCellWorld(wx, wz) != 0) return 0;
    bool wallL = getCellWorld(wx - 1, wz) == 1;
    bool wallR = getCellWorld(wx + 1, wz) == 1;
    bool wallB = getCellWorld(wx, wz - 1) == 1;
    bool wallF = getCellWorld(wx, wz + 1) == 1;
    bool alongZ = wallL && wallR && !wallB && !wallF;
    bool alongX = wallB && wallF && !wallL && !wallR;
    if (!alongZ && !alongX) return 0;
    if (alongZ) {
        if (getCellWorld(wx - 1, wz - 1) == 1 && getCellWorld(wx + 1, wz - 1) == 1) return 0;
        if (getCellWorld(wx - 1, wz + 1) == 1 && getCellWorld(wx + 1, wz + 1) == 1) return 0;
    } else {
        if (getCellWorld(wx - 1, wz - 1) == 1 && getCellWorld(wx - 1, wz + 1) == 1) return 0;
        if (getCellWorld(wx + 1, wz - 1) == 1 && getCellWorld(wx + 1, wz + 1) == 1) return 0;
    }
    if ((hashCell(wx, wz, 0xD00Au) % 100u) >= 60u) return 0;
    return alongZ ? 1 : 2;
}

// Shared by collision and by the mesh builder so they can never drift apart.
struct DoorwayDims {
    float openingHalf;  // half width of the walkable opening
    float openingH;     // height of the opening
    float postW;        // door post width
    float frameT;       // frame depth along the passage
    float fillT;        // depth of the wall infill around the frame
};

inline DoorwayDims doorwayDims() {
    DoorwayDims d;
    d.openingHalf = CS * 0.23f;
    d.openingH = WH * 0.62f;
    d.postW = CS * 0.06f;
    d.frameT = CS * 0.10f;
    d.fillT = CS * 0.092f;
    return d;
}

inline bool collideDoorFrameAtCell(float x, float z, float r, int wx, int wz) {
    int axis = doorwayAxisAt(wx, wz);
    if (axis == 0) return false;
    DoorwayDims d = doorwayDims();
    float cxCell = (wx + 0.5f) * CS;
    float czCell = (wz + 0.5f) * CS;
    // Work in a frame where `a` runs across the opening and `b` along the passage.
    float a = (axis == 1) ? (x - cxCell) : (z - czCell);
    float b = (axis == 1) ? (z - czCell) : (x - cxCell);
    float inner = d.openingHalf - d.postW * 0.5f;
    float half = CS * 0.5f;
    float frameHalf = d.frameT * 0.5f;
    if (circleOverlapsAabb2D(a, b, r, -half, -inner, -frameHalf, frameHalf)) return true;
    if (circleOverlapsAabb2D(a, b, r, inner, half, -frameHalf, frameHalf)) return true;
    return false;
}

inline bool collideWorld(float x, float z, float PR) {
    int wx = (int)floorf(x/CS), wz = (int)floorf(z/CS);
    float PR2 = PR * PR;
    for (int ddx = -1; ddx <= 1; ddx++) for (int ddz = -1; ddz <= 1; ddz++) {
        int chkx = wx+ddx, chkz = wz+ddz;
        if (getCellWorld(chkx, chkz) == 1) { float wx0 = chkx*CS, wx1 = (chkx+1)*CS, wz0 = chkz*CS, wz1 = (chkz+1)*CS;
            float clx = x<wx0?wx0:(x>wx1?wx1:x), clz = z<wz0?wz0:(z>wz1?wz1:z);
            float dx = x-clx, dz = z-clz;
            if (dx*dx+dz*dz < PR2) return true; }}
    for (int ddx = -1; ddx <= 1; ddx++) for (int ddz = -1; ddz <= 1; ddz++) {
        int chkx = wx + ddx, chkz = wz + ddz;
        if (collideDoorFrameAtCell(x, z, PR, chkx, chkz)) return true;
    }
    for (const auto& p : pillars) if (fabsf(x-p.x) < PILLAR_SIZE*0.5f+PR && fabsf(z-p.z) < PILLAR_SIZE*0.5f+PR) return true;
    return false;
}

inline int countReachLocal(int sx, int sz, int range) {
    // Use a flat bitmap for O(1) visited checks instead of O(n) linear scan
    int side = range * 2;
    std::vector<bool> visited(side * side, false);
    std::vector<std::pair<int,int>> q;
    q.reserve(side * side);
    q.push_back({sx, sz});
    int cnt = 0;
    while (!q.empty() && cnt < range*range) {
        auto c = q.back(); q.pop_back();
        int lx = c.first - sx + range;
        int lz = c.second - sz + range;
        if (lx < 0 || lx >= side || lz < 0 || lz >= side) continue;
        int idx = lx * side + lz;
        if (visited[idx]) continue;
        visited[idx] = true;
        if (getCellWorld(c.first, c.second) == 1) continue;
        cnt++;
        int dx[] = {1, -1, 0, 0}, dz[] = {0, 0, 1, -1};
        for (int d = 0; d < 4; d++) {
            int nx = c.first + dx[d], nz = c.second + dz[d];
            if (abs(nx - sx) < range && abs(nz - sz) < range) q.push_back({nx, nz});
        }
    }
    return cnt;
}

inline bool reshuffleBehind(float camX, float camZ, float camYaw) {
    int px = (int)floorf(camX/CS), pz = (int)floorf(camZ/CS); if (getCellWorld(px,pz)!=0) return false;
    float fx = sinf(camYaw), fz = cosf(camYaw); int changed = 0, att = 0, origR = countReachLocal(px,pz,12), minR = (origR*3)/4; if(minR<20)minR=20;
    while (changed < 4 && att < 50) { att++; int rx = px+(int)((rng()%16)-8), rz = pz+(int)((rng()%16)-8);
        float ddx=(float)(rx-px), ddz=(float)(rz-pz), dist=sqrtf(ddx*ddx+ddz*ddz); if(dist<3||dist>10) continue;
        if((ddx*fx+ddz*fz)/dist>-0.4f) continue; if(abs(rx-px)<=1&&abs(rz-pz)<=1) continue;
        if(isForcedDoorway(rx,rz)||isForcedDoorway(rx-1,rz)||isForcedDoorway(rx+1,rz)) continue;
        int ov=getCellWorld(rx,rz), nv=(ov==0)?1:0; setCellWorld(rx,rz,nv);
        if(countReachLocal(px,pz,12)<minR){ setCellWorld(rx,rz,ov); continue; } changed++; }
    if(changed==0) return false;
    lights.erase(std::remove_if(lights.begin(),lights.end(),[](Light&l){return getCellWorld((int)floorf(l.pos.x/CS),(int)floorf(l.pos.z/CS))==1;}),lights.end());
    pillars.erase(std::remove_if(pillars.begin(),pillars.end(),[](Vec3&p){return getCellWorld((int)floorf(p.x/CS),(int)floorf(p.z/CS))==1;}),pillars.end());
    return true;
}

// True when the cell and all 8 neighbours are open (inside a hall, not a passage).
inline bool isOpenHallCell(int wx, int wz) {
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            if (getCellWorld(wx + dx, wz + dz) != 0) return false;
        }
    }
    return true;
}

inline void updateLightsAndPillars(int pcx, int pcz) {
    float cx = (pcx+0.5f)*CHUNK_SIZE*CS, cz = (pcz+0.5f)*CHUNK_SIZE*CS, md = (VIEW_CHUNKS+1)*CHUNK_SIZE*CS;
    lights.erase(std::remove_if(lights.begin(),lights.end(),[&](Light&l){ return fabsf(l.pos.x-cx)>md||fabsf(l.pos.z-cz)>md; }),lights.end());
    pillars.erase(std::remove_if(pillars.begin(),pillars.end(),[&](Vec3&p){
        if (fabsf(p.x-cx)>md||fabsf(p.z-cz)>md) return true;
        int wx = (int)floorf(p.x / CS);
        int wz = (int)floorf(p.z / CS);
        return getCellWorld(wx, wz) == 1;
    }),pillars.end());
    for (int dcx=-VIEW_CHUNKS; dcx<=VIEW_CHUNKS; dcx++) for (int dcz=-VIEW_CHUNKS; dcz<=VIEW_CHUNKS; dcz++) {
        auto it = chunks.find(chunkKey(pcx+dcx, pcz+dcz)); if (it==chunks.end()) continue;
        unsigned int seed = worldSeed ^ (unsigned)((pcx+dcx)*12345+(pcz+dcz)*67890);
        std::mt19937 lr(seed);
        for (int lx=0; lx<CHUNK_SIZE; lx++) for (int lz=0; lz<CHUNK_SIZE; lz++) {
            int gwx = (pcx + dcx) * CHUNK_SIZE + lx;
            int gwz = (pcz + dcz) * CHUNK_SIZE + lz;
            if ((gwx & 1) != 0 || (gwz & 1) != 0) continue;
            if (it->second.cells[lx][lz]!=0) continue;
            float wx = ((pcx+dcx)*CHUNK_SIZE+lx+0.5f)*CS, wz = ((pcz+dcz)*CHUNK_SIZE+lz+0.5f)*CS;
            bool ex=false; for(auto&l:lights)if(fabsf(l.pos.x-wx)<0.1f&&fabsf(l.pos.z-wz)<0.1f){ex=true;break;}
            if(!ex && lr()%100<50){
                Light l; l.pos=Vec3(wx,WH-0.02f,wz); l.sizeX=l.sizeZ=1.2f; l.intensity=1.0f; l.on=(lr()%100>=20); lights.push_back(l);
            }
        }
    }
    for (int dcx=-VIEW_CHUNKS; dcx<=VIEW_CHUNKS; dcx++) for (int dcz=-VIEW_CHUNKS; dcz<=VIEW_CHUNKS; dcz++) {
        auto it = chunks.find(chunkKey(pcx+dcx, pcz+dcz)); if (it==chunks.end()) continue;
        if (!layoutRulesFor(chunkStyleAt(pcx+dcx, pcz+dcz)).pillars) continue;
        for (int lx=0; lx<CHUNK_SIZE; lx++) for (int lz=0; lz<CHUNK_SIZE; lz++) {
            int gwx = (pcx + dcx) * CHUNK_SIZE + lx;
            int gwz = (pcz + dcz) * CHUNK_SIZE + lz;
            if (!isPillarLatticeCell(gwx, gwz)) continue;
            if (!isOpenHallCell(gwx, gwz)) continue;
            float wx = (gwx+0.5f)*CS;
            float wz = (gwz+0.5f)*CS;
            bool ex=false; for(auto& p:pillars) if(fabsf(p.x-wx)<0.1f&&fabsf(p.z-wz)<0.1f){ex=true;break;}
            if(!ex) pillars.push_back(Vec3(wx,0,wz));
        }
    }
}

#include "world_spawn.h"
