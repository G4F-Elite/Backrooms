#pragma once
#pragma once

#include "coop.h"
#include "map_content.h"

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
                    const float uvTile = 2.2f;
                    bool hasHole = isFloorHoleCell(wx,wz) || isAbyssCell(wx,wz);
                    if(!hasHole){
                        float fl[]={px,0,pz,0,0,0,1,0,px,0,pz+CS,0,uvTile,0,1,0,px+CS,0,pz+CS,uvTile,uvTile,0,1,0,
                                   px,0,pz,0,0,0,1,0,px+CS,0,pz+CS,uvTile,uvTile,0,1,0,px+CS,0,pz,uvTile,0,0,1,0};
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
                    float cl[]={px,WH,pz,0,0,0,-1,0,px,WH,pz+CS,0,uvTile,0,-1,0,px+CS,WH,pz+CS,uvTile,uvTile,0,-1,0,
                               px,WH,pz,0,0,0,-1,0,px+CS,WH,pz+CS,uvTile,uvTile,0,-1,0,px+CS,WH,pz,uvTile,0,0,-1,0};
                    for(int i=0;i<48;i++)cv.push_back(cl[i]);
                    if(getCellWorld(wx-1,wz)==1)mkWall(wv,px,pz,0,CS,WH,CS,WH);
                    if(getCellWorld(wx+1,wz)==1)mkWall(wv,px+CS,pz+CS,0,-CS,WH,CS,WH);
                    if(getCellWorld(wx,wz-1)==1)mkWall(wv,px+CS,pz,-CS,0,WH,CS,WH);
                    if(getCellWorld(wx,wz+1)==1)mkWall(wv,px,pz+CS,CS,0,WH,CS,WH);
                }
            }
        }
    }
    for(auto&p:pillars)mkPillar(pv,p.x,p.z,0.6f,WH);
    for(auto&pr:mapProps){
        if(pr.type==MAP_PROP_CRATE_STACK){
            float b = 1.05f * pr.scale;
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, b, 0.82f * pr.scale, b);
            mkBox(dv, pr.pos.x + 0.36f, 0.0f, pr.pos.z - 0.30f, 0.70f * pr.scale, 0.62f * pr.scale, 0.70f * pr.scale);
        }else if(pr.type==MAP_PROP_CONE_CLUSTER){
            float base = 0.42f * pr.scale;
            mkBox(dv, pr.pos.x - 0.35f, 0.0f, pr.pos.z, base, 0.45f * pr.scale, base);
            mkBox(dv, pr.pos.x + 0.28f, 0.0f, pr.pos.z + 0.25f, base, 0.42f * pr.scale, base);
            mkBox(dv, pr.pos.x + 0.10f, 0.0f, pr.pos.z - 0.32f, base, 0.38f * pr.scale, base);
        }else if(pr.type==MAP_PROP_BARRIER){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.55f * pr.scale, 0.74f * pr.scale, 0.42f * pr.scale);
        }else if(pr.type==MAP_PROP_CABLE_REEL){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 0.92f * pr.scale, 0.45f * pr.scale, 0.92f * pr.scale);
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 0.30f * pr.scale, 0.78f * pr.scale, 0.30f * pr.scale);
        }else if(pr.type==MAP_PROP_PUDDLE){
            mkFloorDecal(dv, pr.pos.x, 0.02f, pr.pos.z, 1.7f * pr.scale, 1.3f * pr.scale);
        }else if(pr.type==MAP_PROP_DESK){
            mkBox(dv, pr.pos.x, 0.72f * pr.scale, pr.pos.z, 1.05f * pr.scale, 0.14f * pr.scale, 0.70f * pr.scale);
            mkBox(dv, pr.pos.x - 0.42f * pr.scale, 0.0f, pr.pos.z - 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
            mkBox(dv, pr.pos.x + 0.42f * pr.scale, 0.0f, pr.pos.z - 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
            mkBox(dv, pr.pos.x - 0.42f * pr.scale, 0.0f, pr.pos.z + 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
            mkBox(dv, pr.pos.x + 0.42f * pr.scale, 0.0f, pr.pos.z + 0.24f * pr.scale, 0.12f * pr.scale, 0.72f * pr.scale, 0.12f * pr.scale);
        }else if(pr.type==MAP_PROP_CHAIR){
            mkBox(dv, pr.pos.x, 0.36f * pr.scale, pr.pos.z, 0.48f * pr.scale, 0.10f * pr.scale, 0.48f * pr.scale);
            mkBox(dv, pr.pos.x, 0.46f * pr.scale, pr.pos.z - 0.18f * pr.scale, 0.48f * pr.scale, 0.42f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x - 0.18f * pr.scale, 0.0f, pr.pos.z + 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x + 0.18f * pr.scale, 0.0f, pr.pos.z + 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x - 0.18f * pr.scale, 0.0f, pr.pos.z - 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
            mkBox(dv, pr.pos.x + 0.18f * pr.scale, 0.0f, pr.pos.z - 0.18f * pr.scale, 0.10f * pr.scale, 0.36f * pr.scale, 0.10f * pr.scale);
        }else if(pr.type==MAP_PROP_CABINET){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 0.64f * pr.scale, 1.55f * pr.scale, 0.52f * pr.scale);
            mkBox(dv, pr.pos.x, 1.52f * pr.scale, pr.pos.z, 0.70f * pr.scale, 0.08f * pr.scale, 0.58f * pr.scale);
        }else if(pr.type==MAP_PROP_PARTITION){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.35f * pr.scale, 1.22f * pr.scale, 0.12f * pr.scale);
        }else if(pr.type==MAP_PROP_BOX_PALLET){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.22f * pr.scale, 0.34f * pr.scale, 1.02f * pr.scale);
            mkBox(dv, pr.pos.x, 0.34f * pr.scale, pr.pos.z, 1.10f * pr.scale, 0.78f * pr.scale, 0.96f * pr.scale);
            mkBox(dv, pr.pos.x, 1.12f * pr.scale, pr.pos.z, 0.88f * pr.scale, 0.50f * pr.scale, 0.82f * pr.scale);
        }else if(pr.type==MAP_PROP_DRUM_STACK){
            mkBox(dv, pr.pos.x - 0.22f * pr.scale, 0.0f, pr.pos.z, 0.48f * pr.scale, 1.12f * pr.scale, 0.48f * pr.scale);
            mkBox(dv, pr.pos.x + 0.22f * pr.scale, 0.0f, pr.pos.z, 0.48f * pr.scale, 1.06f * pr.scale, 0.48f * pr.scale);
            mkBox(dv, pr.pos.x, 1.06f * pr.scale, pr.pos.z, 0.44f * pr.scale, 0.44f * pr.scale, 0.44f * pr.scale);
        }else if(pr.type==MAP_PROP_LOCKER_BANK){
            mkBox(dv, pr.pos.x, 0.0f, pr.pos.z, 1.18f * pr.scale, 1.74f * pr.scale, 0.46f * pr.scale);
            mkBox(dv, pr.pos.x - 0.30f * pr.scale, 0.0f, pr.pos.z + 0.22f * pr.scale, 0.04f * pr.scale, 1.62f * pr.scale, 0.04f * pr.scale);
            mkBox(dv, pr.pos.x + 0.30f * pr.scale, 0.0f, pr.pos.z + 0.22f * pr.scale, 0.04f * pr.scale, 1.62f * pr.scale, 0.04f * pr.scale);
        }else{
            mkBox(dv, pr.pos.x - 0.42f, 0.0f, pr.pos.z + 0.24f, 0.68f * pr.scale, 0.35f * pr.scale, 0.68f * pr.scale);
            mkBox(dv, pr.pos.x + 0.28f, 0.0f, pr.pos.z - 0.22f, 0.56f * pr.scale, 0.28f * pr.scale, 0.56f * pr.scale);
        }
    }
    for(const auto& poi:mapPois){
        if(poi.type==MAP_POI_OFFICE){
            mkBox(dv, poi.pos.x, 0.0f, poi.pos.z, 1.00f, 0.22f, 1.00f);
            mkBox(dv, poi.pos.x, 0.22f, poi.pos.z - 0.32f, 0.92f, 0.62f, 0.10f);
            mkBox(dv, poi.pos.x - 0.52f, 0.0f, poi.pos.z + 0.42f, 0.62f, 0.84f, 0.40f);
            mkBox(dv, poi.pos.x + 0.52f, 0.0f, poi.pos.z + 0.42f, 0.62f, 0.84f, 0.40f);
            mkBox(dv, poi.pos.x, 0.84f, poi.pos.z + 0.12f, 0.52f, 0.08f, 0.18f);
            mkBox(dv, poi.pos.x - 0.22f, 0.0f, poi.pos.z - 0.68f, 0.18f, 0.64f, 0.18f);
            mkBox(dv, poi.pos.x + 0.22f, 0.0f, poi.pos.z - 0.68f, 0.18f, 0.64f, 0.18f);
        }else if(poi.type==MAP_POI_SERVER){
            mkBox(dv, poi.pos.x - 0.32f, 0.0f, poi.pos.z, 0.26f, 1.45f, 0.26f);
            mkBox(dv, poi.pos.x + 0.32f, 0.0f, poi.pos.z, 0.26f, 1.45f, 0.26f);
            mkBox(dv, poi.pos.x, 1.42f, poi.pos.z, 0.74f, 0.08f, 0.32f);
        }else if(poi.type==MAP_POI_STORAGE){
            mkBox(dv, poi.pos.x, 0.0f, poi.pos.z, 1.18f, 0.42f, 1.02f);
            mkBox(dv, poi.pos.x, 0.42f, poi.pos.z, 1.04f, 0.52f, 0.88f);
        }else{
            mkBox(dv, poi.pos.x, 0.0f, poi.pos.z, 0.82f, 0.20f, 0.82f);
            mkBox(dv, poi.pos.x, 0.20f, poi.pos.z, 0.54f, 0.12f, 0.54f);
        }
    }
    if(coop.initialized){
        for(int s=0;s<2;s++){
            const Vec3 sp = coop.switches[s];
            mkBox(dv, sp.x, 0.0f, sp.z, 0.84f, 0.22f, 0.84f);
            mkBox(dv, sp.x, 0.22f, sp.z, 0.20f, 0.82f, 0.20f);
            float leverX = sp.x + (coop.switchOn[s] ? 0.14f : -0.14f);
            mkBox(dv, leverX, 0.88f, sp.z, 0.28f, 0.10f, 0.10f);
        }
        bool storyExitReady = isStoryExitReady();
        bool doorOpenVisual = (multiState==MULTI_IN_GAME) ? coop.doorOpen : storyExitReady;
        const Vec3 dp = coop.doorPos;
        mkBox(dv, dp.x - CS * 0.58f, 0.0f, dp.z, 0.16f, 2.82f, 0.36f);
        mkBox(dv, dp.x + CS * 0.58f, 0.0f, dp.z, 0.16f, 2.82f, 0.36f);
        mkBox(dv, dp.x, 2.72f, dp.z, CS * 1.18f, 0.14f, 0.36f);
        if(!doorOpenVisual){
            mkBox(dv, dp.x, 0.0f, dp.z, CS * 1.06f, 2.62f, 0.20f);
        }else{
            mkBox(dv, dp.x, 0.0f, dp.z + 0.18f, CS * 1.06f, 0.10f, 0.54f);
        }
    }
    for(auto&l:lights){
        if(l.on)mkLight(lv,l.pos,l.sizeX,l.sizeZ);
        else mkLight(lvOff,l.pos,l.sizeX,l.sizeZ);
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
    if(multiState==MULTI_IN_GAME && !netMgr.isHost){
        worldSeed=netMgr.worldSeed;
    }else{
        worldSeed=(unsigned int)time(nullptr);
        if(multiState==MULTI_IN_GAME) netMgr.worldSeed = worldSeed;
    }
    
    chunks.clear();lights.clear();pillars.clear();mapProps.clear();mapPois.clear();
    g_lightStates.clear(); // Reset light temporal states on world gen
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
        sp.x+=netMgr.myId*1.5f;
    }
    
    cam.pos=Vec3(sp.x,PH,sp.z);cam.yaw=cam.pitch=0;
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
    invBattery = invMedkit = invBait = 0;
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
    baitEffectTimer = 0.0f;
    itemSpawnTimer = 6.0f;
    playerHealth=playerSanity=100; playerStamina=125;
    flashlightBattery=100;flashlightOn=false;isPlayerDead=false;
    playerEscaped=false;
    flashlightShutdownBlinkActive = false;
    flashlightShutdownBlinkTimer = 0.0f;
    resetScareSystemState(scareState);
    entitySpawnTimer=30;survivalTime=0;reshuffleTimer=15;
    resetPoiRuntime();
    floorHoles.clear();
    playerFalling = false;
    fallVelocity = 0.0f;
    fallTimer = 0.0f;
    abyss = {};
    lastSpawnedNote=-1;noteSpawnTimer=8.0f;
}

void teleportToPlayer(){
    if(multiState!=MULTI_IN_GAME)return;
    if(!netMgr.hasOtherPlayersWithPos()) return;
    Vec3 tp=netMgr.getOtherPlayerPos();
    cam.pos=Vec3(tp.x+1.0f,PH,tp.z);
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    buildGeom();
}

void teleportToExit(){
    if(!coop.initialized) return;
    cam.pos=Vec3(coop.doorPos.x,PH,coop.doorPos.z-2.0f);
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    updateMapContent(playerChunkX,playerChunkZ);
    buildGeom();
}

