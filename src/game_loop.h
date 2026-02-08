#pragma once

#include "coop.h"

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
                    const float uvTile = 2.2f;
                    float fl[]={px,0,pz,0,0,0,1,0,px,0,pz+CS,0,uvTile,0,1,0,px+CS,0,pz+CS,uvTile,uvTile,0,1,0,
                               px,0,pz,0,0,0,1,0,px+CS,0,pz+CS,uvTile,uvTile,0,1,0,px+CS,0,pz,uvTile,0,0,1,0};
                    for(int i=0;i<48;i++)fv.push_back(fl[i]);
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
    
    chunks.clear();lights.clear();pillars.clear();
    g_lightStates.clear(); // Reset light temporal states on world gen
    updateVisibleChunks(0,0);
    updateLightsAndPillars(0,0);
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
    entityMgr.reset();storyMgr.init();
    initTrapCorridor(sp);
    resetPlayerInterpolation();
    initCoopObjectives(coopBase);
    worldItems.clear();
    nextWorldItemId = 1;
    invBattery = invMedkit = invBait = 0;
    clearEchoSignal();
    echoSpawnTimer = 12.0f + (float)(rng()%8);
    echoStatusTimer = 0.0f;
    echoStatusText[0] = '\0';
    trapStatusTimer = 0.0f;
    trapStatusText[0] = '\0';
    anomalyBlur = 0.0f;
    lightsOutTimer = falseDoorTimer = 0.0f;
    baitEffectTimer = 0.0f;
    itemSpawnTimer = 8.0f;
    playerHealth=playerSanity=playerStamina=100;
    flashlightBattery=100;flashlightOn=false;isPlayerDead=false;
    flashlightShutdownBlinkActive = false;
    flashlightShutdownBlinkTimer = 0.0f;
    resetScareSystemState(scareState);
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
    updateMinimapCheat(w);
    
    bool fNow=glfwGetKey(w,GLFW_KEY_F)==GLFW_PRESS;
    if(fNow&&!flashlightPressed&&flashlightBattery>5.0f){
        flashlightOn=!flashlightOn;
        if(!flashlightOn){
            flashlightShutdownBlinkActive = false;
            flashlightShutdownBlinkTimer = 0.0f;
        }
        sndState.flashlightOn=flashlightOn?1.0f:0.0f;
    }
    flashlightPressed=fNow;
    
    bool eNow=glfwGetKey(w,GLFW_KEY_E)==GLFW_PRESS;
    nearbyWorldItemId = -1;
    nearbyWorldItemType = -1;
    for(auto& it:worldItems){
        if(!it.active) continue;
        if(nearPoint2D(cam.pos, it.pos, 2.2f)){
            nearbyWorldItemId = it.id;
            nearbyWorldItemType = it.type;
            break;
        }
    }
    bool nearEchoSignal = echoSignal.active && isEchoInRange(cam.pos, echoSignal.pos, 2.5f);
    if(eNow&&!interactPressed&&nearNoteId>=0){
        if(storyMgr.checkNotePickup(cam.pos,4.0f)){
            if(tryTriggerStoryScare(scareState, storyMgr.currentNote)){
                triggerLocalScare(0.34f, 0.18f, 5.0f);
            }
            gameState=STATE_NOTE;
            playerSanity-=8.0f;
            if(playerSanity<0)playerSanity=0;
            if(multiState==MULTI_IN_GAME) netMgr.sendNoteCollect(nearNoteId);
        }
    }else if(eNow&&!interactPressed&&nearbyWorldItemId>=0){
        if(multiState==MULTI_IN_GAME){
            if(netMgr.isHost){
                for(auto& it:worldItems){
                    if(!it.active||it.id!=nearbyWorldItemId) continue;
                    it.active=false;
                    if(it.type==0) invBattery++;
                    else if(it.type==1) invMedkit++;
                    else invBait++;
                    break;
                }
            }else{
                netMgr.sendInteractRequest(REQ_PICK_ITEM, nearbyWorldItemId);
            }
        }else{
            for(auto& it:worldItems){
                if(!it.active||it.id!=nearbyWorldItemId) continue;
                it.active=false;
                if(it.type==0) invBattery++;
                else if(it.type==1) invMedkit++;
                else invBait++;
                break;
            }
        }
    }else if(eNow&&!interactPressed&&nearEchoSignal){
        resolveEchoInteraction();
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
        if(!flashlightShutdownBlinkActive && shouldStartFlashlightShutdownBlink(flashlightBattery)){
            flashlightShutdownBlinkActive = true;
            flashlightShutdownBlinkTimer = 0.0f;
        }

        if(flashlightShutdownBlinkActive){
            flashlightShutdownBlinkTimer += dTime;
            sndState.flashlightOn = isFlashlightOnDuringShutdownBlink(flashlightShutdownBlinkTimer) ? 1.0f : 0.0f;
            flashlightBattery -= dTime * 1.67f;
            if(flashlightBattery < 0.0f) flashlightBattery = 0.0f;
            if(isFlashlightShutdownBlinkFinished(flashlightShutdownBlinkTimer)){
                flashlightBattery = 0.0f;
                flashlightOn = false;
                flashlightShutdownBlinkActive = false;
                flashlightShutdownBlinkTimer = 0.0f;
                sndState.flashlightOn = 0.0f;
            }
        }else{
            sndState.flashlightOn = 1.0f;
            flashlightBattery-=dTime*1.67f;
            if(flashlightBattery<=0){flashlightBattery=0;flashlightOn=false;sndState.flashlightOn=0;}
        }
    }else{
        flashlightShutdownBlinkActive = false;
        flashlightShutdownBlinkTimer = 0.0f;
        flashlightBattery+=dTime*10.0f;
        if(flashlightBattery>100)flashlightBattery=100;
    }
}

