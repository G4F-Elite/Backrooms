#pragma once

GLuint noteVAO=0, noteVBO=0;
int noteVC=0;
bool playerModelsInit = false;
#include "reconnect_policy.h"
void buildGeom();

enum InteractRequestType {
    REQ_TOGGLE_SWITCH = 1,
    REQ_PICK_ITEM
};

enum RoamEventType {
    ROAM_NONE = 0,
    ROAM_LIGHTS_OUT = 1,
    ROAM_GEOM_SHIFT = 2,
    ROAM_FALSE_DOOR = 3
};

struct CoopObjectives {
    Vec3 switches[2];
    bool switchOn[2];
    Vec3 doorPos;
    bool doorOpen;
    bool initialized;
} coop = {};

struct WorldItem {
    int id;
    int type; // 0 battery, 1 medkit, 2 bait
    Vec3 pos;
    bool active;
};

std::vector<WorldItem> worldItems;
int nextWorldItemId = 1;
float itemSpawnTimer = 12.0f;
float baitEffectTimer = 0.0f;
float lightsOutTimer = 0.0f;
float falseDoorTimer = 0.0f;
Vec3 falseDoorPos(0,0,0);
int invBattery = 0, invMedkit = 0, invBait = 0;

struct SessionSnapshot {
    bool valid;
    char hostIP[64];
    Vec3 camPos;
    float camYaw, camPitch;
    float health, sanity, stamina, battery;
    float survival;
    int invB, invM, invT;
};

SessionSnapshot lastSession = {};
bool reconnectInProgress = false;
bool restoreAfterReconnect = false;
float reconnectAttemptTimer = 0.0f;
int reconnectAttempts = 0;

inline void captureSessionSnapshot(){
    if(multiState!=MULTI_IN_GAME || netMgr.isHost) return;
    lastSession.valid = true;
    snprintf(lastSession.hostIP, sizeof(lastSession.hostIP), "%s", netMgr.hostIP);
    lastSession.camPos = cam.pos;
    lastSession.camYaw = cam.yaw;
    lastSession.camPitch = cam.pitch;
    lastSession.health = playerHealth;
    lastSession.sanity = playerSanity;
    lastSession.stamina = playerStamina;
    lastSession.battery = flashlightBattery;
    lastSession.survival = survivalTime;
    lastSession.invB = invBattery;
    lastSession.invM = invMedkit;
    lastSession.invT = invBait;
}

inline void restoreSessionSnapshot(){
    if(!lastSession.valid) return;
    cam.pos = lastSession.camPos;
    cam.yaw = lastSession.camYaw;
    cam.pitch = lastSession.camPitch;
    playerHealth = lastSession.health;
    playerSanity = lastSession.sanity;
    playerStamina = lastSession.stamina;
    flashlightBattery = lastSession.battery;
    survivalTime = lastSession.survival;
    invBattery = lastSession.invB;
    invMedkit = lastSession.invM;
    invBait = lastSession.invT;
}

inline void initCoopObjectives(const Vec3& basePos){
    coop.switches[0] = Vec3(basePos.x + CS * 2.0f, 0, basePos.z + CS * 1.0f);
    coop.switches[1] = Vec3(basePos.x - CS * 2.0f, 0, basePos.z + CS * 1.0f);
    coop.switchOn[0] = false;
    coop.switchOn[1] = false;
    coop.doorPos = Vec3(basePos.x, 0, basePos.z + CS * 4.0f);
    coop.doorOpen = false;
    coop.initialized = true;
}

inline bool nearPoint2D(const Vec3& a, const Vec3& b, float r){
    Vec3 d = a - b; d.y = 0;
    return sqrtf(d.x*d.x + d.z*d.z) < r;
}

inline void updateCoopObjectiveHost(){
    if(!coop.initialized) return;
    for(int s=0;s<2;s++) {
        bool on = nearPoint2D(cam.pos, coop.switches[s], 2.4f);
        for(int p=0;p<MAX_PLAYERS;p++){
            if(p==netMgr.myId || !netMgr.players[p].active || !netMgr.players[p].hasValidPos) continue;
            if(nearPoint2D(netMgr.players[p].pos, coop.switches[s], 2.4f)) { on = true; break; }
        }
        coop.switchOn[s] = on;
    }
    coop.doorOpen = coop.switchOn[0] && coop.switchOn[1];
}

inline bool collideCoopDoor(float x, float z, float r){
    if(!coop.initialized || coop.doorOpen) return false;
    return fabsf(x - coop.doorPos.x) < (CS * 0.6f + r) && fabsf(z - coop.doorPos.z) < (CS * 0.2f + r);
}

inline void syncCoopFromNetwork(){
    if(!netMgr.objectiveStateReceived) return;
    netMgr.objectiveStateReceived = false;
    coop.switchOn[0] = netMgr.objectiveSwitches[0];
    coop.switchOn[1] = netMgr.objectiveSwitches[1];
    coop.doorOpen = netMgr.objectiveDoorOpen;
}

inline void hostSpawnItem(int type, const Vec3& around){
    WorldItem it;
    it.id = nextWorldItemId++ % 250;
    it.type = type;
    it.pos = findSpawnPos(around, 6.0f);
    it.active = true;
    worldItems.push_back(it);
}

