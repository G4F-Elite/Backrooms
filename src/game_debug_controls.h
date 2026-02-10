#pragma once
// NOTE: included into main translation unit (game.cpp); relies on prior includes.

struct GLFWwindow;
#include "math.h"
#include "item_types.h"
inline Vec3 debugCursorSpawnPos(GLFWwindow* w){
    double cx = 0.0, cy = 0.0;
    glfwGetCursorPos(w, &cx, &cy);
    float nx = (2.0f * (float)cx / (float)winW) - 1.0f;
    float ny = 1.0f - (2.0f * (float)cy / (float)winH);
    float fov = 1.2f;
    float t = tanf(fov * 0.5f);
    float asp = (float)winW / (float)winH;
    Vec3 fwd(mSin(cam.yaw) * mCos(cam.pitch), mSin(cam.pitch), mCos(cam.yaw) * mCos(cam.pitch));
    Vec3 right(mCos(cam.yaw), 0, -mSin(cam.yaw));
    Vec3 up = right.cross(fwd).norm();
    Vec3 dir = (fwd + right * (nx * asp * t) + up * (ny * t)).norm();
    if (fabsf(dir.y) > 0.001f) {
        float hitT = -cam.pos.y / dir.y;
        if (hitT > 0.5f && hitT < 80.0f) {
            Vec3 hit = cam.pos + dir * hitT;
            if (!collideWorld(hit.x, hit.z, PR)) {
                return Vec3(hit.x, 0.0f, hit.z);
            }
        }
    }
    return findSpawnPos(cam.pos, 6.0f);
}

void executeDebugAction(int action){
    action = clampDebugActionIndex(action);
    bool canMutateWorld = (multiState!=MULTI_IN_GAME || netMgr.isHost);
    if(action==DEBUG_ACT_TOGGLE_FLY){
        debugTools.flyMode = !debugTools.flyMode;
        setTrapStatus(debugTools.flyMode ? "DEBUG: FLY ENABLED" : "DEBUG: FLY DISABLED");
        return;
    }
    if(action==DEBUG_ACT_TOGGLE_INFINITE_STAMINA){
        debugTools.infiniteStamina = !debugTools.infiniteStamina;
        setTrapStatus(debugTools.infiniteStamina ? "DEBUG: INFINITE STAMINA ON" : "DEBUG: INFINITE STAMINA OFF");
        return;
    }
    if(action==DEBUG_ACT_TP_NOTE){
        int target = -1;
        for(auto& n:storyMgr.notes){
            if(!n.active || n.collected) continue;
            target = n.id;
            break;
        }
        if(target < 0 && lastSpawnedNote < 11){
            trySpawnNote(lastSpawnedNote + 1);
            for(auto& n:storyMgr.notes){
                if(!n.active || n.collected) continue;
                target = n.id;
                break;
            }
        }
        if(target >= 0){
            Vec3 p = storyMgr.notes[target].pos;
            cam.pos = Vec3(p.x, PH, p.z);
            updateVisibleChunks(cam.pos.x, cam.pos.z);
            updateLightsAndPillars(playerChunkX,playerChunkZ);
            updateMapContent(playerChunkX,playerChunkZ);
            buildGeom();
            setEchoStatus("DEBUG: TELEPORTED TO NOTE");
        }
        return;
    }
    if(action==DEBUG_ACT_TP_ECHO){
        if(!echoSignal.active) spawnEchoSignal();
        cam.pos = Vec3(echoSignal.pos.x, PH, echoSignal.pos.z);
        updateVisibleChunks(cam.pos.x, cam.pos.z);
        updateLightsAndPillars(playerChunkX,playerChunkZ);
        updateMapContent(playerChunkX,playerChunkZ);
        buildGeom();
        setEchoStatus("DEBUG: TELEPORTED TO ECHO");
        return;
    }
    if(action==DEBUG_ACT_TP_EXIT){
        extern void teleportToExit();
        teleportToExit();
        setEchoStatus("DEBUG: TELEPORTED TO EXIT");
        return;
    }
    if(action==DEBUG_ACT_TRIGGER_EYE){
        if(smileEvent.corridorActive){
            setTrapStatus("DEBUG: EYE EVENT ALREADY ACTIVE");
            return;
        }
        if(!smileEvent.eyeActive){
            Vec3 fwd(mSin(cam.yaw), 0.0f, mCos(cam.yaw));
            Vec3 right(mCos(cam.yaw), 0.0f, -mSin(cam.yaw));
            Vec3 cand = cam.pos + fwd * 10.0f + right * (((rng()%2)==0) ? -4.5f : 4.5f);
            int wx = (int)floorf(cand.x / CS);
            int wz = (int)floorf(cand.z / CS);
            if(getCellWorld(wx, wz) != 0){
                cand = findSpawnPos(cam.pos, 8.0f);
            }
            smileEvent.eyePos = Vec3(cand.x, PH + 0.3f, cand.z);
            smileEvent.eyeLife = 18.0f;
            smileEvent.eyeLookTime = 0.0f;
            smileEvent.eyeActive = true;
            setEchoStatus("DEBUG: EYE SPAWNED");
        }else{
            triggerSmileCorridor();
            setEchoStatus("DEBUG: EYE CORRIDOR TRIGGERED");
        }
        return;
    }
    if(!canMutateWorld){
        setTrapStatus("DEBUG ACTION: HOST ONLY");
        return;
    }
    if(debugActionSpawnsEntity(action)){
        EntityType t = debugActionEntityType(action);
        if(multiState==MULTI_IN_GAME && !netMgr.isHost){
            int reqType = REQ_DEBUG_SPAWN_STALKER;
            if(t==ENTITY_CRAWLER) reqType = REQ_DEBUG_SPAWN_CRAWLER;
            else if(t==ENTITY_SHADOW) reqType = REQ_DEBUG_SPAWN_SHADOW;
            netMgr.sendInteractRequest(reqType, 0);
            setEchoStatus("DEBUG: SPAWN REQUEST SENT");
            return;
        }
        Vec3 sp = debugCursorSpawnPos(gWin);
        entityMgr.spawnEntity(t, sp, nullptr, 0, 0);
        setEchoStatus("DEBUG: ENTITY SPAWNED");
        return;
    }
    if(action==DEBUG_ACT_FORCE_HOLES){
        spawnFloorHoleEvent(cam.pos, floorHoleCountFromRoll((int)rng()), floorHoleDurationFromRoll((int)rng()));
        buildGeom();
        return;
    }
    if(action==DEBUG_ACT_FORCE_SUPPLY){
        applyRoamEvent(ROAM_SUPPLY_CACHE, playerChunkX, playerChunkZ, 10.0f);
        return;
    }
}