void renderScene(){
    struct MainUniforms {
        GLint P, V, M, vp, tm, danger, flashOn, flashDir, rfc, rfp, rfd, nl, lp;
    };
    struct LightUniforms {
        GLint P, V, M, inten, tm, fade;
    };
    static MainUniforms mu = {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    static LightUniforms lu = {-1,-1,-1,-1,-1,-1};
    if(mu.P < 0){
        mu.P = glGetUniformLocation(mainShader,"P");
        mu.V = glGetUniformLocation(mainShader,"V");
        mu.M = glGetUniformLocation(mainShader,"M");
        mu.vp = glGetUniformLocation(mainShader,"vp");
        mu.tm = glGetUniformLocation(mainShader,"tm");
        mu.danger = glGetUniformLocation(mainShader,"danger");
        mu.flashOn = glGetUniformLocation(mainShader,"flashOn");
        mu.flashDir = glGetUniformLocation(mainShader,"flashDir");
        mu.rfc = glGetUniformLocation(mainShader,"rfc");
        mu.rfp = glGetUniformLocation(mainShader,"rfp");
        mu.rfd = glGetUniformLocation(mainShader,"rfd");
        mu.nl = glGetUniformLocation(mainShader,"nl");
        mu.lp = glGetUniformLocation(mainShader,"lp");
    }
    if(lu.P < 0){
        lu.P = glGetUniformLocation(lightShader,"P");
        lu.V = glGetUniformLocation(lightShader,"V");
        lu.M = glGetUniformLocation(lightShader,"M");
        lu.inten = glGetUniformLocation(lightShader,"inten");
        lu.tm = glGetUniformLocation(lightShader,"tm");
        lu.fade = glGetUniformLocation(lightShader,"fade");
    }

    glUseProgram(mainShader);
    Mat4 proj=Mat4::persp(1.2f,(float)winW/winH,0.1f,100.0f);
    float shX=camShake*(rand()%100-50)/500.0f,shY=camShake*(rand()%100-50)/500.0f;
    Vec3 la=cam.pos+Vec3(sinf(cam.yaw+shX)*cosf(cam.pitch+shY),sinf(cam.pitch+shY),
                         cosf(cam.yaw+shX)*cosf(cam.pitch+shY));
    Mat4 view=Mat4::look(cam.pos,la,Vec3(0,1,0)),model;
    
    glUniformMatrix4fv(mu.P,1,GL_FALSE,proj.m);
    glUniformMatrix4fv(mu.V,1,GL_FALSE,view.m);
    glUniformMatrix4fv(mu.M,1,GL_FALSE,model.m);
    glUniform3f(mu.vp,cam.pos.x,cam.pos.y,cam.pos.z);
    glUniform1f(mu.tm,vhsTime);
    glUniform1f(mu.danger,entityMgr.dangerLevel);
    bool flashVisualOn = flashlightOn;
    if(flashlightOn && flashlightShutdownBlinkActive){
        flashVisualOn = isFlashlightOnDuringShutdownBlink(flashlightShutdownBlinkTimer);
    }
    glUniform1i(mu.flashOn,flashVisualOn?1:0);
    glUniform3f(mu.flashDir,sinf(cam.yaw)*cosf(cam.pitch),
                sinf(cam.pitch),cosf(cam.yaw)*cosf(cam.pitch));
    float remoteFlashPos[12] = {0};
    float remoteFlashDir[12] = {0};
    int remoteFlashCount = 0;
    if(multiState==MULTI_IN_GAME){
        remoteFlashCount = gatherRemoteFlashlights(netMgr.myId, remoteFlashPos, remoteFlashDir);
    }
    glUniform1i(mu.rfc,remoteFlashCount);
    if(remoteFlashCount>0){
        glUniform3fv(mu.rfp,remoteFlashCount,remoteFlashPos);
        glUniform3fv(mu.rfd,remoteFlashCount,remoteFlashDir);
    }
    
    // Gather nearest lights - fade is now computed in shader based on distance
    float lpos[SCENE_LIGHT_LIMIT * 3] = {0};
    int nl = gatherNearestSceneLights(lights, cam.pos, lpos);
    glUniform1i(mu.nl,nl);
    if(nl>0) glUniform3fv(mu.lp,nl,lpos);
    
    glBindTexture(GL_TEXTURE_2D,wallTex);glBindVertexArray(wallVAO);glDrawArrays(GL_TRIANGLES,0,wallVC);
    glBindVertexArray(pillarVAO);glDrawArrays(GL_TRIANGLES,0,pillarVC);
    glBindTexture(GL_TEXTURE_2D,floorTex);glBindVertexArray(floorVAO);glDrawArrays(GL_TRIANGLES,0,floorVC);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D,ceilTex);glBindVertexArray(ceilVAO);glDrawArrays(GL_TRIANGLES,0,ceilVC);
    if(noteVC>0){glBindTexture(GL_TEXTURE_2D,lightTex);glBindVertexArray(noteVAO);glDrawArrays(GL_TRIANGLES,0,noteVC);}
    glEnable(GL_CULL_FACE);
    
    if(multiState==MULTI_IN_GAME && playerModelsInit){
        renderPlayers(mainShader, proj, view, netMgr.myId);
    }
    
    glUseProgram(lightShader);
    glUniformMatrix4fv(lu.P,1,GL_FALSE,proj.m);
    glUniformMatrix4fv(lu.V,1,GL_FALSE,view.m);
    glUniformMatrix4fv(lu.M,1,GL_FALSE,model.m);
    glUniform1f(lu.inten,1.2f);
    glUniform1f(lu.tm,vhsTime);
    glUniform1f(lu.fade,1.0f); // Light sprites always full brightness when visible
    glBindTexture(GL_TEXTURE_2D,lightTex);glBindVertexArray(lightVAO);glDrawArrays(GL_TRIANGLES,0,lightVC);
    if(lightOffVC>0){
        glUniform1f(lu.inten,0.15f);
        glUniform1f(lu.fade,1.0f);
        glBindVertexArray(lightOffVAO);glDrawArrays(GL_TRIANGLES,0,lightOffVC);
    }
    entityMgr.render(mainShader,proj,view);
}

