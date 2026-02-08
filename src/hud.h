#pragma once
#include "coop.h"

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
            drawText(invBuf,-0.95f,0.84f,1.35f,0.55f,0.7f,0.5f,0.75f);
            int switchCount = (coop.switchOn[0]?1:0)+(coop.switchOn[1]?1:0);
            drawText("OBJECTIVE",0.44f,0.82f,1.35f,0.88f,0.92f,0.68f,0.97f);
            char objProgress[64];
            snprintf(objProgress,64,"SWITCHES: %d / 2",switchCount);
            drawText(objProgress,0.44f,0.75f,1.25f,0.84f,0.88f,0.64f,0.95f);
            drawText(coop.doorOpen?"DOOR: OPEN":"DOOR: LOCKED",0.44f,0.68f,1.25f,0.88f,0.83f,0.6f,0.95f);
            if(!coop.doorOpen) drawText("ACTION: HOLD 2 SWITCHES",0.44f,0.61f,1.15f,0.8f,0.84f,0.6f,0.92f);
            if(!coop.doorOpen){
                if(nearPoint2D(cam.pos, coop.switches[0], 2.6f)||nearPoint2D(cam.pos, coop.switches[1], 2.6f))
                    drawText("HOLD SWITCH POSITION",-0.24f,-0.35f,1.4f,0.75f,0.8f,0.55f,0.85f);
            }
            if(falseDoorTimer>0) drawText("FALSE DOOR SHIFT",0.48f,0.67f,1.0f,0.9f,0.35f,0.25f,0.7f);
            if(storyMgr.totalCollected>0)drawNoteCounter(storyMgr.totalCollected);
            drawPhaseIndicator((int)storyMgr.getPhase());
            if(nearNoteId>=0)drawInteractPrompt();
            if(nearbyWorldItemId>=0){
                if(nearbyWorldItemType==0) drawText("[E] PICK BATTERY",-0.18f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.8f);
                else if(nearbyWorldItemType==1) drawText("[E] PICK MEDKIT",-0.16f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.8f);
                else if(nearbyWorldItemType==2) drawText("[E] PICK BAIT",-0.14f,-0.43f,1.4f,0.8f,0.8f,0.55f,0.8f);
            }
            if(multiState!=MULTI_IN_GAME && echoSignal.active){
                Vec3 d = echoSignal.pos - cam.pos;
                d.y = 0;
                float dist = d.len();
                char echoBuf[72];
                snprintf(echoBuf,72,"ECHO SIGNAL %.0fm",dist);
                drawText(echoBuf,-0.95f,0.50f,1.15f,0.62f,0.85f,0.86f,0.76f);
                if(isEchoInRange(cam.pos, echoSignal.pos, 2.5f)){
                    drawText("[E] ATTUNE ECHO",-0.17f,-0.50f,1.35f,0.72f,0.88f,0.9f,0.85f);
                }else{
                    float sx=0, sy=0;
                    if(projectToScreen(echoSignal.pos + Vec3(0,1.1f,0), sx, sy)){
                        drawText("ECHO",sx-0.045f,sy,1.05f,0.7f,0.88f,0.92f,0.82f);
                    }
                }
            }
            if(multiState!=MULTI_IN_GAME && echoStatusTimer>0.0f){
                drawText(echoStatusText,-0.25f,0.56f,1.2f,0.7f,0.86f,0.9f,0.8f);
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
                    drawText(nm, sx - xOff, sy, 1.1f, 0.85f, 0.9f, 0.7f, 0.85f);
                }
            }
            if(multiState==MULTI_IN_GAME){
                char netBuf[96];
                snprintf(netBuf,96,"RTT %.0fms TX %d RX %d",netMgr.rttMs,netMgr.packetsSent,netMgr.packetsRecv);
                drawText(netBuf,0.45f,0.60f,1.0f,0.55f,0.65f,0.8f,0.7f);
            }
            if(trapCorridor.active && trapStatusTimer > 0.0f){
                drawText(trapStatusText,-0.33f,0.50f,1.2f,0.82f,0.78f,0.9f,0.85f);
            }
            if(trapCorridor.locked){
                float sx=0, sy=0;
                if(projectToScreen(trapCorridor.anomalyPos, sx, sy)){
                    float jitter = ((rng()%100)-50) * 0.0007f;
                    float alpha = flashlightOn ? 0.45f : 0.9f;
                    drawText(":)",sx-0.018f+jitter,sy,1.35f,0.95f,0.95f,0.95f,alpha);
                }
            }
            drawText(minimapEnabled?"MINIMAP ON [F6/F7/HOME/M]":"MINIMAP OFF [F6/F7/HOME/M]",0.36f,-0.96f,0.95f,0.55f,0.7f,0.8f,0.68f);
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
