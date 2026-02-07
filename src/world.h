#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_map>
#include "math.h"

extern const float CS, WH;
extern std::mt19937 rng;
extern unsigned int worldSeed;  // Seed based on time

const int CHUNK_SIZE = 16;
const int VIEW_CHUNKS = 2;

struct Chunk { int cx, cz, cells[CHUNK_SIZE][CHUNK_SIZE]; bool gen; };
struct Light { Vec3 pos; float sizeX, sizeZ, intensity; bool on; };

extern std::unordered_map<long long, Chunk> chunks;
extern std::vector<Light> lights;
extern std::vector<Vec3> pillars;
extern int playerChunkX, playerChunkZ;

inline long long chunkKey(int cx, int cz) { return ((long long)cx << 32) | (cz & 0xFFFFFFFF); }

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
    Chunk c; c.cx = cx; c.cz = cz; c.gen = true;
    // Use world seed + position for randomness
    unsigned int seed = worldSeed ^ (unsigned)(cx * 73856093) ^ (unsigned)(cz * 19349663);
    std::mt19937 cr(seed);
    for (int x = 0; x < CHUNK_SIZE; x++) for (int z = 0; z < CHUNK_SIZE; z++) c.cells[x][z] = 1;
    std::vector<std::pair<int,int>> stk;
    int sx = 1 + cr() % (CHUNK_SIZE-2), sz = 1 + cr() % (CHUNK_SIZE-2);
    c.cells[sx][sz] = 0; stk.push_back({sx, sz});
    int dx[] = {0,0,2,-2}, dz[] = {2,-2,0,0};
    while (!stk.empty()) {
        int x = stk.back().first, z = stk.back().second;
        std::vector<int> dirs;
        for (int d = 0; d < 4; d++) { int nx = x+dx[d], nz = z+dz[d];
            if (nx > 0 && nx < CHUNK_SIZE-1 && nz > 0 && nz < CHUNK_SIZE-1 && c.cells[nx][nz] == 1) dirs.push_back(d); }
        if (dirs.empty()) stk.pop_back();
        else { int d = dirs[cr() % dirs.size()]; c.cells[x+dx[d]/2][z+dz[d]/2] = 0; c.cells[x+dx[d]][z+dz[d]] = 0; stk.push_back({x+dx[d], z+dz[d]}); }
    }
    for (int i = 0; i < 3; i++) { int rx = 1+cr()%(CHUNK_SIZE-4), rz = 1+cr()%(CHUNK_SIZE-4), rw = 2+cr()%2, rh = 2+cr()%2;
        for (int x = rx; x < rx+rw && x < CHUNK_SIZE-1; x++) for (int z = rz; z < rz+rh && z < CHUNK_SIZE-1; z++) c.cells[x][z] = 0; }
    for (int x = 1; x < CHUNK_SIZE-1; x++) for (int z = 1; z < CHUNK_SIZE-1; z++) if (c.cells[x][z] == 1 && cr()%100 < 15) c.cells[x][z] = 0;
    for (int i = 2; i < CHUNK_SIZE-2; i += 3) { c.cells[0][i] = 0; c.cells[CHUNK_SIZE-1][i] = 0; c.cells[i][0] = 0; c.cells[i][CHUNK_SIZE-1] = 0; }
    chunks[key] = c;
}

inline void updateVisibleChunks(float px, float pz) {
    int pcx = (int)floorf(px / (CS * CHUNK_SIZE)), pcz = (int)floorf(pz / (CS * CHUNK_SIZE));
    for (int dx = -VIEW_CHUNKS; dx <= VIEW_CHUNKS; dx++) for (int dz = -VIEW_CHUNKS; dz <= VIEW_CHUNKS; dz++) generateChunk(pcx+dx, pcz+dz);
    std::vector<long long> rm; for (auto& p : chunks) { int d = abs(p.second.cx-pcx) > abs(p.second.cz-pcz) ? abs(p.second.cx-pcx) : abs(p.second.cz-pcz); if (d > VIEW_CHUNKS+1) rm.push_back(p.first); }
    for (auto k : rm) chunks.erase(k);
    playerChunkX = pcx; playerChunkZ = pcz;
}

inline bool collideWorld(float x, float z, float PR) {
    int wx = (int)floorf(x/CS), wz = (int)floorf(z/CS);
    for (int ddx = -1; ddx <= 1; ddx++) for (int ddz = -1; ddz <= 1; ddz++) {
        int chkx = wx+ddx, chkz = wz+ddz;
        if (getCellWorld(chkx, chkz) == 1) { float wx0 = chkx*CS, wx1 = (chkx+1)*CS, wz0 = chkz*CS, wz1 = (chkz+1)*CS;
            float clx = x<wx0?wx0:(x>wx1?wx1:x), clz = z<wz0?wz0:(z>wz1?wz1:z); if (sqrtf((x-clx)*(x-clx)+(z-clz)*(z-clz)) < PR) return true; }}
    for (auto& p : pillars) if (fabsf(x-p.x) < 0.5f+PR && fabsf(z-p.z) < 0.5f+PR) return true;
    return false;
}