#include "hud.h"

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
    buildGeom();
    computeRenderTargetSize(winW, winH, SCENE_RENDER_SCALE, renderW, renderH);
    initFBO(fbo,fboTex,rbo,renderW,renderH);
    initText();
    entityMgr.init();
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
                updateEchoSignal();
                updateTrapCorridor();
                gameInput(gWin);
                int targetChunkX = (int)floorf(cam.pos.x / (CS * CHUNK_SIZE));
                int targetChunkZ = (int)floorf(cam.pos.z / (CS * CHUNK_SIZE));
                if(targetChunkX!=playerChunkX || targetChunkZ!=playerChunkZ){
                    updateVisibleChunks(cam.pos.x,cam.pos.z);
                }
                if(playerChunkX!=lastBuildChunkX||playerChunkZ!=lastBuildChunkZ){
                    updateLightsAndPillars(playerChunkX,playerChunkZ);buildGeom();
                }
                storyMgr.update(dTime,survivalTime,playerSanity,rng);
                if(tryTriggerRandomScare(scareState, dTime, storyMgr.getPhase(), playerSanity, (int)(rng()%100))){
                    triggerLocalScare(0.26f, 0.14f, 3.0f);
                }
                
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
                
                bool canSpawnEnt = (multiState!=MULTI_IN_GAME || netMgr.isHost);
                entitySpawnTimer-=dTime;
                int maxEnt = computeEntityCap(survivalTime);
                if(canSpawnEnt && entitySpawnTimer<=0&&(int)entityMgr.entities.size()<maxEnt){
                    EntityType type = chooseSpawnEntityType(survivalTime, (int)rng(), (int)rng());
                    Vec3 spawnP = findSpawnPos(cam.pos,25.0f);
                    if(!hasEntityNearPos(entityMgr.entities, spawnP, 14.0f)){
                        entityMgr.spawnEntity(type,spawnP,nullptr,0,0);
                    }
                    entitySpawnTimer = computeEntitySpawnDelay(survivalTime, (int)rng());
                }
                entityMgr.update(dTime,cam.pos,cam.yaw,nullptr,0,0,CS);
                if(baitEffectTimer>0){
                    entityMgr.dangerLevel *= 0.45f;
                }
                
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
                
                updateMultiplayer();
            }
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glViewport(0,0,renderW,renderH);
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
        float vI=settings.vhsIntensity+entityMgr.dangerLevel*0.5f+sP + anomalyBlur * 0.55f;
        static GLint vhsTmLoc = -1;
        static GLint vhsIntenLoc = -1;
        if(vhsTmLoc<0){
            vhsTmLoc = glGetUniformLocation(vhsShader,"tm");
            vhsIntenLoc = glGetUniformLocation(vhsShader,"inten");
        }
        glUniform1f(vhsTmLoc,vhsTime);
        glUniform1f(vhsIntenLoc,vI);
        glBindVertexArray(quadVAO);glDrawArrays(GL_TRIANGLES,0,6);
        glEnable(GL_DEPTH_TEST);
        
        drawUI();
        
        glfwSwapBuffers(gWin);glfwPollEvents();
    }
    audioRunning=false;SetEvent(hEvent);aT.join();glfwTerminate();return 0;
}
