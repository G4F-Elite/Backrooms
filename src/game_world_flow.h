#pragma once

#include "coop.h"
#include "map_content.h"
#include "device_models.h"
#include "structure_models.h"
#include "world_mesh.h"
#include "decor_atlas.h"
#include "heist_render.h"
#include "rig_render.h"

// World mutations (trap corridor, exit portal, floor holes, reshuffles) can leave decor inside
// a wall, in the exit doorway or above a pit. Drop it before meshing.
inline bool isDecorSpotLost(float x, float z){
    int wx=(int)floorf(x/CS), wz=(int)floorf(z/CS);
    return getCellWorld(wx,wz)!=0 || isForcedDoorway(wx,wz) || isFallCell(wx,wz) || heistIsReservedCell(wx,wz);
}

// Objectives are placed after the props, so a prop may end up on top of one.
inline bool isPropOnObjective(const MapProp& p){
    float hx=0.0f, hz=0.0f;
    mapPropHalfExtents(p,hx,hz);
    const float clearance=0.90f; // room for the objective machine itself
    auto hit=[&](const Vec3& o){ return fabsf(o.x-p.pos.x)<hx+clearance && fabsf(o.z-p.pos.z)<hz+clearance; };
    // a player spawned inside a prop could not move at all
    if(fabsf(cam.pos.x-p.pos.x)<hx+0.2f && fabsf(cam.pos.z-p.pos.z)<hz+0.2f) return true;
    HeistSolid solids[4];
    int n = collectHeistSolids(solids);
    for(int i=0;i<n;i++) if(hit(solids[i].pos)) return true;
    return false;
}

inline void pruneInvalidDecor(){
    mapProps.erase(std::remove_if(mapProps.begin(),mapProps.end(),[](const MapProp& p){
        if(isDecorSpotLost(p.pos.x,p.pos.z)) return true;
        if(p.type==MAP_PROP_PUDDLE) return false;
        if(isPropOnObjective(p)) return true;
        return doorwayAxisAt((int)floorf(p.pos.x/CS),(int)floorf(p.pos.z/CS))!=0;
    }),mapProps.end());
    mapPois.erase(std::remove_if(mapPois.begin(),mapPois.end(),[](const MapPoi& p){
        return isDecorSpotLost(p.pos.x,p.pos.z);
    }),mapPois.end());
    pillars.erase(std::remove_if(pillars.begin(),pillars.end(),[](const Vec3& p){
        return isDecorSpotLost(p.x,p.z);
    }),pillars.end());
}