void mouse(GLFWwindow*,double xp,double yp){
    if(gameState!=STATE_GAME&&gameState!=STATE_INTRO)return;
    if(debugTools.open) {
        lastX = (float)xp;
        lastY = (float)yp;
        firstMouse = true;
        return;
    }
    if(isPlayerDead || playerEscaped || playerFalling) return;
    if(firstMouse){lastX=(float)xp;lastY=(float)yp;firstMouse=false;}
    cam.yaw-=((float)xp-lastX)*settings.mouseSens;
    cam.pitch+=(lastY-(float)yp)*settings.mouseSens;
    cam.pitch=cam.pitch>1.4f?1.4f:(cam.pitch<-1.4f?-1.4f:cam.pitch);
    lastX=(float)xp;lastY=(float)yp;
}

void gameInput(GLFWwindow*w){
    static bool debugTogglePressed = false;
    static bool debugUpPressed = false;
    static bool debugDownPressed = false;
    static bool debugEnterPressed = false;
    static bool debugEscPressed = false;
    static bool perfTogglePressed = false;
    static bool hudTogglePressed = false;
    bool debugToggleNow = glfwGetKey(w,GLFW_KEY_F10)==GLFW_PRESS;
    if(settings.debugMode && debugToggleNow && !debugTogglePressed){
        debugTools.open = !debugTools.open;
        if(debugTools.open){
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
            firstMouse = true;
        }else{
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
    }
    debugTogglePressed = debugToggleNow;
    bool perfToggleNow = glfwGetKey(w,GLFW_KEY_F3)==GLFW_PRESS;
    if(settings.debugMode && perfToggleNow && !perfTogglePressed){
        gPerfDebugOverlay = !gPerfDebugOverlay;
        triggerMenuConfirmSound();
    }
    perfTogglePressed = perfToggleNow;
    bool hudToggleNow = glfwGetKey(w,GLFW_KEY_F6)==GLFW_PRESS;
    if(settings.debugMode && hudToggleNow && !hudTogglePressed){
        gHudTelemetryVisible = !gHudTelemetryVisible;
        triggerMenuConfirmSound();
    }
    hudTogglePressed = hudToggleNow;

    // Close debug tools if debugMode was turned off
    if(!settings.debugMode && debugTools.open){
        debugTools.open = false;
        glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        firstMouse = true;
    }

    if(debugTools.open){
        bool upNow = glfwGetKey(w,GLFW_KEY_UP)==GLFW_PRESS || glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS;
        bool downNow = glfwGetKey(w,GLFW_KEY_DOWN)==GLFW_PRESS || glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS;
        bool enterNow = glfwGetKey(w,GLFW_KEY_ENTER)==GLFW_PRESS || glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS;
        bool escNow = glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS;
        if(upNow && !debugUpPressed){
            debugTools.selectedAction = clampDebugActionIndex(debugTools.selectedAction - 1);
            triggerMenuNavigateSound();
        }
        if(downNow && !debugDownPressed){
            debugTools.selectedAction = clampDebugActionIndex(debugTools.selectedAction + 1);
            triggerMenuNavigateSound();
        }
        if(enterNow && !debugEnterPressed){
            triggerMenuConfirmSound();
            executeDebugAction(debugTools.selectedAction);
        }
        if(escNow && !debugEscPressed){
            debugTools.open = false;
            triggerMenuConfirmSound();
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
            firstMouse = true;
        }
        debugUpPressed = upNow;
        debugDownPressed = downNow;
        debugEnterPressed = enterNow;
        debugEscPressed = escNow;
        escPressed=glfwGetKey(w,settings.binds.pause)==GLFW_PRESS;
        return;
    }

    if(glfwGetKey(w,settings.binds.pause)==GLFW_PRESS&&!escPressed){
        gameState=STATE_PAUSE;menuSel=0;
        glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    }
    escPressed=glfwGetKey(w,settings.binds.pause)==GLFW_PRESS;
    updateMinimapCheat(w);
    
    bool fNow=glfwGetKey(w,settings.binds.flashlight)==GLFW_PRESS;
    if(fNow&&!flashlightPressed&&flashlightBattery>5.0f && activeDeviceSlot == 1){
        flashlightOn=!flashlightOn;
        if(!flashlightOn){
            flashlightShutdownBlinkActive = false;
            flashlightShutdownBlinkTimer = 0.0f;
        }
        sndState.flashlightOn=flashlightOn?1.0f:0.0f;
    }
    flashlightPressed=fNow;
    
    bool eNow=glfwGetKey(w,settings.binds.interact)==GLFW_PRESS;
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
    bool nearExitDoor = nearPoint2D(cam.pos, coop.doorPos, 2.4f);
    bool exitReady = false;
    if(multiState==MULTI_IN_GAME) exitReady = coop.doorOpen && storyMgr.totalCollected>=5;
    else exitReady = isStoryExitReady();
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
                    if(it.type==ITEM_BATTERY) invBattery++;
                    else if(it.type==ITEM_PLUSH_TOY) invPlush++;
                    break;
                }
            }else{
                netMgr.sendInteractRequest(REQ_PICK_ITEM, nearbyWorldItemId);
            }
        }else{
            for(auto& it:worldItems){
                if(!it.active||it.id!=nearbyWorldItemId) continue;
                it.active=false;
                if(it.type==ITEM_BATTERY) invBattery++;
                else if(it.type==ITEM_PLUSH_TOY) invPlush++;
                break;
            }
        }
    }else if(eNow&&!interactPressed&&nearEchoSignal){
        resolveEchoInteraction();
    }else if(eNow&&!interactPressed&&nearExitDoor&&exitReady){
        playerEscaped=true;
        glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
    }
    interactPressed=eNow;
    
    static bool key1Pressed=false,key2Pressed=false,key3Pressed=false,key4Pressed=false;
    bool k1=glfwGetKey(w,settings.binds.item1)==GLFW_PRESS;
    bool k2=glfwGetKey(w,settings.binds.item2)==GLFW_PRESS;
    bool k3=glfwGetKey(w,settings.binds.item3)==GLFW_PRESS;
    bool k4=glfwGetKey(w,settings.binds.item4)==GLFW_PRESS;
    if(k1&&!key1Pressed){
        if(activeDeviceSlot != 1){
            activeDeviceSlot = 1;
        }else{
            activeDeviceSlot = 0;
            flashlightOn = false;
            flashlightShutdownBlinkActive = false;
            flashlightShutdownBlinkTimer = 0.0f;
            sndState.flashlightOn = 0.0f;
        }
    }
    if(k2&&!key2Pressed){
        if(activeDeviceSlot != 2){
            activeDeviceSlot = 2;
            if(flashlightOn){
                flashlightOn = false;
                flashlightShutdownBlinkActive = false;
                flashlightShutdownBlinkTimer = 0.0f;
                sndState.flashlightOn = 0.0f;
            }
        }else{
            activeDeviceSlot = 0;
        }
    }
    if(k3&&!key3Pressed) {
        if(invBattery > 0) applyItemUse(ITEM_BATTERY);
        else if(invPlush > 0) applyItemUse(ITEM_PLUSH_TOY);
    }
    if(k4&&!key4Pressed){
        if(invPlush > 0) applyItemUse(ITEM_PLUSH_TOY);
        else if(invBattery > 0) applyItemUse(ITEM_BATTERY);
    }
    key1Pressed=k1; key2Pressed=k2; key3Pressed=k3; key4Pressed=k4;
    
    // --- Falling physics ---
    if(playerFalling){
        fallTimer += dTime;
        fallVelocity += 12.0f * dTime;
        cam.pos.y -= fallVelocity * dTime;
        // Camera shake while falling
        camShake = 0.08f + fallTimer * 0.04f;
        // Tilt camera slightly downward
        if(cam.pitch > -1.0f) cam.pitch -= 0.5f * dTime;
        // Kill after falling deep enough or timeout
        if(cam.pos.y < -25.0f || fallTimer > 3.0f){
            playerHealth = 0.0f;
            isPlayerDead = true;
            playerFalling = false;
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
        }
        return;
    }

    float spd=4.0f*dTime;
    bool sprinting=glfwGetKey(w,settings.binds.sprint)==GLFW_PRESS&&playerStamina>0&&staminaCooldown<=0;
    if(debugTools.infiniteStamina){
        sprinting = glfwGetKey(w,settings.binds.sprint)==GLFW_PRESS;
        playerStamina = 125.0f;
        staminaCooldown = 0.0f;
    }
    if(sprinting){
        spd*=1.6f;
        if(!debugTools.infiniteStamina){
            playerStamina-=20.0f*dTime;
            if(playerStamina<0){playerStamina=0;staminaCooldown=1.5f;}
        }
    }else{
        if(!debugTools.infiniteStamina){
            playerStamina+=15.0f*dTime;
            if(playerStamina>125)playerStamina=125;
        }
    }
    if(staminaCooldown>0)staminaCooldown-=dTime;

    if(debugTools.flyMode){
        cam.crouch=false;
        cam.targetH=PH;
        cam.curH=PH;
    }else{
        if(glfwGetKey(w,settings.binds.crouch)==GLFW_PRESS){
            cam.targetH=PH_CROUCH;cam.crouch=true;spd*=0.5f;
        }else{cam.targetH=PH;cam.crouch=false;}
        cam.curH+=(cam.targetH-cam.curH)*10.0f*dTime;
    }

    Vec3 fwd(mSin(cam.yaw),0,mCos(cam.yaw)),right(mCos(cam.yaw),0,-mSin(cam.yaw));
    Vec3 np=cam.pos;bool mv=false;
    if(glfwGetKey(w,settings.binds.forward)==GLFW_PRESS){np=np+fwd*spd;mv=true;}
    if(glfwGetKey(w,settings.binds.back)==GLFW_PRESS){np=np-fwd*spd;mv=true;}
    if(glfwGetKey(w,settings.binds.left)==GLFW_PRESS){np=np+right*spd;mv=true;}
    if(glfwGetKey(w,settings.binds.right)==GLFW_PRESS){np=np-right*spd;mv=true;}
    if(debugTools.flyMode){
        if(glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS){np.y += spd; mv=true;}
        if(glfwGetKey(w,settings.binds.crouch)==GLFW_PRESS){np.y -= spd; mv=true;}
    }

    if(debugTools.flyMode){
        cam.pos=np;
    }else{
        bool xBlockedWorld = collideWorld(np.x,cam.pos.z,PR) || collideCoopDoor(np.x,cam.pos.z,PR) ||
                             (falseDoorTimer>0&&nearPoint2D(Vec3(np.x,0,cam.pos.z),falseDoorPos,1.0f));
        bool xBlockedProps = collideMapProps(np.x,cam.pos.z,PR);
        if(!xBlockedWorld && !xBlockedProps){
            cam.pos.x=np.x;
        }else if(!xBlockedWorld && xBlockedProps){
            if(tryPushMapProps(np.x,cam.pos.z,PR,np.x-cam.pos.x,0.0f)) cam.pos.x=np.x;
        }

        bool zBlockedWorld = collideWorld(cam.pos.x,np.z,PR) || collideCoopDoor(cam.pos.x,np.z,PR) ||
                             (falseDoorTimer>0&&nearPoint2D(Vec3(cam.pos.x,0,np.z),falseDoorPos,1.0f));
        bool zBlockedProps = collideMapProps(cam.pos.x,np.z,PR);
        if(!zBlockedWorld && !zBlockedProps){
            cam.pos.z=np.z;
        }else if(!zBlockedWorld && zBlockedProps){
            if(tryPushMapProps(cam.pos.x,np.z,PR,0.0f,np.z-cam.pos.z)) cam.pos.z=np.z;
        }
    }

    static float bobT=0,lastB=0;
    if(mv && !debugTools.flyMode){
        bobT+=dTime*(spd>5.0f?12.0f:8.0f);
        float cb=mSin(bobT);cam.pos.y=cam.curH+cb*0.04f;
        if(lastB>-0.7f&&cb<=-0.7f&&!sndState.stepTrig){
            sndState.stepTrig=true;
            sndState.footPhase=0;
            float pitchRnd = 0.88f + (float)(rand()%30) / 100.0f;
            if(spd > 5.0f) pitchRnd += 0.10f;
            sndState.stepPitch = pitchRnd;
        }
        lastB=cb;
    }else{
        cam.pos.y=cam.curH+(cam.pos.y-cam.curH)*0.9f;bobT=0;lastB=0;
    }

    float moveIntensity = mv ? (sprinting ? 1.0f : 0.62f) : 0.0f;
    updateGameplayAudioState(
        moveIntensity,
        sprinting ? 1.0f : 0.0f,
        playerStamina / 125.0f,
        sndState.monsterProximity,
        sndState.monsterMenace
    );
    
    // Scanner signal strength & beeps (slot 2)
    float targetSignal = 0.0f;
    if(activeDeviceSlot == 2 && echoSignal.active && multiState!=MULTI_IN_GAME){
        Vec3 toEcho = echoSignal.pos - cam.pos;
        float dist = toEcho.len();
        if(dist < 0.01f) dist = 0.01f;
        Vec3 fwdScan(mSin(cam.yaw), 0.0f, mCos(cam.yaw));
        Vec3 toEchoFlat = toEcho; toEchoFlat.y = 0.0f;
        float dir = 0.0f;
        float len = toEchoFlat.len();
        if(len > 0.001f) dir = fwdScan.dot(toEchoFlat / len);
        float facing = (dir + 1.0f) * 0.5f;
        float proximity = 1.0f - (dist / 60.0f);
        if(proximity < 0.0f) proximity = 0.0f;
        if(proximity > 1.0f) proximity = 1.0f;
        targetSignal = proximity * (0.25f + 0.75f * facing);
    }
    scannerSignal += (targetSignal - scannerSignal) * (0.8f * dTime * 3.0f);
    if(scannerSignal < 0.0f) scannerSignal = 0.0f;
    if(scannerSignal > 1.0f) scannerSignal = 1.0f;
    static float scannerBeepTimer = 0.0f;
    if(activeDeviceSlot == 2 && scannerSignal > 0.05f){
        float rate = 0.25f + (1.0f - scannerSignal) * 0.9f;
        scannerBeepTimer -= dTime;
        if(scannerBeepTimer <= 0.0f){
            sndState.scannerBeepPitch = 0.55f + scannerSignal * 1.0f;
            sndState.scannerBeepVol = 0.35f + scannerSignal * 0.55f;
            sndState.scannerBeepTrig = true;
            scannerBeepTimer = rate;
        }
    }else{
        scannerBeepTimer = 0.0f;
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
        float rechargeRate = (activeDeviceSlot == 1) ? 10.0f : 6.0f;
        flashlightBattery+=dTime*rechargeRate;
        if(flashlightBattery>100)flashlightBattery=100;
    }
}













