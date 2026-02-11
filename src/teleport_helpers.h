#pragma once

inline void teleportToPlayer(){
    if(multiState!=MULTI_IN_GAME)return;
    static int lastTeleportTarget = -1;
    int chosen = -1;
    for(int step=1; step<=MAX_PLAYERS; step++){
        int idx = (lastTeleportTarget + step) % MAX_PLAYERS;
        if(idx == netMgr.myId) continue;
        if(!netMgr.players[idx].active || !netMgr.players[idx].hasValidPos) continue;
        chosen = idx;
        break;
    }
    if(chosen < 0) return;
    lastTeleportTarget = chosen;
    Vec3 tp = netMgr.players[chosen].pos;
    cam.pos=Vec3(tp.x+1.0f,PH,tp.z);
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    buildGeom();
}

inline void teleportToExit(){
    if(!coop.initialized) return;
    cam.pos=Vec3(coop.doorPos.x,PH,coop.doorPos.z-2.0f);
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    buildGeom();
}
