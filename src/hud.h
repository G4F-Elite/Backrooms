#pragma once
#include "menu_hud.h"
#include "coop.h"
#include "perf_overlay.h"
#include "progression.h"
#include "smile_event.h"

inline void drawHudText(const char* s, float x, float y, float sc, float r, float g, float b, float a = 0.95f) {
    drawText(s, x - 0.002f, y - 0.002f, sc, 0.0f, 0.0f, 0.0f, a * 0.72f);
    drawText(s, x, y, sc, r, g, b, a);
}

inline void drawHudTextCentered(const char* s, float x, float y, float sc, float r, float g, float b, float a = 0.95f) {
    drawTextCentered(s, x - 0.002f, y - 0.002f, sc, 0.0f, 0.0f, 0.0f, a * 0.72f);
    drawTextCentered(s, x, y, sc, r, g, b, a);
}

inline void drawEyeMarker(float sx, float sy, float scale, float alpha) {
    float w = 0.050f * scale;
    float h = 0.022f * scale;
    drawOverlayRectNdc(sx - w, sy - h, sx + w, sy + h, 0.46f, 0.08f, 0.08f, alpha * 0.62f);
    drawOverlayRectNdc(sx - w * 0.86f, sy - h * 0.68f, sx + w * 0.86f, sy + h * 0.68f, 0.84f, 0.16f, 0.14f, alpha * 0.92f);
    drawOverlayRectNdc(sx - w * 0.28f, sy - h * 0.58f, sx + w * 0.28f, sy + h * 0.58f, 0.05f, 0.02f, 0.02f, alpha);
    drawOverlayRectNdc(sx - w * 0.10f, sy - h * 0.20f, sx + w * 0.10f, sy + h * 0.20f, 0.90f, 0.30f, 0.24f, alpha);
}