void buildGeom(){
    pruneInvalidDecor();
    heistApplyDarkZone();
    int pcx=playerChunkX,pcz=playerChunkZ;
    WorldMesh world;
    buildWorldBaseMesh(world,pcx,pcz,[](int wx,int wz){ return isFloorHoleCell(wx,wz) || isAbyssCell(wx,wz); });
    if(coop.initialized) buildHeistModels(world.decor);
    buildWorldLampMesh(world);
    // Faces that share a plane would flicker through each other: the later one keeps the spot.
    gZfightWorld = resolveWorldMesh(world);
    std::vector<float>&wv=world.walls,&fv=world.floors,&cv=world.ceilings,&pv=world.pillars;
    std::vector<float>&dv=world.decor,&lv=world.lampsOn,&lvOff=world.lampsOff;
    {
        std::vector<float> m;
        buildFlashlightModel(m);
        resolveModelMesh(m, "flashlight");
        flashlightVC = (int)m.size()/8;
        if(flashlightVC>0) setupVAO(flashlightVAO, flashlightVBO, m, true);
    }
    {
        std::vector<float> m;
        buildScannerModel(m);
        resolveModelMesh(m, "scanner");
        scannerVC = (int)m.size()/8;
        if(scannerVC>0) setupVAO(scannerVAO, scannerVBO, m, true);
    }
    {
        std::vector<float> m;
        buildPlushToyModel(m);
        resolveModelMesh(m, "plush toy");
        plushVC = (int)m.size()/8;
        if(plushVC>0) setupVAO(plushVAO, plushVBO, m, true);
    }
    {
        std::vector<float> m;
        buildBatteryModel(m);
        resolveModelMesh(m, "battery");
        batteryVC = (int)m.size()/8;
        if(batteryVC>0) setupVAO(batteryVAO, batteryVBO, m, true);
    }
    buildHeistItemMeshes();
    {
        std::vector<float> m;
        buildMedSprayModel(m);
        resolveModelMesh(m, "med spray");
        medSprayVC = (int)m.size()/8;
        if(medSprayVC>0) setupVAO(medSprayVAO, medSprayVBO, m, true);
    }
    wallVC=(int)wv.size()/8;floorVC=(int)fv.size()/8;ceilVC=(int)cv.size()/8;
    pillarVC=(int)pv.size()/8;decorVC=(int)dv.size()/8;lightVC=(int)lv.size()/5;lightOffVC=(int)lvOff.size()/5;
    setupVAO(wallVAO,wallVBO,wv,true);setupVAO(floorVAO,floorVBO,fv,true);
    setupVAO(ceilVAO,ceilVBO,cv,true);setupVAO(pillarVAO,pillarVBO,pv,true);
    if(decorVC>0)setupVAO(decorVAO,decorVBO,dv,true);
    setupVAO(lightVAO,lightVBO,lv,false);
    if(!lvOff.empty())setupVAO(lightOffVAO,lightOffVBO,lvOff,false);
    initQuad(quadVAO,quadVBO);lastBuildChunkX=pcx;lastBuildChunkZ=pcz;
}
void buildNotes(float tm){
    std::vector<float>nv;
    for(auto&n:storyMgr.notes){
        if(!n.active||n.collected)continue;
        mkNoteGlow(nv,n.pos,n.bobPhase);
    }
    noteVC=(int)nv.size()/8;
    if(noteVC>0)setupVAO(noteVAO,noteVBO,nv,true);
}
void trySpawnNote(int noteId){
    if(noteId>=12||storyMgr.notesCollected[noteId])return;
    if(multiState==MULTI_IN_GAME && !netMgr.isHost) return;
    Vec3 sp=findSpawnPos(cam.pos,12.0f);
    Vec3 d=sp-cam.pos;d.y=0;
    if(sqrtf(d.x*d.x+d.z*d.z)>8.0f){
        storyMgr.spawnNote(sp,noteId);
        lastSpawnedNote=noteId;
        if(multiState==MULTI_IN_GAME) netMgr.sendNoteSpawn(noteId, sp);
    }
}
void cleanupFarNotes(){
    for(auto&n:storyMgr.notes){
        if(!n.active||n.collected)continue;
        Vec3 d=n.pos-cam.pos;d.y=0;
        if(sqrtf(d.x*d.x+d.z*d.z)>80.0f){
            n.active=false;
            if(n.id==lastSpawnedNote)lastSpawnedNote=n.id-1;
        }
    }
}

