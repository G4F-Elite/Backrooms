#pragma once

#include "coop.h"

void buildGeom(){
    std::vector<float>wv,fv,cv,pv,lv,lvOff;
    int pcx=playerChunkX,pcz=playerChunkZ;
    
    for(int dcx=-VIEW_CHUNKS; dcx<=VIEW_CHUNKS; dcx++){
        for(int dcz=-VIEW_CHUNKS; dcz<=VIEW_CHUNKS; dcz++){
            auto it=chunks.find(chunkKey(pcx+dcx,pcz+dcz));
            if(it==chunks.end()) continue;
            
            for(int lx=0; lx<CHUNK_SIZE; lx++){
                for(int lz=0; lz<CHUNK_SIZE; lz++){
                    int wx=(pcx+dcx)*CHUNK_SIZE+lx;
                    int wz=(pcz+dcz)*CHUNK_SIZE+lz;
                    if(it->second.cells[lx][lz]!=0) continue;
                    
                    float px=wx*CS, pz=wz*CS;
                    const float uvTile = 2.2f;
                    
                    float fl[]={
                        px,0,pz,0,0,0,1,0,
                        px,0,pz+CS,0,uvTile,0,1,0,
                        px+CS,0,pz+CS,uvTile,uvTile,0,1,0,
                        px,0,pz,0,0,0,1,0,
                        px+CS,0,pz+CS,uvTile,uvTile,0,1,0,
                        px+CS,0,pz,uvTile,0,0,1,0
                    };
                    for(int i=0;i<48;i++) fv.push_back(fl[i]);
                    
                    float cl[]={
                        px,WH,pz,0,0,0,-1,0,
                        px,WH,pz+CS,0,uvTile,0,-1,0,
                        px+CS,WH,pz+CS,uvTile,uvTile,0,-1,0,
                        px,WH,pz,0,0,0,-1,0,
                        px+CS,WH,pz+CS,uvTile,uvTile,0,-1,0,
                        px+CS,WH,pz,uvTile,0,0,-1,0
                    };
                    for(int i=0;i<48;i++) cv.push_back(cl[i]);
                    
                    if(getCellWorld(wx-1,wz)==1) mkWall(wv,px,pz,0,CS,WH,CS,WH);
                    if(getCellWorld(wx+1,wz)==1) mkWall(wv,px+CS,pz+CS,0,-CS,WH,CS,WH);
                    if(getCellWorld(wx,wz-1)==1) mkWall(wv,px+CS,pz,-CS,0,WH,CS,WH);
                    if(getCellWorld(wx,wz+1)==1) mkWall(wv,px,pz+CS,CS,0,WH,CS,WH);
                }
            }
        }
    }
    
    for(auto&p:pillars) mkPillar(pv,p.x,p.z,0.6f,WH);
    
    for(auto&l:lights){
        if(l.on) mkLight(lv,l.pos,l.sizeX,l.sizeZ);
        else mkLight(lvOff,l.pos,l.sizeX,l.sizeZ);
    }
    
    wallVC=(int)wv.size()/8;
    floorVC=(int)fv.size()/8;
    ceilVC=(int)cv.size()/8;
    pillarVC=(int)pv.size()/8;
    lightVC=(int)lv.size()/5;
    lightOffVC=(int)lvOff.size()/5;
    
    setupVAO(wallVAO,wallVBO,wv,true);
    setupVAO(floorVAO,floorVBO,fv,true);
    setupVAO(ceilVAO,ceilVBO,cv,true);
    setupVAO(pillarVAO,pillarVBO,pv,true);
    setupVAO(lightVAO,lightVBO,lv,false);
    
    if(!lvOff.empty()) setupVAO(lightOffVAO,lightOffVBO,lvOff,false);
    
    initQuad(quadVAO,quadVBO);
    lastBuildChunkX=pcx;
    lastBuildChunkZ=pcz;
}

void buildNotes(float tm){
    std::vector<float>nv;
    for(auto&n:storyMgr.notes){
        if(!n.active||n.collected) continue;
        mkNoteGlow(nv,n.pos,n.bobPhase);
    }
    noteVC=(int)nv.size()/8;
    if(noteVC>0) setupVAO(noteVAO,noteVBO,nv,true);
}

void trySpawnNote(int noteId){
    if(noteId>=12||storyMgr.notesCollected[noteId]) return;
    if(multiState==MULTI_IN_GAME && !netMgr.isHost) return;
    
    Vec3 sp=findSpawnPos(cam.pos,12.0f);
    Vec3 d=sp-cam.pos;
    d.y=0;
    
    if(sqrtf(d.x*d.x+d.z*d.z)>8.0f){
        storyMgr.spawnNote(sp,noteId);
        lastSpawnedNote=noteId;
        if(multiState==MULTI_IN_GAME) netMgr.sendNoteSpawn(noteId, sp);
    }
}

