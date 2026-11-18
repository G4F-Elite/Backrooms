#pragma once
#include "menu_hud.h"
#include "coop.h"
#include "perf_overlay.h"
#include "progression.h"
#include "smile_event.h"
#include "item_types.h"

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

#include "hud_gameplay.h"
#include "hud_shift_report.h"

// The squad size line in the co-op pause menu.
inline const char* pauseSquadNote(){
    static char note[32];
    if(multiState!=MULTI_IN_GAME) return nullptr;
    snprintf(note,sizeof(note),"SQUAD %d/%d",netMgr.getPlayerCount(),MAX_PLAYERS);
    return note;
}

// F10 debug tools: a list of cheats and teleports.
inline void drawDebugToolsPanel(){
    const float w=420.0f, rowH=30.0f;
    float h=UI_HEADER_H+16.0f+rowH*(float)DEBUG_ACTION_COUNT+14.0f;
    float l=-w*0.5f, r=w*0.5f, t=h*0.5f+20.0f, b=t-h;
    drawFullscreenOverlay(0.02f,0.03f,0.04f,0.45f);
    uiPanel(l,b,r,t);
    float y=uiPanelHeader(l,r,t,"DEBUG TOOLS","F10")-16.0f-rowH*0.5f;
    for(int i=0;i<DEBUG_ACTION_COUNT;i++){
        const char* lbl = debugActionLabel(i);
        if(i==DEBUG_ACT_TP_NOTE) lbl = "TP NEAREST LOOT";
        else if(i==DEBUG_ACT_TP_ECHO) lbl = "TP KIOSK";
        else if(i==DEBUG_ACT_TP_EXIT) lbl = "TP DRILL";
        uiListItem(i,lbl,l+14.0f,r-14.0f,y-rowH*(float)i,rowH-4.0f,debugTools.selectedAction==i,UI_ROW_TEXT);
    }
    static const UiHint hints[]={{"F10","CLOSE"}};
    uiHintBar(hints,1);
}

