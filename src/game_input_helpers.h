#pragma once
#include "math.h"

// Helper functions extracted from game_debug_controls.h to maintain style requirements

inline void updateScannerHeat(float& scannerHeat, bool& scannerOverheated, float& scannerOverheatTimer, bool isActive, bool scannerLock, float dTime){
    const float heatUpPerSec = 0.12f;
    const float heatDownPerSec = 0.10f;
    if(isActive && scannerLock && !scannerOverheated) scannerHeat += heatUpPerSec * dTime;
    else scannerHeat -= heatDownPerSec * dTime;
    if(scannerHeat < 0.0f) scannerHeat = 0.0f;
    if(scannerHeat > 1.0f) scannerHeat = 1.0f;
    if(!scannerOverheated && scannerHeat >= 0.98f){
        scannerOverheated = true;
        scannerOverheatTimer = 2.8f;
        setTrapStatus("SCANNER OVERHEAT");
    }
    if(scannerOverheated){
        scannerOverheatTimer -= dTime;
        if(scannerOverheatTimer <= 0.0f && scannerHeat <= 0.34f){
            scannerOverheated = false;
            scannerOverheatTimer = 0.0f;
        }
    }
}

inline void updateScannerPhantom(float& scannerPhantomTimer, float& scannerPhantomBias, bool isActive, float playerSanity, float dTime){
    if(isActive && scannerPhantomTimer <= 0.0f){
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
}

inline void updateScannerBeep(float& scannerBeepTimer, bool isActive, bool scannerOverheated, float scannerSignal, float scannerHeat, float dTime){
    if(isActive){
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
}

inline void updateFlashlightBattery(float& flashlightBattery, bool& flashlightOn, bool& flashlightShutdownBlinkActive, float& flashlightShutdownBlinkTimer, int activeDeviceSlot, float dTime){
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