void drawUI(){
    if(gameState==STATE_MENU) drawMenu(vhsTime);
    else if(gameState==STATE_GUIDE) drawGuideScreen();
    else if(gameState==STATE_MULTI) drawMultiMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_HOST) drawHostLobbyScreen(vhsTime,netMgr.getPlayerCount());
    else if(gameState==STATE_MULTI_JOIN) drawJoinMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_WAIT) drawWaitingScreen(vhsTime);
    else if(gameState==STATE_PAUSE){
        if(multiState==MULTI_IN_GAME) drawMultiPause(netMgr.getPlayerCount());
        else drawPause();
    }else if(gameState==STATE_SETTINGS||gameState==STATE_SETTINGS_PAUSE) drawSettings(gameState==STATE_SETTINGS_PAUSE);
    else if(gameState==STATE_KEYBINDS||gameState==STATE_KEYBINDS_PAUSE) drawKeybindsMenu(gameState==STATE_KEYBINDS_PAUSE, menuSel, keybindCaptureIndex);
    else if(gameState==STATE_INTRO) drawIntro(storyMgr.introLine,storyMgr.introTimer,storyMgr.introLineTime,INTRO_LINES);
    else if(gameState==STATE_NOTE&&storyMgr.readingNote&&storyMgr.currentNote>=0)
        drawNote(storyMgr.currentNote,NOTE_TITLES[storyMgr.currentNote],NOTE_CONTENTS[storyMgr.currentNote]);
    else if(gameState==STATE_GAME){
        gSurvivalTime=survivalTime;
        if(playerEscaped) drawEscape(vhsTime);
        else if(isPlayerDead) drawDeath(vhsTime);
        else{
            drawDamageOverlay(damageFlash,playerHealth);
            drawSurvivalTime(survivalTime);

            // === DEBUG MODE: FPS/telemetry overlay ===
            if(settings.debugMode){
                char fpsBuf[48];
                snprintf(fpsBuf,48,"FPS %.0f",gPerfFpsSmoothed);
                char fgBuf[80];
                formatFrameGenPipeline(fgBuf,80,gPerfRefreshHz,gPerfFrameGenBaseCap,settings.frameGenMode,settings.vsync);
                char upBuf[96];
                formatUpscalePipeline(upBuf,96,settings.upscalerMode,renderW,renderH,winW,winH);
                if(gHudTelemetryVisible){
                    char pingBuf[48];
                    if(multiState==MULTI_IN_GAME) snprintf(pingBuf,48,"PING %.0fms",netMgr.rttMs);
                    else snprintf(pingBuf,48,"PING --");
                    char netBuf[40];
                    if(multiState==MULTI_IN_GAME) snprintf(netBuf,40,"NET %s",netMgr.connectionQualityLabel((float)glfwGetTime()));
                    else snprintf(netBuf,40,"NET --");
                    char perfRow[300];
                    snprintf(perfRow,300,"%s | %s | %s | %s | %s",fpsBuf,fgBuf,upBuf,pingBuf,netBuf);
                    drawHudText(perfRow,-0.95f,0.95f,1.20f,0.88f,0.93f,0.78f,0.98f);
                }else{
                    drawHudText("HUD HIDDEN",-0.95f,0.95f,1.00f,0.80f,0.86f,0.74f,0.95f);
                }
            }

            if(playerHealth<100)drawHealthBar(playerHealth);
            if(playerSanity<100)drawSanityBar(playerSanity);
            drawStaminaBar(playerStamina);
            if(flashlightBattery<100)drawFlashlightBattery(flashlightBattery,flashlightOn);
            if(activeDeviceSlot == 2){
                float y = -0.70f;
                drawHudText("SCANNER",0.52f,y,1.15f,0.55f,0.82f,0.86f,0.92f);
                float sig = scannerSignal;
                if(sig < 0.0f) sig = 0.0f;
                if(sig > 1.0f) sig = 1.0f;
                drawSlider(0.66f,y,0.30f,sig,0.45f,0.85f,0.95f);
            }

            // === DEBUG MODE: full objective block (top-right) ===
            if(settings.debugMode){
                float blockX = 0.44f;
                float blockY = 0.90f;
                const char* phaseNames[] = {"INTRO","EXPLORATION","SURVIVAL","DESPERATION"};
                int phaseIdx = (int)storyMgr.getPhase();
                if(phaseIdx < 0) phaseIdx = 0;
                if(phaseIdx > 3) phaseIdx = 3;
                drawHudText("OBJECTIVE",blockX,blockY,1.28f,0.90f,0.94f,0.72f,0.97f);
                blockY -= 0.07f;
                if(multiState==MULTI_IN_GAME){
                    int switchCount = (coop.switchOn[0]?1:0)+(coop.switchOn[1]?1:0);
                    char objProgress[64];
                    snprintf(objProgress,64,"SWITCHES %d/2",switchCount);
                    drawHudText(objProgress,blockX,blockY,1.18f,0.86f,0.90f,0.68f,0.95f);
                    blockY -= 0.06f;
                    drawHudText(coop.doorOpen?"DOOR OPEN":"DOOR LOCKED",blockX,blockY,1.16f,0.88f,0.84f,0.62f,0.95f);
                    blockY -= 0.06f;
                    if(!coop.doorOpen) drawHudText("ACTION HOLD 2 SWITCHES",blockX,blockY,1.02f,0.82f,0.86f,0.62f,0.90f);
                    blockY -= 0.06f;
                }else{
                    const int notesRequired = storyNotesRequired();
                    bool exitReady = isStoryExitReady();
                    char notesLine[64];
                    snprintf(notesLine,64,"NOTES %d/%d",storyMgr.totalCollected,notesRequired);
                    drawHudText(notesLine,blockX,blockY,1.18f,0.86f,0.90f,0.68f,0.95f);
                    blockY -= 0.06f;
                    drawHudText(storyEchoAttuned?"ECHO ATTUNED":"ECHO NOT ATTUNED",blockX,blockY,1.08f,0.72f,0.86f,0.90f,0.93f);
                    blockY -= 0.06f;
                    drawHudText(exitReady?"EXIT DOOR READY":"EXIT DOOR LOCKED",blockX,blockY,1.16f,0.88f,0.84f,0.62f,0.95f);
                    blockY -= 0.06f;
                    drawHudText(exitReady?"ACTION GO TO DOOR AND PRESS E":"ACTION NOTES + ECHO REQUIRED",blockX,blockY,1.02f,0.82f,0.86f,0.62f,0.90f);
                    blockY -= 0.06f;
                }
                char phaseBuf[64];
                snprintf(phaseBuf,64,"PHASE %s",phaseNames[phaseIdx]);
                drawHudText(phaseBuf,blockX,blockY,1.14f,0.86f,0.80f,0.56f,0.94f);
                blockY -= 0.06f;
                char levelBuf[48];
                buildLevelLabel(gCurrentLevel, levelBuf, 48);
                drawHudText(levelBuf,blockX,blockY,1.06f,0.80f,0.86f,0.66f,0.93f);
                blockY -= 0.06f;
                char invBuf[64];
                snprintf(invBuf,64,"SUPPLIES B:%d",invBattery);
                drawHudText(invBuf,blockX,blockY,1.06f,0.76f,0.86f,0.70f,0.93f);
                blockY -= 0.06f;
                if(falseDoorTimer>0) {
                    drawHudText("EVENT FALSE DOOR SHIFT",blockX,blockY,1.05f,0.95f,0.45f,0.36f,0.92f);
                    blockY -= 0.05f;
                }
            }

            if(multiState==MULTI_IN_GAME && !coop.doorOpen){
                if(settings.debugMode){
                    if(nearPoint2D(cam.pos, coop.switches[0], 2.6f)||nearPoint2D(cam.pos, coop.switches[1], 2.6f))
                        drawHudTextCentered("HOLD SWITCH POSITION",0.0f,-0.35f,1.4f,0.75f,0.8f,0.55f,0.90f);
                }
            }
            if(multiState!=MULTI_IN_GAME){
                bool nearExit = nearPoint2D(cam.pos, coop.doorPos, 2.4f);
                if(nearExit && settings.debugMode){
                    if(isStoryExitReady()) drawHudTextCentered("[E] EXIT LEVEL",0.0f,-0.35f,1.4f,0.75f,0.88f,0.70f,0.95f);
                    else drawHudTextCentered("COLLECT NOTES + ATTUNE ECHO TO UNLOCK EXIT",0.0f,-0.35f,1.2f,0.88f,0.72f,0.58f,0.93f);
                }
            }else{
                bool nearExit = nearPoint2D(cam.pos, coop.doorPos, 2.4f);
                if(nearExit && settings.debugMode){
                    if(coop.doorOpen && storyMgr.totalCollected>=5) drawHudTextCentered("[E] EXIT LEVEL",0.0f,-0.35f,1.4f,0.75f,0.88f,0.70f,0.95f);
                    else drawHudTextCentered("OPEN DOOR + COLLECT 5 NOTES TO EXIT",0.0f,-0.35f,1.25f,0.88f,0.72f,0.58f,0.93f);
                }
            }
            drawNoteCounter(storyMgr.totalCollected);
            if(nearNoteId>=0 && settings.debugMode) drawInteractPrompt();
            if(nearbyWorldItemId>=0 && settings.debugMode){
                if(nearbyWorldItemType==0) drawHudTextCentered("[E] PICK BATTERY",0.0f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.9f);
            }
            // === ECHO SIGNAL: immersive pulsing indicator (always), debug shows distance ===
            if(multiState!=MULTI_IN_GAME && echoSignal.active){
                Vec3 d = echoSignal.pos - cam.pos;
                d.y = 0;
                float dist = d.len();
                if(settings.debugMode){
                    char echoBuf[72];
                    snprintf(echoBuf,72,"ECHO SIGNAL %.0fm",dist);
                    drawHudText(echoBuf,-0.95f,0.50f,1.18f,0.62f,0.85f,0.86f,0.90f);
                }else{
                    // Immersive echo: faint, flickering beacon with a VHS-like pulse (no text)
                    float proximity = 1.0f - (dist / 60.0f);
                    if(proximity < 0.05f) proximity = 0.05f;
                    if(proximity > 1.0f) proximity = 1.0f;
                    float t = (float)glfwGetTime();
                    float pulse = 0.32f + 0.68f * proximity;
                    float flicker = 0.72f + 0.28f * sinf(t * (1.4f + proximity * 6.2f));
                    float alpha = pulse * flicker;
                    float driftX = 0.006f * sinf(t * 0.9f + dist * 0.04f);
                    float driftY = 0.006f * cosf(t * 0.7f + dist * 0.03f);
                    float baseX = -0.90f + driftX;
                    float baseY = 0.50f + driftY;
                    float core = 0.010f + 0.010f * proximity;
                    float glow = core * 2.8f;
                    drawOverlayRectNdc(baseX - glow, baseY - glow, baseX + glow, baseY + glow, 0.18f, 0.36f, 0.42f, alpha * 0.20f);
                    drawOverlayRectNdc(baseX - core * 1.8f, baseY - core * 0.12f, baseX + core * 1.8f, baseY + core * 0.12f, 0.28f, 0.68f, 0.78f, alpha * 0.55f);
                    drawOverlayRectNdc(baseX - core * 0.12f, baseY - core * 1.8f, baseX + core * 0.12f, baseY + core * 1.8f, 0.38f, 0.82f, 0.92f, alpha * 0.70f);
                    float smearY = baseY - 0.035f + 0.01f * sinf(t * 1.25f + proximity);
                    float smearW = 0.04f + 0.05f * proximity;
                    drawOverlayRectNdc(baseX - smearW, smearY - 0.004f, baseX + smearW, smearY + 0.004f, 0.22f, 0.50f, 0.58f, alpha * 0.28f);
                }
                if(settings.debugMode && isEchoInRange(cam.pos, echoSignal.pos, 2.5f)){
                    drawHudTextCentered("[E]",0.0f,-0.50f,1.6f,0.72f,0.88f,0.9f,0.90f);
                }
            }
            if(settings.debugMode && multiState!=MULTI_IN_GAME && echoStatusTimer>0.0f){
                drawHudTextCentered(echoStatusText,0.0f,0.62f,1.18f,0.7f,0.86f,0.9f,0.92f);
            }
            // === SMILE EVENT: red corridor overlay always, text hint only in debug ===
            if(smileEvent.corridorActive){
                drawFullscreenOverlay(0.20f,0.02f,0.02f,0.18f);
                if(settings.debugMode) drawHudTextCentered("RED CORRIDOR. MOVE TO THE END.",0.0f,0.70f,1.18f,0.92f,0.46f,0.40f,0.95f);
            }else if(smileEvent.eyeActive){
                // === DEBUG MODE: eye marker wallhack/ESP ===
                if(settings.debugMode){
                    float sx = 0.0f, sy = 0.0f;
                    if(projectToScreen(smileEvent.eyePos, sx, sy)){
                        drawEyeMarker(sx, sy, 1.0f, 0.98f);
                    }else{
                        Vec3 rightHint(mCos(cam.yaw), 0.0f, -mSin(cam.yaw));
                        Vec3 eyeDir = smileEvent.eyePos - cam.pos;
                        eyeDir.y = 0.0f;
                        float side = rightHint.dot(eyeDir);
                        if(side >= 0.0f){
                            drawEyeMarker(0.88f, 0.31f, 0.82f, 0.96f);
                            drawHudText(">>",0.80f,0.27f,1.20f,0.90f,0.42f,0.38f,0.95f);
                        }else{
                            drawEyeMarker(-0.88f, 0.31f, 0.82f, 0.96f);
                            drawHudText("<<",-0.95f,0.27f,1.20f,0.90f,0.42f,0.38f,0.95f);
                        }
                    }
                }
            }
            if(minimapEnabled) drawMinimapOverlay();
            if(storyMgr.hasHallucinations())drawHallucinationEffect((50.0f-playerSanity)/50.0f);
            if(multiState==MULTI_IN_GAME)drawMultiHUD(netMgr.getPlayerCount(),netMgr.isHost);
            if(multiState==MULTI_IN_GAME){
                for(int i=0;i<MAX_PLAYERS;i++){
                    if(i==netMgr.myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) continue;
                    if(!playerInterpReady[i]) continue;
                    Vec3 wp = playerRenderPos[i] + Vec3(0, 2.2f, 0);
                    float sx=0, sy=0;
                    if(!projectToScreen(wp, sx, sy)) continue;
                    Vec3 dd = playerRenderPos[i] - cam.pos;
                    float dist = dd.len();
                    if(dist > 40.0f) continue;
                    const char* nm = netMgr.players[i].name[0] ? netMgr.players[i].name : "Player";
                    float xOff = (float)strlen(nm) * 0.012f;
                    drawHudText(nm, sx - xOff, sy, 1.1f, 0.85f, 0.9f, 0.7f, 0.90f);
                }
            }
            // === DEBUG MODE: trap status text, floor hazards, anomaly lock, minimap state, perf graph ===
            if(settings.debugMode){
                if(trapCorridor.active && trapStatusTimer > 0.0f){
                    drawHudTextCentered(trapStatusText,0.0f,0.55f,1.2f,0.82f,0.78f,0.9f,0.90f);
                }
                if(!floorHoles.empty()){
                    char holeBuf[64];
                    snprintf(holeBuf,64,"FLOOR HAZARDS: %d", (int)floorHoles.size());
                    drawHudText(holeBuf,-0.95f,0.44f,1.10f,0.92f,0.58f,0.38f,0.90f);
                }
                if(trapCorridor.locked){
                    float t = trapCorridor.stareProgress / 2.6f;
                    if(t < 0.0f) t = 0.0f;
                    if(t > 1.0f) t = 1.0f;
                    char anomBuf[64];
                    snprintf(anomBuf,64,"ANOMALY LOCK %.0f%%",t * 100.0f);
                    drawHudText(anomBuf,-0.95f,0.38f,1.0f,0.88f,0.78f,0.90f,0.92f);
                }
                const char* mmState = minimapEnabled ? "MINIMAP ON [M/F8]" : "MINIMAP OFF [M/F8]";
                drawHudText(mmState,-0.95f,0.84f,0.95f,0.88f,0.93f,0.78f,0.95f);
                if(gPerfDebugOverlay){
                    char graph[40];
                    buildFrameTimeGraph(
                        gPerfFrameTimeHistory,
                        PERF_GRAPH_SAMPLES,
                        gPerfFrameTimeHead,
                        32,
                        graph,
                        40
                    );
                    float avgMs = averageFrameTimeMs(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES);
                    float p95Ms = percentileFrameTimeMs(gPerfFrameTimeHistory, PERF_GRAPH_SAMPLES, 0.95f);
                    char dbgA[96];
                    char dbgB[96];
                    snprintf(dbgA,96,"DEBUG PERF [F3] FT %.2fms AVG %.2f P95 %.2f",gPerfFrameMs,avgMs,p95Ms);
                    snprintf(dbgB,96,"GRAPH %s",graph);
                    drawHudText(dbgA,0.12f,-0.74f,1.00f,0.70f,0.85f,0.92f,0.93f);
                    drawHudText(dbgB,0.12f,-0.80f,1.00f,0.72f,0.82f,0.88f,0.93f);
                }
                if(debugTools.flyMode){
                    drawHudText("DEBUG FLY: ON",0.52f,0.95f,1.10f,0.78f,0.95f,0.85f,0.98f);
                }
                if(debugTools.infiniteStamina){
                    drawHudText("DEBUG STAMINA: INF",0.52f,0.90f,1.02f,0.75f,0.92f,0.78f,0.96f);
                }
            }
            if(multiState==MULTI_IN_GAME && netMgr.connectionUnstable((float)glfwGetTime())){
                drawHudTextCentered("NETWORK UNSTABLE - RECONNECTING MAY OCCUR",0.0f,0.74f,1.12f,0.95f,0.64f,0.44f,0.95f);
            }
            // === DEBUG MODE: debug tools panel (F10) ===
            if(settings.debugMode && debugTools.open){
                drawFullscreenOverlay(0.02f,0.03f,0.04f,0.62f);
                drawHudText("DEBUG TOOLS",-0.28f,0.56f,1.8f,0.9f,0.95f,0.82f,0.98f);
                for(int i=0;i<DEBUG_ACTION_COUNT;i++){
                    float y = 0.47f - i*0.08f;
                    float s = (debugTools.selectedAction==i)?1.0f:0.65f;
                    if(debugTools.selectedAction==i) drawHudText(">",-0.39f,y,1.4f,0.92f,0.9f,0.65f,0.95f);
                    drawHudText(debugActionLabel(i),-0.34f,y,1.35f,0.82f*s,0.88f*s,0.72f*s,0.92f);
                }
                drawHudText("F10 TOGGLE  ENTER APPLY  ESC CLOSE",-0.42f,-0.20f,1.15f,0.67f,0.72f,0.78f,0.90f);
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