inline void hostUpdateItems(){
    itemSpawnTimer -= dTime;
    if(itemSpawnTimer > 0) return;
    itemSpawnTimer = 10.0f + (rng()%10);
    if((int)worldItems.size() > 10) return;
    int type = rng()%3;
    hostSpawnItem(type, cam.pos);
}

inline void applyItemUse(int type){
    if(type==0 && invBattery>0){
        invBattery--;
        flashlightBattery += 35.0f;
        if(flashlightBattery>100.0f) flashlightBattery = 100.0f;
    }else if(type==1 && invMedkit>0){
        invMedkit--;
        playerHealth += 40.0f;
        if(playerHealth>100.0f) playerHealth = 100.0f;
    }else if(type==2 && invBait>0){
        invBait--;
        baitEffectTimer = 12.0f;
    }
}

inline void processHostInteractRequests(){
    if(!netMgr.isHost) return;
    for(int i=0;i<netMgr.interactRequestCount;i++){
        if(!netMgr.interactRequests[i].valid) continue;
        int pid = netMgr.interactRequests[i].playerId;
        int type = netMgr.interactRequests[i].requestType;
        int target = netMgr.interactRequests[i].targetId;
        if(type==REQ_PICK_ITEM){
            for(auto& it:worldItems){
                if(!it.active || it.id!=target) continue;
                it.active = false;
                if(pid>=0 && pid<MAX_PLAYERS){
                    if(it.type==0) netMgr.inventoryBattery[pid]++;
                    else if(it.type==1) netMgr.inventoryMedkit[pid]++;
                    else netMgr.inventoryBait[pid]++;
                }
                break;
            }
        }else if(type==REQ_TOGGLE_SWITCH){
            if(target>=0 && target<2) coop.switchOn[target] = !coop.switchOn[target];
        }
    }
    netMgr.interactRequestCount = 0;
}

inline void hostSyncFeatureState(){
    NetWorldItemSnapshotEntry entries[MAX_SYNC_ITEMS];
    int count = 0;
    for(auto& it:worldItems){
        if(count>=MAX_SYNC_ITEMS) break;
        entries[count].id = it.id;
        entries[count].type = it.type;
        entries[count].pos = it.pos;
        entries[count].active = it.active;
        count++;
    }
    netMgr.sendItemSnapshot(entries, count);
    netMgr.sendObjectiveState(coop.switchOn[0], coop.switchOn[1], coop.doorOpen);
    netMgr.inventoryBattery[netMgr.myId] = invBattery;
    netMgr.inventoryMedkit[netMgr.myId] = invMedkit;
    netMgr.inventoryBait[netMgr.myId] = invBait;
    netMgr.sendInventorySync();
}

inline void clientApplyFeatureState(){
    if(netMgr.itemSnapshotReceived){
        netMgr.itemSnapshotReceived = false;
        worldItems.clear();
        for(int i=0;i<netMgr.itemSnapshotCount;i++){
            WorldItem it;
            it.id = netMgr.itemSnapshot[i].id;
            it.type = netMgr.itemSnapshot[i].type;
            it.pos = netMgr.itemSnapshot[i].pos;
            it.active = netMgr.itemSnapshot[i].active;
            worldItems.push_back(it);
        }
    }
    if(netMgr.inventorySyncReceived){
        netMgr.inventorySyncReceived = false;
        invBattery = netMgr.inventoryBattery[netMgr.myId];
        invMedkit = netMgr.inventoryMedkit[netMgr.myId];
        invBait = netMgr.inventoryBait[netMgr.myId];
    }
    syncCoopFromNetwork();
}

inline void applyRoamEvent(int type, int a, int b, float duration){
    if(type==ROAM_LIGHTS_OUT){
        lightsOutTimer = duration;
        for(auto& l:lights) l.on = false;
    }else if(type==ROAM_GEOM_SHIFT){
        (void)a; (void)b;
        reshuffleBehind(cam.pos.x, cam.pos.z, cam.yaw);
        buildGeom();
    }else if(type==ROAM_FALSE_DOOR){
        falseDoorTimer = duration;
        falseDoorPos = findSpawnPos(cam.pos, 2.0f);
    }
}

inline void updateRoamEventsHost(){
    static float roamTimer = 18.0f;
    roamTimer -= dTime;
    if(roamTimer > 0) return;
    roamTimer = 18.0f + (rng()%15);
    int type = 1 + (rng()%3);
    float duration = (type==ROAM_GEOM_SHIFT) ? 0.1f : 8.0f;
    applyRoamEvent(type, playerChunkX, playerChunkZ, duration);
    netMgr.sendRoamEvent(type, playerChunkX & 0xFF, playerChunkZ & 0xFF, duration);
}

