#pragma once
#include "session.h"

inline void carveCellSafe(int wx, int wz){
    setCellWorld(wx, wz, 0);
}

inline void carveLineAxisAligned(int x0, int z0, int x1, int z1){
    int x = x0, z = z0;
    while(x != x1){ carveCellSafe(x,z); x += (x1 > x) ? 1 : -1; }
    while(z != z1){ carveCellSafe(x,z); z += (z1 > z) ? 1 : -1; }
    carveCellSafe(x,z);
}

inline void initTrapCorridor(const Vec3& spawnPos){
    trapCorridor = {};
    if(multiState==MULTI_IN_GAME) return;
    int sx = (int)floorf(spawnPos.x / CS);
    int sz = (int)floorf(spawnPos.z / CS);
    int dir = (rng()%2)==0 ? 1 : -1;
    trapCorridor.startX = sx + 4;
    trapCorridor.startZ = sz + dir * (3 + (rng()%3));
    trapCorridor.length = 8 + (rng()%3);
    trapCorridor.gateX = trapCorridor.startX - 1;
    trapCorridor.gateZ = trapCorridor.startZ;

    carveLineAxisAligned(sx, sz, trapCorridor.gateX, trapCorridor.gateZ);
    for(int i=0;i<=trapCorridor.length;i++){
        int x = trapCorridor.startX + i;
        int z = trapCorridor.startZ;
        carveCellSafe(x,z);
        setCellWorld(x, z - 1, 1);
        setCellWorld(x, z + 1, 1);
    }
    setCellWorld(trapCorridor.startX + trapCorridor.length + 1, trapCorridor.startZ, 1);
    setCellWorld(trapCorridor.gateX, trapCorridor.gateZ, 0);

    trapCorridor.active = true;
    trapCorridor.triggered = false;
    trapCorridor.locked = false;
    trapCorridor.resolved = false;
    trapCorridor.lockTimer = 0.0f;
    trapCorridor.stareProgress = 0.0f;
    trapCorridor.anomalyPos = Vec3((trapCorridor.startX + trapCorridor.length + 0.5f) * CS, 1.35f, (trapCorridor.startZ + 0.5f) * CS);
}

inline void triggerTrapCorridor(){
    if(!trapCorridor.active || trapCorridor.triggered) return;
    trapCorridor.triggered = true;
    trapCorridor.locked = true;
    trapCorridor.lockTimer = 18.0f;
    trapCorridor.stareProgress = 0.0f;
    setCellWorld(trapCorridor.gateX, trapCorridor.gateZ, 1);
    buildGeom();
    setTrapStatus("PASSAGE SEALED. DARKNESS LISTENS.");
}

inline void unlockTrapCorridor(){
    if(!trapCorridor.locked) return;
    trapCorridor.locked = false;
    trapCorridor.resolved = true;
    setCellWorld(trapCorridor.gateX, trapCorridor.gateZ, 0);
    buildGeom();
    setTrapStatus("THE PASSAGE OPENS.");
}

inline void updateTrapCorridor(){
    if(!trapCorridor.active || multiState==MULTI_IN_GAME) return;
    if(trapStatusTimer > 0.0f) trapStatusTimer -= dTime;
    int wx = (int)floorf(cam.pos.x / CS);
    int wz = (int)floorf(cam.pos.z / CS);
    if(!trapCorridor.triggered && isInsideTrapTrigger(wx, wz, trapCorridor.startX, trapCorridor.startZ, trapCorridor.length)){
        triggerTrapCorridor();
    }

    bool lookingAtAnomaly = isLookingAtPoint(cam.pos, cam.yaw, cam.pitch, trapCorridor.anomalyPos, 0.94f, 12.0f);
    anomalyBlur = updateAnomalyBlur(anomalyBlur, dTime, lookingAtAnomaly);

    if(trapCorridor.locked){
        trapCorridor.lockTimer -= dTime;
        bool progressActive = lookingAtAnomaly && !flashlightOn;
        trapCorridor.stareProgress = updateTrapStareProgress(trapCorridor.stareProgress, dTime, progressActive);
        if(progressActive && (rng()%100)<4){
            setTrapStatus("DO NOT BLINK.");
        }
        if(trapCorridor.stareProgress >= 2.6f || trapCorridor.lockTimer <= 0.0f){
            unlockTrapCorridor();
        }
    }
}

inline void spawnEchoSignal(){
    echoSignal.active = true;
    echoSignal.type = chooseEchoTypeFromRoll((int)rng());
    echoSignal.pos = findSpawnPos(cam.pos, 10.0f + (float)(rng()%7));
    echoSignal.ttl = 40.0f;
}

inline void clearEchoSignal(){
    echoSignal.active = false;
    echoSignal.ttl = 0.0f;
}

inline void updateEchoSignal(){
    if(multiState==MULTI_IN_GAME) return;
    if(echoStatusTimer>0.0f) echoStatusTimer -= dTime;
    if(echoSignal.active){
        echoSignal.ttl -= dTime;
        if(echoSignal.ttl <= 0.0f){
            clearEchoSignal();
            setEchoStatus("ECHO SIGNAL LOST");
        }
        return;
    }
    echoSpawnTimer -= dTime;
    if(echoSpawnTimer <= 0.0f){
        spawnEchoSignal();
        echoSpawnTimer = nextEchoSpawnDelaySeconds((int)rng());
        setEchoStatus("NEW ECHO SIGNAL DETECTED");
    }
}

inline void resolveEchoInteraction(){
    if(!echoSignal.active || multiState==MULTI_IN_GAME) return;
    bool breach = false;
    applyEchoOutcome(
        echoSignal.type,
        (int)rng(),
        invBattery,
        invMedkit,
        invBait,
        playerHealth,
        playerSanity,
        playerStamina,
        breach
    );
    if(echoSignal.type==ECHO_CACHE){
        setEchoStatus("ECHO CACHE: SUPPLY FOUND");
    }else if(echoSignal.type==ECHO_RESTORE){
        setEchoStatus("ECHO RESONANCE: VITALS RESTORED");
    }else{
        setEchoStatus("ECHO BREACH: HOSTILE SURGE");
    }
    if(breach){
        triggerLocalScare(0.30f, 0.17f, 8.0f);
        Vec3 ep = findSpawnPos(cam.pos, 12.0f);
        EntityType type = ((rng()%2)==0) ? ENTITY_SHADOW : ENTITY_CRAWLER;
        entityMgr.spawnEntity(type, ep, nullptr, 0, 0);
    }
    clearEchoSignal();
}

inline void drawMinimapOverlay(){
    int playerWX = (int)floorf(cam.pos.x / CS);
    int playerWZ = (int)floorf(cam.pos.z / CS);
    char rows[MINIMAP_DIAMETER][MINIMAP_DIAMETER + 1];
    buildMinimapRows(rows, playerWX, playerWZ, minimapWallSampler);

    drawFullscreenOverlay(0.02f,0.025f,0.03f,0.34f);
    drawText("MINIMAP",-0.95f,0.56f,1.20f,0.88f,0.92f,0.72f,0.95f);
    float y = 0.50f;
    for(int r=0;r<MINIMAP_DIAMETER;r++){
        drawText(rows[r],-0.95f,y,1.10f,0.86f,0.90f,0.72f,0.90f);
        y -= 0.045f;
    }
}