void cleanupFarNotes(){
    for(auto&n:storyMgr.notes){
        if(!n.active||n.collected) continue;
        Vec3 d=n.pos-cam.pos;
        d.y=0;
        if(sqrtf(d.x*d.x+d.z*d.z)>80.0f){
            n.active=false;
            if(n.id==lastSpawnedNote) lastSpawnedNote=n.id-1;
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
    
    chunks.clear();
    lights.clear();
    pillars.clear();
    g_lightStates.clear();
    
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
    
    cam.pos=Vec3(sp.x,PH,sp.z);
    cam.yaw=cam.pitch=0;
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    
    entityMgr.reset();
    storyMgr.init();
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
    flashlightBattery=100;
    flashlightOn=false;
    isPlayerDead=false;
    flashlightShutdownBlinkActive = false;
    flashlightShutdownBlinkTimer = 0.0f;
    resetScareSystemState(scareState);
    entitySpawnTimer=30;
    survivalTime=0;
    reshuffleTimer=15;
    lastSpawnedNote=-1;
    noteSpawnTimer=15.0f;
}

void teleportToPlayer(){
    if(multiState!=MULTI_IN_GAME) return;
    if(!netMgr.hasOtherPlayersWithPos()) return;
    
    Vec3 tp=netMgr.getOtherPlayerPos();
    cam.pos=Vec3(tp.x+1.0f,PH,tp.z);
    updateVisibleChunks(cam.pos.x,cam.pos.z);
    updateLightsAndPillars(playerChunkX,playerChunkZ);
    buildGeom();
}

void mouse(GLFWwindow*,double xp,double yp){
    if(gameState!=STATE_GAME&&gameState!=STATE_INTRO) return;
    
    if(firstMouse){
        lastX=(float)xp;
        lastY=(float)yp;
        firstMouse=false;
    }
    
    cam.yaw-=((float)xp-lastX)*settings.mouseSens;
    cam.pitch+=(lastY-(float)yp)*settings.mouseSens;
    
    if(cam.pitch>1.4f) cam.pitch=1.4f;
    if(cam.pitch<-1.4f) cam.pitch=-1.4f;
    
    lastX=(float)xp;
    lastY=(float)yp;
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
    
    float shX=camShake*(rand()%100-50)/500.0f;
    float shY=camShake*(rand()%100-50)/500.0f;
    
    Vec3 la=cam.pos+Vec3(
        sinf(cam.yaw+shX)*cosf(cam.pitch+shY),
        sinf(cam.pitch+shY),
        cosf(cam.yaw+shX)*cosf(cam.pitch+shY)
    );
    
    Mat4 view=Mat4::look(cam.pos,la,Vec3(0,1,0));
    Mat4 model;
    
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
    glUniform3f(mu.flashDir,
        sinf(cam.yaw)*cosf(cam.pitch),
        sinf(cam.pitch),
        cosf(cam.yaw)*cosf(cam.pitch)
    );
    
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
    
    float lpos[SCENE_LIGHT_LIMIT * 3] = {0};
    int nl = gatherNearestSceneLights(lights, cam.pos, lpos);
    glUniform1i(mu.nl,nl);
    if(nl>0) glUniform3fv(mu.lp,nl,lpos);
    
    glBindTexture(GL_TEXTURE_2D,wallTex);
    glBindVertexArray(wallVAO);
    glDrawArrays(GL_TRIANGLES,0,wallVC);
    
    glBindVertexArray(pillarVAO);
    glDrawArrays(GL_TRIANGLES,0,pillarVC);
    
    glBindTexture(GL_TEXTURE_2D,floorTex);
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES,0,floorVC);
    
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D,ceilTex);
    glBindVertexArray(ceilVAO);
    glDrawArrays(GL_TRIANGLES,0,ceilVC);
    
    if(noteVC>0){
        glBindTexture(GL_TEXTURE_2D,lightTex);
        glBindVertexArray(noteVAO);
        glDrawArrays(GL_TRIANGLES,0,noteVC);
    }
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
    glUniform1f(lu.fade,1.0f);
    
    glBindTexture(GL_TEXTURE_2D,lightTex);
    glBindVertexArray(lightVAO);
    glDrawArrays(GL_TRIANGLES,0,lightVC);
    
    if(lightOffVC>0){
        glUniform1f(lu.inten,0.15f);
        glUniform1f(lu.fade,1.0f);
        glBindVertexArray(lightOffVAO);
        glDrawArrays(GL_TRIANGLES,0,lightOffVC);
    }
    
    entityMgr.render(mainShader,proj,view);
}

#include "hud.h"
#include "game_state.h"