void genWorld(){
    static int lastTextureLevel = -999;
    if(lastTextureLevel != gCurrentLevel){
        if(wallTex) glDeleteTextures(1, &wallTex);
        if(floorTex) glDeleteTextures(1, &floorTex);
        if(ceilTex) glDeleteTextures(1, &ceilTex);
        if(lampTex) glDeleteTextures(1, &lampTex);
        if(propTex) glDeleteTextures(1, &propTex);
        if(decorTex) glDeleteTextures(1, &decorTex);
        if(deviceTex) glDeleteTextures(1, &deviceTex);
        if(plushTex) glDeleteTextures(1, &plushTex);
        wallTex=genTex(0); floorTex=genTex(1); ceilTex=genTex(2); lampTex=genTex(4); propTex=genTex(5);
        deviceTex=genTex(6); plushTex=genTex(7);
        decorTex=genDecorAtlasTex();
        lastTextureLevel = gCurrentLevel;
    }

    if(multiState==MULTI_IN_GAME && !netMgr.isHost){
        worldSeed=netMgr.worldSeed;
    }else{
        unsigned int prevSeed = worldSeed;
        worldSeed=(unsigned int)time(nullptr);
        if(worldSeed==prevSeed) worldSeed++; // a restart must always look like a new world to clients
        if(multiState==MULTI_IN_GAME) netMgr.worldSeed = worldSeed;
    }
    
    chunks.clear();lights.clear();pillars.clear();resetMapContent();forcedDoorways.clear();gCellOverrides.clear();
    g_lightStates.clear();
    updateVisibleChunks(0,0);
    updateLightsAndPillars(0,0);
    updateMapContent(0,0);
    Vec3 sp=findSafeSpawn();
    Vec3 coopBase = sp;
    
    if(multiState==MULTI_IN_GAME){
        if(netMgr.isHost){
            netMgr.spawnPos=sp;
        }else{
            sp=netMgr.spawnPos;
            coopBase = netMgr.spawnPos;
        }
        if(netMgr.myId > 0) sp = squadSpawnSlot(sp, netMgr.myId);
    }
    
    cam.pos=Vec3(sp.x,PH,sp.z);cam.yaw=cam.pitch=0;
    if(collideWorld(cam.pos.x, cam.pos.z, 0.42f)){
        Vec3 safe = findSpawnPos(cam.pos, 5.0f);
        cam.pos = Vec3(safe.x, PH, safe.z);
    }
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    entityMgr.reset();storyMgr.init();
    storyMgr.introComplete = true;
    trapCorridor = {};
    if(!isLevelZero(gCurrentLevel)) initTrapCorridor(sp); // the first floor is the tutorial

    resetPlayerInterpolation();
    initCoopObjectives(coopBase);
    resetVoidShiftState();
    resetCoopRunState(); resetPoiRuntime();
    if(isLevelZero(gCurrentLevel)) resetShiftStats();
    worldItems.clear();
    nextWorldItemId = 1;
    if(isLevelZero(gCurrentLevel)){ invBattery = 0; invPlush = 0; }
    // Garage + portal on the first floor, drill / kiosk / loot everywhere (moves the player too).
    heistBeginLevel(coopBase);
    coop.doorPos = heist.drillPos;
    placeAbyss(coopBase);
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    clearEchoSignal();
    echoSpawnTimer = 7.0f + (float)(rng()%6);
    echoStatusTimer = 0.0f;
    echoStatusText[0] = '\0';
    storyEchoAttuned = false;
    storyEchoAttunedCount = 0;
    trapStatusTimer = 0.0f;
    trapStatusText[0] = '\0';
    smileEvent = {false, Vec3(0,0,0), 0.0f, 0.0f, 24.0f + (float)(rng()%16), false, Vec3(0,0,0), Vec3(0,0,0), 0.0f};
    anomalyBlur = 0.0f;
    lightsOutTimer = falseDoorTimer = 0.0f;
    itemSpawnTimer = 6.0f;
    playerHealth=playerSanity=100; playerStamina=125;
    flashlightBattery=100;flashlightOn=false;isPlayerDead=false;
    playerDowned = false;
    playerDownedTimer = 0.0f;
    snprintf(gDeathReason,sizeof(gDeathReason),"CAUSE: INCIDENT UNCONFIRMED");
    playerEscaped=false;
    flashlightShutdownBlinkActive = false;
    flashlightShutdownBlinkTimer = 0.0f;
    resetScareSystemState(scareState);
    entitySpawnTimer=30;survivalTime=0;reshuffleTimer=15;
    floorHoles.clear();
    playerFalling = false;
    fallVelocity = 0.0f;
    fallTimer = 0.0f;
    gFall = FallState();
    lastSpawnedNote=-1;noteSpawnTimer=8.0f;
}

#include "teleport_helpers.h"
