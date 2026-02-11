#pragma once

inline Vec3 findSafeSpawn() {
    generateChunk(0, 0);
    updateLightsAndPillars(0, 0);
    for (auto& l : lights) {
        if (!l.on) continue;
        int wx = (int)floorf(l.pos.x / CS), wz = (int)floorf(l.pos.z / CS);
        if (getCellWorld(wx, wz) == 0) {
            float sx = (wx + 0.5f) * CS, sz = (wz + 0.5f) * CS;
            if (!collideWorld(sx, sz, 0.4f)) return Vec3(sx, 0, sz);
        }
    }
    for (int r = 1; r < 10; r++) {
        for (int dx = -r; dx <= r; dx++) for (int dz = -r; dz <= r; dz++) {
            int wx = 8 + dx, wz = 8 + dz;
            if (getCellWorld(wx, wz) == 0) {
                float sx = (wx + 0.5f) * CS, sz = (wz + 0.5f) * CS;
                if (!collideWorld(sx, sz, 0.4f)) return Vec3(sx, 0, sz);
            }
        }
    }
    return Vec3(8 * CS, 0, 8 * CS);
}

inline Vec3 findSpawnPos(Vec3 pPos, float minD) {
    const float spawnRadius = 0.42f;
    float minD2 = minD * minD;
    for(int a=0;a<80;a++){
        int dx=(rng()%34)-17, dz=(rng()%34)-17;
        int wx=(int)floorf(pPos.x/CS)+dx, wz=(int)floorf(pPos.z/CS)+dz;
        if(getCellWorld(wx,wz)!=0) continue;
        Vec3 p((wx+0.5f)*CS,0,(wz+0.5f)*CS);
        float ddx = p.x-pPos.x, ddz = p.z-pPos.z;
        if(ddx*ddx+ddz*ddz<minD2) continue;
        if(collideWorld(p.x,p.z,spawnRadius)) continue;
        return p;
    }
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
