#pragma once
#include "progression.h"
#include "sanity_balance.h"
#include "window_title.h"
int main(){
    std::random_device rd;rng.seed(rd());
    if(!glfwInit())return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    gWin=glfwCreateWindow(winW,winH,"Backrooms: Void Shift",NULL,NULL);
    if(!gWin){glfwTerminate();return -1;}
    glfwMakeContextCurrent(gWin);
    int appliedSwapInterval = settings.vsync ? 1 : 0;
    glfwSwapInterval(appliedSwapInterval);
    glfwSetCursorPosCallback(gWin,mouse);
    glfwSetFramebufferSizeCallback(gWin,windowResize);
    if(!gladLoadGL())return -1;
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
    genWorld();
    loadArchiveMetaProgress(archivePoints, archiveTier, perkQuietSteps, perkFastHands, perkEchoBuffer);
    wallTex=genTex(0);floorTex=genTex(1);ceilTex=genTex(2);lightTex=genTex(3);lampTex=genTex(4);propTex=genTex(5);
    deviceTex=genTex(6);
    plushTex=genTex(7);
    playerTex=propTex; // players use a stable texture so held-item textures don't bleed
    mainShader=mkShader(mainVS,mainFS);lightShader=mkShader(lightVS,lightFS);vhsShader=mkShader(vhsVS,vhsFS);
    buildGeom();
    computeRenderTargetSize(winW, winH, effectiveRenderScale(settings.upscalerMode, settings.renderScalePreset), renderW, renderH);
    initFBO(fbo,fboTex,fboDepthTex,renderW,renderH);
    initTaaTargets();
    initText();
    entityMgr.init();
    initPlayerModels(); playerModelsInit = true;
    std::thread aT(audioThread);
    int cachedRefreshRateHz = detectActiveRefreshRateHz(gWin);
    gPerfRefreshHz = cachedRefreshRateHz;
    double nextRefreshProbeTime = 0.0;
    float sanityCollapseTimer = 0.0f;
    initFrameTimeHistory(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES, 16.6f);
    gPerfFrameTimeHead = PERF_GRAPH_SAMPLES - 1;
    while(!glfwWindowShouldClose(gWin)){
        double frameStartTime = glfwGetTime();
        float now=(float)frameStartTime;
        float rawDt = now - lastFrame;
        if(lastFrame <= 0.0f || rawDt <= 0.0f || rawDt > 0.5f) rawDt = 0.016f;
        dTime = rawDt;
        lastFrame = now;
        vhsTime = now;
        updateWindowTitleForLevel();
        gPerfFrameMs = dTime * 1000.0f;
        pushFrameTimeSample(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES, gPerfFrameTimeHead, gPerfFrameMs);
        float fpsNow = 1.0f / dTime;
        if(gPerfFpsSmoothed < 1.0f) gPerfFpsSmoothed = fpsNow;
        else gPerfFpsSmoothed = gPerfFpsSmoothed * 0.90f + fpsNow * 0.10f;
        if(now >= (float)nextRefreshProbeTime){
            cachedRefreshRateHz = detectActiveRefreshRateHz(gWin);
            gPerfRefreshHz = cachedRefreshRateHz;
            nextRefreshProbeTime = (double)now + 1.0;
        }
        int desiredSwapInterval = settings.vsync ? 1 : 0;
        if(desiredSwapInterval != appliedSwapInterval){
            glfwSwapInterval(desiredSwapInterval);
            appliedSwapInterval = desiredSwapInterval;
        }
        int desiredRenderW = 0, desiredRenderH = 0;
        computeRenderTargetSize(winW, winH, effectiveRenderScale(settings.upscalerMode, settings.renderScalePreset), desiredRenderW, desiredRenderH);
        if(desiredRenderW != renderW || desiredRenderH != renderH){
            renderW = desiredRenderW;
            renderH = desiredRenderH;
            if(fbo) glDeleteFramebuffers(1, &fbo);
            if(fboTex) glDeleteTextures(1, &fboTex);
            if(fboDepthTex) glDeleteTextures(1, &fboDepthTex);
            initFBO(fbo, fboTex, fboDepthTex, renderW, renderH);
        }
        sndState.masterVol=settings.masterVol;sndState.dangerLevel=entityMgr.dangerLevel;
        sndState.musicVol=settings.musicVol;
        sndState.ambienceVol=settings.ambienceVol;
        sndState.sfxVol=settings.sfxVol;
        sndState.voiceVol=settings.voiceVol;
        sndState.deathMode = isPlayerDead;

        netMgr.updatePingMarkTtl(dTime);

        gPlayerNoise = fastClamp01(sndState.moveIntensity * 0.45f + sndState.sprintIntensity * 0.85f);
        if(isSmileSilenceActive()){ sndState.musicVol*=0.08f; sndState.ambienceVol*=0.02f; sndState.voiceVol*=0.12f; }
        sndState.sanityLevel=playerSanity/100.0f;currentWinW=winW;currentWinH=winH;
        g_fastMathEnabled = settings.fastMath;
        if(gameState!=STATE_GAME){
            sndState.moveIntensity *= 0.88f;
            sndState.sprintIntensity *= 0.86f;
            sndState.lowStamina *= 0.92f;
            sndState.monsterProximity *= 0.84f;
            sndState.monsterMenace *= 0.84f;
        }

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
        }else if(gameState==STATE_SETTINGS) settingsInput(gWin,false);
        else if(gameState==STATE_SETTINGS_PAUSE) settingsInput(gWin,true);
        else if(gameState==STATE_KEYBINDS) keybindsInput(gWin,false);
        else if(gameState==STATE_KEYBINDS_PAUSE) keybindsInput(gWin,true);
        else if(gameState==STATE_MENU||gameState==STATE_GUIDE||gameState==STATE_PAUSE||
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
            if(gameState==STATE_MULTI_WAIT && multiWaitBeforeStart > 0.0f) multiWaitBeforeStart -= dTime;
            if(gameState==STATE_MULTI_WAIT&&netMgr.gameStarted&&netMgr.welcomeReceived&&multiWaitBeforeStart<=0.0f){
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
            static int lastHoleCount = -1;
            if(playerEscaped){
                bool eN=glfwGetKey(gWin,GLFW_KEY_ENTER)==GLFW_PRESS||
                        glfwGetKey(gWin,GLFW_KEY_SPACE)==GLFW_PRESS;
                bool esN=glfwGetKey(gWin,GLFW_KEY_ESCAPE)==GLFW_PRESS;
                if(eN&&!enterPressed){
                    if(isLevelZero(gCurrentLevel)){ gCurrentLevel = 1; gCompletedLevels++; genWorld(); buildGeom(); gameState=STATE_INTRO; }
                    else{ gCompletedLevels++; gCurrentLevel = 0; gameState=STATE_MENU; menuSel=0; glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL); }
                }
                if(esN&&!escPressed){
                    gameState=STATE_MENU;menuSel=0;
                    glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                }
                enterPressed=eN;escPressed=esN;
            }else if(isPlayerDead){
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
                        updateMapContent(playerChunkX,playerChunkZ);
                        buildGeom();
                    }
                }
                if(falseDoorTimer>0) falseDoorTimer-=dTime;
                updateFloorHoles();
                gameInput(gWin);
                int holeCountNow = (int)floorHoles.size();
                if(holeCountNow != lastHoleCount){
                    buildGeom();
                    lastHoleCount = holeCountNow;
                }
                int targetChunkX = (int)floorf(cam.pos.x / (CS * CHUNK_SIZE));
                int targetChunkZ = (int)floorf(cam.pos.z / (CS * CHUNK_SIZE));
                if(targetChunkX!=playerChunkX || targetChunkZ!=playerChunkZ){
                    updateVisibleChunks(cam.pos.x,cam.pos.z);
                }
                if(playerChunkX!=lastBuildChunkX||playerChunkZ!=lastBuildChunkZ){
                    updateLightsAndPillars(playerChunkX,playerChunkZ);
                    updateMapContent(playerChunkX,playerChunkZ);
                    buildGeom();
                }
                storyMgr.update(dTime,survivalTime,playerSanity,rng);
                if(tryTriggerRandomScare(scareState, dTime, storyMgr.getPhase(), playerSanity, (int)(rng()%100))){
                    triggerLocalScare(0.26f, 0.14f, 3.0f);
                }
                nearNoteId=-1;
                bool canSpawnEnt = (multiState!=MULTI_IN_GAME || netMgr.isHost);
                entitySpawnTimer-=dTime;
                int maxEnt = computeEntityCap(survivalTime);
                maxEnt += levelEntityCapBonus(gCurrentLevel);
                if(maxEnt > 8) maxEnt = 8;
                if(canSpawnEnt && entitySpawnTimer<=0&&(int)entityMgr.entities.size()<maxEnt){
                    EntityType type = chooseSpawnEntityType(survivalTime, (int)rng(), (int)rng());
                    Vec3 spawnP = findSpawnPos(cam.pos,25.0f);
                    if(!hasEntityNearPos(entityMgr.entities, spawnP, 14.0f)){
                        entityMgr.spawnEntity(type,spawnP,nullptr,0,0);
                    }
                    entitySpawnTimer = computeEntitySpawnDelay(survivalTime, (int)rng()) * levelSpawnDelayScale(gCurrentLevel);
                }
                entityMgr.update(dTime,cam.pos,cam.yaw,nullptr,0,0,CS);
                float nearestMonster = 9999.0f;
                float menace = 0.0f;
                for(const auto& e:entityMgr.entities){
                    if(!e.active) continue;
                    Vec3 dd = e.pos - cam.pos;
                    dd.y = 0;
                    float dist = dd.len();
                    if(dist < nearestMonster) nearestMonster = dist;
                    float localMenace = 0.35f;
                    if(e.type==ENTITY_STALKER) localMenace = 0.45f;
                    else if(e.type==ENTITY_CRAWLER) localMenace = 0.75f;
                    else if(e.type==ENTITY_SHADOW) localMenace = 1.0f;
                    if(e.state==ENT_ATTACKING || e.state==ENT_CHASING) localMenace += 0.20f;
                    if(localMenace > menace) menace = localMenace;
                }
                float monsterProx = 0.0f;
                if(nearestMonster < 30.0f){
                    monsterProx = 1.0f - nearestMonster / 30.0f;
                    if(monsterProx < 0.0f) monsterProx = 0.0f;
                    if(monsterProx > 1.0f) monsterProx = 1.0f;
                }
                sndState.monsterProximity = monsterProx;
                sndState.monsterMenace = menace > 1.0f ? 1.0f : menace;
                
                reshuffleTimer-=dTime;
                float reshuffleChance=30.0f+survivalTime*0.1f;
                if(reshuffleChance>80.0f)reshuffleChance=80.0f;
                float reshuffleDelay=25.0f-survivalTime*0.03f;
                if(reshuffleDelay<5.0f)reshuffleDelay=5.0f;
                bool canReshuffle = (multiState!=MULTI_IN_GAME || netMgr.isHost);
                bool frontReshuffle=survivalTime>300&&rng()%100<20;
                if(canReshuffle && reshuffleTimer<=0&&rng()%100<(int)reshuffleChance){
                    if(frontReshuffle||reshuffleBehind(cam.pos.x,cam.pos.z,cam.yaw)){
                        updateMapContent(playerChunkX,playerChunkZ);
                        buildGeom();
                        if(multiState==MULTI_IN_GAME && netMgr.isHost){
                            netMgr.sendReshuffle(playerChunkX, playerChunkZ, worldSeed);
                        }
                    }
                    reshuffleTimer=reshuffleDelay+(rng()%10);
                }else if(reshuffleTimer<=0)reshuffleTimer=8.0f+(rng()%8);
                
                float levelDanger = levelDangerScale(gCurrentLevel);
                if(activeDeviceSlot==3 && heldConsumableType==ITEM_PLUSH_TOY) playerSanity += 3.8f*dTime;
                else playerSanity -= sanityPassiveDrainPerSec(levelDanger) * dTime;
                if(entityMgr.dangerLevel>0.1f) playerSanity -= entityMgr.dangerLevel*(8.0f * levelDanger)*dTime;
                playerSanity=playerSanity>100?100:(playerSanity<0?0:playerSanity);
                int cellX = (int)floorf(cam.pos.x / CS);
                int cellZ = (int)floorf(cam.pos.z / CS);
                if(!playerFalling && isFallCell(cellX, cellZ)){
                    float cellCenterX = (cellX + 0.5f) * CS;
                    float cellCenterZ = (cellZ + 0.5f) * CS;
                    float dx = cam.pos.x - cellCenterX;
                    float dz = cam.pos.z - cellCenterZ;
                    float distFromCenter = sqrtf(dx*dx + dz*dz);
                    if(distFromCenter < CS * 0.35f){
                        playerFalling = true;
                        fallVelocity = 0.0f;
                        fallTimer = 0.0f;
                        if(isAbyssCell(cellX, cellZ))
                            setTrapStatus("THE VOID SWALLOWS YOU.");
                        else
                            setTrapStatus("FLOOR COLLAPSE. YOU FELL.");
                    }
                }
                if(playerSanity<=0.0f && !isPlayerDead){
                    sanityCollapseTimer += dTime;
                    if(sanityCollapseTimer>=2.3f){
                        if(!playerDowned){
                            playerDowned = true;
                            playerDownedTimer = 60.0f;
                            snprintf(gDeathReason,sizeof(gDeathReason),"DOWNED: COGNITIVE OVERLOAD");
                            setEchoStatus("DOWNED: USE PLUSH TO STABILIZE");
                        }else{
                            isPlayerDead=true;playerHealth=0;snprintf(gDeathReason,sizeof(gDeathReason),"CAUSE: COGNITIVE OVERLOAD"); glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                        }
                    }
                }else if(!isPlayerDead){
                    sanityCollapseTimer -= dTime * 1.7f;
                    if(sanityCollapseTimer < 0.0f) sanityCollapseTimer = 0.0f;
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
                        if(!playerDowned){
                            playerDowned = true;
                            playerDownedTimer = 60.0f;
                            playerHealth = 15.0f;
                            snprintf(gDeathReason,sizeof(gDeathReason),"DOWNED: HOSTILE CONTACT");
                            setEchoStatus("DOWNED: SELF-REVIVE WITH PLUSH");
                        }else{
                            isPlayerDead=true;playerHealth=0;snprintf(gDeathReason,sizeof(gDeathReason),"CAUSE: HOSTILE CONTACT");
                            glfwSetInputMode(gWin,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
                        }
                    }
                }
                if(playerDowned && !isPlayerDead){
                    playerDownedTimer -= dTime;
                    playerStamina -= dTime * 14.0f;
                    if(playerStamina < 0.0f) playerStamina = 0.0f;
                    if(playerDownedTimer <= 0.0f){
                        playerDownedTimer = 0.0f;
                        isPlayerDead = true;
                        playerHealth = 0.0f;
                        snprintf(gDeathReason,sizeof(gDeathReason),"CAUSE: BLEED OUT");
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
        if((gameState==STATE_GAME&&!isPlayerDead)||gameState==STATE_PAUSE||gameState==STATE_SETTINGS_PAUSE||gameState==STATE_KEYBINDS_PAUSE||gameState==STATE_NOTE)
            renderScene();
        
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glViewport(0,0,winW,winH);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        glUseProgram(vhsShader);
        bool vhsMenu=(gameState==STATE_MENU||gameState==STATE_GUIDE||gameState==STATE_MULTI||gameState==STATE_MULTI_HOST||gameState==STATE_MULTI_JOIN||gameState==STATE_MULTI_WAIT),vhsGameplay=(gameState==STATE_GAME||gameState==STATE_PAUSE||gameState==STATE_SETTINGS_PAUSE||gameState==STATE_KEYBINDS_PAUSE||gameState==STATE_INTRO); float vI=0.0f;
        if(vhsMenu) vI=0.0f; else if(vhsGameplay){
            float sanityLoss=1.0f-(playerSanity/100.0f);
            if(sanityLoss<0.0f) sanityLoss=0.0f;
            if(sanityLoss>1.0f) sanityLoss=1.0f;
            float stress=entityMgr.dangerLevel*0.65f+sanityLoss*0.35f;
            if(stress<0.0f) stress=0.0f;
            if(stress>1.0f) stress=1.0f;
            float panicBoost = sanityLoss * 0.20f + entityMgr.dangerLevel * 0.15f;
            vI=settings.vhsIntensity*(0.62f+0.78f*stress)+panicBoost;
            static float deathFx = 0.0f; deathFx += isPlayerDead ? dTime * 0.95f : -dTime * 1.4f;
            if(deathFx < 0.0f) deathFx = 0.0f; if(deathFx > 1.0f) deathFx = 1.0f;
            if(isPlayerDead) vI += deathFx * (0.52f + (0.55f + 0.45f * sinf(vhsTime * 5.8f)) * 0.44f);
            if(vI>1.35f) vI=1.35f;
        }
        static GLint vhsTmLoc=-1,vhsIntenLoc=-1,vhsUpscalerLoc=-1,vhsAaModeLoc=-1;
        static GLint vhsSharpnessLoc=-1,vhsTexelXLoc=-1,vhsTexelYLoc=-1;
        static GLint vhsTaaHistLoc=-1,vhsTaaBlendLoc=-1,vhsTaaJitterLoc=-1,vhsTaaValidLoc=-1;
        static GLint vhsFrameGenLoc=-1,vhsFrameGenBlendLoc=-1;
        static GLint vhsRtxALoc=-1,vhsRtxGLoc=-1,vhsRtxRLoc=-1,vhsRtxBLoc=-1,vhsRtxDenoiseOnLoc=-1,vhsRtxDenoiseStrengthLoc=-1,vhsDepthTexLoc=-1,vhsDeathFxLoc=-1;
        if(vhsTmLoc<0){
            glUniform1i(glGetUniformLocation(vhsShader,"tex"),0);
            vhsTmLoc=glGetUniformLocation(vhsShader,"tm"); vhsIntenLoc=glGetUniformLocation(vhsShader,"inten");
            vhsUpscalerLoc=glGetUniformLocation(vhsShader,"upscaler"); vhsAaModeLoc=glGetUniformLocation(vhsShader,"aaMode");
            vhsSharpnessLoc=glGetUniformLocation(vhsShader,"sharpness"); vhsTexelXLoc=glGetUniformLocation(vhsShader,"texelX");
            vhsTexelYLoc=glGetUniformLocation(vhsShader,"texelY"); vhsTaaHistLoc=glGetUniformLocation(vhsShader,"histTex");
            vhsTaaBlendLoc=glGetUniformLocation(vhsShader,"taaBlend"); vhsTaaJitterLoc=glGetUniformLocation(vhsShader,"taaJitter");
            vhsTaaValidLoc=glGetUniformLocation(vhsShader,"taaValid"); vhsFrameGenLoc=glGetUniformLocation(vhsShader,"frameGen");
            vhsFrameGenBlendLoc=glGetUniformLocation(vhsShader,"frameGenBlend"); vhsDepthTexLoc=glGetUniformLocation(vhsShader,"depthTex");
            vhsRtxALoc=glGetUniformLocation(vhsShader,"rtxA"); vhsRtxGLoc=glGetUniformLocation(vhsShader,"rtxG");
            vhsRtxRLoc=glGetUniformLocation(vhsShader,"rtxR"); vhsRtxBLoc=glGetUniformLocation(vhsShader,"rtxB");
            vhsRtxDenoiseOnLoc=glGetUniformLocation(vhsShader,"rtxDenoiseOn"); vhsRtxDenoiseStrengthLoc=glGetUniformLocation(vhsShader,"rtxDenoiseStrength");
            vhsDeathFxLoc=glGetUniformLocation(vhsShader,"deathFx");
        }
        float preDeathFx = sanityCollapseTimer / 2.3f; if(preDeathFx < 0.0f) preDeathFx = 0.0f; if(preDeathFx > 1.0f) preDeathFx = 1.0f;
        float deathFxUniform = (gameState==STATE_GAME || gameState==STATE_PAUSE) ? (isPlayerDead ? (0.55f + 0.45f * sinf(vhsTime * 2.9f)) : preDeathFx) : (isPlayerDead ? 1.0f : 0.0f);
        static int prevAaMode = -1;
        int aaMode = clampAaMode(settings.aaMode);
        if(aaMode != prevAaMode){ prevAaMode = aaMode; taaHistoryValid = false; taaFrameIndex = 0; }
        float jitterX = 0.0f;
        float jitterY = 0.0f;
        if(aaMode == AA_MODE_TAA){
            static const float jitterSeq[8][2] = {
                {0.5f, 0.5f}, {0.75f, 0.25f}, {0.25f, 0.75f}, {0.875f, 0.625f},
                {0.375f, 0.125f}, {0.625f, 0.875f}, {0.125f, 0.375f}, {0.9375f, 0.9375f}
            };
            jitterX = jitterSeq[taaFrameIndex & 7][0] - 0.5f;
            jitterY = jitterSeq[taaFrameIndex & 7][1] - 0.5f;
        }
        if(aaMode==AA_MODE_TAA){
            glBindFramebuffer(GL_FRAMEBUFFER,taaResolveFbo);
            glViewport(0,0,winW,winH);
            glClear(GL_COLOR_BUFFER_BIT);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,fboTex);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D,taaHistoryTex);
            glUniform1i(vhsTaaHistLoc,1);
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D,fboDepthTex);
            glUniform1i(vhsDepthTexLoc,2);
            glUniform1i(vhsRtxALoc,clampSsao(settings.ssaoQuality)); glUniform1i(vhsRtxGLoc,clampGi(settings.giQuality)); glUniform1i(vhsRtxRLoc,settings.godRays?1:0); glUniform1i(vhsRtxBLoc,settings.bloom?1:0);
            glUniform1i(vhsRtxDenoiseOnLoc,settings.rtxDenoise?1:0); glUniform1f(vhsRtxDenoiseStrengthLoc,settings.rtxDenoiseStrength);
            glActiveTexture(GL_TEXTURE0);
            glUniform1f(vhsTmLoc,vhsTime);
            glUniform1f(vhsIntenLoc,vI);
            glUniform1i(vhsUpscalerLoc,clampUpscalerMode(settings.upscalerMode));
            glUniform1i(vhsAaModeLoc,AA_MODE_TAA);
            glUniform1f(vhsSharpnessLoc,clampFsrSharpness(settings.fsrSharpness));
            glUniform1f(vhsTexelXLoc,1.0f/(float)renderW);
            glUniform1f(vhsTexelYLoc,1.0f/(float)renderH);
            glUniform1f(vhsTaaBlendLoc,0.88f);
            glUniform3f(vhsTaaJitterLoc,jitterX,jitterY,0.0f);
            glUniform1f(vhsTaaValidLoc,taaHistoryValid?1.0f:0.0f);
            glUniform1i(vhsFrameGenLoc,isFrameGenEnabled(settings.frameGenMode)?1:0);
            glUniform1f(vhsFrameGenBlendLoc,frameGenBlendStrength(settings.frameGenMode));
            glUniform1f(vhsDeathFxLoc,deathFxUniform);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES,0,6);

            taaHistoryValid = true;
            taaFrameIndex = (taaFrameIndex + 1) & 7;

            GLuint taaTmp = taaHistoryTex;
            taaHistoryTex = taaResolveTex;
            taaResolveTex = taaTmp;
            glBindFramebuffer(GL_FRAMEBUFFER,taaResolveFbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,taaResolveTex,0);

            glBindFramebuffer(GL_FRAMEBUFFER,0);
            glViewport(0,0,winW,winH);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,taaHistoryTex);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D,taaHistoryTex);
            glUniform1i(vhsTaaHistLoc,1);
            glActiveTexture(GL_TEXTURE0);
            glUniform1f(vhsTmLoc,vhsTime);
            glUniform1f(vhsIntenLoc,0.0f);
            glUniform1i(vhsUpscalerLoc,UPSCALER_MODE_OFF);
            glUniform1i(vhsAaModeLoc,AA_MODE_OFF);
            glUniform1f(vhsSharpnessLoc,0.0f);
            glUniform1f(vhsTexelXLoc,1.0f/(float)winW);
            glUniform1f(vhsTexelYLoc,1.0f/(float)winH);
            glUniform1f(vhsTaaBlendLoc,0.0f);
            glUniform3f(vhsTaaJitterLoc,0.0f,0.0f,0.0f);
            glUniform1f(vhsTaaValidLoc,0.0f);
            glUniform1i(vhsFrameGenLoc,0);
            glUniform1f(vhsFrameGenBlendLoc,0.0f);
            glUniform1f(vhsDeathFxLoc,0.0f);
            glUniform1i(vhsRtxALoc,0); glUniform1i(vhsRtxGLoc,0); glUniform1i(vhsRtxRLoc,0); glUniform1i(vhsRtxBLoc,0);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES,0,6);
        }else{
            taaHistoryValid = false;
            taaFrameIndex = 0;
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D,fboTex);
            glActiveTexture(GL_TEXTURE0 + 1);
            glBindTexture(GL_TEXTURE_2D,taaHistoryTex);
            glUniform1i(vhsTaaHistLoc,1);
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_2D,fboDepthTex);
            glUniform1i(vhsDepthTexLoc,2);
            glUniform1i(vhsRtxALoc,clampSsao(settings.ssaoQuality)); glUniform1i(vhsRtxGLoc,clampGi(settings.giQuality)); glUniform1i(vhsRtxRLoc,settings.godRays?1:0); glUniform1i(vhsRtxBLoc,settings.bloom?1:0);
            glUniform1i(vhsRtxDenoiseOnLoc,settings.rtxDenoise?1:0); glUniform1f(vhsRtxDenoiseStrengthLoc,settings.rtxDenoiseStrength);
            glActiveTexture(GL_TEXTURE0);
            glUniform1f(vhsTmLoc,vhsTime);
            glUniform1f(vhsIntenLoc,vI);
            glUniform1i(vhsUpscalerLoc,clampUpscalerMode(settings.upscalerMode));
            glUniform1i(vhsAaModeLoc,aaMode);
            glUniform1f(vhsSharpnessLoc,clampFsrSharpness(settings.fsrSharpness));
            glUniform1f(vhsTexelXLoc,1.0f/(float)renderW);
            glUniform1f(vhsTexelYLoc,1.0f/(float)renderH);
            glUniform1f(vhsTaaBlendLoc,0.0f);
            glUniform3f(vhsTaaJitterLoc,0.0f,0.0f,0.0f);
            glUniform1f(vhsTaaValidLoc,0.0f);
            glUniform1i(vhsFrameGenLoc,0);
            glUniform1f(vhsFrameGenBlendLoc,0.0f);
            glUniform1f(vhsDeathFxLoc,deathFxUniform);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES,0,6);
        }
        glEnable(GL_DEPTH_TEST);
        drawUI();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        int frameGenBaseCap = frameGenBaseFpsCap(cachedRefreshRateHz, settings.frameGenMode, settings.vsync);
        gPerfFrameGenBaseCap = frameGenBaseCap;
        glfwSwapBuffers(gWin);glfwPollEvents();
        if(!settings.vsync) applyFramePacing(frameStartTime, frameGenBaseCap);
    }
    audioRunning=false;SetEvent(hEvent);aT.join();glfwTerminate();return 0;
}