inline int countReachLocal(int sx, int sz, int range) {
    std::vector<std::pair<int,int>> vis; vis.reserve(range*range);
    std::vector<std::pair<int,int>> q; q.push_back({sx, sz}); int cnt = 0;
    while (!q.empty() && cnt < range*range) { auto c = q.back(); q.pop_back();
        bool f = false; for (auto& v : vis) if (v.first==c.first && v.second==c.second) { f=true; break; } if (f) continue;
        vis.push_back(c); if (getCellWorld(c.first, c.second)==1) continue; cnt++;
        int dx[]={1,-1,0,0}, dz[]={0,0,1,-1}; for (int d=0; d<4; d++) { int nx=c.first+dx[d], nz=c.second+dz[d]; if (abs(nx-sx)<range && abs(nz-sz)<range) q.push_back({nx, nz}); }}
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

inline void updateLightsAndPillars(int pcx, int pcz) {
    float cx = (pcx+0.5f)*CHUNK_SIZE*CS, cz = (pcz+0.5f)*CHUNK_SIZE*CS, md = (VIEW_CHUNKS+1)*CHUNK_SIZE*CS;
    lights.erase(std::remove_if(lights.begin(),lights.end(),[&](Light&l){ return fabsf(l.pos.x-cx)>md||fabsf(l.pos.z-cz)>md; }),lights.end());
    pillars.erase(std::remove_if(pillars.begin(),pillars.end(),[&](Vec3&p){ return fabsf(p.x-cx)>md||fabsf(p.z-cz)>md; }),pillars.end());
    for (int dcx=-VIEW_CHUNKS; dcx<=VIEW_CHUNKS; dcx++) for (int dcz=-VIEW_CHUNKS; dcz<=VIEW_CHUNKS; dcz++) {
        auto it = chunks.find(chunkKey(pcx+dcx, pcz+dcz)); if (it==chunks.end()) continue;
        unsigned int seed = worldSeed ^ (unsigned)((pcx+dcx)*12345+(pcz+dcz)*67890);
        std::mt19937 lr(seed);
        for (int lx=1; lx<CHUNK_SIZE-1; lx+=2) for (int lz=1; lz<CHUNK_SIZE-1; lz+=2) { if (it->second.cells[lx][lz]!=0) continue;
            float wx = ((pcx+dcx)*CHUNK_SIZE+lx+0.5f)*CS, wz = ((pcz+dcz)*CHUNK_SIZE+lz+0.5f)*CS;
            bool ex=false; for(auto&l:lights)if(fabsf(l.pos.x-wx)<0.1f&&fabsf(l.pos.z-wz)<0.1f){ex=true;break;}
            if(!ex && lr()%100<50){ Light l; l.pos=Vec3(wx,WH-0.02f,wz); l.sizeX=l.sizeZ=1.2f; l.intensity=1.0f; l.on=(lr()%100>=20); lights.push_back(l);}}}}

// Find safe spawn position near a light
inline Vec3 findSafeSpawn() {
    // First ensure spawn chunk is generated
    generateChunk(0, 0);
    updateLightsAndPillars(0, 0);
    
    // Try to spawn near a light
    for (auto& l : lights) {
        if (!l.on) continue;
        int wx = (int)floorf(l.pos.x / CS), wz = (int)floorf(l.pos.z / CS);
        // Check cell and neighbors
        if (getCellWorld(wx, wz) == 0) {
            float sx = (wx + 0.5f) * CS, sz = (wz + 0.5f) * CS;
            if (!collideWorld(sx, sz, 0.4f)) return Vec3(sx, 0, sz);
        }
    }
    
    // Fallback: find any open cell
    for (int r = 1; r < 10; r++) {
        for (int dx = -r; dx <= r; dx++) for (int dz = -r; dz <= r; dz++) {
            int wx = 8 + dx, wz = 8 + dz;
            if (getCellWorld(wx, wz) == 0) {
                float sx = (wx + 0.5f) * CS, sz = (wz + 0.5f) * CS;
                if (!collideWorld(sx, sz, 0.4f)) return Vec3(sx, 0, sz);
            }
        }
    }
    return Vec3(8 * CS, 0, 8 * CS);  // Absolute fallback
}

inline Vec3 findSpawnPos(Vec3 pPos, float minD) { for(int a=0;a<50;a++){ int dx=(rng()%30)-15, dz=(rng()%30)-15;
    int wx=(int)floorf(pPos.x/CS)+dx, wz=(int)floorf(pPos.z/CS)+dz; if(getCellWorld(wx,wz)==0){ Vec3 p((wx+0.5f)*CS,0,(wz+0.5f)*CS);
    if(sqrtf((p.x-pPos.x)*(p.x-pPos.x)+(p.z-pPos.z)*(p.z-pPos.z))>=minD) return p;}} return Vec3(pPos.x+20,0,pPos.z+20);}