void buildGeom(){
    std::vector<float>wv,fv,cv,pv,lv,lvOff;
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
                    float fl[]={px,0,pz,0,0,0,1,0,px,0,pz+CS,0,1,0,1,0,px+CS,0,pz+CS,1,1,0,1,0,
                               px,0,pz,0,0,0,1,0,px+CS,0,pz+CS,1,1,0,1,0,px+CS,0,pz,1,0,0,1,0};
                    for(int i=0;i<48;i++)fv.push_back(fl[i]);
                    float cl[]={px,WH,pz,0,0,0,-1,0,px,WH,pz+CS,0,1,0,-1,0,px+CS,WH,pz+CS,1,1,0,-1,0,
                               px,WH,pz,0,0,0,-1,0,px+CS,WH,pz+CS,1,1,0,-1,0,px+CS,WH,pz,1,0,0,-1,0};
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
    for(auto&l:lights){
        if(l.on)mkLight(lv,l.pos,l.sizeX,l.sizeZ);
        else mkLight(lvOff,l.pos,l.sizeX,l.sizeZ);
    }
    wallVC=(int)wv.size()/8;floorVC=(int)fv.size()/8;ceilVC=(int)cv.size()/8;
    pillarVC=(int)pv.size()/8;lightVC=(int)lv.size()/5;lightOffVC=(int)lvOff.size()/5;
    setupVAO(wallVAO,wallVBO,wv,true);setupVAO(floorVAO,floorVBO,fv,true);
    setupVAO(ceilVAO,ceilVBO,cv,true);setupVAO(pillarVAO,pillarVBO,pv,true);
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
    // Only host spawns notes in multiplayer
    if(multiState==MULTI_IN_GAME && !netMgr.isHost) return;
    Vec3 sp=findSpawnPos(cam.pos,12.0f);
    Vec3 d=sp-cam.pos;d.y=0;
    if(sqrtf(d.x*d.x+d.z*d.z)>8.0f){
        storyMgr.spawnNote(sp,noteId);
        lastSpawnedNote=noteId;
        // Sync note spawn
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
    // Use network seed if in multiplayer
    if(multiState==MULTI_IN_GAME && !netMgr.isHost){
        worldSeed=netMgr.worldSeed;
    }else{
        worldSeed=(unsigned int)time(nullptr);
        if(multiState==MULTI_IN_GAME) netMgr.worldSeed = worldSeed;
    }
    
    chunks.clear();lights.clear();pillars.clear();
    updateVisibleChunks(0,0);
    updateLightsAndPillars(0,0);
    Vec3 sp=findSafeSpawn();
    Vec3 coopBase = sp;
    
    // In multiplayer, use shared spawn position
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
    entityMgr.reset();storyMgr.init();
    resetPlayerInterpolation();
    initCoopObjectives(coopBase);
    worldItems.clear();
    nextWorldItemId = 1;
    invBattery = invMedkit = invBait = 0;
    lightsOutTimer = falseDoorTimer = 0.0f;
    baitEffectTimer = 0.0f;
    itemSpawnTimer = 8.0f;
    playerHealth=playerSanity=playerStamina=100;
    flashlightBattery=100;flashlightOn=false;isPlayerDead=false;
    entitySpawnTimer=30;survivalTime=0;reshuffleTimer=15;
    lastSpawnedNote=-1;noteSpawnTimer=15.0f;
}

void teleportToPlayer(){
    if(multiState!=MULTI_IN_GAME)return;
    if(!netMgr.hasOtherPlayersWithPos()) return;
    Vec3 tp=netMgr.getOtherPlayerPos();
    cam.pos=Vec3(tp.x+1.0f,PH,tp.z);
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    buildGeom();
}

void mouse(GLFWwindow*,double xp,double yp){
    if(gameState!=STATE_GAME&&gameState!=STATE_INTRO)return;
    if(firstMouse){lastX=(float)xp;lastY=(float)yp;firstMouse=false;}
    cam.yaw-=((float)xp-lastX)*settings.mouseSens;
    cam.pitch+=(lastY-(float)yp)*settings.mouseSens;
    cam.pitch=cam.pitch>1.4f?1.4f:(cam.pitch<-1.4f?-1.4f:cam.pitch);
    lastX=(float)xp;lastY=(float)yp;
}

void gameInput(GLFWwindow*w){
    if(glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS&&!escPressed){
        gameState=STATE_PAUSE;menuSel=0;
        glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    }
    escPressed=glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS;
    
    bool fNow=glfwGetKey(w,GLFW_KEY_F)==GLFW_PRESS;
    if(fNow&&!flashlightPressed&&flashlightBattery>5.0f){
        flashlightOn=!flashlightOn;
        sndState.flashlightOn=flashlightOn?1.0f:0.0f;
    }
    flashlightPressed=fNow;
    
    bool eNow=glfwGetKey(w,GLFW_KEY_E)==GLFW_PRESS;
    int nearItemId = -1;
    for(auto& it:worldItems){
        if(!it.active) continue;
        if(nearPoint2D(cam.pos, it.pos, 2.2f)){ nearItemId = it.id; break; }
    }
    if(eNow&&!interactPressed&&nearNoteId>=0){
        if(storyMgr.checkNotePickup(cam.pos,4.0f)){
            gameState=STATE_NOTE;
            playerSanity-=8.0f;
            if(playerSanity<0)playerSanity=0;
            // Sync note collection
            if(multiState==MULTI_IN_GAME) netMgr.sendNoteCollect(nearNoteId);
        }
    }else if(eNow&&!interactPressed&&nearItemId>=0){
        if(multiState==MULTI_IN_GAME){
            if(netMgr.isHost){
                for(auto& it:worldItems){
                    if(!it.active||it.id!=nearItemId) continue;
                    it.active=false;
                    if(it.type==0) invBattery++;
                    else if(it.type==1) invMedkit++;
                    else invBait++;
                    break;
                }
            }else{
                netMgr.sendInteractRequest(REQ_PICK_ITEM, nearItemId);
            }
        }else{
            for(auto& it:worldItems){
                if(!it.active||it.id!=nearItemId) continue;
                it.active=false;
                if(it.type==0) invBattery++;
                else if(it.type==1) invMedkit++;
                else invBait++;
                break;
            }
        }
    }
    interactPressed=eNow;
    
    static bool key1Pressed=false,key2Pressed=false,key3Pressed=false;
    bool k1=glfwGetKey(w,GLFW_KEY_1)==GLFW_PRESS;
    bool k2=glfwGetKey(w,GLFW_KEY_2)==GLFW_PRESS;
    bool k3=glfwGetKey(w,GLFW_KEY_3)==GLFW_PRESS;
    if(k1&&!key1Pressed) applyItemUse(0);
    if(k2&&!key2Pressed) applyItemUse(1);
    if(k3&&!key3Pressed) applyItemUse(2);
    key1Pressed=k1; key2Pressed=k2; key3Pressed=k3;
    
    float spd=4.0f*dTime;
    bool sprinting=glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS&&playerStamina>0&&staminaCooldown<=0;
    if(sprinting){
        spd*=1.6f;playerStamina-=20.0f*dTime;
        if(playerStamina<0){playerStamina=0;staminaCooldown=1.5f;}
    }else{
        playerStamina+=15.0f*dTime;
        if(playerStamina>100)playerStamina=100;
    }
    if(staminaCooldown>0)staminaCooldown-=dTime;
    
    if(glfwGetKey(w,GLFW_KEY_LEFT_CONTROL)==GLFW_PRESS){
        cam.targetH=PH_CROUCH;cam.crouch=true;spd*=0.5f;
    }else{cam.targetH=PH;cam.crouch=false;}
    cam.curH+=(cam.targetH-cam.curH)*10.0f*dTime;
    
    Vec3 fwd(sinf(cam.yaw),0,cosf(cam.yaw)),right(cosf(cam.yaw),0,-sinf(cam.yaw));
    Vec3 np=cam.pos;bool mv=false;
    if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS){np=np+fwd*spd;mv=true;}
    if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS){np=np-fwd*spd;mv=true;}
    if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS){np=np+right*spd;mv=true;}
    if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS){np=np-right*spd;mv=true;}
    
    if(!collideWorld(np.x,cam.pos.z,PR)&&!collideCoopDoor(np.x,cam.pos.z,PR)&&!(falseDoorTimer>0&&nearPoint2D(Vec3(np.x,0,cam.pos.z),falseDoorPos,1.0f)))cam.pos.x=np.x;
    if(!collideWorld(cam.pos.x,np.z,PR)&&!collideCoopDoor(cam.pos.x,np.z,PR)&&!(falseDoorTimer>0&&nearPoint2D(Vec3(cam.pos.x,0,np.z),falseDoorPos,1.0f)))cam.pos.z=np.z;
    
    static float bobT=0,lastB=0;
    if(mv){
        bobT+=dTime*(spd>5.0f?12.0f:8.0f);
        float cb=sinf(bobT);cam.pos.y=cam.curH+cb*0.04f;
        if(lastB>-0.7f&&cb<=-0.7f&&!sndState.stepTrig){
            sndState.stepTrig=true;sndState.footPhase=0;
        }
        lastB=cb;
    }else{
        cam.pos.y=cam.curH+(cam.pos.y-cam.curH)*0.9f;bobT=0;lastB=0;
    }
    
    if(flashlightOn){
        if(flashlightBattery<18.0f){
            float blinkT = vhsTime * (6.0f + (18.0f - flashlightBattery) * 0.35f);
            bool blinkOff = sinf(blinkT) > 0.78f;
            sndState.flashlightOn = blinkOff ? 0.0f : 1.0f;
        }else{
            sndState.flashlightOn = 1.0f;
        }
        flashlightBattery-=dTime*1.67f;
        if(flashlightBattery<=0){flashlightBattery=0;flashlightOn=false;sndState.flashlightOn=0;}
    }else{
        flashlightBattery+=dTime*10.0f;
        if(flashlightBattery>100)flashlightBattery=100;
    }
}

