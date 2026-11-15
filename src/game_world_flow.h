#pragma once

#include "coop.h"
#include "map_content.h"
#include "device_models.h"
#include "structure_models.h"
#include "decor_atlas.h"

void buildGeom(){
    std::vector<float>wv,fv,cv,pv,lv,lvOff,dv;
    int pcx=playerChunkX,pcz=playerChunkZ;
    for(int dcx=-VIEW_CHUNKS;dcx<=VIEW_CHUNKS;dcx++){
        for(int dcz=-VIEW_CHUNKS;dcz<=VIEW_CHUNKS;dcz++){
            auto it=chunks.find(chunkKey(pcx+dcx,pcz+dcz));
            if(it==chunks.end())continue;
            for(int lx=0;lx<CHUNK_SIZE;lx++){
                for(int lz=0;lz<CHUNK_SIZE;lz++){
                    int wx=(pcx+dcx)*CHUNK_SIZE+lx,wz=(pcz+dcz)*CHUNK_SIZE+lz;
                    if(it->second.cells[lx][lz]!=0)continue;
                    float px=wx*CS,pz=wz*CS;
                    const float uvFloor = 1.0f;
                    const float uvCeil = 1.0f;
                    bool hasHole = isFloorHoleCell(wx,wz) || isAbyssCell(wx,wz);
                    if(!hasHole){
                        float fl[]={px,0,pz,0,0,0,1,0,px,0,pz+CS,0,uvFloor,0,1,0,px+CS,0,pz+CS,uvFloor,uvFloor,0,1,0,
                                   px,0,pz,0,0,0,1,0,px+CS,0,pz+CS,uvFloor,uvFloor,0,1,0,px+CS,0,pz,uvFloor,0,0,1,0};
                        for(int i=0;i<48;i++)fv.push_back(fl[i]);
                    }else{
                        const float shaftDepth = 30.0f;
                        bool leftSolid = getCellWorld(wx-1,wz)==1 || (!isFloorHoleCell(wx-1,wz) && !isAbyssCell(wx-1,wz) && getCellWorld(wx-1,wz)==0);
                        bool rightSolid = getCellWorld(wx+1,wz)==1 || (!isFloorHoleCell(wx+1,wz) && !isAbyssCell(wx+1,wz) && getCellWorld(wx+1,wz)==0);
                        bool backSolid = getCellWorld(wx,wz-1)==1 || (!isFloorHoleCell(wx,wz-1) && !isAbyssCell(wx,wz-1) && getCellWorld(wx,wz-1)==0);
                        bool frontSolid = getCellWorld(wx,wz+1)==1 || (!isFloorHoleCell(wx,wz+1) && !isAbyssCell(wx,wz+1) && getCellWorld(wx,wz+1)==0);
                        if(leftSolid) mkShaftWall(wv,px,pz,0,CS,0,shaftDepth,CS);
                        if(rightSolid) mkShaftWall(wv,px+CS,pz+CS,0,-CS,0,shaftDepth,CS);
                        if(backSolid) mkShaftWall(wv,px+CS,pz,-CS,0,0,shaftDepth,CS);
                        if(frontSolid) mkShaftWall(wv,px,pz+CS,CS,0,0,shaftDepth,CS);
                    }
                    float cl[]={px,WH,pz,0,0,0,-1,0,px,WH,pz+CS,0,uvCeil,0,-1,0,px+CS,WH,pz+CS,uvCeil,uvCeil,0,-1,0,
                               px,WH,pz,0,0,0,-1,0,px+CS,WH,pz+CS,uvCeil,uvCeil,0,-1,0,px+CS,WH,pz,uvCeil,0,0,-1,0};
                    for(int i=0;i<48;i++)cv.push_back(cl[i]);
                    bool wallL = getCellWorld(wx-1,wz)==1;
                    bool wallR = getCellWorld(wx+1,wz)==1;
                    bool wallB = getCellWorld(wx,wz-1)==1;
                    bool wallF = getCellWorld(wx,wz+1)==1;
                    if(wallL)mkWall(wv,px,pz,0,CS,WH,CS,WH);
                    if(wallR)mkWall(wv,px+CS,pz+CS,0,-CS,WH,CS,WH);
                    if(wallB)mkWall(wv,px+CS,pz,-CS,0,WH,CS,WH);
                    if(wallF)mkWall(wv,px,pz+CS,CS,0,WH,CS,WH);

                    int doorAxis = doorwayAxisAt(wx,wz);
                    if(doorAxis!=0) buildDoorwayModel(dv,wx,wz,doorAxis);
                }
            }
        }
    }
    bool parkingLevel = isParkingLevel(gCurrentLevel);
    for(auto&p:pillars){
        mkPillar(pv,p.x,p.z,PILLAR_SIZE,WH);
        buildPillarTrimModel(dv,p,parkingLevel);
    }
    for(const auto& pr:mapProps) buildMapPropModel(dv,pr);
    for(const auto& poi:mapPois) buildMapPoiModel(dv,poi);
    if(parkingLevel){
        // Bay lines are laid per open cell of parking halls, so they stop at walls.
        for(int dcx=-VIEW_CHUNKS;dcx<=VIEW_CHUNKS;dcx++){
            for(int dcz=-VIEW_CHUNKS;dcz<=VIEW_CHUNKS;dcz++){
                if(chunkStyleAt(pcx+dcx,pcz+dcz)!=CHUNK_STYLE_PARKING) continue;
                if(chunks.find(chunkKey(pcx+dcx,pcz+dcz))==chunks.end()) continue;
                for(int lx=0;lx<CHUNK_SIZE;lx++){
                    for(int lz=0;lz<CHUNK_SIZE;lz+=3){
                        int wx=(pcx+dcx)*CHUNK_SIZE+lx, wz=(pcz+dcz)*CHUNK_SIZE+lz;
                        if(getCellWorld(wx,wz)!=0 || isFloorHoleCell(wx,wz) || isAbyssCell(wx,wz)) continue;
                        DecorBuilder line(dv,((float)wx+0.5f)*CS,((float)wz+0.5f)*CS,0.0f,1.0f);
                        line.decal(0.0f,0.012f,0.0f,CS,0.16f,DECOR_MAT_WHITE);
                    }
                }
            }
        }
    }
    if(coop.initialized){
        for(int s=0;s<2;s++) buildSwitchModel(dv,coop.switches[s],coop.switchOn[s]);
        bool storyExitReady = isStoryExitReady();
        bool doorOpenVisual = (multiState==MULTI_IN_GAME) ? coop.doorOpen : storyExitReady;
        buildExitDoorModel(dv,coop.doorPos,doorOpenVisual);
    }
    for(auto&l:lights){
        if(l.on)mkLight(lv,l.pos,l.sizeX,l.sizeZ);
        else mkLight(lvOff,l.pos,l.sizeX,l.sizeZ);
    }
    {
        std::vector<float> m;
        buildFlashlightModel(m);
        flashlightVC = (int)m.size()/8;
        if(flashlightVC>0) setupVAO(flashlightVAO, flashlightVBO, m, true);
    }
    {
        std::vector<float> m;
        buildScannerModel(m);
        scannerVC = (int)m.size()/8;
        if(scannerVC>0) setupVAO(scannerVAO, scannerVBO, m, true);
    }
    {
        std::vector<float> m;
        buildPlushToyModel(m);
        plushVC = (int)m.size()/8;
        if(plushVC>0) setupVAO(plushVAO, plushVBO, m, true);
    }
    {
        std::vector<float> m;
        buildBatteryModel(m);
        batteryVC = (int)m.size()/8;
        if(batteryVC>0) setupVAO(batteryVAO, batteryVBO, m, true);
    }
    {
        std::vector<float> m;
        buildMedSprayModel(m);
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
        worldSeed=(unsigned int)time(nullptr);
        if(multiState==MULTI_IN_GAME) netMgr.worldSeed = worldSeed;
    }
    
    chunks.clear();lights.clear();pillars.clear();resetMapContent();forcedDoorways.clear();
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
        if(netMgr.myId > 0) sp = findSpawnPos(sp, 2.2f + (float)netMgr.myId * 0.7f);
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
    initTrapCorridor(sp);

    // Spawn abyss location away from player spawn
    {
        int spawnWX = (int)floorf(sp.x / CS);
        int spawnWZ = (int)floorf(sp.z / CS);
        abyss.radius = 3 + (int)(rng() % 3); // radius 3-5 cells
        int attempts = 0;
        abyss.active = false;
        while(attempts < 60 && !abyss.active){
            attempts++;
            int offX = 20 + (int)(rng() % 20) - 10; // 10-30 cells away
            int offZ = 20 + (int)(rng() % 20) - 10;
            if(rng() % 2) offX = -offX;
            if(rng() % 2) offZ = -offZ;
            int cx = spawnWX + offX;
            int cz = spawnWZ + offZ;
            // Verify center and some cells around it are open
            bool valid = true;
            for(int dx = -1; dx <= 1 && valid; dx++)
                for(int dz = -1; dz <= 1 && valid; dz++)
                    if(getCellWorld(cx+dx, cz+dz) != 0) valid = false;
            if(!valid) continue;
            abyss.centerX = cx;
            abyss.centerZ = cz;
            abyss.active = true;
            // Carve out the abyss area (ensure open cells)
            for(int dx = -abyss.radius - 1; dx <= abyss.radius + 1; dx++){
                for(int dz = -abyss.radius - 1; dz <= abyss.radius + 1; dz++){
                    if(dx*dx + dz*dz <= (abyss.radius+1)*(abyss.radius+1)){
                        setCellWorld(cx+dx, cz+dz, 0);
                    }
                }
            }
        }
    }

    resetPlayerInterpolation();
    initCoopObjectives(coopBase);
    resetVoidShiftState(cam.pos, coop.doorPos);
    {
        Vec3 d = cam.pos - coop.doorPos;
        d.y = 0.0f;
        if(d.len() < 8.0f){
            Vec3 alt = findSpawnPos(coop.doorPos, 14.0f);
            cam.pos = Vec3(alt.x, PH, alt.z);
        }
    }
    worldItems.clear();
    nextWorldItemId = 1;
    invBattery = 0;
    invPlush = 0;
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
    abyss = {};
    lastSpawnedNote=-1;noteSpawnTimer=8.0f;
}

#include "teleport_helpers.h"