void drawUI(){
    uiBeginFrame(vhsTime,dTime);
    if(gameState==STATE_MENU) drawMenu(vhsTime);
    else if(gameState==STATE_GUIDE) drawGuideScreen();
    else if(gameState==STATE_MULTI) drawMultiMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_HOST) drawHostLobbyScreen(vhsTime,netMgr.getPlayerCount());
    else if(gameState==STATE_MULTI_JOIN) drawJoinMenuScreen(vhsTime);
    else if(gameState==STATE_MULTI_WAIT) drawWaitingScreen(vhsTime);
    else if(gameState==STATE_PAUSE) drawPause(multiState==MULTI_IN_GAME,pauseSquadNote());
    else if(gameState==STATE_SETTINGS||gameState==STATE_SETTINGS_PAUSE) drawSettings(gameState==STATE_SETTINGS_PAUSE,vhsTime);
    else if(gameState==STATE_GAME){
        gSurvivalTime=survivalTime;
        if(playerEscaped) drawShiftReportScreen(vhsTime);
        else if(isPlayerDead) drawHeistDeathScreen(vhsTime);
        else{
            // Ghosts watch the shift through a clean lens: no hurt vignette, no empty health bar.
            float hudHealth = gCoopDead ? 100.0f : playerHealth;
            drawDamageOverlay(damageFlash,hudHealth);
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

            // Vital bars fade in only while they have something to say.
            drawHealthBar(hudHealth);
            drawSanityBar(playerSanity);
            drawStaminaBar(playerStamina);
            drawFlashlightBattery(flashlightBattery,flashlightOn);
            if(activeDeviceSlot == 2){
                // scanner: signal strength, and heat while it is locked on
                float y = -0.78f;
                float sig = scannerSignal < 0.0f ? 0.0f : (scannerSignal > 1.0f ? 1.0f : scannerSignal);
                drawIconC(HUD_ICON_SCANNER, 0.53f, y, 0.085f, scannerOverheated ? HUD_BAD : HUD_COOL);
                drawGauge(0.58f, y + 0.004f, 0.95f, y + 0.034f, sig, HUD_COOL.r, HUD_COOL.g, HUD_COOL.b);
                drawGauge(0.58f, y - 0.034f, 0.95f, y - 0.012f, scannerOverheated ? 1.0f : scannerHeat, HUD_BAD.r, HUD_BAD.g, HUD_BAD.b);
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
                char stageBuf[96];
                snprintf(stageBuf,96,"HEIST STAGE %d  QUOTA %d/%d  CASH %d",heist.stage,heist.quotaHave,heist.quotaNeed,heist.cash);
                drawHudText(stageBuf,blockX,blockY,1.02f,0.86f,0.90f,0.68f,0.95f);
                blockY -= 0.06f;
                char attBuf[64];
                snprintf(attBuf,64,"ATTENTION %.0f%%",attentionLevel);
                drawHudText(attBuf,blockX,blockY,1.02f,0.78f,0.86f,0.76f,0.92f);
                blockY -= 0.06f;
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

            drawGameplayGuidance();
            if(minimapEnabled) drawMinimapOverlay();
            if(storyMgr.hasHallucinations())drawHallucinationEffect((50.0f-playerSanity)/50.0f);
            if(multiState==MULTI_IN_GAME){ drawIconC(HUD_ICON_GROUP,0.80f,0.80f,0.06f,HUD_DIM); drawHudNumber(netMgr.getPlayerCount(),0.83f,0.785f,1.4f,HUD_DIM.r,HUD_DIM.g,HUD_DIM.b); }
            drawHeistFlash();
            if(squadCalloutTimer > 0.0f) drawHudTextCentered(squadCalloutText,0.0f,-0.52f,1.08f,0.88f,0.84f,0.62f,0.92f);
            if(multiState==MULTI_IN_GAME){
                for(int i=0;i<MAX_PLAYERS;i++){
                    if(i==netMgr.myId || !netMgr.players[i].active || !netMgr.players[i].hasValidPos) continue;
                    if(!playerInterpReady[i]) continue;
                    Vec3 wp = playerRenderPos[i] + Vec3(0, 2.2f, 0);
                    float sx=0, sy=0;
                    if(!hasLabelLineOfSight(wp) || !projectToScreen(wp, sx, sy)) continue;
                    Vec3 dd = playerRenderPos[i] - cam.pos;
                    float dist = dd.len();
                    if(dist > 40.0f) continue;
                    const char* nm = netMgr.players[i].name[0] ? netMgr.players[i].name : "Player";
                    drawHudTextCentered(nm, sx, sy, 1.1f, 0.85f, 0.9f, 0.7f, 0.90f);
                }

                netMgr.updatePingMarkTtl(dTime);
            }
            // === DEBUG MODE: trap status text, floor hazards, anomaly lock, minimap state, perf graph ===
            if(settings.debugMode){
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
                drawIconC(HUD_ICON_WARNING,0.0f,0.80f,0.09f,HUD_BAD,0.6f+0.35f*hudPulse(6.0f));
            }
            // === DEBUG MODE: debug tools panel (F10) ===
            if(settings.debugMode && debugTools.open) drawDebugToolsPanel();
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
            // Entities are matched by id and glide to the new spot instead of being rebuilt.
            entityMgr.beginSnapshot();
            for(int i=0;i<netMgr.entitySnapshotCount;i++){
                const NetEntitySnapshotEntry& se = netMgr.entitySnapshot[i];
                if(!se.active) continue;
                entityMgr.applySnapshotEntity(se.id, se.type, se.pos, se.yaw, (unsigned char)se.state);
            }
            entityMgr.endSnapshot();
        }
    }else{
        processHostInteractRequests();
        hostUpdateItems();
        if(heistDangerActive()) updateRoamEventsHost();
        if(stateSyncTimer>=0.12f){
            stateSyncTimer = 0;
            NetEntitySnapshotEntry snap[MAX_SYNC_ENTITIES];
            int c = 0;
            for(auto& e:entityMgr.entities){
                if(c>=MAX_SYNC_ENTITIES) break;
                snap[c].id = e.uid;
                snap[c].type = (int)e.type;
                snap[c].pos = e.pos;
                snap[c].yaw = e.yaw;
                snap[c].state = packEntityState((int)e.state, e.mood);
                snap[c].active = e.active;
                c++;
            }
            netMgr.sendEntitySnapshot(snap, c);
            hostSyncFeatureState();
        }
    }
}