void renderScene(){
    glUseProgram(mainShader);
    Mat4 proj=Mat4::persp(1.2f,(float)winW/winH,0.1f,100.0f);
    float shX=camShake*(rand()%100-50)/500.0f,shY=camShake*(rand()%100-50)/500.0f;
    Vec3 la=cam.pos+Vec3(sinf(cam.yaw+shX)*cosf(cam.pitch+shY),sinf(cam.pitch+shY),
                         cosf(cam.yaw+shX)*cosf(cam.pitch+shY));
    Mat4 view=Mat4::look(cam.pos,la,Vec3(0,1,0)),model;
    
    glUniformMatrix4fv(glGetUniformLocation(mainShader,"P"),1,GL_FALSE,proj.m);
    glUniformMatrix4fv(glGetUniformLocation(mainShader,"V"),1,GL_FALSE,view.m);
    glUniformMatrix4fv(glGetUniformLocation(mainShader,"M"),1,GL_FALSE,model.m);
    glUniform3f(glGetUniformLocation(mainShader,"vp"),cam.pos.x,cam.pos.y,cam.pos.z);
    glUniform1f(glGetUniformLocation(mainShader,"tm"),vhsTime);
    glUniform1f(glGetUniformLocation(mainShader,"danger"),entityMgr.dangerLevel);
    bool flashVisualOn = flashlightOn;
    if(flashlightOn && flashlightBattery < 18.0f){
        float blinkT = vhsTime * (6.0f + (18.0f - flashlightBattery) * 0.35f);
        if(sinf(blinkT) > 0.78f) flashVisualOn = false;
    }
    glUniform1i(glGetUniformLocation(mainShader,"flashOn"),flashVisualOn?1:0);
    glUniform3f(glGetUniformLocation(mainShader,"flashDir"),sinf(cam.yaw)*cosf(cam.pitch),
                sinf(cam.pitch),cosf(cam.yaw)*cosf(cam.pitch));
    float remoteFlashPos[12] = {0};
    float remoteFlashDir[12] = {0};
    int remoteFlashCount = 0;
    if(multiState==MULTI_IN_GAME){
        remoteFlashCount = gatherRemoteFlashlights(netMgr.myId, remoteFlashPos, remoteFlashDir);
    }
    glUniform1i(glGetUniformLocation(mainShader,"rfc"),remoteFlashCount);
    if(remoteFlashCount>0){
        glUniform3fv(glGetUniformLocation(mainShader,"rfp"),remoteFlashCount,remoteFlashPos);
        glUniform3fv(glGetUniformLocation(mainShader,"rfd"),remoteFlashCount,remoteFlashDir);
    }
    
    std::vector<float>lpos;
    for(auto&l:lights)if(l.on){
        lpos.push_back(l.pos.x);lpos.push_back(l.pos.y);lpos.push_back(l.pos.z);
    }
    int nl=(int)lpos.size()/3;if(nl>64)nl=64;
    glUniform1i(glGetUniformLocation(mainShader,"nl"),nl);
    if(!lpos.empty())glUniform3fv(glGetUniformLocation(mainShader,"lp"),nl,lpos.data());
    
    glBindTexture(GL_TEXTURE_2D,wallTex);glBindVertexArray(wallVAO);glDrawArrays(GL_TRIANGLES,0,wallVC);
    glBindVertexArray(pillarVAO);glDrawArrays(GL_TRIANGLES,0,pillarVC);
    glBindTexture(GL_TEXTURE_2D,floorTex);glBindVertexArray(floorVAO);glDrawArrays(GL_TRIANGLES,0,floorVC);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D,ceilTex);glBindVertexArray(ceilVAO);glDrawArrays(GL_TRIANGLES,0,ceilVC);
    if(noteVC>0){glBindTexture(GL_TEXTURE_2D,lightTex);glBindVertexArray(noteVAO);glDrawArrays(GL_TRIANGLES,0,noteVC);}
    glEnable(GL_CULL_FACE);
    
    // Render other players in multiplayer
    if(multiState==MULTI_IN_GAME && playerModelsInit){
        renderPlayers(mainShader, proj, view, netMgr.myId);
    }
    
    glUseProgram(lightShader);
    glUniformMatrix4fv(glGetUniformLocation(lightShader,"P"),1,GL_FALSE,proj.m);
    glUniformMatrix4fv(glGetUniformLocation(lightShader,"V"),1,GL_FALSE,view.m);
    glUniformMatrix4fv(glGetUniformLocation(lightShader,"M"),1,GL_FALSE,model.m);
    glUniform1f(glGetUniformLocation(lightShader,"inten"),1.2f);
    glUniform1f(glGetUniformLocation(lightShader,"tm"),vhsTime);
    glBindTexture(GL_TEXTURE_2D,lightTex);glBindVertexArray(lightVAO);glDrawArrays(GL_TRIANGLES,0,lightVC);
    if(lightOffVC>0){
        glUniform1f(glGetUniformLocation(lightShader,"inten"),0.15f);
        glBindVertexArray(lightOffVAO);glDrawArrays(GL_TRIANGLES,0,lightOffVC);
    }
    entityMgr.render(mainShader,proj,view);
}