int main(){
    std::random_device rd;
    rng.seed(rd());
    
    if(!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    
    gWin=glfwCreateWindow(winW,winH,"Backrooms - Level 0",NULL,NULL);
    if(!gWin){
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(gWin);
    glfwSetCursorPosCallback(gWin,mouse);
    glfwSetFramebufferSizeCallback(gWin,windowResize);
    
    if(!gladLoadGL()) return -1;
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    genWorld();
    wallTex=genTex(0);
    floorTex=genTex(1);
    ceilTex=genTex(2);
    lightTex=genTex(3);
    
    mainShader=mkShader(mainVS,mainFS);
    lightShader=mkShader(lightVS,lightFS);
    vhsShader=mkShader(vhsVS,vhsFS);
    
    buildGeom();
    computeRenderTargetSize(winW, winH, renderScaleFromPreset(settings.renderScalePreset), renderW, renderH);
    initFBO(fbo,fboTex,rbo,renderW,renderH);
    initText();
    entityMgr.init();
    initPlayerModels();
    playerModelsInit = true;
    
    std::thread aT(audioThread);
    
    while(!glfwWindowShouldClose(gWin)){
        float now=(float)glfwGetTime();
        dTime=now-lastFrame;
        lastFrame=now;
        vhsTime=now;
        
        int desiredRenderW = 0, desiredRenderH = 0;
        computeRenderTargetSize(winW, winH, renderScaleFromPreset(settings.renderScalePreset), 
                               desiredRenderW, desiredRenderH);
        
        if(desiredRenderW != renderW || desiredRenderH != renderH){
            renderW = desiredRenderW;
            renderH = desiredRenderH;
            if(fbo) glDeleteFramebuffers(1, &fbo);
            if(fboTex) glDeleteTextures(1, &fboTex);
            if(rbo) glDeleteRenderbuffers(1, &rbo);
            initFBO(fbo, fboTex, rbo, renderW, renderH);
        }
        
        sndState.masterVol=settings.masterVol;
        sndState.dangerLevel=entityMgr.dangerLevel;
        sndState.musicVol=settings.musicVol;
        sndState.ambienceVol=settings.ambienceVol;
        sndState.sfxVol=settings.sfxVol;
        sndState.voiceVol=settings.voiceVol;
        sndState.sanityLevel=playerSanity/100.0f;
        currentWinW=winW;
        currentWinH=winH;
        
        processGameState();
        
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glViewport(0,0,renderW,renderH);
        glClearColor(0.02f,0.02f,0.02f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        
        if(gameState==STATE_GAME||gameState==STATE_PAUSE||
           gameState==STATE_SETTINGS_PAUSE||gameState==STATE_NOTE){
            renderScene();
        }
        
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
        static GLint vhsUpscalerLoc = -1;
        static GLint vhsSharpnessLoc = -1;
        static GLint vhsTexelXLoc = -1;
        static GLint vhsTexelYLoc = -1;
        static GLint vhsInMenuLoc = -1;
        
        if(vhsTmLoc<0){
            vhsTmLoc = glGetUniformLocation(vhsShader,"tm");
            vhsIntenLoc = glGetUniformLocation(vhsShader,"inten");
            vhsUpscalerLoc = glGetUniformLocation(vhsShader,"upscaler");
            vhsSharpnessLoc = glGetUniformLocation(vhsShader,"sharpness");
            vhsTexelXLoc = glGetUniformLocation(vhsShader,"texelX");
            vhsTexelYLoc = glGetUniformLocation(vhsShader,"texelY");
            vhsInMenuLoc = glGetUniformLocation(vhsShader,"inMenu");
        }
        
        // Dust particles only in menu states
        int inMenu = (gameState == STATE_MENU || gameState == STATE_MULTI || 
                      gameState == STATE_MULTI_HOST || gameState == STATE_MULTI_JOIN ||
                      gameState == STATE_MULTI_WAIT || gameState == STATE_SETTINGS) ? 1 : 0;
        
        glUniform1f(vhsTmLoc,vhsTime);
        glUniform1f(vhsIntenLoc,vI);
        glUniform1i(vhsUpscalerLoc,clampUpscalerMode(settings.upscalerMode));
        glUniform1f(vhsSharpnessLoc,clampFsrSharpness(settings.fsrSharpness));
        glUniform1f(vhsTexelXLoc,1.0f/(float)renderW);
        glUniform1f(vhsTexelYLoc,1.0f/(float)renderH);
        glUniform1i(vhsInMenuLoc,inMenu);
        
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES,0,6);
        glEnable(GL_DEPTH_TEST);
        
        drawUI();
        
        glfwSwapBuffers(gWin);
        glfwPollEvents();
    }
    
    audioRunning=false;
    SetEvent(hEvent);
    aT.join();
    glfwTerminate();
    
    return 0;
}