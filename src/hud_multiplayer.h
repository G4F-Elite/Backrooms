#pragma once

inline void updateMultiplayer(){
    if(multiState!=MULTI_IN_GAME) return;
    
    static float netSendTimer=0;
    static float stateSyncTimer=0;
    netSendTimer+=dTime;
    stateSyncTimer+=dTime;
    if(netSendTimer>=0.05f){
        netMgr.sendPlayerState(cam.pos,cam.yaw,cam.pitch,flashlightOn);
        netSendTimer=0;
    }
    netMgr.update();
    netMgr.sendPing((float)glfwGetTime());
    updatePlayerInterpolation(netMgr.myId, dTime);
    if(!netMgr.isHost && netMgr.clientTimedOut((float)glfwGetTime())){
        captureSessionSnapshot();
        netMgr.shutdown();
        reconnectInProgress = true;
        restoreAfterReconnect = true;
        reconnectAttemptTimer = 0.0f;
        reconnectAttempts = 0;
        multiState = MULTI_CONNECTING;
        gameState = STATE_MULTI_WAIT;
        return;
    }
    
    if(netMgr.roamEventReceived){
        netMgr.roamEventReceived = false;
        applyRoamEvent(netMgr.roamEventType, netMgr.roamEventA, netMgr.roamEventB, netMgr.roamEventDuration);
    }
    
    if(!netMgr.isHost && netMgr.reshuffleReceived){
        netMgr.reshuffleReceived = false;
        long long key = chunkKey(netMgr.reshuffleChunkX, netMgr.reshuffleChunkZ);
        auto it = chunks.find(key);
        if (it == chunks.end()) {
            generateChunk(netMgr.reshuffleChunkX, netMgr.reshuffleChunkZ);
            it = chunks.find(key);
        }
        if (it != chunks.end()) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    int idx = x * CHUNK_SIZE + z;
                    it->second.cells[x][z] = (int)netMgr.reshuffleCells[idx];
                }
            }
        }
        worldSeed = netMgr.reshuffleSeed;
        updateLightsAndPillars(playerChunkX, playerChunkZ);
        buildGeom();
    }
    
    if(!netMgr.isHost){
        entitySpawnTimer = 999;
        clientApplyFeatureState();
        if(netMgr.entitySnapshotReceived){
            netMgr.entitySnapshotReceived = false;
            entityMgr.entities.clear();
            for(int i=0;i<netMgr.entitySnapshotCount;i++){
                if(!netMgr.entitySnapshot[i].active) continue;
                Entity e;
                e.type = (EntityType)netMgr.entitySnapshot[i].type;
                e.pos = netMgr.entitySnapshot[i].pos;
                e.yaw = netMgr.entitySnapshot[i].yaw;
                e.state = (EntityState)netMgr.entitySnapshot[i].state;
                e.active = true;
                if(e.type==ENTITY_STALKER){ e.speed=1.5f; e.detectionRange=20.0f; e.attackRange=1.0f; }
                else if(e.type==ENTITY_CRAWLER){ e.speed=5.0f; e.detectionRange=15.0f; e.attackRange=1.5f; e.pos.y=-0.8f; }
                else if(e.type==ENTITY_SHADOW){ e.speed=0.5f; e.detectionRange=8.0f; e.attackRange=2.0f; }
                entityMgr.entities.push_back(e);
            }
        }
    }else{
        processHostInteractRequests();
        updateCoopObjectiveHost();
        hostUpdateItems();
        updateRoamEventsHost();
        if(stateSyncTimer>=0.12f){
            stateSyncTimer = 0;
            NetEntitySnapshotEntry snap[MAX_SYNC_ENTITIES];
            int c = 0;
            for(auto& e:entityMgr.entities){
                if(c>=MAX_SYNC_ENTITIES) break;
                snap[c].id = c;
                snap[c].type = (int)e.type;
                snap[c].pos = e.pos;
                snap[c].yaw = e.yaw;
                snap[c].state = (int)e.state;
                snap[c].active = e.active;
                c++;
            }
            netMgr.sendEntitySnapshot(snap, c);
            hostSyncFeatureState();
        }
    }
}
