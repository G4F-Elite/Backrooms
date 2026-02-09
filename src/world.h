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

// Count walls around a cell (for dead-end detection)
inline int countWallsAround(const Chunk& c, int x, int z) {
    int walls = 0;
    if (x <= 0 || c.cells[x-1][z] == 1) walls++;
    if (x >= CHUNK_SIZE-1 || c.cells[x+1][z] == 1) walls++;
    if (z <= 0 || c.cells[x][z-1] == 1) walls++;
    if (z >= CHUNK_SIZE-1 || c.cells[x][z+1] == 1) walls++;
    return walls;
}

// Remove dead-ends by opening passages
inline void removeDeadEnds(Chunk& c, std::mt19937& cr) {
    bool changed = true;
    int iterations = 0;
    while (changed && iterations < 7) {
        changed = false;
        iterations++;
        for (int x = 1; x < CHUNK_SIZE - 1; x++) {
            for (int z = 1; z < CHUNK_SIZE - 1; z++) {
                if (c.cells[x][z] != 0) continue;  // Only check open cells
                
                int walls = countWallsAround(c, x, z);
                if (walls >= 3) {  // Dead-end detected (3 walls = only 1 exit)
                    // Higher chance to remove dead-ends for better flow.
                    if (cr() % 100 < 88) {
                        // Find a wall to remove
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

// Add extra connections to reduce isolation
inline void addExtraConnections(Chunk& c, std::mt19937& cr) {
    int connections = 2 + cr() % 3;  // Add 2-4 extra connections
    for (int i = 0; i < connections; i++) {
        int x = 2 + cr() % (CHUNK_SIZE - 4);
        int z = 2 + cr() % (CHUNK_SIZE - 4);
        
        // Find a wall that separates two open areas
        if (c.cells[x][z] == 1) {
            bool hasOpenH = (x > 0 && c.cells[x-1][z] == 0) && (x < CHUNK_SIZE-1 && c.cells[x+1][z] == 0);
            bool hasOpenV = (z > 0 && c.cells[x][z-1] == 0) && (z < CHUNK_SIZE-1 && c.cells[x][z+1] == 0);
            if (hasOpenH || hasOpenV) {
                c.cells[x][z] = 0;  // Create shortcut
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
    // More gates for better connectivity
    for (int i = 0; i < 6; i++) {
        int gate = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        c.cells[laneA][gate] = 0;
        c.cells[laneB][gate] = 0;
    }
    // Fewer blocking boxes
    for (int i = 0; i < 3; i++) {
        int bx = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        int bz = 2 + (int)(cr() % (CHUNK_SIZE - 4));
        fillRect(c, bx, bz, 2, 2);
    }
}

inline void applyChunkArchitecturePattern(Chunk& c, std::mt19937& cr) {
    int p = (int)(cr() % 100);
    if (p < 18) applyAtriumPattern(c, cr);
    else if (p < 34) applyOfficePattern(c, cr);
    else if (p < 49) applyServicePattern(c, cr);
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

inline void generateChunk(int cx, int cz) {
    long long key = chunkKey(cx, cz);
    if (chunks.find(key) != chunks.end()) return;
    Chunk c; c.cx = cx; c.cz = cz; c.gen = true;
    // Use world seed + position for randomness
    unsigned int seed = worldSeed ^ (unsigned)(cx * 73856093) ^ (unsigned)(cz * 19349663);
    std::mt19937 cr(seed);
    for (int x = 0; x < CHUNK_SIZE; x++) for (int z = 0; z < CHUNK_SIZE; z++) c.cells[x][z] = 1;
    
    // Generate base maze with DFS
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
    
    // Apply architecture patterns
    applyChunkArchitecturePattern(c, cr);
    
    // Add random rooms (3-4) to avoid too many giant open zones.
    for (int i = 0; i < 3 + cr() % 2; i++) { 
        int rx = 1+cr()%(CHUNK_SIZE-4), rz = 1+cr()%(CHUNK_SIZE-4);
        int rw = 2+cr()%3, rh = 2+cr()%3;  // Slightly larger rooms
        for (int x = rx; x < rx+rw && x < CHUNK_SIZE-1; x++) 
            for (int z = rz; z < rz+rh && z < CHUNK_SIZE-1; z++) 
                c.cells[x][z] = 0; 
    }
    
    // Controlled wall removal to keep layout readable and less over-open.
    for (int x = 1; x < CHUNK_SIZE-1; x++) 
        for (int z = 1; z < CHUNK_SIZE-1; z++) 
            if (c.cells[x][z] == 1 && cr()%100 < 14) 
                c.cells[x][z] = 0;
    
    // Remove dead-ends
    removeDeadEnds(c, cr);
    
    // Add extra connections for better flow
    addExtraConnections(c, cr);
    
    // Chunk border passages (more passages)
    for (int i = 1; i < CHUNK_SIZE-1; i += 2) { 
        c.cells[0][i] = 0; 
        c.cells[CHUNK_SIZE-1][i] = 0; 
        c.cells[i][0] = 0; 
        c.cells[i][CHUNK_SIZE-1] = 0; 
    }
    
    // Ensure minimum open space
    if (countOpenCells(c) < 50) carveRect(c, 2, 2, CHUNK_SIZE - 4, CHUNK_SIZE - 4);
    
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

// Check if cell is a corridor (walls on opposite sides - would block passage if pillar placed)
inline bool isCorridor(const Chunk& chunk, int lx, int lz) {
    bool wallLeft = (lx > 0) ? chunk.cells[lx-1][lz] == 1 : true;
    bool wallRight = (lx < CHUNK_SIZE-1) ? chunk.cells[lx+1][lz] == 1 : true;
    bool wallUp = (lz > 0) ? chunk.cells[lx][lz-1] == 1 : true;
    bool wallDown = (lz < CHUNK_SIZE-1) ? chunk.cells[lx][lz+1] == 1 : true;
    
    // Horizontal corridor (walls above and below)
    if (wallUp && wallDown) return true;
    // Vertical corridor (walls left and right)
    if (wallLeft && wallRight) return true;
    
    return false;
}

// Check if placing pillar would block a narrow passage
inline bool wouldBlockPassage(const Chunk& chunk, int lx, int lz) {
    // Don't place in corridors
    if (isCorridor(chunk, lx, lz)) return true;
    
    // Count open neighbors - need at least 3 open sides for pillar placement
    int openSides = 0;
    if (lx > 0 && chunk.cells[lx-1][lz] == 0) openSides++;
    if (lx < CHUNK_SIZE-1 && chunk.cells[lx+1][lz] == 0) openSides++;
    if (lz > 0 && chunk.cells[lx][lz-1] == 0) openSides++;
    if (lz < CHUNK_SIZE-1 && chunk.cells[lx][lz+1] == 0) openSides++;
    
    // Only place pillar if at least 3 sides are open (corner placement OK)
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
        for (int lx=1; lx<CHUNK_SIZE-1; lx+=2) for (int lz=1; lz<CHUNK_SIZE-1; lz+=2) {
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
            
            // NEW: Skip if this would block a passage
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

inline Vec3 findSpawnPos(Vec3 pPos, float minD) {
    const float spawnRadius = 0.42f;
    for(int a=0;a<80;a++){
        int dx=(rng()%34)-17, dz=(rng()%34)-17;
        int wx=(int)floorf(pPos.x/CS)+dx, wz=(int)floorf(pPos.z/CS)+dz;
        if(getCellWorld(wx,wz)!=0) continue;
        Vec3 p((wx+0.5f)*CS,0,(wz+0.5f)*CS);
        float ddx = p.x-pPos.x, ddz = p.z-pPos.z;
        if(sqrtf(ddx*ddx+ddz*ddz)<minD) continue;
        if(collideWorld(p.x,p.z,spawnRadius)) continue;
        return p;
    }
    // Fallback: search in expanding rings
    for(int r=5; r<20; r++){
        for(int a=0;a<16;a++){
            float ang = (float)a * 0.3926991f;
            float fx = pPos.x + sinf(ang) * (float)r * CS * 0.5f;
            float fz = pPos.z + cosf(ang) * (float)r * CS * 0.5f;
            int wx=(int)floorf(fx/CS), wz=(int)floorf(fz/CS);
            if(getCellWorld(wx,wz)!=0) continue;
            Vec3 p((wx+0.5f)*CS,0,(wz+0.5f)*CS);
            if(collideWorld(p.x,p.z,spawnRadius)) continue;
            return p;
        }
    }
    return Vec3(pPos.x+20,0,pPos.z+20);
}
