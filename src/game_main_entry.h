#pragma once
#include "progression.h"
#include "sanity_balance.h"
#include "window_title.h"
#include "dev_selftest.h"
#include "scene_presentation.h"
#include "game_post_intensity.h"
int main(){
    std::random_device rd;rng.seed(rd());
    if(!glfwInit())return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    selftestInit();
    gWin=glfwCreateWindow(winW,winH,"Backrooms: Seed Round",NULL,NULL);
    if(!gWin){glfwTerminate();return -1;}
    glfwMakeContextCurrent(gWin);
    int appliedSwapInterval = settings.vsync ? 1 : 0;
    glfwSwapInterval(appliedSwapInterval);
    glfwSetCursorPosCallback(gWin,mouse);
    uiInstallCallbacks(gWin);
    glfwSetFramebufferSizeCallback(gWin,windowResize);
    if(!gladLoadGL())return -1;
    glEnable(GL_DEPTH_TEST);glEnable(GL_CULL_FACE);
    heistLoadTutorialSeen();
    genWorld();
    loadArchiveMetaProgress(archivePoints, archiveTier, perkQuietSteps, perkFastHands, perkEchoBuffer);
    wallTex=genTex(0);floorTex=genTex(1);ceilTex=genTex(2);lightTex=genTex(3);lampTex=genTex(4);propTex=genTex(5);
    deviceTex=genTex(6);
    if(!decorTex) decorTex=genDecorAtlasTex();
    plushTex=genTex(7);
    playerTex=propTex; // players use a stable texture so held-item textures don't bleed
    mainShader=mkShader(mainVS,mainFS);lightShader=mkShader(lightVS,lightFS);vhsShader=mkShader(vhsVS,vhsFS);
    scenePrograms();
    buildGeom();
    computeRenderTargetSize(winW, winH, effectiveRenderScale(settings.upscalerMode, settings.renderScalePreset), renderW, renderH);
    initFBO(fbo,fboTex,fboDepthTex,renderW,renderH);
    initTaaTargets();
    initText();
    initHudIcons();
    entityMgr.init();
    initPlayerModels(); playerModelsInit = true;
    legsInitGfx();
    std::thread aT(audioThread);
    int cachedRefreshRateHz = detectActiveRefreshRateHz(gWin);
    gPerfRefreshHz = cachedRefreshRateHz;
    double nextRefreshProbeTime = 0.0;
    initFrameTimeHistory(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES, 16.6f);
    gPerfFrameTimeHead = PERF_GRAPH_SAMPLES - 1;
    double previousFrameTime = 0.0;
    while(!glfwWindowShouldClose(gWin)){
        double frameStartTime = glfwGetTime();
        float now=(float)frameStartTime;
        const float elapsed = static_cast<float>(frameStartTime - previousFrameTime);
        const float frameSeconds = previousFrameTime > 0 && elapsed > 0 ? elapsed : 0.016f;
        previousFrameTime = frameStartTime;
        dTime = frameSeconds > 0.5f ? 0.016f : frameSeconds;
        vhsTime = now;
        updateWindowTitleForLevel();
        gPerfFrameMs = frameSeconds * 1000.0f;
        pushFrameTimeSample(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES, gPerfFrameTimeHead, gPerfFrameMs);
        float fpsNow = 1.0f / frameSeconds;
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
        sndState.healthLevel = playerHealth / 100.0f; sndState.crouch = cam.crouch ? 1.0f : 0.0f;
        sndState.depthLevel = (float)gCurrentLevel / 6.0f;

        netMgr.updatePingMarkTtl(dTime);
        // Keep the session alive while this player sits in pause/settings/intro (a paused host used to time everyone out).
        if(multiState==MULTI_IN_GAME && gameState!=STATE_GAME) updateMultiplayer();

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
            sndState.monsterAlert = 0.0f; sndState.monsterCall = 0; sndState.pacing *= 0.9f;
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
                        menuSel = MULTI_ITEM_JOIN;
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
                if(eN&&!enterPressed) leaveRunToMenu(); // extracted: the run is over
                if(esN&&!escPressed) leaveRunToMenu();
                enterPressed=eN;escPressed=esN;
            }else if(isPlayerDead){
                bool eN=glfwGetKey(gWin,GLFW_KEY_ENTER)==GLFW_PRESS||
                        glfwGetKey(gWin,GLFW_KEY_SPACE)==GLFW_PRESS;
                bool esN=glfwGetKey(gWin,GLFW_KEY_ESCAPE)==GLFW_PRESS;
                if(eN&&!enterPressed){ heist.cash = cashAfterWipe(heist.cash); reloadLevelAndResume(nullptr); }
                if(esN&&!escPressed) leaveRunToMenu();
                enterPressed=eN;escPressed=esN;
            }else{
                if(lightsOutTimer>0){
                    lightsOutTimer-=dTime;
                    if(lightsOutTimer<=0){
                        lightsOutTimer=0;
                        lights.clear(); // regenerate: lamps that were switched off by the blackout come back
                        updateLightsAndPillars(playerChunkX,playerChunkZ);
                        updateMapContent(playerChunkX,playerChunkZ);
                        buildGeom();
                    }
                }
                if(falseDoorTimer>0) falseDoorTimer-=dTime;
                updateFloorHoles();
                gameInput(gWin);
                updatePoiRuntime();
                if(multiState!=MULTI_IN_GAME){ if(heistDangerActive()) updateRoamEventsHost(); hostUpdateItems(); }
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
                if(heistDangerActive() && tryTriggerRandomScare(scareState, dTime, storyMgr.getPhase(), playerSanity, (int)(rng()%100))){
                    triggerLocalScare(0.26f, 0.14f, 3.0f);
                }
                nearNoteId=-1;
                // Nothing hunts the squad before the accident: the first shift is a guided tour.
                bool canSpawnEnt = (multiState!=MULTI_IN_GAME || netMgr.isHost) && heistDangerActive();
                collectEntityTargets();
                entitySpawnTimer-=dTime;
                int maxEnt = computeEntityCap(survivalTime);
                maxEnt += levelEntityCapBonus(gCurrentLevel);
                if(maxEnt > 8) maxEnt = 8;
                // The pacing director holds spawns back while the squad catches its breath.
                if(canSpawnEnt && entitySpawnTimer<=0&&(int)entityMgr.entities.size()<maxEnt&&entityMgr.directorOut.allowSpawn){
                    EntityType type = chooseSpawnEntityType(survivalTime, (int)rng(), (int)rng());
                    Vec3 huntAround = entityMgr.targetCount>0 ? entityMgr.targets[rng()%(unsigned)entityMgr.targetCount].pos : cam.pos;
                    entityMgr.spawnOutOfSight(type, huntAround);
                    entitySpawnTimer = computeEntitySpawnDelay(survivalTime, (int)rng()) * levelSpawnDelayScale(gCurrentLevel);
                }
                entityMgr.update(dTime,cam.pos,cam.yaw,nullptr,0,0,CS);
                float nearestMonster = 9999.0f;
                float menace = 0.0f;
                EntityType dominantMonster = ENTITY_NONE;
                float monsterSide = 0.0f, monsterAlert = 0.0f, monsterScore = -1.0f;
                int monstersInEarshot = 0;
                for(const auto& e:entityMgr.entities){
                    if(!e.active) continue;
                    Vec3 dd = e.pos - cam.pos;
                    dd.y = 0;
                    float dist = dd.len();
                    if(dist < 30.0f) monstersInEarshot++;
                    // The one the sound follows: close matters, but one that is hunting matters more.
                    float alert = monsterAlertFromEntity(e);
                    float score = (1.0f - dist / 30.0f) + alert * 0.35f;
                    if(dist < 30.0f && score > monsterScore){
                        monsterScore = score; nearestMonster = dist; dominantMonster = e.type; monsterAlert = alert;
                        // Stereo side: the camera's "right" vector points to screen-left.
                        monsterSide = dist > 0.05f ? (dd.z * mSin(cam.yaw) - dd.x * mCos(cam.yaw)) / dist : 0.0f;
                    }
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
                    if(monsterProx < 0.0f) monsterProx = 0.0f; else if(monsterProx > 1.0f) monsterProx = 1.0f;
                }
                sndState.monsterProximity = monsterProx;
                sndState.monsterPan = monsterSide;
                sndState.monsterAlert = monsterAlert; sndState.monsterCall = monsterCallFromAlert(monsterAlert);
                sndState.monsterCrowd = (float)monstersInEarshot * 0.25f;
                sndState.pacing = entityMgr.director.intensity;
                sndState.monsterMenace = menace > 1.0f ? 1.0f : menace; sndState.monsterType = monsterTypeMixFromEntity(dominantMonster);
                
                reshuffleTimer-=dTime;
                float reshuffleChance=30.0f+survivalTime*0.1f;
                if(reshuffleChance>80.0f)reshuffleChance=80.0f;
                float reshuffleDelay=25.0f-survivalTime*0.03f;
                if(reshuffleDelay<5.0f)reshuffleDelay=5.0f;
                // Walls only shift in solo: in co-op teammates would watch them pop and peers would desync.
                bool canReshuffle = (multiState!=MULTI_IN_GAME) && heistDangerActive();
                bool frontReshuffle=survivalTime>300&&rng()%100<20;
                if(canReshuffle && reshuffleTimer<=0&&rng()%100<(int)reshuffleChance){
                    if(frontReshuffle||reshuffleBehind(cam.pos.x,cam.pos.z,cam.yaw)){
                        reopenObjectiveCells();
                        updateMapContent(playerChunkX,playerChunkZ);
                        buildGeom();
                    }
                    reshuffleTimer=reshuffleDelay+(rng()%10);
                }else if(reshuffleTimer<=0)reshuffleTimer=8.0f+(rng()%8);
                
                heistUpdate();
                updatePlayerVitals();
                if(!heistDangerActive()){ playerSanity = 100.0f; survivalTime = 0.0f; }
                camShake*=0.9f;damageFlash*=0.92f;survivalTime+=dTime;
                if(multiState==MULTI_IN_GAME && !netMgr.isHost) captureSessionSnapshot();
                updateMultiplayer();
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER,fbo);
        glViewport(0,0,renderW,renderH);
        glClearColor(0.02f,0.02f,0.02f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        gPostFx.sceneDrawn=false;
        spectatorBeginView(); // dead co-op players watch through the spectator camera
        legsBeginView(); // the living look out of the head of their simulated body
        if((gameState==STATE_GAME&&!isPlayerDead)||gameState==STATE_PAUSE||gameState==STATE_SETTINGS_PAUSE||gameState==STATE_NOTE)
            renderScene();
        
        presentGameScene();
        glEnable(GL_DEPTH_TEST);
        drawUI();
        legsEndView();
        spectatorEndView();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        selftestFrame();
        glfwSwapBuffers(gWin);glfwPollEvents();
    }
    audioRunning=false;SetEvent(hEvent);aT.join();glfwTerminate();return 0;
}