void drawUI(){
    if(gameState==STATE_MENU) drawMenu(vhsTime);
    else if(gameState==STATE_MULTI) drawMultiMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_HOST) drawHostLobbyScreen(vhsTime,netMgr.getPlayerCount());
    else if(gameState==STATE_MULTI_JOIN) drawJoinMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_WAIT) drawWaitingScreen(vhsTime);
    else if(gameState==STATE_PAUSE){
        if(multiState==MULTI_IN_GAME) drawMultiPause(netMgr.getPlayerCount());
        else drawPause();
    }else if(gameState==STATE_SETTINGS||gameState==STATE_SETTINGS_PAUSE) drawSettings(gameState==STATE_SETTINGS_PAUSE);
    else if(gameState==STATE_INTRO) drawIntro(storyMgr.introLine,storyMgr.introTimer,storyMgr.introLineTime,INTRO_LINES);
    else if(gameState==STATE_NOTE&&storyMgr.readingNote&&storyMgr.currentNote>=0)
        drawNote(storyMgr.currentNote,NOTE_TITLES[storyMgr.currentNote],NOTE_CONTENTS[storyMgr.currentNote]);
    else if(gameState==STATE_GAME){
        gSurvivalTime=survivalTime;
        if(isPlayerDead) drawDeath(vhsTime);
        else{
            drawDamageOverlay(damageFlash,playerHealth);
            drawSurvivalTime(survivalTime);
            if(playerHealth<100)drawHealthBar(playerHealth);
            if(playerSanity<100)drawSanityBar(playerSanity);
            drawStaminaBar(playerStamina);
            if(flashlightBattery<100)drawFlashlightBattery(flashlightBattery,flashlightOn);
            char invBuf[64];
            snprintf(invBuf,64,"INV B:%d M:%d T:%d",invBattery,invMedkit,invBait);
            drawText(invBuf,-0.95f,0.84f,1.2f,0.55f,0.7f,0.5f,0.75f);
            char objBuf[64];
            snprintf(objBuf,64,"SW:%d/%d DOOR:%s",(coop.switchOn[0]?1:0)+(coop.switchOn[1]?1:0),2,coop.doorOpen?"OPEN":"LOCK");
            drawText(objBuf,0.50f,0.74f,1.2f,0.8f,0.85f,0.6f,0.75f);
            if(!coop.doorOpen){
                if(nearPoint2D(cam.pos, coop.switches[0], 2.6f)||nearPoint2D(cam.pos, coop.switches[1], 2.6f))
                    drawText("HOLD SWITCH POSITION",-0.24f,-0.35f,1.4f,0.75f,0.8f,0.55f,0.85f);
            }
            if(falseDoorTimer>0) drawText("FALSE DOOR SHIFT",0.48f,0.67f,1.0f,0.9f,0.35f,0.25f,0.7f);
            if(storyMgr.totalCollected>0)drawNoteCounter(storyMgr.totalCollected);
            drawPhaseIndicator((int)storyMgr.getPhase());
            if(nearNoteId>=0)drawInteractPrompt();
            for(auto& it:worldItems){
                if(!it.active) continue;
                if(nearPoint2D(cam.pos,it.pos,2.2f)){
                    if(it.type==0) drawText("[E] PICK BATTERY",-0.18f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.8f);
                    else if(it.type==1) drawText("[E] PICK MEDKIT",-0.16f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.8f);
                    else drawText("[E] PICK BAIT",-0.14f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.8f);
                    break;
                }
            }
            if(storyMgr.hasHallucinations())drawHallucinationEffect((50.0f-playerSanity)/50.0f);
            if(multiState==MULTI_IN_GAME)drawMultiHUD(netMgr.getPlayerCount(),netMgr.isHost);
            if(multiState==MULTI_IN_GAME){
                char netBuf[96];
                snprintf(netBuf,96,"RTT %.0fms TX %d RX %d",netMgr.rttMs,netMgr.packetsSent,netMgr.packetsRecv);
                drawText(netBuf,0.45f,0.60f,1.0f,0.55f,0.65f,0.8f,0.7f);
            }
        }
    }
}

