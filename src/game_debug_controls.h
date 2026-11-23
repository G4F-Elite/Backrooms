#pragma once
struct GLFWwindow;
#include "math.h"
#include "item_types.h"
#include "consumable_slot.h"
#include "spectator_cam.h"
#include "game_mouse_input.h"
#include "player_vitals.h"
#include "squad_play.h"
#include "heist_play.h"
#include "debug_actions.h"
void gameInput(GLFWwindow*w){
    static bool debugTogglePressed = false;
    static bool debugEnterPressed = false;
    static bool debugEscPressed = false;
    static int debugHoldDir = 0;
    static double debugNextNavTime = 0.0;
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
    
    bool eNow=glfwGetKey(w,settings.binds.interact)==GLFW_PRESS && !gCoopDead; // ghosts cannot interact
    if(playerDowned && eNow && !interactPressed && invPlush > 0){ invPlush--; playerDowned=false; playerDownedTimer=0.0f; playerHealth=45.0f; playerSanity+=20.0f; if(playerSanity>100.0f) playerSanity=100.0f; setEchoStatus("THE PLUSH TOOK THE HIT. BACK ON THE CLOCK"); }
    heistHandleInteract(eNow && !playerDowned, eNow && !interactPressed && !playerDowned);
    interactPressed=eNow;
    
    static bool key1Pressed=false,key2Pressed=false,key3Pressed=false,key4Pressed=false;
    bool k1=glfwGetKey(w,settings.binds.item1)==GLFW_PRESS, k2=glfwGetKey(w,settings.binds.item2)==GLFW_PRESS, k3=glfwGetKey(w,settings.binds.item3)==GLFW_PRESS, k4=glfwGetKey(w,settings.binds.item4)==GLFW_PRESS;
    static bool dropPressed=false;
    bool dropNow = glfwGetKey(w, GLFW_KEY_R) == GLFW_PRESS && !gCoopDead && !playerDowned;
    if(dropNow && !dropPressed && carryCount(heistMyCarry()) > 0){ heistRequestAction(HEIST_ACT_DROP); triggerToySound(TOY_SHOVE, 0.3f, 1.2f); }
    dropPressed = dropNow;
    // At the kiosk the item keys buy things instead of switching tools.
    if(heistHandleShopKeys(k1&&!key1Pressed, k2&&!key2Pressed, k3&&!key3Pressed, k4&&!key4Pressed)){ key1Pressed=k1; key2Pressed=k2; key3Pressed=k3; key4Pressed=k4; }
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
        // Slot 4: quick-use battery without equipping.
        if(invBattery > 0) applyItemUse(ITEM_BATTERY);
    }

    if(multiState==MULTI_IN_GAME && pingNow && !pingPressed){
        netMgr.sendPingMark(cam.pos);
        netMgr.pingMarkFrom = netMgr.myId;
        netMgr.pingMarkPos = cam.pos;
        netMgr.pingMarkTtl = 6.0f;
        netMgr.pingMarkReceived = true;
        addAttention(1.0f);
        emitSquadNoise(netMgr.myId, NOISE_PING);
    }
    updateSquadPlay(w);
    updateSpectator(w);

    key1Pressed=k1; key2Pressed=k2; key3Pressed=k3; key4Pressed=k4;
    pingPressed = pingNow;
    
    if(playerFalling){
        legsRest();
        // Free fall down the shaft (fall_sim.h): the camera is the body's eyes, tumbling with it.
        fallStep(gFall, dTime, CS, isFallCell);
        fallTimer = gFall.time;
        fallVelocity = -gFall.vel.y;
        cam.pos = gFall.pos;
        cam.yaw = fallViewYaw(gFall);     // the head on its neck: thrown about by the walls, late for the body
        cam.pitch = fallViewPitch(gFall);
        float shake = fallShake(gFall);
        if(camShake < shake) camShake = shake;
        if(gFall.hit > 2.0f){ sndState.hitTrig = true; damageFlash = 0.25f; } // slammed into the shaft wall
        if(cam.pos.y < -25.0f || fallTimer > 3.0f){
            playerHealth = 0.0f;
            isPlayerDead = true;
            snprintf(gDeathReason,sizeof(gDeathReason),"CAUSE: VOID BREACH FALL");
            playerFalling = false;
            glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_NORMAL);
        }
        return;
    }

    float spd=LEGS_WALK_SPEED*dTime;
    bool sprinting=glfwGetKey(w,settings.binds.sprint)==GLFW_PRESS&&playerStamina>0&&staminaCooldown<=0&&!playerDowned;
    if(debugTools.infiniteStamina){
        sprinting = glfwGetKey(w,settings.binds.sprint)==GLFW_PRESS;
        playerStamina = 125.0f;
        staminaCooldown = 0.0f;
    }
    if(playerDowned) spd *= 0.42f;
    spd *= heistMoveScale(); // hauling loot is heavy
    if(sprinting){
        spd*=LEGS_SPRINT_SCALE;
        if(!debugTools.infiniteStamina){
            playerStamina-=(14.0f+2.0f*(float)carryWeight(heistMyCarry()))*dTime;
            if(playerStamina<0){playerStamina=0;staminaCooldown=1.0f;}
        }
    }else{
        if(!debugTools.infiniteStamina){
            playerStamina+=20.0f*dTime;
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
            cam.targetH=PH_CROUCH;cam.crouch=true;spd*=LEGS_CROUCH_SCALE;
        }else{cam.targetH=PH;cam.crouch=false;}
        cam.curH+=(cam.targetH-cam.curH)*10.0f*dTime;
    }

    // On foot the player is the body of the leg simulation (player_legs.h): the keys push it,
    // the camera rides on its head and the footsteps are its heel strikes.
    bool mv=false;
    bool onFoot=!spectatorRiding(); // a riding ghost goes where its ride goes; the keys drive the camera
    if(debugTools.flyMode){
        legsRest();
        Vec3 fwd(mSin(cam.yaw),0,mCos(cam.yaw)),right(mCos(cam.yaw),0,-mSin(cam.yaw));
        Vec3 np=cam.pos;
        if(onFoot&&glfwGetKey(w,settings.binds.forward)==GLFW_PRESS){np=np+fwd*spd;mv=true;}
        if(onFoot&&glfwGetKey(w,settings.binds.back)==GLFW_PRESS){np=np-fwd*spd;mv=true;}
        if(onFoot&&glfwGetKey(w,settings.binds.left)==GLFW_PRESS){np=np+right*spd;mv=true;}
        if(onFoot&&glfwGetKey(w,settings.binds.right)==GLFW_PRESS){np=np-right*spd;mv=true;}
        if(glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS){np.y += spd; mv=true;}
        if(glfwGetKey(w,settings.binds.crouch)==GLFW_PRESS){np.y -= spd; mv=true;}
        cam.pos=np;
        cam.pos.y=cam.curH+(cam.pos.y-cam.curH)*0.9f;
    }else if(!onFoot){
        legsRest();
        cam.pos.y=cam.curH+(cam.pos.y-cam.curH)*0.9f;
    }else{
        mv=legsWalk(w,spd/dTime,sprinting,onFoot);
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
    bool scannerHasTarget = (activeDeviceSlot == 2 && heistScannerTarget(resonanceTarget));
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

    const float heatUpPerSec = 0.12f;
    const float heatDownPerSec = 0.10f;
    if(activeDeviceSlot == 2 && scannerLock && !scannerOverheated) scannerHeat += heatUpPerSec * dTime;
    else scannerHeat -= heatDownPerSec * dTime;
    if(scannerHeat < 0.0f) scannerHeat = 0.0f;
    if(scannerHeat > 1.0f) scannerHeat = 1.0f;
    if(!scannerOverheated && scannerHeat >= 0.98f){
        scannerOverheated = true;
        scannerOverheatTimer = 2.8f;
        setTrapStatus("SCANNER OVERHEATED. THE WARRANTY DID NOT COVER THIS");
    }
    if(scannerOverheated){
        scannerOverheatTimer -= dTime;
        if(scannerOverheatTimer <= 0.0f && scannerHeat <= 0.34f){
            scannerOverheated = false;
            scannerOverheatTimer = 0.0f;
        }
    }

    if(activeDeviceSlot == 2 && !scannerOverheated && scannerPhantomTimer <= 0.0f){
        float sanityLoss = 1.0f - (playerSanity / 100.0f);
        if(sanityLoss < 0.0f) sanityLoss = 0.0f;
        if(sanityLoss > 1.0f) sanityLoss = 1.0f;
        float lateInsanity = (sanityLoss - 0.72f) / 0.28f;
        if(lateInsanity < 0.0f) lateInsanity = 0.0f;
        if(lateInsanity > 1.0f) lateInsanity = 1.0f;
        float p = lateInsanity * 0.020f;
        if(((float)(rng()%10000) / 10000.0f) < p * dTime){
            scannerPhantomTimer = 0.9f + (float)(rng()%90) / 100.0f;
            scannerPhantomBias = ((float)(rng()%1000) / 1000.0f) * 0.24f - 0.12f;
        }
    }
    if(scannerPhantomTimer > 0.0f){
        scannerPhantomTimer -= dTime;
        if(scannerPhantomTimer <= 0.0f){
            scannerPhantomTimer = 0.0f;
            scannerPhantomBias = 0.0f;
        }
    }

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
    if(activeDeviceSlot == 2){
        scannerBeepTimer -= dTime;
        if(scannerOverheated){
            if(scannerBeepTimer <= 0.0f){
                float r = (float)(rng()%1000) / 1000.0f;
                sndState.scannerBeepPitch = 0.40f + r * 0.95f;
                sndState.scannerBeepVol = 0.08f + r * 0.06f;
                sndState.scannerBeepTrig = true;
                scannerBeepTimer = 0.22f + r * 0.22f;
            }
        }else if(scannerSignal > 0.05f){
            float heatMuffle = 1.0f - fastClamp01(scannerHeat * 0.48f);
            float rel = 1.0f - scannerSignal;
            if(rel < 0.0f) rel = 0.0f;
            if(rel > 1.0f) rel = 1.0f;
            float rate = 0.18f + 0.85f * (rel * rel);
            if(scannerBeepTimer <= 0.0f){
                sndState.scannerBeepPitch = 0.55f + scannerSignal * 1.0f;
                sndState.scannerBeepVol = (0.35f + scannerSignal * 0.55f) * heatMuffle;
                sndState.scannerBeepTrig = true;
                scannerBeepTimer = rate;
            }
        }else{
            scannerBeepTimer = 0.0f;
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
