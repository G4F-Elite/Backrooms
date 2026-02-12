#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_map>
#include "math.h"
#include "progression.h"
#include "elevation.h"

extern const float CS, WH;
extern std::mt19937 rng;
extern unsigned int worldSeed;

const int CHUNK_SIZE = 16;
const int VIEW_CHUNKS = 2;

struct Chunk { int cx, cz, cells[CHUNK_SIZE][CHUNK_SIZE]; int8_t elev[CHUNK_SIZE][CHUNK_SIZE]; bool gen; };
struct Light { Vec3 pos; float sizeX, sizeZ, intensity; bool on; };

extern std::unordered_map<long long, Chunk> chunks;
extern std::vector<Light> lights;
extern std::vector<Vec3> pillars;
extern int playerChunkX, playerChunkZ;

inline long long chunkKey(int cx, int cz) { return ((long long)cx << 32) | (cz & 0xFFFFFFFF); }

inline void applyElevation(Chunk& c, std::mt19937& cr);

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

inline void removeDeadEnds(Chunk& c, std::mt19937& cr) {
    bool changed = true;
    int iterations = 0;
    while (changed && iterations < 7) {
        changed = false;
        iterations++;
        for (int x = 1; x < CHUNK_SIZE - 1; x++) {
            for (int z = 1; z < CHUNK_SIZE - 1; z++) {
                if (c.cells[x][z] != 0) continue;
                
                int walls = countWallsAround(c, x, z);
                if (walls >= 3) {
                    if (cr() % 100 < 88) {
                        std::vector<std::pair<int,int>> wallDirs;
                        if (x > 1 && c.cells[x-1][z] == 1) wallDirs.push_back({x-1, z});
                        if (x < CHUNK_SIZE-2 && c.cells[x+1][z] == 1) wallDirs.push_back({x+1, z});
                        if (z > 1 && c.cells[x][z-1] == 1) wallDirs.push_back({x, z-1});
                        if (z < CHUNK_SIZE-2 && c.cells[x][z+1] == 1) wallDirs.push_back({x, z+1});
                        
                        if (!wallDirs.empty()) {
                            auto& w = wallDirs[cr() % wallDirs.size()];
                            c.cells[w.first][w.second] = 0;
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

inline void addExtraConnections(Chunk& c, std::mt19937& cr) {
    int connections = 2 + cr() % 3;
    for (int i = 0; i < connections; i++) {
        int x = 2 + cr() % (CHUNK_SIZE - 4);
        int z = 2 + cr() % (CHUNK_SIZE - 4);
        
        if (c.cells[x][z] == 1) {
            bool hasOpenH = (x > 0 && c.cells[x-1][z] == 0) && (x < CHUNK_SIZE-1 && c.cells[x+1][z] == 0);
            bool hasOpenV = (z > 0 && c.cells[x][z-1] == 0) && (z < CHUNK_SIZE-1 && c.cells[x][z+1] == 0);
            if (hasOpenH || hasOpenV) {
                c.cells[x][z] = 0;
            }
        }
    }
}

inline void applyAtriumPattern(Chunk& c, std::mt19937& cr) {
    int x0 = 2 + (int)(cr() % 2);
    int z0 = 2 + (int)(cr() % 2);
    int w = CHUNK_SIZE - 4 - (int)(cr() % 2);
    int h = CHUNK_SIZE - 4 - (int)(cr() % 2);
    carveRect(c, x0, z0, w, h);

    int cx = CHUNK_SIZE / 2;
    int cz = CHUNK_SIZE / 2;
    fillRect(c, cx - 1, cz - 1, 3, 3);
    c.cells[cx][cz] = 0;
    c.cells[cx - 2][cz] = 0;
    c.cells[cx + 2][cz] = 0;
    c.cells[cx][cz - 2] = 0;
    c.cells[cx][cz + 2] = 0;
}

inline void applyOfficePattern(Chunk& c, std::mt19937& cr) {
    carveRect(c, 1, 1, CHUNK_SIZE - 2, CHUNK_SIZE - 2);

    int step = 3 + (int)(cr() % 2);
    for (int x = 3; x < CHUNK_SIZE - 3; x += step) {
        for (int z = 2; z < CHUNK_SIZE - 2; z++) {
            c.cells[x][z] = 1;
        }
        // Add more doors per wall section
        int numDoors = 2 + cr() % 2;
        for (int d = 0; d < numDoors; d++) {
            int doorZ = 2 + (int)(cr() % (CHUNK_SIZE - 4));
            c.cells[x][doorZ] = 0;
            if (doorZ + 1 < CHUNK_SIZE - 1) c.cells[x][doorZ + 1] = 0;
        }
    }
}

inline void applyServicePattern(Chunk& c, std::mt19937& cr) {
    carveRect(c, 1, 1, CHUNK_SIZE - 2, CHUNK_SIZE - 2);
    int laneA = 3 + (int)(cr() % 3);
    int laneB = CHUNK_SIZE - 4 - (int)(cr() % 3);
    for (int z = 2; z < CHUNK_SIZE - 2; z++) {
        c.cells[laneA][z] = 1;
        c.cells[laneB][z] = 1;
    }
    for (int i = 0; i < 6; i++) {
        int gate = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        c.cells[laneA][gate] = 0;
        c.cells[laneB][gate] = 0;
    }
    for (int i = 0; i < 3; i++) {
        int bx = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        int bz = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        fillRect(c, bx, bz, 2, 2);
    }
}

inline void applyRampRoutePattern(Chunk& c, std::mt19937& cr);
inline void applyParkingPattern(Chunk& c, std::mt19937& cr);

inline void applyChunkArchitecturePattern(Chunk& c, std::mt19937& cr) {
    int p = (int)(cr() % 100);
    if (isLevelZero(gCurrentLevel)) {
        if (p < 20) applyAtriumPattern(c, cr);
        else if (p < 40) applyOfficePattern(c, cr);
        else if (p < 58) applyServicePattern(c, cr);
        else if (p < 78) applyRampRoutePattern(c, cr);
        else applyAtriumPattern(c, cr);
        return;
    }
    if (p < 56) applyParkingPattern(c, cr);
    else if (p < 78) applyServicePattern(c, cr);
    else if (p < 93) applyOfficePattern(c, cr);
    else applyParkingPattern(c, cr);
}

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

inline int8_t getElevWorld(int wx, int wz) {
    int cx = wx >= 0 ? wx / CHUNK_SIZE : (wx - CHUNK_SIZE + 1) / CHUNK_SIZE;
    int cz = wz >= 0 ? wz / CHUNK_SIZE : (wz - CHUNK_SIZE + 1) / CHUNK_SIZE;
    int lx = wx - cx * CHUNK_SIZE, lz = wz - cz * CHUNK_SIZE;
    auto it = chunks.find(chunkKey(cx, cz));
    return (it == chunks.end()) ? 0 : it->second.elev[lx][lz];
}

inline float getGroundY(float x, float z) {
    int wx = (int)floorf(x / CS), wz = (int)floorf(z / CS);
    int8_t e = getElevWorld(wx, wz);
    if (e == 0) return 0.0f;
    if (e == 1) return FLOOR2_Y;
    float fracX = (x - wx * CS) / CS, fracZ = (z - wz * CS) / CS;
    if (fracX < 0.0f) fracX = 0.0f;
    if (fracX > 1.0f) fracX = 1.0f;
    if (fracZ < 0.0f) fracZ = 0.0f;
    if (fracZ > 1.0f) fracZ = 1.0f;
    return getRampY(e, fracX, fracZ);
}

inline void generateChunk(int cx, int cz) {
    long long key = chunkKey(cx, cz);
    if (chunks.find(key) != chunks.end()) return;
    Chunk c; c.cx = cx; c.cz = cz; c.gen = true;
    unsigned int seed = worldSeed ^ (unsigned)(cx * 73856093) ^ (unsigned)(cz * 19349663);
    std::mt19937 cr(seed);
    for (int x = 0; x < CHUNK_SIZE; x++) for (int z = 0; z < CHUNK_SIZE; z++) { c.cells[x][z] = 1; c.elev[x][z] = 0; }
    
    std::vector<std::pair<int,int>> stk;
    int sx = 1 + cr() % (CHUNK_SIZE-2), sz = 1 + cr() % (CHUNK_SIZE-2);
    c.cells[sx][sz] = 0; stk.push_back({sx, sz});
    int dx[] = {0,0,2,-2}, dz[] = {2,-2,0,0};
    while (!stk.empty()) {
        int x = stk.back().first, z = stk.back().second;
        std::vector<int> dirs;
        for (int d = 0; d < 4; d++) { 
            int nx = x+dx[d], nz = z+dz[d];
            if (nx > 0 && nx < CHUNK_SIZE-1 && nz > 0 && nz < CHUNK_SIZE-1 && c.cells[nx][nz] == 1) 
                dirs.push_back(d); 
        }
        if (dirs.empty()) stk.pop_back();
        else { 
            int d = dirs[cr() % dirs.size()]; 
            c.cells[x+dx[d]/2][z+dz[d]/2] = 0; 
            c.cells[x+dx[d]][z+dz[d]] = 0; 
            stk.push_back({x+dx[d], z+dz[d]}); 
        }
    }
    
    applyChunkArchitecturePattern(c, cr);
    
    for (int i = 0; i < 3 + cr() % 2; i++) { 
        int rx = 1+cr()%(CHUNK_SIZE-4), rz = 1+cr()%(CHUNK_SIZE-4);
        int rw = 2+cr()%3, rh = 2+cr()%3;
        for (int x = rx; x < rx+rw && x < CHUNK_SIZE-1; x++) 
            for (int z = rz; z < rz+rh && z < CHUNK_SIZE-1; z++) 
                c.cells[x][z] = 0; 
    }
    
    for (int x = 1; x < CHUNK_SIZE-1; x++) 
        for (int z = 1; z < CHUNK_SIZE-1; z++) 
            if (c.cells[x][z] == 1 && cr()%100 < 14) 
                c.cells[x][z] = 0;
    
    removeDeadEnds(c, cr);
    
    addExtraConnections(c, cr);
    
    for (int i = 1; i < CHUNK_SIZE-1; i += 2) { 
        c.cells[0][i] = 0; 
        c.cells[CHUNK_SIZE-1][i] = 0; 
        c.cells[i][0] = 0; 
        c.cells[i][CHUNK_SIZE-1] = 0; 
    }
    
    if (countOpenCells(c) < 50) carveRect(c, 2, 2, CHUNK_SIZE - 4, CHUNK_SIZE - 4);

    if (isLevelZero(gCurrentLevel)) applyElevation(c, cr);

    chunks[key] = c;
}

inline void updateVisibleChunks(float px, float pz) {
    int pcx = (int)floorf(px / (CS * CHUNK_SIZE)), pcz = (int)floorf(pz / (CS * CHUNK_SIZE));
    for (int dx = -VIEW_CHUNKS; dx <= VIEW_CHUNKS; dx++) for (int dz = -VIEW_CHUNKS; dz <= VIEW_CHUNKS; dz++) generateChunk(pcx+dx, pcz+dz);
    std::vector<long long> rm; for (auto& p : chunks) { int d = abs(p.second.cx-pcx) > abs(p.second.cz-pcz) ? abs(p.second.cx-pcx) : abs(p.second.cz-pcz); if (d > VIEW_CHUNKS+1) rm.push_back(p.first); }
    for (auto k : rm) chunks.erase(k);
    playerChunkX = pcx; playerChunkZ = pcz;
}

#include "world_patterns.h"

inline bool circleOverlapsAabb2D(float x, float z, float r, float minX, float maxX, float minZ, float maxZ) {
    float clx = x < minX ? minX : (x > maxX ? maxX : x);
    float clz = z < minZ ? minZ : (z > maxZ ? maxZ : z);
    float dx = x - clx;
    float dz = z - clz;
    return dx * dx + dz * dz < r * r;
}

#include "world_collision.h"

inline bool collideWorld(float x, float z, float PR) {
    int wx = (int)floorf(x/CS), wz = (int)floorf(z/CS);
    float PR2 = PR * PR;
    int8_t myElev = getElevWorld(wx, wz);
    float myH = getGroundY(x, z);
    for (int ddx = -1; ddx <= 1; ddx++) for (int ddz = -1; ddz <= 1; ddz++) {
        int chkx = wx+ddx, chkz = wz+ddz;
        int cellVal = getCellWorld(chkx, chkz);
        bool blocked = (cellVal == 1);
        // Block elevation transitions (ground↔elevated) unless ramp
        if (!blocked && cellVal == 0 && (ddx != 0 || ddz != 0)) {
            int8_t nElev = getElevWorld(chkx, chkz);
            if (nElev != myElev && !isRamp(nElev) && !isRamp(myElev)) {
                float nH = getFloorYFromElev(nElev);
                if (fabsf(myH - nH) > 0.5f) blocked = true;
            }
        }
        if (blocked) {
            float wx0 = chkx*CS, wx1 = (chkx+1)*CS, wz0 = chkz*CS, wz1 = (chkz+1)*CS;
            float clx = x<wx0?wx0:(x>wx1?wx1:x), clz = z<wz0?wz0:(z>wz1?wz1:z);
            float dx = x-clx, dz = z-clz;
            if (dx*dx+dz*dz < PR2) return true;
        }
    }
    for (int ddx = -1; ddx <= 1; ddx++) for (int ddz = -1; ddz <= 1; ddz++) {
        int chkx = wx + ddx, chkz = wz + ddz;
        if (collideDoorFrameAtCell(x, z, PR, chkx, chkz)) return true;
    }
    for (const auto& p : pillars) if (fabsf(x-p.x) < 0.5f+PR && fabsf(z-p.z) < 0.5f+PR) return true;
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
        int ov=getCellWorld(rx,rz), nv=(ov==0)?1:0; setCellWorld(rx,rz,nv);
        if(countReachLocal(px,pz,12)<minR){ setCellWorld(rx,rz,ov); continue; } changed++; }
    if(changed==0) return false;
    lights.erase(std::remove_if(lights.begin(),lights.end(),[](Light&l){return getCellWorld((int)floorf(l.pos.x/CS),(int)floorf(l.pos.z/CS))==1;}),lights.end());
    pillars.erase(std::remove_if(pillars.begin(),pillars.end(),[](Vec3&p){return getCellWorld((int)floorf(p.x/CS),(int)floorf(p.z/CS))==1;}),pillars.end());
    return true;
}

inline bool isCorridor(const Chunk& chunk, int lx, int lz) {
    bool wallLeft = (lx > 0) ? chunk.cells[lx-1][lz] == 1 : true;
    bool wallRight = (lx < CHUNK_SIZE-1) ? chunk.cells[lx+1][lz] == 1 : true;
    bool wallUp = (lz > 0) ? chunk.cells[lx][lz-1] == 1 : true;
    bool wallDown = (lz < CHUNK_SIZE-1) ? chunk.cells[lx][lz+1] == 1 : true;
    
    if (wallUp && wallDown) return true;
    if (wallLeft && wallRight) return true;
    
    return false;
}

inline bool wouldBlockPassage(const Chunk& chunk, int lx, int lz) {
    if (isCorridor(chunk, lx, lz)) return true;

    int openSides = 0;
    if (lx > 0 && chunk.cells[lx-1][lz] == 0) openSides++;
    if (lx < CHUNK_SIZE-1 && chunk.cells[lx+1][lz] == 0) openSides++;
    if (lz > 0 && chunk.cells[lx][lz-1] == 0) openSides++;
    if (lz < CHUNK_SIZE-1 && chunk.cells[lx][lz+1] == 0) openSides++;
    
    if (openSides < 3) return true;
    
    return false;
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
        for (int lx=1; lx<CHUNK_SIZE-1; lx++) for (int lz=1; lz<CHUNK_SIZE-1; lz++) {
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
        unsigned int seed = worldSeed ^ (unsigned)((pcx+dcx)*9871+(pcz+dcz)*4231);
        std::mt19937 pr(seed);
        for (int lx=2; lx<CHUNK_SIZE-2; lx++) for (int lz=2; lz<CHUNK_SIZE-2; lz++) {
            if (it->second.cells[lx][lz]!=0) continue;
            
            if (wouldBlockPassage(it->second, lx, lz)) continue;
            
            int wallAdj = 0;
            if (it->second.cells[lx-1][lz]==1) wallAdj++;
            if (it->second.cells[lx+1][lz]==1) wallAdj++;
            if (it->second.cells[lx][lz-1]==1) wallAdj++;
            if (it->second.cells[lx][lz+1]==1) wallAdj++;
            if (wallAdj < 2) continue;
            if (pr()%100 >= 6) continue;
            float wx = ((pcx+dcx)*CHUNK_SIZE+lx+0.5f)*CS;
            float wz = ((pcz+dcz)*CHUNK_SIZE+lz+0.5f)*CS;
            bool ex=false; for(auto& p:pillars) if(fabsf(p.x-wx)<0.1f&&fabsf(p.z-wz)<0.1f){ex=true;break;}
            if(!ex) pillars.push_back(Vec3(wx,0,wz));
        }
    }
}

#include "elevation_gen.h"
#include "world_spawn.h"