void updateMultiplayer(){
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
    
    // Handle reshuffle from host
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
    
    // Only host controls entities
    if(!netMgr.isHost){
        entitySpawnTimer = 999;  // Prevent clients from spawning
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

int main(){
    std::random_device rd;rng.seed(rd());
    if(!glfwInit())return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    gWin=glfwCreateWindow(winW,winH,"Backrooms - Level 0",NULL,NULL);
    if(!gWin){glfwTerminate();return -1;}
    glfwMakeContextCurrent(gWin);
    glfwSetCursorPosCallback(gWin,mouse);
    glfwSetFramebufferSizeCallback(gWin,windowResize);
    if(!gladLoadGL())return -1;
    
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
    genWorld();
    wallTex=genTex(0);floorTex=genTex(1);ceilTex=genTex(2);lightTex=genTex(3);
    mainShader=mkShader(mainVS,mainFS);lightShader=mkShader(lightVS,lightFS);vhsShader=mkShader(vhsVS,vhsFS);
    buildGeom();initFBO(fbo,fboTex,rbo,winW,winH);initText();entityMgr.init();
    initPlayerModels(); playerModelsInit = true;
    std::thread aT(audioThread);
    
    while(!glfwWindowShouldClose(gWin)){
        float now=(float)glfwGetTime();dTime=now-lastFrame;lastFrame=now;vhsTime=now;
        sndState.masterVol=settings.masterVol;sndState.dangerLevel=entityMgr.dangerLevel;
        sndState.musicVol=settings.musicVol;
        sndState.ambienceVol=settings.ambienceVol;
        sndState.sfxVol=settings.sfxVol;
        sndState.voiceVol=settings.voiceVol;
        sndState.sanityLevel=playerSanity/100.0f;currentWinW=winW;currentWinH=winH;
        
        if(gameState==STATE_INTRO){
            bool spNow=glfwGetKey(gWin,GLFW_KEY_SPACE)==GLFW_PRESS;
            if(spNow&&!spacePressed){storyMgr.introComplete=true;gameState=STATE_GAME;}
            spacePressed=spNow;
            storyMgr.update(dTime,survivalTime,playerSanity,rng);
            if(storyMgr.introComplete){
                gameState=STATE_GAME;
                glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
                firstMouse=true;
            }
        }else if(gameState==STATE_NOTE){
            static bool wasKeyDown=true;
            bool anyKey=glfwGetKey(gWin,GLFW_KEY_SPACE)==GLFW_PRESS||
                        glfwGetKey(gWin,GLFW_KEY_E)==GLFW_PRESS||
                        glfwGetKey(gWin,GLFW_KEY_ESCAPE)==GLFW_PRESS;
            if(!anyKey)wasKeyDown=false;
            if(anyKey&&!wasKeyDown&&storyMgr.readingNote){
                storyMgr.closeNote();gameState=STATE_GAME;wasKeyDown=true;
            }
        }else if(gameState==STATE_SETTINGS) settingsInput(gWin,false);
        else if(gameState==STATE_SETTINGS_PAUSE) settingsInput(gWin,true);
        else if(gameState==STATE_MENU||gameState==STATE_PAUSE||
                 gameState==STATE_MULTI||gameState==STATE_MULTI_HOST||gameState==STATE_MULTI_JOIN||gameState==STATE_MULTI_WAIT){
            menuInput(gWin);
            if(gameState==STATE_MULTI_WAIT && reconnectInProgress){
                reconnectAttemptTimer -= dTime;
                if(reconnectAttemptTimer<=0){
                    reconnectAttempts++;
                    reconnectAttemptTimer = nextReconnectDelaySeconds(reconnectAttempts);
                    if(shouldContinueReconnect(reconnectAttempts, 12) && lastSession.valid){
                        netMgr.shutdown();
                        netMgr.init();
                        netMgr.joinGame(lastSession.hostIP);
                    }else{
                        reconnectInProgress = false;
                        restoreAfterReconnect = false;
                        multiState = MULTI_NONE;
                        gameState = STATE_MULTI;
                        menuSel = 1;
                    }
                }
            }
            if(gameState==STATE_PAUSE&&enterPressed&&menuSel==2){
                gameState=STATE_MENU;menuSel=0;genWorld();buildGeom();
            }
            if(gameState==STATE_MULTI_WAIT&&netMgr.gameStarted){
                multiState=MULTI_IN_GAME;
                genWorld();buildGeom();
                if(restoreAfterReconnect){
                    restoreSessionSnapshot();
                    reconnectInProgress = false;
                    restoreAfterReconnect = false;
                    gameState=STATE_GAME;
                    glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
                    firstMouse=true;
                }else{
                    gameState=STATE_INTRO;
                }
            }
        }else if(gameState==STATE_GAME){
            if(isPlayerDead){
                bool eN=glfwGetKey(gWin,GLFW_KEY_ENTER)==GLFW_PRESS||
                        glfwGetKey(gWin,GLFW_KEY_SPACE)==GLFW_PRESS;
                bool esN=glfwGetKey(gWin,GLFW_KEY_ESCAPE)==GLFW_PRESS;
                if(eN&&!enterPressed){genWorld();buildGeom();gameState=STATE_INTRO;}
                if(esN&&!escPressed){
                    gameState=STATE_MENU;menuSel=0;
                    glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                }
                enterPressed=eN;escPressed=esN;
            }else{
                if(lightsOutTimer>0){
                    lightsOutTimer-=dTime;
                    if(lightsOutTimer<=0){
                        lightsOutTimer=0;
                        updateLightsAndPillars(playerChunkX,playerChunkZ);
                        buildGeom();
                    }
                }
                if(falseDoorTimer>0) falseDoorTimer-=dTime;
                if(baitEffectTimer>0) baitEffectTimer-=dTime;
                gameInput(gWin);
                updateVisibleChunks(cam.pos.x,cam.pos.z);
                if(playerChunkX!=lastBuildChunkX||playerChunkZ!=lastBuildChunkZ){
                    updateLightsAndPillars(playerChunkX,playerChunkZ);buildGeom();
                }
                storyMgr.update(dTime,survivalTime,playerSanity,rng);
                
                for(auto&n:storyMgr.notes)if(n.active&&!n.collected)n.bobPhase+=dTime*3.0f;
                buildNotes(vhsTime);
                
                nearNoteId=-1;
                for(auto&n:storyMgr.notes){
                    if(!n.active||n.collected)continue;
                    Vec3 d=n.pos-cam.pos;d.y=0;
                    if(sqrtf(d.x*d.x+d.z*d.z)<4.0f){nearNoteId=n.id;break;}
                }
                
                cleanupFarNotes();
                noteSpawnTimer-=dTime;
                if(noteSpawnTimer<=0&&lastSpawnedNote<11){
                    int nn=lastSpawnedNote+1;trySpawnNote(nn);
                    noteSpawnTimer=20.0f+(rng()%20);
                }
                
                // Only host spawns entities in multiplayer
                bool canSpawnEnt = (multiState!=MULTI_IN_GAME || netMgr.isHost);
                entitySpawnTimer-=dTime;
                int maxEnt=1+(int)(survivalTime/60.0f);if(maxEnt>6)maxEnt=6;
                float spawnDelay=40.0f-survivalTime*0.05f;if(spawnDelay<10.0f)spawnDelay=10.0f;
                if(canSpawnEnt && entitySpawnTimer<=0&&(int)entityMgr.entities.size()<maxEnt){
                    EntityType type=ENTITY_STALKER;
                    if(survivalTime>120&&rng()%100<40)type=ENTITY_CRAWLER;
                    if(survivalTime>240&&rng()%100<30)type=ENTITY_SHADOW;
                    Vec3 spawnP = findSpawnPos(cam.pos,25.0f);
                    entityMgr.spawnEntity(type,spawnP,nullptr,0,0);
                    entitySpawnTimer=spawnDelay+(rng()%15);
                }
                entityMgr.update(dTime,cam.pos,cam.yaw,nullptr,0,0,CS);
                if(baitEffectTimer>0){
                    entityMgr.dangerLevel *= 0.45f;
                }
                
                // Only host does reshuffles in multiplayer
                reshuffleTimer-=dTime;
                float reshuffleChance=30.0f+survivalTime*0.1f;
                if(reshuffleChance>80.0f)reshuffleChance=80.0f;
                float reshuffleDelay=25.0f-survivalTime*0.03f;
                if(reshuffleDelay<5.0f)reshuffleDelay=5.0f;
                bool canReshuffle = (multiState!=MULTI_IN_GAME || netMgr.isHost);
                bool frontReshuffle=survivalTime>300&&rng()%100<20;
                if(canReshuffle && reshuffleTimer<=0&&rng()%100<(int)reshuffleChance){
                    if(frontReshuffle||reshuffleBehind(cam.pos.x,cam.pos.z,cam.yaw)){
                        buildGeom();
                        // Sync reshuffle to clients
                        if(multiState==MULTI_IN_GAME && netMgr.isHost){
                            netMgr.sendReshuffle(playerChunkX, playerChunkZ, worldSeed);
                        }
                    }
                    reshuffleTimer=reshuffleDelay+(rng()%10);
                }else if(reshuffleTimer<=0)reshuffleTimer=8.0f+(rng()%8);
                
                if(entityMgr.dangerLevel>0.1f)playerSanity-=entityMgr.dangerLevel*8.0f*dTime;
                else playerSanity+=2.0f*dTime;
                playerSanity=playerSanity>100?100:(playerSanity<0?0:playerSanity);
                if(playerSanity<=0&&rng()%1000<5){
                    isPlayerDead=true;playerHealth=0;
                    glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                }
                if(entityMgr.checkPlayerAttack(cam.pos)){
                    playerHealth-=35.0f*dTime;playerSanity-=15.0f*dTime;
                    camShake=0.15f;damageFlash=0.4f;
                    if(multiState==MULTI_IN_GAME){
                        netMgr.sendScare(netMgr.myId);
                        if(netMgr.isHost) triggerScare();
                    }else{
                        triggerScare();
                    }
                    if(playerHealth<=0){
                        isPlayerDead=true;playerHealth=0;
                        glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                    }
                }
                camShake*=0.9f;damageFlash*=0.92f;survivalTime+=dTime;
                if(multiState==MULTI_IN_GAME && !netMgr.isHost) captureSessionSnapshot();
                
                // Multiplayer update
                updateMultiplayer();
            }
        }
        
        // Rendering
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glViewport(0,0,winW,winH);
        glClearColor(0.02f,0.02f,0.02f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        
        if(gameState==STATE_GAME||gameState==STATE_PAUSE||gameState==STATE_SETTINGS_PAUSE||gameState==STATE_NOTE)
            renderScene();
        
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glViewport(0,0,winW,winH);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        
        glUseProgram(vhsShader);
        glBindTexture(GL_TEXTURE_2D,fboTex);
        float sP=(100-playerSanity)/100*0.4f;
        float vI=settings.vhsIntensity+entityMgr.dangerLevel*0.5f+sP;
        glUniform1f(glGetUniformLocation(vhsShader,"tm"),vhsTime);
        glUniform1f(glGetUniformLocation(vhsShader,"inten"),vI);
        glBindVertexArray(quadVAO);glDrawArrays(GL_TRIANGLES,0,6);
        glEnable(GL_DEPTH_TEST);
        
        drawUI();
        
        glfwSwapBuffers(gWin);glfwPollEvents();
    }
    audioRunning=false;SetEvent(hEvent);aT.join();glfwTerminate();return 0;
}
