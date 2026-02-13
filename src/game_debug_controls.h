#pragma once
struct GLFWwindow;
#include "math.h"
#include "item_types.h"
#include "consumable_slot.h"
#include "debug_actions.h"
#include "game_mouse_input.h"
#include "game_input_helpers.h"
void gameInput(GLFWwindow*w){
    static bool debugTogglePressed = false;
    static bool debugEnterPressed = false;
    static bool debugEscPressed = false;
    static int debugHoldDir = 0;
    static double debugNextNavTime = 0.0;
    static bool perfTogglePressed = false;
    static bool hudTogglePressed = false;
    static bool recordPressed = false, playbackPressed = false;
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

    if(settings.debugMode){
        static bool vmUpPressed=false, vmDownPressed=false, vmLeftPressed=false, vmRightPressed=false;
        static bool vmFwdPressed=false, vmBackPressed=false, vmShakeUpPressed=false, vmShakeDownPressed=false;
        bool shift = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
        float step = shift ? 0.05f : 0.01f;
        float stepShake = shift ? 0.10f : 0.02f;

        bool num8 = glfwGetKey(w, GLFW_KEY_KP_8)==GLFW_PRESS;
        bool num2 = glfwGetKey(w, GLFW_KEY_KP_2)==GLFW_PRESS;
        bool num4 = glfwGetKey(w, GLFW_KEY_KP_4)==GLFW_PRESS;
        bool num6 = glfwGetKey(w, GLFW_KEY_KP_6)==GLFW_PRESS;
        bool num9 = glfwGetKey(w, GLFW_KEY_KP_9)==GLFW_PRESS;
        bool num3 = glfwGetKey(w, GLFW_KEY_KP_3)==GLFW_PRESS;
        bool numAdd = glfwGetKey(w, GLFW_KEY_KP_ADD)==GLFW_PRESS;
        bool numSub = glfwGetKey(w, GLFW_KEY_KP_SUBTRACT)==GLFW_PRESS;

        if(num8 && !vmUpPressed){ vmHandUp += step; }
        if(num2 && !vmDownPressed){ vmHandUp -= step; }
        if(num4 && !vmLeftPressed){ vmHandSide -= step; }
        if(num6 && !vmRightPressed){ vmHandSide += step; }
        if(num9 && !vmFwdPressed){ vmHandFwd += step; }
        if(num3 && !vmBackPressed){ vmHandFwd -= step; }
        if(numAdd && !vmShakeUpPressed){ vmShake += stepShake; if(vmShake>1.0f) vmShake=1.0f; }
        if(numSub && !vmShakeDownPressed){ vmShake -= stepShake; if(vmShake<0.0f) vmShake=0.0f; }

        vmUpPressed=num8; vmDownPressed=num2; vmLeftPressed=num4; vmRightPressed=num6;
        vmFwdPressed=num9; vmBackPressed=num3; vmShakeUpPressed=numAdd; vmShakeDownPressed=numSub;

        static float vmPrintTimer = 0.0f;
        vmPrintTimer -= dTime;
        if(vmPrintTimer <= 0.0f){
            char buf[96];
            snprintf(buf, sizeof(buf), "VM fwd=%.2f side=%.2f up=%.2f shake=%.2f", vmHandFwd, vmHandSide, vmHandUp, vmShake);
            setTrapStatus(buf);
            vmPrintTimer = 0.35f;
        }
    }

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
        int navDir = upNow ? -1 : (downNow ? 1 : 0);
        double now = glfwGetTime();
        if(navDir == 0){ debugHoldDir = 0; debugNextNavTime = 0.0; }
        else if(debugHoldDir != navDir){ debugHoldDir = navDir; debugTools.selectedAction = wrapDebugActionIndex(debugTools.selectedAction + navDir); debugNextNavTime = now + 0.26; triggerMenuNavigateSound(); }
        else if(now >= debugNextNavTime){ debugTools.selectedAction = wrapDebugActionIndex(debugTools.selectedAction + navDir); debugNextNavTime = now + 0.06; triggerMenuNavigateSound(); }
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
    if(playerDowned && eNow && !interactPressed && invPlush > 0){ invPlush--; playerDowned=false; playerDownedTimer=0.0f; playerHealth=45.0f; playerSanity+=20.0f; if(playerSanity>100.0f) playerSanity=100.0f; setEchoStatus("SELF-REVIVE SUCCESS"); }
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
    bool nearExitDoor = nearPoint2D(cam.pos, coop.doorPos, 2.4f), exitReady = (multiState==MULTI_IN_GAME) ? (coop.doorOpen && isStoryExitReady()) : isStoryExitReady();
    if(eNow&&!interactPressed&&nearbyWorldItemId>=0){
        if(multiState==MULTI_IN_GAME){
            if(netMgr.isHost){
                for(auto& it:worldItems){
                    if(!it.active||it.id!=nearbyWorldItemId) continue;
                    it.active=false;
                    if(it.type==ITEM_BATTERY) invBattery++;
                    else if(it.type==ITEM_PLUSH_TOY) invPlush++;
                    else if(it.type==ITEM_MED_SPRAY) applyItemUse(ITEM_MED_SPRAY);
                    notifyVoidShiftItemPickup(it.type);
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
                else if(it.type==ITEM_MED_SPRAY) applyItemUse(ITEM_MED_SPRAY);
                addAttention(1.2f);
                notifyVoidShiftItemPickup(it.type);
                break;
            }
        }
    }else if(eNow&&!interactPressed){
        bool contractHandled = (multiState!=MULTI_IN_GAME) ? tryHandleVoidShiftInteract(cam.pos) : false;
        if(multiState==MULTI_IN_GAME){
            if(netMgr.isHost) contractHandled = tryHandleVoidShiftInteract(cam.pos);
            else { char prompt[96]; buildVoidShiftInteractPrompt(cam.pos, prompt, 96); if(prompt[0] != '\0') { netMgr.sendInteractRequest(REQ_VOID_SHIFT_INTERACT, 0); contractHandled = true; } }
        }
        if(!contractHandled && nearExitDoor && exitReady){
            playerEscaped=true; glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
        }
    }
    interactPressed=eNow;
    
    static bool key1Pressed=false,key2Pressed=false,key3Pressed=false,key4Pressed=false;
    bool k1=glfwGetKey(w,settings.binds.item1)==GLFW_PRESS, k2=glfwGetKey(w,settings.binds.item2)==GLFW_PRESS, k3=glfwGetKey(w,settings.binds.item3)==GLFW_PRESS, k4=glfwGetKey(w,settings.binds.item4)==GLFW_PRESS;
    bool craftN = glfwGetKey(w, GLFW_KEY_J)==GLFW_PRESS, craftB = glfwGetKey(w, GLFW_KEY_K)==GLFW_PRESS, craftF = glfwGetKey(w, GLFW_KEY_L)==GLFW_PRESS, craftX = glfwGetKey(w, GLFW_KEY_H)==GLFW_PRESS;
    bool callHere = glfwGetKey(w, GLFW_KEY_T)==GLFW_PRESS, callDanger = glfwGetKey(w, GLFW_KEY_Y)==GLFW_PRESS, callBattery = glfwGetKey(w, GLFW_KEY_U)==GLFW_PRESS, callCode = glfwGetKey(w, GLFW_KEY_I)==GLFW_PRESS, callDoor = glfwGetKey(w, GLFW_KEY_O)==GLFW_PRESS;
    bool recordNow = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS, playbackNow = glfwGetKey(w, GLFW_KEY_P) == GLFW_PRESS;
    static bool pingPressed=false;
    bool pingNow = glfwGetKey(w, GLFW_KEY_G) == GLFW_PRESS;
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
            resonatorMode = RESONATOR_SCAN;
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
        if(flashlightOn){
            flashlightOn = false;
            flashlightShutdownBlinkActive = false;
            flashlightShutdownBlinkTimer = 0.0f;
            sndState.flashlightOn = 0.0f;
        }
        handleConsumableSlot3Press();
    }
    if(k4&&!key4Pressed){
        // Slot 4: quick-use consumable (prefer plush, else battery) without equipping.
        if(invPlush > 0) applyItemUse(ITEM_PLUSH_TOY);
        else if(invBattery > 0) applyItemUse(ITEM_BATTERY);
    }

    if(multiState==MULTI_IN_GAME && pingNow && !pingPressed){
        netMgr.sendPingMark(cam.pos);
        netMgr.pingMarkFrom = netMgr.myId;
        netMgr.pingMarkPos = cam.pos;
        netMgr.pingMarkTtl = 6.0f;
        netMgr.pingMarkReceived = true;
        addAttention(1.0f);
        notifyVoidShiftPingUsed();
    }
    if((callHere||callDanger||callBattery||callCode||callDoor) && !pingPressed){ if(callHere) setSquadCallout("CALLOUT: MOVE HERE"); else if(callDanger) setSquadCallout("CALLOUT: DANGER"); else if(callBattery) setSquadCallout("CALLOUT: NEED BATTERY"); else if(callCode) setSquadCallout("CALLOUT: CODE FOUND"); else if(callDoor) setSquadCallout("CALLOUT: HOLDING DOOR"); }
    if((craftN||craftB||craftF||craftX) && !pingPressed){ if(craftN) tryCraftNoiseLure(); else if(craftB) tryCraftBeacon(); else if(craftF) tryCraftFlashLamp(); else if(craftX) tryCraftFixator(); }

    if(recordNow && !recordPressed){
        resonatorMode = RESONATOR_RECORD;
        if(!echoRecording) startEchoRecordingTrack();
        else stopEchoRecordingTrack();
    }
    if(playbackNow && !playbackPressed){
        resonatorMode = RESONATOR_PLAYBACK;
        startEchoPlaybackTrack();
    }

    key1Pressed=k1; key2Pressed=k2; key3Pressed=k3; key4Pressed=k4;
    pingPressed = pingNow;
    recordPressed = recordNow;
    playbackPressed = playbackNow;
    
    if(playerFalling){
        fallTimer += dTime;
        fallVelocity += 12.0f * dTime;
        cam.pos.y -= fallVelocity * dTime;
        camShake = 0.08f + fallTimer * 0.04f;
        if(cam.pitch > -1.0f) cam.pitch -= 0.5f * dTime;
        if(cam.pos.y < -25.0f || fallTimer > 3.0f){
            playerHealth = 0.0f;
            isPlayerDead = true;
            snprintf(gDeathReason,sizeof(gDeathReason),"CAUSE: VOID BREACH FALL");
            playerFalling = false;
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
        }
        return;
    }

    float spd=4.0f*dTime;
    bool sprinting=glfwGetKey(w,settings.binds.sprint)==GLFW_PRESS&&playerStamina>0&&staminaCooldown<=0&&!playerDowned;
    if(debugTools.infiniteStamina){
        sprinting = glfwGetKey(w,settings.binds.sprint)==GLFW_PRESS;
        playerStamina = 125.0f;
        staminaCooldown = 0.0f;
    }
    if(playerDowned) spd *= 0.42f;
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
    if(squadCalloutTimer>0.0f){ squadCalloutTimer-=dTime; if(squadCalloutTimer<0.0f) squadCalloutTimer=0.0f; }
    updateVoidShiftSystems(dTime, sprinting, flashlightOn);

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

    float targetGroundY = debugTools.flyMode ? 0.0f : getGroundY(cam.pos.x, cam.pos.z);
    static float smoothGroundY = 0.0f;
    float groundLerp = 12.0f * dTime;
    if (groundLerp > 1.0f) groundLerp = 1.0f;
    smoothGroundY += (targetGroundY - smoothGroundY) * groundLerp;
    float groundY = smoothGroundY;
    static float bobT=0,lastB=0;
    if(mv && !debugTools.flyMode){
        bobT+=dTime*(spd>5.0f?12.0f:8.0f);
        float cb=mSin(bobT);cam.pos.y=groundY+cam.curH+cb*0.04f;
        if(lastB>-0.7f&&cb<=-0.7f&&!sndState.stepTrig){
            sndState.stepTrig=true;
            sndState.footPhase=0;
            float pitchRnd = 0.88f + (float)(rand()%30) / 100.0f;
            if(spd > 5.0f) pitchRnd += 0.10f;
            sndState.stepPitch = pitchRnd;
        }
        lastB=cb;
    }else{
        cam.pos.y=groundY+cam.curH+(cam.pos.y-groundY-cam.curH)*0.9f;bobT=0;lastB=0;
    }

    float moveIntensity = mv ? (sprinting ? 1.0f : 0.62f) : 0.0f;
    updateGameplayAudioState(
        moveIntensity,
        sprinting ? 1.0f : 0.0f,
        playerStamina / 125.0f,
        sndState.monsterProximity,
        sndState.monsterMenace,
        sndState.monsterType
    );
    
    float rawSignal = 0.0f;
    Vec3 resonanceTarget(0, 0, 0);
    bool scannerHasTarget = (activeDeviceSlot == 2 && multiState!=MULTI_IN_GAME && getVoidShiftResonanceTarget(resonanceTarget));
    if(scannerHasTarget){
        Vec3 toEcho = resonanceTarget - cam.pos;
        float dist = toEcho.len();
        if(dist < 0.01f) dist = 0.01f;
        Vec3 fwdScan(mSin(cam.yaw), 0.0f, mCos(cam.yaw));
        Vec3 toEchoFlat = toEcho; toEchoFlat.y = 0.0f;
        float dir = 0.0f;
        float len = toEchoFlat.len();
        if(len > 0.001f) dir = fwdScan.dot(toEchoFlat / len);
        float facing = (dir + 1.0f) * 0.5f;
        float proximity = 1.0f - (dist / 70.0f);
        if(proximity < 0.0f) proximity = 0.0f;
        if(proximity > 1.0f) proximity = 1.0f;
        rawSignal = proximity * (0.38f + 0.62f * facing);
    }

    static bool scannerLock = false;
    static float scannerEnterTimer = 0.0f;
    static float scannerExitTimer = 0.0f;
    const float lockEnterThreshold = 0.58f;
    const float lockExitThreshold = 0.44f;
    const float lockEnterHold = 0.10f;
    const float lockExitHold = 0.18f;
    if(!scannerLock){
        if(rawSignal >= lockEnterThreshold) scannerEnterTimer += dTime;
        else scannerEnterTimer = 0.0f;
        scannerExitTimer = 0.0f;
        if(scannerEnterTimer >= lockEnterHold){
            scannerLock = true;
            scannerEnterTimer = 0.0f;
        }
    }else{
        if(rawSignal <= lockExitThreshold) scannerExitTimer += dTime;
        else scannerExitTimer = 0.0f;
        scannerEnterTimer = 0.0f;
        if(scannerExitTimer >= lockExitHold){
            scannerLock = false;
            scannerExitTimer = 0.0f;
        }
    }
    float targetSignal = scannerLock ? rawSignal : rawSignal * 0.12f;

    updateScannerHeat(scannerHeat, scannerOverheated, scannerOverheatTimer, activeDeviceSlot == 2, scannerLock, dTime);

    updateScannerPhantom(scannerPhantomTimer, scannerPhantomBias, activeDeviceSlot == 2 && !scannerOverheated, playerSanity, dTime);

    float biasedTarget = targetSignal;
    if(scannerPhantomTimer > 0.0f) biasedTarget += scannerPhantomBias;
    float heatWarn = (scannerHeat - 0.76f) / 0.24f;
    if(heatWarn < 0.0f) heatWarn = 0.0f;
    if(heatWarn > 1.0f) heatWarn = 1.0f;
    biasedTarget *= (1.0f - heatWarn * 0.72f);
    if(scannerOverheated){
        float n = ((float)(rng()%1000) / 1000.0f) * 2.0f - 1.0f;
        biasedTarget = n * 0.16f;
    }

    float moveK = moveIntensity;
    if(moveK < 0.0f) moveK = 0.0f;
    if(moveK > 1.0f) moveK = 1.0f;
    float tau = 0.13f + (0.06f - 0.13f) * moveK;
    if(tau < 0.03f) tau = 0.03f;
    float alpha = 1.0f - expf(-dTime / tau);
    scannerSignal += (biasedTarget - scannerSignal) * alpha;
    if(scannerSignal < 0.0f) scannerSignal = 0.0f;
    if(scannerSignal > 1.0f) scannerSignal = 1.0f;
    static float scannerBeepTimer = 0.0f;
    updateScannerBeep(scannerBeepTimer, activeDeviceSlot == 2, scannerOverheated, scannerSignal, scannerHeat, dTime);
    updateFlashlightBattery(flashlightBattery, flashlightOn, flashlightShutdownBlinkActive, flashlightShutdownBlinkTimer, activeDeviceSlot, dTime);
}
